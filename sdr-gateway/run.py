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
    
    def on_packet(data, crc_ok):
        packet_count[0] += 1
        print_packet_info(data, crc_ok, packet_count[0])
        forwarder.send_push_data([{'data': data, 'crc_ok': crc_ok, 'rssi': -60, 'snr': 10.0}])
    
    receiver = Receiver(on_packet)
    
    print("\nStarting gateway... Press Ctrl+C to stop\n")
    
    forwarder.send_stat()
    forwarder.send_keepalive()
    receiver.start()
    
    def keepalive_loop():
        while True:
            time.sleep(10)
            forwarder.send_stat()
            forwarder.send_keepalive()
            forwarder.send_pull_data()
            forwarder.check_downlink()
    threading.Thread(target=keepalive_loop, daemon=True).start()
    
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\n\nStopping gateway...")
    
    receiver.stop()
    receiver.wait()
    print(f"Total packets received: {packet_count[0]}")


if __name__ == "__main__":
    main()
