from gnuradio import gr, blocks
import numpy as np
import pmt
import osmosdr
import time
from gnuradio.lora_sdr import lora_sdr_python as lora_sdr
from config import (CENTER_FREQ, BANDWIDTH, SPREADING_FACTOR, CODING_RATE,
                    SYNC_WORD, IMPLICIT_HEADER, SAMP_RATE,
                    RF_GAIN, IF_GAIN, BB_GAIN)


class Transmitter(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self, "HackRF LoRa Transmitter")

        self.whitening = lora_sdr.whitening(True, False, ',', 'frame_len')
        # LoRaWAN downlinks are sent WITHOUT CRC (uplinks have CRC)
        self.header = lora_sdr.header(IMPLICIT_HEADER, False, CODING_RATE)
        self.add_crc = lora_sdr.add_crc(False)
        self.hamming_enc = lora_sdr.hamming_enc(CODING_RATE, SPREADING_FACTOR)
        self.interleaver = lora_sdr.interleaver(CODING_RATE, SPREADING_FACTOR, 0, BANDWIDTH)
        self.gray_decode = lora_sdr.gray_demap(SPREADING_FACTOR)
        self.modulate = lora_sdr.modulate(SPREADING_FACTOR, SAMP_RATE, BANDWIDTH, [SYNC_WORD], 0, 8)

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
        self.connect((self.modulate, 0), (self.osmosdr_sink, 0))

    def send(self, data):
        """Send a LoRaWAN downlink packet via HackRF."""
        # gr-lora_sdr whitening block decodes hex string back to bytes (is_hex=True)
        hex_str = data.hex()
        msg = pmt.intern(hex_str)
        self.whitening.to_basic_block()._post(pmt.intern("msg"), msg)

        self.start()
        
        time.sleep(2) # TODO: magic number
        self.stop()
        self.wait()