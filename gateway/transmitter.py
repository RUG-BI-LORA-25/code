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
    """Wrapper around GnuRadio LoRa TX flowgraph.
    """

    def __init__(self):
        self._tb = None
        self._sf = None

    def prepare(self, sf=None):
        """Pre-create the TX flowgraph and open the HackRF sink."""
        sf = sf or SPREADING_FACTOR
        self._sf = sf
        self._tb = _TxFlowgraph(sf)
        self._tb.start()

    def transmit(self, data):
        """Post data to an already-running flowgraph and wait for airtime"""
        if self._tb is None:
            raise RuntimeError("transmit() called without prepare()")
        sf = self._sf
        hex_str = data.hex()
        msg = pmt.intern(hex_str)
        self._tb.whitening.to_basic_block()._post(pmt.intern("msg"), msg)

        tx_time = self._airtime(len(data), sf)
        print(f"    TX: {len(data)} bytes, SF{sf}, ~{tx_time:.1f}s airtime")
        time.sleep(tx_time)
        self._tb.stop()
        self._tb.wait()
        del self._tb
        self._tb = None

    def send(self, data, sf=None):
        """Legacy one-shot send (prepare + transmit in one call)."""
        sf = sf or SPREADING_FACTOR
        self.prepare(sf)
        time.sleep(0.05)
        self.transmit(data)

    @staticmethod
    def _airtime(n_bytes, sf):
        symbol_time = (2 ** sf) / BANDWIDTH
        n_payload_symbols = max(8, int(np.ceil(
            (8 * n_bytes + 28 - 4 * sf) /
            (4 * sf)
        )) * (CODING_RATE + 4))
        padding_time = 20 * (2**sf) / BANDWIDTH
        return (8 + 4.25 + n_payload_symbols) * symbol_time + padding_time + 0.5


class _TxFlowgraph(gr.top_block):
    def __init__(self, sf):
        gr.top_block.__init__(self, "HackRF LoRa Transmitter")

        
        self.whitening = lora_sdr.whitening(True, False, ',', 'frame_len')
        
        self.header = lora_sdr.header(False, False, CODING_RATE)
        self.add_crc = lora_sdr.add_crc(False)
        self.hamming_enc = lora_sdr.hamming_enc(CODING_RATE, sf)
        self.interleaver = lora_sdr.interleaver(CODING_RATE, sf, 2, BANDWIDTH)  # ldro=2 (AUTO)
        self.gray_decode = lora_sdr.gray_demap(sf)
        self.modulate = lora_sdr.modulate(sf, SAMP_RATE, BANDWIDTH, [SYNC_WORD],
                                          int(20 * 2**sf * SAMP_RATE / BANDWIDTH), 8)
        self.modulate.set_min_output_buffer(10_000_000)

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