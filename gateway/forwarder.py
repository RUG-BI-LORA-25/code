import time
import json
import struct
import socket
import base64
import logging
from enum import Enum
from config import CENTER_FREQ, BANDWIDTH, SPREADING_FACTOR, CODING_RATE, STAT, DL_DELAY_S, TMST_OFFSET_US

logger = logging.getLogger(__name__)

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
        self.uid = 0
        self.gateway_eui = gateway_eui
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.setblocking(False)
        self.transmitter = None
        self._tmst_epoch = time.monotonic()
        self._last_uplink_mono = None  # monotonic time of last uplink
    
    def set_transmitter(self, tx):
        self.transmitter = tx
    
    def set_receiver(self, rx_start, rx_stop):
        """Set callbacks to stop/start the receiver for half-duplex TX."""
        self._rx_start = rx_start
        self._rx_stop = rx_stop
    
    def check_downlink(self):
        """Drain all pending responses from ChirpStack. Handle any PULL_RESPs."""
        while True:
            try:
                data, _ = self.sock.recvfrom(4096)
            except BlockingIOError:
                break
            if len(data) < 4:
                continue
            ident = data[3]
            logger.debug(f"[UDP] Received packet: ident=0x{ident:02x} ({len(data)} bytes)")
            if ident == Identifiers.PULL_RESP.value:
                logger.info(f"[UDP] Got PULL_RESP ({len(data)} bytes)")
                token = struct.unpack('>H', data[1:3])[0]
                self._handle_downlink(token, data[4:])
            elif ident == Identifiers.PUSH_ACK.value:
                pass  # expected after PUSH_DATA
            elif ident == Identifiers.PULL_ACK.value:
                pass  # expected after PULL_DATA
    
    def _handle_downlink(self, token, payload):
        try:
            msg = json.loads(payload.decode())
            if 'txpk' not in msg:
                return
            txpk = msg['txpk']
            data = base64.b64decode(txpk['data'])

            # Log the full txpk so we can see what ChirpStack wants
            logger.info(f"[DL] txpk: freq={txpk.get('freq')}MHz, datr={txpk.get('datr')}, "
                  f"tmst={txpk.get('tmst')}, powe={txpk.get('powe')}, "
                  f"modu={txpk.get('modu')}, size={txpk.get('size')}")

            # ---- Fixed delay from last uplink ----
            # We ignore ChirpStack's tmst scheduling entirely because the
            # HackRF has no hardware-timed TX.  Instead we target a fixed
            # wall-clock delay (DL_DELAY_S) after we received the uplink.
            t_target = None
            if self._last_uplink_mono is not None:
                t_target = self._last_uplink_mono + DL_DELAY_S

            # Stop receiver (this takes a while with SF12)
            t_before_stop = time.monotonic()
            if hasattr(self, '_rx_stop'):
                self._rx_stop()
            stop_took = time.monotonic() - t_before_stop

            # Parse SF from datr field (e.g. "SF9BW125" -> 9)
            tx_sf = None
            datr = txpk.get('datr', '')
            if datr.startswith('SF') and 'BW' in datr:
                try:
                    tx_sf = int(datr[2:datr.index('BW')])
                except ValueError:
                    pass

            if self.transmitter:
                try:
                    self.transmitter.prepare(sf=tx_sf)
                except Exception as e:
                    logger.error(f"[DL] TX prepare error: {e}")

            prep_took = time.monotonic() - t_before_stop - stop_took

            # Sleep until the target time
            if t_target is not None:
                remaining = t_target - time.monotonic()
                if remaining > 0:
                    logger.info(f"[DL] RX stop {stop_took:.2f}s + TX prep {prep_took:.2f}s, "
                                f"sleeping {remaining:.2f}s to hit DL_DELAY={DL_DELAY_S}s")
                    time.sleep(remaining)
                else:
                    logger.warning(f"[DL] RX stop {stop_took:.2f}s + TX prep {prep_took:.2f}s "
                                   f"— already {-remaining:.2f}s past target (DL_DELAY={DL_DELAY_S}s)")
            else:
                logger.warning("[DL] No uplink timestamp — TX immediately after prep")

            total_elapsed = time.monotonic() - (self._last_uplink_mono or t_before_stop)
            logger.info(f"[DL] TX at {total_elapsed:.2f}s after uplink")

            error = ""
            if self.transmitter:
                try:
                    logger.debug(f"[DL] TX payload: {data.hex()}")
                    self.transmitter.transmit(data)
                    logger.info(f"[DL] Transmitted {len(data)} bytes (SF{tx_sf})")
                except Exception as e:
                    error = str(e)
                    logger.error(f"[DL] TX error: {e}")
            else:
                error = "NO_TRANSMITTER"

            if hasattr(self, '_rx_start'):
                self._rx_start()
            self._send_tx_ack(token, error)
        except Exception as e:
            logger.error(f"Downlink error: {e}", exc_info=True)
            self._send_tx_ack(token, str(e))

    def _send_tx_ack(self, token, error=""):
        """Send TX_ACK to ChirpStack so it pops the downlink from the queue."""
        header = struct.pack('>BHB', 2, token, Identifiers.TX_ACK.value) + self.gateway_eui
        if error:
            ack_payload = json.dumps({"txpk_ack": {"error": error}}).encode()
        else:
            ack_payload = json.dumps({"txpk_ack": {"error": "NONE"}}).encode()
        self.sock.sendto(header + ack_payload, (self.host, self.port))
        logger.debug(f"[DL] TX_ACK sent (token={token}, error={error or 'NONE'})")

    def send_pull_data(self):
        packet = struct.pack('>BHB', 0x02, self._next_id(), Identifiers.PULL_DATA.value) + self.gateway_eui
        self.sock.sendto(packet, (self.host, self.port))

    def _next_id(self):
        self.uid = (self.uid + 1) & 0xFFFF
        return self.uid
    
    def _get_tmst(self):
        """Microsecond counter since gateway start, wrapping at 32 bits."""
        return int((time.monotonic() - self._tmst_epoch) * 1_000_000) & 0xFFFFFFFF

    def send_push_data(self, packets):
        uid = self._next_id()
        self._last_uplink_mono = time.monotonic()
        
        rxpk = []
        for p in packets:
            sf = p.get('sf', SPREADING_FACTOR)
            tmst = (self._get_tmst() - TMST_OFFSET_US) & 0xFFFFFFFF
            rxpk.append({
                "tmst": tmst,
                "freq": CENTER_FREQ / 1e6,
                "chan": 0,
                "rfch": 0,
                "stat": 1 if p.get('crc_ok', True) else -1,
                "modu": "LORA",
                "datr": f"SF{sf}BW{BANDWIDTH // 1000}",
                "codr": f"4/{4 + CODING_RATE}",
                "rssi": int(round(p.get('rssi', -60))),
                "lsnr": round(p.get('snr', 10.0), 1),
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
        logger.info(f"Sent PUSH_DATA ({len(packets)} packets)")
        
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
