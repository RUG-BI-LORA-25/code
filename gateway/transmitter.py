from gnuradio import gr, blocks
import numpy as np
import pmt
import osmosdr
import time
from gnuradio.lora_sdr import lora_sdr_python as lora_sdr
from config import (CENTER_FREQ, BANDWIDTH, SPREADING_FACTOR, CODING_RATE,
                    SYNC_WORD, HAS_CRC, IMPLICIT_HEADER, SAMP_RATE,
                    RF_GAIN, IF_GAIN, BB_GAIN)

class Transmitter(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self, "HackRF LoRa Transmitter")

        # is_hex=True: we pass hex string, block decodes to bytes
        self.whitening = lora_sdr.whitening(True, False, ',', 'frame_len')
        self.header = lora_sdr.header(False, False, CODING_RATE)  # explicit header, NO CRC for downlinks
        self.add_crc = lora_sdr.add_crc(False)                    # NO CRC for downlinks
        self.hamming_enc = lora_sdr.hamming_enc(CODING_RATE, SPREADING_FACTOR)
        self.interleaver = lora_sdr.interleaver(CODING_RATE, SPREADING_FACTOR, 0, BANDWIDTH)
        self.gray_decode = lora_sdr.gray_demap(SPREADING_FACTOR)
        self.modulate = lora_sdr.modulate(SPREADING_FACTOR, SAMP_RATE, BANDWIDTH, [SYNC_WORD], 0, 8)

        # Multiply by -1j to invert IQ (swap I and Q)
        # This is equivalent to conjugating the complex signal
        self.iq_invert = blocks.multiply_const_cc(-1j)

        self.osmosdr_sink = osmosdr.sink(args="hackrf=0")
        self.osmosdr_sink.set_sample_rate(SAMP_RATE)
        self.osmosdr_sink.set_center_freq(CENTER_FREQ, 0)
        self.osmosdr_sink.set_gain(RF_GAIN, 0)
        self.osmosdr_sink.set_if_gain(IF_GAIN, 0)
        self.osmosdr_sink.set_bb_gain(BB_GAIN, 0)

        self.connect((self.whitening, 0), (self.header, 0))
        self.connect((self.header, 0), (self.add_crc, 0))
        self.connect((self.add_crc, 0), (self.hamming_enc, 0))
        self.connect((self.hamming_enc, 0), (self.interleaver, 0))
        self.connect((self.interleaver, 0), (self.gray_decode, 0))
        self.connect((self.gray_decode, 0), (self.modulate, 0))
        self.connect((self.modulate, 0), (self.iq_invert, 0))   # invert IQ
        self.connect((self.iq_invert, 0), (self.osmosdr_sink, 0))

    def send(self, data):
        hex_str = data.hex()
        msg = pmt.intern(hex_str)
        self.whitening.to_basic_block()._post(pmt.intern("msg"), msg)

        self.start()
        symbol_time = (2 ** SPREADING_FACTOR) / BANDWIDTH
        n_payload_symbols = max(8, int(np.ceil(
            (8 * len(data) + 28 - 4 * SPREADING_FACTOR) /
            (4 * SPREADING_FACTOR)
        )) * (CODING_RATE + 4))
        tx_time = (8 + 4.25 + n_payload_symbols) * symbol_time + 0.5
        print(f"    TX: {len(data)} bytes, ~{tx_time:.1f}s airtime")
        time.sleep(tx_time)
        self.stop()
        self.wait()