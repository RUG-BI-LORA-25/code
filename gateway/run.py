#!/usr/bin/env python3
import csv
import json
import os
import time
import logging
import threading
from datetime import datetime
from config import (CENTER_FREQ, BANDWIDTH,
                    CHIRPSTACK_HOST, CHIRPSTACK_PORT, GATEWAY_EUI,
                    UPLINK_INTERVAL_S, TDMA_NUM_SLOTS)
from util import notify
from receiver import Receiver
from forwarder import PacketForwarder
from transmitter import Transmitter

# Shared state file written by the PD controller (algo/wrapper.py)
# Maps DevEUI -> last-commanded TX power (dBm)
TX_STATE_PATH = os.path.join(os.path.dirname(__file__), 'tx_state.json')
DEFAULT_TX_POWER = 14  # initial node TX power before any PD command

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [%(levelname)s] %(name)s: %(message)s',
    datefmt='%H:%M:%S'
)
logger = logging.getLogger(__name__)

if not __import__('shutil').which('hackrf_transfer'):
    logger.error("Please install hackrf tools (hackrf_transfer)")
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
        logger.info("Starting cartof transmission...")
        subprocess.run([
            'hackrf_transfer', '-t', cartof_file,
            '-f', str(int(CENTER_FREQ)), '-s', '2000000', '-x', '20', '-a', '1'
        ], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    else:
        logger.warning("cartof.iqhackrf file not found, skipping transmission.")


def main():
    cartof()
    print("=" * 60)
    print("  HackRF LoRa Gateway")
    print("=" * 60)
    print(f"  Frequency: {CENTER_FREQ/1e6:.3f} MHz")
    print(f"  RX SF: 7-12 (multi-SF)")
    print(f"  Bandwidth: {BANDWIDTH/1000} kHz")
    print(f"  Gateway EUI: {GATEWAY_EUI.hex()}")
    print("=" * 60)

    forwarder = PacketForwarder(CHIRPSTACK_HOST, CHIRPSTACK_PORT, GATEWAY_EUI)
    transmitter = Transmitter()

    packet_count = [0]
    radio_lock = threading.Lock()
    receiver_ref = [None]

    # --- per-node CSV logging (one file per TDMA slot) ---
    slot_interval_s = UPLINK_INTERVAL_S / TDMA_NUM_SLOTS
    csv_dir = os.path.dirname(__file__)
    csv_files = {}
    csv_writers = {}
    for slot in range(TDMA_NUM_SLOTS):
        path = os.path.join(csv_dir, f'packets_node{slot}.csv')
        f = open(path, 'w', newline='')
        w = csv.writer(f)
        w.writerow(['time', 'snr', 'rssi', 'sf', 'tx_power'])
        f.flush()
        csv_files[slot] = f
        csv_writers[slot] = w

    # DevEUI -> last known TX power, read from the shared state file
    _tx_power_cache: dict[str, int] = {}
    _tx_state_mtime: list[float] = [0.0]

    def _read_tx_power(dev_eui: str | None = None) -> int:
        """Read the last-commanded TX power from the shared state file.
        Returns DEFAULT_TX_POWER if unknown."""
        try:
            mtime = os.path.getmtime(TX_STATE_PATH)
            if mtime != _tx_state_mtime[0]:
                _tx_state_mtime[0] = mtime
                with open(TX_STATE_PATH) as f:
                    _tx_power_cache.update(json.load(f))
        except (FileNotFoundError, json.JSONDecodeError):
            pass
        if dev_eui and dev_eui in _tx_power_cache:
            return _tx_power_cache[dev_eui]
        # If no specific DevEUI, return the most recently written value
        if _tx_power_cache:
            return list(_tx_power_cache.values())[-1]
        return DEFAULT_TX_POWER

    def _tdma_slot(ts: datetime) -> int:
        """Determine which TDMA slot a packet belongs to based on its
        arrival time within the uplink interval cycle."""
        epoch_s = ts.timestamp()
        pos_in_cycle = epoch_s % UPLINK_INTERVAL_S  # 0..59.999
        return int(pos_in_cycle // slot_interval_s)

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

    def start_receiver_again():
        """Restart the same receiver after TX completes."""
        with radio_lock:
            if receiver_ref[0]:
                receiver_ref[0].start()
            else:
                receiver_ref[0] = start_receiver(on_packet)

    # Give the forwarder the real transmitter + RX stop/start callbacks
    forwarder.set_transmitter(transmitter)
    forwarder.set_receiver(start_receiver_again, stop_receiver)

    def on_packet(data, crc_ok, sf=12, rssi=-60.0, snr=0.0):
        packet_count[0] += 1
        now = datetime.now()
        slot = _tdma_slot(now)

        # Try to extract DevAddr from LoRaWAN PHYPayload (bytes 1-4, LE)
        dev_eui = None
        if len(data) >= 5:
            dev_addr = int.from_bytes(data[1:5], 'little')
            # Look up DevEUI by DevAddr if we had a mapping;
            # for now just pass None and use slot-based fallback
            _ = dev_addr

        tx_power = _read_tx_power(dev_eui)
        logger.info(f"RX: {len(data)} bytes (SF{sf}) RSSI={rssi:.1f}dBm "
                    f"SNR={snr:.1f}dB TX={tx_power}dBm  [node{slot}]")

        csv_writers[slot].writerow([now.isoformat(), round(snr, 1), round(rssi, 1), sf, tx_power])
        csv_files[slot].flush()
        # notify
        notify({
            'event': 'packet_received',
            'time': now.isoformat(),
            'snr': round(snr, 1),
            'rssi': round(rssi, 1),
            'sf': sf,
            'tx_power': tx_power
        })
        forwarder.send_push_data([{
            'data': data, 'crc_ok': crc_ok, 'rssi': rssi, 'snr': snr, 'sf': sf
        }])

    logger.info("Starting gateway... Press Ctrl+C to stop")

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
        logger.info("Stopping gateway...")

    if receiver_ref[0]:
        receiver_ref[0].stop()
        receiver_ref[0].wait()
    for f in csv_files.values():
        f.close()
    logger.info(f"Total packets received: {packet_count[0]}")


if __name__ == "__main__":
    main()