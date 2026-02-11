from gnuradio import gr, blocks, analog
import numpy as np
import pmt
import osmosdr
import logging
from gnuradio.lora_sdr import lora_sdr_python as lora_sdr
from config import (CENTER_FREQ, BANDWIDTH, CODING_RATE,
                    SYNC_WORD, HAS_CRC, IMPLICIT_HEADER, SAMP_RATE,
                    RF_GAIN, IF_GAIN, BB_GAIN)

logger = logging.getLogger(__name__)

# SFs to scan simultaneously
RX_SPREADING_FACTORS = [7, 8, 9, 10, 11, 12]

class LoRaPacketSink(gr.sync_block):
    def __init__(self, callback):
        gr.sync_block.__init__(self, name="lora_packet_sink", in_sig=[np.uint8], out_sig=None)
        self.callback = callback
        self.message_port_register_in(pmt.intern("msg"))
        self.set_msg_handler(pmt.intern("msg"), self.handle_msg)
        
    def handle_msg(self, msg):
        if pmt.is_pair(msg):
            meta = pmt.car(msg)
            data = pmt.cdr(msg)
            if pmt.is_u8vector(data):
                payload = bytes(pmt.u8vector_elements(data))
                crc_ok = True
                if pmt.is_dict(meta):
                    crc_key = pmt.intern("crc_valid")
                    if pmt.dict_has_key(meta, crc_key):
                        crc_ok = pmt.to_bool(pmt.dict_ref(meta, crc_key, pmt.PMT_T))
                self.callback(payload, crc_ok)
    
    def work(self, input_items, output_items):
        data = bytes(input_items[0])
        if len(data) > 0:
            self.callback(data, True)
        return len(input_items[0])


class Receiver(gr.top_block):
    """Multi-SF LoRa receiver: one osmosdr source feeds parallel decode
    chains for each SF in RX_SPREADING_FACTORS.

    The packet_callback signature is:
        callback(data: bytes, crc_ok: bool, sf: int, rssi: float, snr: float)
    """

    # Empirical dBFS → dBm offset for HackRF One.
    # HackRF has no calibrated power reference; this constant should be
    # tuned against a known-power signal.  A typical value at 433 MHz with
    # RF=40 IF=40 BB=20 is around -10 to +5 depending on the individual
    # board.  Set via HACKRF_RSSI_OFFSET env-var to fine-tune.
    HACKRF_RSSI_OFFSET = -30.0

    def __init__(self, packet_callback):
        gr.top_block.__init__(self, "HackRF LoRa Multi-SF Receiver")
        self.packet_callback = packet_callback

        # --- Single HackRF source shared by all SF chains ---
        self.osmosdr_source = osmosdr.source(args="hackrf=0")
        self.osmosdr_source.set_sample_rate(SAMP_RATE)
        self.osmosdr_source.set_center_freq(CENTER_FREQ, 0)
        self.osmosdr_source.set_freq_corr(0, 0)
        self.osmosdr_source.set_gain(RF_GAIN, 0)
        self.osmosdr_source.set_if_gain(IF_GAIN, 0)
        self.osmosdr_source.set_bb_gain(BB_GAIN, 0)
        self.osmosdr_source.set_bandwidth(0, 0)

        max_sf = max(RX_SPREADING_FACTORS)
        # Buffer must hold at least one full symbol at highest SF.
        # Round up to next power of 2 *strictly greater* than the minimum.
        min_needed = int(np.ceil(SAMP_RATE / BANDWIDTH * (2**max_sf + 2)))
        min_buffer = 1
        while min_buffer <= min_needed:
            min_buffer <<= 1
        self.osmosdr_source.set_min_output_buffer(min_buffer)

        os_factor = int(SAMP_RATE / BANDWIDTH)

        # --- RSSI measurement via averaged power probe ---
        # probe_avg_mag_sqrd_c keeps an IIR average of |x|², so the
        # reading stays valid even right after an RX restart.
        alpha = 1.0 - np.exp(-1.0 / (SAMP_RATE * 0.1))
        self.power_probe_blk = analog.probe_avg_mag_sqrd_c(0, alpha)
        self.connect((self.osmosdr_source, 0),
                     (self.power_probe_blk, 0))

        # --- One full decode chain per SF, all fed from osmosdr source ---
        for sf in RX_SPREADING_FACTORS:
            frame_sync = lora_sdr.frame_sync(
                int(CENTER_FREQ), BANDWIDTH, sf, IMPLICIT_HEADER,
                [SYNC_WORD], os_factor, 8
            )
            fft_demod = lora_sdr.fft_demod(True, True)
            gray_mapping = lora_sdr.gray_mapping(True)
            deinterleaver = lora_sdr.deinterleaver(True)
            hamming_dec = lora_sdr.hamming_dec(True)
            header_decoder = lora_sdr.header_decoder(
                IMPLICIT_HEADER, CODING_RATE, 255, HAS_CRC, 2, False
            )
            dewhitening = lora_sdr.dewhitening()
            crc_verif = lora_sdr.crc_verif(False, False)
            packet_sink = LoRaPacketSink(
                lambda data, crc_ok, _sf=sf: self._on_packet(data, crc_ok, _sf),
            )

            # osmosdr source → decode chain (GNU Radio auto-fans-out)
            self.connect((self.osmosdr_source, 0), (frame_sync, 0))
            self.connect((frame_sync, 0), (fft_demod, 0))
            self.connect((fft_demod, 0), (gray_mapping, 0))
            self.connect((gray_mapping, 0), (deinterleaver, 0))
            self.connect((deinterleaver, 0), (hamming_dec, 0))
            self.connect((hamming_dec, 0), (header_decoder, 0))
            self.connect((header_decoder, 0), (dewhitening, 0))
            self.connect((dewhitening, 0), (crc_verif, 0))
            self.connect((crc_verif, 0), (packet_sink, 0))

            # Message connections
            self.msg_connect((header_decoder, "frame_info"), (frame_sync, "frame_info"))
            self.msg_connect((crc_verif, "msg"), (packet_sink, "msg"))

        logger.info(f"Multi-SF receiver initialized: {RX_SPREADING_FACTORS}")

    def _on_packet(self, data, crc_ok, sf):
        mean_pwr = self.power_probe_blk.level()
        dbfs = 10.0 * np.log10(mean_pwr) if mean_pwr > 0 else -120.0
        rssi_dbm = dbfs + self.HACKRF_RSSI_OFFSET
        self.packet_callback(data, crc_ok, sf, rssi_dbm)
