from datetime import datetime
CENTER_FREQ = 433.175e6
BANDWIDTH = 125000
SPREADING_FACTOR = 12
CODING_RATE = 1
SYNC_WORD = 0x34
HAS_CRC = True
IMPLICIT_HEADER = False
SAMP_RATE = 250000

RF_GAIN = 40
IF_GAIN = 40
BB_GAIN = 20

CHIRPSTACK_HOST = "192.168.178.212"
CHIRPSTACK_PORT = 1700
GATEWAY_EUI = bytes([0x64, 0xb7, 0x08, 0xff, 0xff, 0x8a, 0x0a, 0x74])

STAT = {
    "stat": {
        "time": datetime.now().strftime("%Y-%m-%d %H:%M:%S GMT"),
        "lati": 53.240532,
        "long": 6.536756,
        "alti": 14,
        "rxnb": 0,
        "rxok": 0,
        "rxfw": 0,
        "ackr": 100.0,
        "dwnb": 0,
        "txnb": 0,
    }
}