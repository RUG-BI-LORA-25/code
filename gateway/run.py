#!/usr/bin/env python3
import time
import threading
from datetime import datetime
from config import (CENTER_FREQ, SPREADING_FACTOR, BANDWIDTH,
                    CHIRPSTACK_HOST, CHIRPSTACK_PORT, GATEWAY_EUI)
from receiver import Receiver
from forwarder import PacketForwarder
from transmitter import Transmitter

if not __import__('shutil').which('hackrf_transfer'):
    print("Please install hackrf tools (hackrf_transfer)")
    exit(1)

def cartof():
    print("""
▗▞▀▘▗▞▀▜▌ ▄▄▄ ■   ▄▄▄  ▗▞▀▀▘
▝▚▄▖▝▚▄▟▌█ ▗▄▟▙▄▖█   █ ▐▌   
         █   ▐▌  ▀▄▄▄▀ ▐▛▀▘ 
             ▐▌        ▐▌   
             ▐▌             
    """)
    import subprocess, os
    cartof_file = os.path.join(os.path.dirname(__file__), 'cartof.iqhackrf')
    if os.path.isfile(cartof_file):
        print("Starting cartof transmission...")
        subprocess.run([
            'hackrf_transfer', '-t', cartof_file,
            '-f', str(int(CENTER_FREQ)), '-s', '2000000', '-x', '20', '-a', '1'
        ], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    else:
        print("cartof.iqhackrf file not found, skipping transmission.")


def main():
    cartof()
    print("=" * 60)
    print("  HackRF LoRa Gateway")
    print("=" * 60)
    print(f"  Frequency: {CENTER_FREQ/1e6:.3f} MHz")
    print(f"  SF: {SPREADING_FACTOR}")
    print(f"  Bandwidth: {BANDWIDTH/1000} kHz")
    print(f"  Gateway EUI: {GATEWAY_EUI.hex()}")
    print("=" * 60)

    forwarder = PacketForwarder(CHIRPSTACK_HOST, CHIRPSTACK_PORT, GATEWAY_EUI)
    transmitter = Transmitter()

    packet_count = [0]
    radio_lock = threading.Lock()
    receiver_ref = [None]

    def start_receiver(on_pkt):
        rx = Receiver(on_pkt)
        rx.start()
        return rx

    def stop_receiver():
        """Stop the receiver (half-duplex: must stop RX before TX)."""
        with radio_lock:
            if receiver_ref[0]:
                receiver_ref[0].stop()
                receiver_ref[0].wait()
                receiver_ref[0] = None

    def start_receiver_again():
        """Restart the receiver after TX completes."""
        with radio_lock:
            receiver_ref[0] = start_receiver(on_packet)

    # Give the forwarder the real transmitter + RX stop/start callbacks
    forwarder.set_transmitter(transmitter)
    forwarder.set_receiver(start_receiver_again, stop_receiver)

    def on_packet(data, crc_ok):
        packet_count[0] += 1
        ts = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        print(f"\n[{ts}] RX: {len(data)} bytes")

        # TODO: rssi snr retrieving
        forwarder.send_push_data([{
            'data': data, 'crc_ok': crc_ok, 'rssi': -60, 'snr': 10.0
        }])

    print("\nStarting gateway... Press Ctrl+C to stop\n")

    forwarder.send_stat()
    forwarder.send_keepalive()
    forwarder.send_pull_data()
    receiver_ref[0] = start_receiver(on_packet)

    def keepalive_loop():
        while True:
            time.sleep(5)
            forwarder.send_stat()
            forwarder.send_keepalive()
            forwarder.send_pull_data()

    def downlink_loop():
        while True:
            forwarder.check_downlink()
            time.sleep(0.01)

    threading.Thread(target=keepalive_loop, daemon=True).start()
    threading.Thread(target=downlink_loop, daemon=True).start()

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\n\nStopping gateway...")

    if receiver_ref[0]:
        receiver_ref[0].stop()
        receiver_ref[0].wait()
    print(f"Total packets received: {packet_count[0]}")


if __name__ == "__main__":
    main()