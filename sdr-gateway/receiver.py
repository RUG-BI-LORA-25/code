from gnuradio import gr, blocks
import numpy as np
import pmt
import osmosdr
from gnuradio.lora_sdr import lora_sdr_python as lora_sdr
from config import (CENTER_FREQ, BANDWIDTH, SPREADING_FACTOR, CODING_RATE,
                    SYNC_WORD, HAS_CRC, IMPLICIT_HEADER, SAMP_RATE,
                    RF_GAIN, IF_GAIN, BB_GAIN)


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


class LoRaReceiver(gr.top_block):
    def __init__(self, packet_callback):
        gr.top_block.__init__(self, "HackRF LoRa Receiver")
        self.packet_callback = packet_callback
        
        self.osmosdr_source = osmosdr.source(args="hackrf=0")
        self.osmosdr_source.set_sample_rate(SAMP_RATE)
        self.osmosdr_source.set_center_freq(CENTER_FREQ, 0)
        self.osmosdr_source.set_freq_corr(0, 0)
        self.osmosdr_source.set_gain(RF_GAIN, 0)
        self.osmosdr_source.set_if_gain(IF_GAIN, 0)
        self.osmosdr_source.set_bb_gain(BB_GAIN, 0)
        self.osmosdr_source.set_bandwidth(0, 0)
        
        min_buffer = int(np.ceil(SAMP_RATE / BANDWIDTH * (2**SPREADING_FACTOR + 2)))
        self.osmosdr_source.set_min_output_buffer(min_buffer)
        
        os_factor = int(SAMP_RATE / BANDWIDTH)
        
        self.frame_sync = lora_sdr.frame_sync(
            int(CENTER_FREQ), BANDWIDTH, SPREADING_FACTOR, IMPLICIT_HEADER,
            [SYNC_WORD], os_factor, 8
        )
        self.fft_demod = lora_sdr.fft_demod(True, True)
        self.gray_mapping = lora_sdr.gray_mapping(True)
        self.deinterleaver = lora_sdr.deinterleaver(True)
        self.hamming_dec = lora_sdr.hamming_dec(True)
        self.header_decoder = lora_sdr.header_decoder(
            IMPLICIT_HEADER, CODING_RATE, 255, HAS_CRC, 2, False
        )
        self.dewhitening = lora_sdr.dewhitening()
        self.crc_verif = lora_sdr.crc_verif(False, False)
        self.packet_sink = LoRaPacketSink(self._on_packet)
        
        self.connect((self.osmosdr_source, 0), (self.frame_sync, 0))
        self.connect((self.frame_sync, 0), (self.fft_demod, 0))
        self.connect((self.fft_demod, 0), (self.gray_mapping, 0))
        self.connect((self.gray_mapping, 0), (self.deinterleaver, 0))
        self.connect((self.deinterleaver, 0), (self.hamming_dec, 0))
        self.connect((self.hamming_dec, 0), (self.header_decoder, 0))
        self.connect((self.header_decoder, 0), (self.dewhitening, 0))
        self.connect((self.dewhitening, 0), (self.crc_verif, 0))
        self.connect((self.crc_verif, 0), (self.packet_sink, 0))
        
        self.msg_connect((self.header_decoder, "frame_info"), (self.frame_sync, "frame_info"))
        self.msg_connect((self.crc_verif, "msg"), (self.packet_sink, "msg"))
        
    def _on_packet(self, data, crc_ok):
        self.packet_callback(data, crc_ok)
