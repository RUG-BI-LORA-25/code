#!/usr/bin/env python3
import time
import struct
import threading
from datetime import datetime
from config import (CENTER_FREQ, SPREADING_FACTOR, BANDWIDTH,
                    CHIRPSTACK_HOST, CHIRPSTACK_PORT, GATEWAY_EUI)
from receiver import Receiver
from forwarder import PacketForwarder
from transmitter import Transmitter
# we assume u have hackrf_transfer
# sike, we check for it
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

    # transmit
    import subprocess
    # cjheck for cartof.hackrf file
    import os
    cartof_file = os.path.join(os.path.dirname(__file__), 'cartof.iqhackrf')
    if os.path.isfile(cartof_file):
        print("Starting cartof transmission...")
        subprocess.run([
            'hackrf_transfer',
            '-t', cartof_file,
            '-f', str(int(CENTER_FREQ)),
            '-s', '2000000',
            '-x', '20',
            '-a', '1'
        ], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    else:
        print("cartof.iqhackrf file not found, skipping transmission.")

def print_packet_info(data, crc_ok, count):
    timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
    
    print(f"\n[{timestamp}]")
    print(f" {len(data)} bytes")
    


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
    forwarder.set_transmitter(transmitter)
    packet_count = [0]
    receiver = [None]  # mutable ref so we can stop/restart
    
    def on_packet(data, crc_ok):
        packet_count[0] += 1
        print_packet_info(data, crc_ok, packet_count[0])
        forwarder.send_push_data([{'data': data, 'crc_ok': crc_ok, 'rssi': -60, 'snr': 10.0}])
    
    def start_receiver():
        rx = Receiver(on_packet)
        rx.start()
        receiver[0] = rx
        print("    [RX] Receiver started")
    
    def stop_receiver():
        if receiver[0]:
            print("    [RX] Stopping receiver for TX...")
            receiver[0].stop()
            receiver[0].wait()
            receiver[0] = None
    
    forwarder.set_receiver(start_receiver, stop_receiver)
    
    print("\nStarting gateway... Press Ctrl+C to stop\n")
    
    forwarder.send_stat()
    forwarder.send_pull_data()
    start_receiver()
    
    # Send PULL_DATA every 10s so ChirpStack knows we're alive.
    # Send stats alongside.
    def keepalive_loop():
        while True:
            time.sleep(10)
            forwarder.send_stat()
            forwarder.send_pull_data()
    threading.Thread(target=keepalive_loop, daemon=True).start()

    # Poll for downlinks frequently — ChirpStack sends PULL_RESP
    # shortly after receiving our PUSH_DATA uplink.
    def downlink_loop():
        while True:
            forwarder.check_downlink()
            time.sleep(0.1)
    threading.Thread(target=downlink_loop, daemon=True).start()
    
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\n\nStopping gateway...")
    
    if receiver[0]:
        receiver[0].stop()
        receiver[0].wait()
    print(f"Total packets received: {packet_count[0]}")


if __name__ == "__main__":
    main()
