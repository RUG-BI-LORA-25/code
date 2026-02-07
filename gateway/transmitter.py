from gnuradio import gr, blocks
import numpy as np
import pmt
import osmosdr
import time
from gnuradio.lora_sdr import lora_sdr_python as lora_sdr
from config import (CENTER_FREQ, BANDWIDTH, SPREADING_FACTOR, CODING_RATE,
                    SYNC_WORD, HAS_CRC, IMPLICIT_HEADER, SAMP_RATE,
                    RF_GAIN, IF_GAIN, BB_GAIN)

class Transmitter:
    """Wrapper that creates a fresh GnuRadio flowgraph per TX burst."""

    def send(self, data, sf=None):
        sf = sf or SPREADING_FACTOR
        tb = _TxFlowgraph(sf)

        # The whitening block with is_hex=True parses every 2 chars as a hex
        # byte (e.g. "60f306fa01..."), so pass a contiguous hex string.
        hex_str = data.hex()
        
        msg = pmt.intern(hex_str)
        
        tb.whitening.to_basic_block()._post(pmt.intern("msg"), msg)

        tb.start()
        
        symbol_time = (2 ** sf) / BANDWIDTH
        n_payload_symbols = max(8, int(np.ceil(
            (8 * len(data) + 28 - 4 * sf) /
            (4 * sf)
        )) * (CODING_RATE + 4))
        # Account for preamble + payload + inter_frame_padding flush time
        padding_time = 20 * (2**sf) / BANDWIDTH  # matches modulate's padding
        tx_time = (8 + 4.25 + n_payload_symbols) * symbol_time + padding_time + 0.5
        
        print(f"    TX: {len(data)} bytes, SF{sf}, ~{tx_time:.1f}s airtime")
        time.sleep(tx_time)
        tb.stop()
        tb.wait()


class _TxFlowgraph(gr.top_block):
    def __init__(self, sf):
        gr.top_block.__init__(self, "HackRF LoRa Transmitter")

        # Config: is_hex=True, has_crc=False, delimiter=',', len_tag_name='frame_len'
        # We must keep is_hex=True so it accepts the string we generate above.
        self.whitening = lora_sdr.whitening(True, False, ',', 'frame_len')
        
        self.header = lora_sdr.header(False, False, CODING_RATE)
        self.add_crc = lora_sdr.add_crc(False)
        self.hamming_enc = lora_sdr.hamming_enc(CODING_RATE, sf)
        self.interleaver = lora_sdr.interleaver(CODING_RATE, sf, 2, BANDWIDTH)  # ldro=2 (AUTO)
        self.gray_decode = lora_sdr.gray_demap(sf)
        self.modulate = lora_sdr.modulate(sf, SAMP_RATE, BANDWIDTH, [SYNC_WORD],
                                          int(20 * 2**sf * SAMP_RATE / BANDWIDTH), 8)
        self.modulate.set_min_output_buffer(10_000_000)

        # Conjugate the signal to invert IQ (Standard for LoRaWAN Downlinks)
        # NOTE: IQ inversion = conjugation, NOT multiplication by -j.
        # conj(I+jQ) = I-jQ  (flips upchirps↔downchirps correctly)
        # (-j)*(I+jQ) = Q-jI  (wrong — rotates 90° instead of inverting)
        self.iq_invert = blocks.conjugate_cc()

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
        self.connect((self.modulate, 0), (self.iq_invert, 0))
        self.connect((self.iq_invert, 0), (self.osmosdr_sink, 0))