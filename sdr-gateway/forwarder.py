import time
import json
import struct
import socket
import base64
from enum import Enum
from config import CENTER_FREQ, BANDWIDTH, SPREADING_FACTOR, CODING_RATE, STAT

# LoRaWAN Packet headers
# https://github.com/Lora-net/packet_forwarder/blob/master/PROTOCOL.TXT
class Identifiers(Enum):
    PUSH_DATA = 0x00
    PUSH_ACK = 0x01
    PULL_DATA = 0x02
    PULL_RESP = 0x03
    PULL_ACK = 0x04
    TX_ACK = 0x05

class PacketForwarder:
    def __init__(self, host, port, gateway_eui):
        self.host = host
        self.port = port
        self.gateway_eui = gateway_eui
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.id = 0
        
    def _next_id(self):
        self.id = (self.id + 1) & 0xFFFF
        return self.id
    
    def send_push_data(self, packets):
        uid = self._next_id()
        
        rxpk = []
        for p in packets:
            rxpk.append({
                "tmst": int(time.time() * 1000000) & 0xFFFFFFFF, 
                "freq": CENTER_FREQ / 1e6,
                "chan": 0,
                "rfch": 0,
                "stat": 1 if p.get('crc_ok', True) else -1,
                "modu": "LORA",
                "datr": f"SF{SPREADING_FACTOR}BW{BANDWIDTH // 1000}",
                "codr": f"4/{4 + CODING_RATE}",
                "rssi": p.get('rssi', -60),
                "lsnr": p.get('snr', 10.0),
                "size": len(p['data']),
                "data": base64.b64encode(p['data']).decode('ascii')
            })
        
        json_data = json.dumps({"rxpk": rxpk})
        # We need
        # 1 byte protocol version (2)
        # 2 bytes random token (we do 1-up (obviously & FFFF for truncation))
        header = struct.pack('>BHB', 2, uid, Identifiers.PUSH_DATA.value) + self.gateway_eui
        message = header + json_data.encode('utf-8')
        
        self.sock.sendto(message, (self.host, self.port))
        print(f"Sent PUSH_DATA ({len(packets)} packets)")
        
    def send_keepalive(self):
        uid = self._next_id()
        # header is the
        header = struct.pack('>BHB', 2, uid, Identifiers.PULL_DATA.value) + self.gateway_eui
        self.sock.sendto(header, (self.host, self.port))
    
    def send_stat(self):
        uid = self._next_id()
        stat = STAT
        json_data = json.dumps(stat)
        header = struct.pack('>BHB', 2, uid, Identifiers.PUSH_DATA.value) + self.gateway_eui
        message = header + json_data.encode('utf-8')
        self.sock.sendto(message, (self.host, self.port))
