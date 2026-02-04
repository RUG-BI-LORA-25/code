from gnuradio import gr, blocks
import numpy as np
import pmt
import osmosdr
from gnuradio.lora_sdr import lora_sdr_python as lora_sdr
from config import (CENTER_FREQ, BANDWIDTH, SPREADING_FACTOR, CODING_RATE,
                    SYNC_WORD, HAS_CRC, IMPLICIT_HEADER, SAMP_RATE,
                    RF_GAIN, IF_GAIN, BB_GAIN)


class Transmitter(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self, "HackRF LoRa Transmitter")
        
        # TX chain blocks with correct constructor arguments
        # whitening(is_hex, use_length_tag, separator, length_tag_name)
        self.whitening = lora_sdr.whitening(False, False, ',', 'frame_len')
        # header(impl_head, has_crc, cr)
        self.header = lora_sdr.header(IMPLICIT_HEADER, HAS_CRC, CODING_RATE)
        # add_crc(has_crc)
        self.add_crc = lora_sdr.add_crc(HAS_CRC)
        # hamming_enc(cr, sf)
        self.hamming_enc = lora_sdr.hamming_enc(CODING_RATE, SPREADING_FACTOR)
        # interleaver(cr, sf, ldro, bw)
        self.interleaver = lora_sdr.interleaver(CODING_RATE, SPREADING_FACTOR, 0, BANDWIDTH)
        # gray_demap(sf)
        self.gray_decode = lora_sdr.gray_demap(SPREADING_FACTOR)
        # modulate(sf, samp_rate, bw, sync_words, inter_frame_padd, preamble_len)
        self.modulate = lora_sdr.modulate(SPREADING_FACTOR, SAMP_RATE, BANDWIDTH, [SYNC_WORD], 0, 8)
        
        self.osmosdr_sink = osmosdr.sink(args="hackrf=0")
        self.osmosdr_sink.set_sample_rate(SAMP_RATE)
        self.osmosdr_sink.set_center_freq(CENTER_FREQ, 0)
        self.osmosdr_sink.set_gain(RF_GAIN, 0)
        self.osmosdr_sink.set_if_gain(IF_GAIN, 0)
        self.osmosdr_sink.set_bb_gain(BB_GAIN, 0)
        
        # Stream connections for TX chain
        self.connect((self.whitening, 0), (self.header, 0))
        self.connect((self.header, 0), (self.add_crc, 0))
        self.connect((self.add_crc, 0), (self.hamming_enc, 0))
        self.connect((self.hamming_enc, 0), (self.interleaver, 0))
        self.connect((self.interleaver, 0), (self.gray_decode, 0))
        self.connect((self.gray_decode, 0), (self.modulate, 0))
        self.connect((self.modulate, 0), (self.osmosdr_sink, 0))
    
    def send(self, data):
        """Send a LoRaWAN downlink packet."""
        # Create payload as PMT message and send to whitening block
        payload = pmt.init_u8vector(len(data), list(data))
        msg = pmt.cons(pmt.PMT_NIL, payload)
        
        # Post message to whitening block's input message port
        self.whitening.to_basic_block()._post(pmt.intern("msg"), msg)
        
        self.start()
        self.wait()
        self.stop()