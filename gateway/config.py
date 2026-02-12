import math
import os
from datetime import datetime

CENTER_FREQ = float(os.environ.get('CENTER_FREQ', 433.175e6))
BANDWIDTH = int(os.environ.get('BANDWIDTH', 125000))

HACKRF_NF_DB = float(os.environ.get('HACKRF_NF_DB', 11.0))

#   noise_floor = -174 + 10*log10(BANDWIDTH) + NF
NOISE_FLOOR_DBM = -174.0 + 10.0 * math.log10(BANDWIDTH) + HACKRF_NF_DB
SPREADING_FACTOR = int(os.environ.get('SPREADING_FACTOR', 12))
CODING_RATE = int(os.environ.get('CODING_RATE', 1))
SYNC_WORD = int(os.environ.get('SYNC_WORD', 0x34))
HAS_CRC = os.environ.get('HAS_CRC', 'True').lower() in ('true', '1', 'yes')
IMPLICIT_HEADER = os.environ.get('IMPLICIT_HEADER', 'False').lower() in ('true', '1', 'yes')
SAMP_RATE = int(os.environ.get('SAMP_RATE', 250000))

RF_GAIN = int(os.environ.get('RF_GAIN', 40))
IF_GAIN = int(os.environ.get('IF_GAIN', 40))
BB_GAIN = int(os.environ.get('BB_GAIN', 20))

# Fixed delay (seconds) between receiving an uplink and transmitting the
# downlink.  Must be tuned so the RF arrives when the node's RX1 window is
# open (node opens RX1 at 5.5 s after its own TX end).  Accounts for
# RX-stop + TX-prep + USB buffering latency.  Env-overridable per machine.
DL_DELAY_S = float(os.environ.get('DL_DELAY_S', 5.0))

CHIRPSTACK_HOST = os.environ.get('CHIRPSTACK_HOST', '192.168.178.212')
CHIRPSTACK_PORT = int(os.environ.get('CHIRPSTACK_PORT', 1700))
GATEWAY_EUI = bytes.fromhex(os.environ.get('GATEWAY_EUI', '64b708ffff8a0a74'))

UPLINK_INTERVAL_S = int(os.environ.get('UPLINK_INTERVAL_S', 60))
TDMA_NUM_SLOTS = int(os.environ.get('TDMA_NUM_SLOTS', 2))

GATEWAY_LAT = float(os.environ.get('GATEWAY_LAT', 53.240532))
GATEWAY_LON = float(os.environ.get('GATEWAY_LON', 6.536756))
GATEWAY_ALT = int(os.environ.get('GATEWAY_ALT', 14))

STAT = {
    "stat": {
        "time": datetime.now().strftime("%Y-%m-%d %H:%M:%S GMT"),
        "lati": GATEWAY_LAT,
        "long": GATEWAY_LON,
        "alti": GATEWAY_ALT,
        "rxnb": 0,
        "rxok": 0,
        "rxfw": 0,
        "ackr": 100.0,
        "dwnb": 0,
        "txnb": 0,
    }
}