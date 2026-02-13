import asyncio
import ctypes
import json
import logging
import os
import struct

from util import notify
import aiomqtt
import grpc.aio
from chirpstack_api.api import device_pb2, device_pb2_grpc

# Shared state file so the gateway can read last-commanded TX power per node
TX_STATE_PATH = os.path.join(os.path.dirname(__file__), '..', 'gateway', 'tx_state.json')

def _write_tx_state(dev_eui: str, tx_power: int) -> None:
    """Atomically update the per-node TX power state file."""
    try:
        state = {}
        if os.path.exists(TX_STATE_PATH):
            with open(TX_STATE_PATH) as f:
                state = json.load(f)
        state[dev_eui] = tx_power
        tmp = TX_STATE_PATH + '.tmp'
        with open(tmp, 'w') as f:
            json.dump(state, f)
        os.replace(tmp, TX_STATE_PATH)
    except Exception:
        logger.exception('Failed to write TX state')

logging.basicConfig(
    level=logging.DEBUG if os.environ.get("DEBUG") else logging.INFO,
)
logger = logging.getLogger(__name__)

lib = ctypes.CDLL("./libalgo.so")


class Uplink(ctypes.Structure):
    _fields_ = [
        ("dr", ctypes.c_int),
        ("bw", ctypes.c_float),
        ("rxPower", ctypes.c_int),
        ("nodeId", ctypes.c_uint64),
    ]


class Command(ctypes.Structure):
    _fields_ = [
        ("dr", ctypes.c_int),
        ("txPower", ctypes.c_int),
        ("calculatedSNR", ctypes.c_double),
    ]


lib.pid.argtypes = [ctypes.POINTER(Uplink)]
lib.pid.restype = Command

DR_TO_SF = {0: 12, 1: 11, 2: 10, 3: 9, 4: 8, 5: 7}
SF_TO_DR = {v: k for k, v in DR_TO_SF.items()}

CHIRPSTACK_HOST = os.environ.get("CHIRPSTACK_HOST", "localhost:8080")
if ":" not in CHIRPSTACK_HOST:
    CHIRPSTACK_HOST += ":8080"
CHIRPSTACK_API_KEY = os.environ["CHIRPSTACK_API_KEY"]
DEV_EUIS: set[str] = set(os.environ["DEV_EUIS"].split(","))

MQTT_HOST = os.environ.get("MQTT_HOST", "localhost")
MQTT_PORT = int(os.environ.get("MQTT_PORT", "1883"))
APPLICATION_ID = os.environ["APPLICATION_ID"]


def _auth_metadata() -> list[tuple[str, str]]:
    return [("authorization", f"Bearer {CHIRPSTACK_API_KEY}")]


def dev_eui_to_u64(eui: str) -> int:
    """Convert a hex DevEUI string (e.g. '975c335979fc518c') to uint64."""
    return int(eui, 16)


async def send_downlink(
    device_client: device_pb2_grpc.DeviceServiceStub,
    dev_eui: str,
    cmd: Command,
    auth: list[tuple[str, str]],
) -> None:
    payload = struct.pack("<ii", cmd.dr, cmd.txPower)

    req = device_pb2.EnqueueDeviceQueueItemRequest(
        queue_item=device_pb2.DeviceQueueItem(
            dev_eui=dev_eui,
            confirmed=False,
            f_port=2,
            data=payload,
        ),
    )
    resp = await device_client.Enqueue(req, metadata=auth)
    logger.info("Enqueued downlink for %s (DR%d, TX %d dBm): id=%s",
                dev_eui, cmd.dr, cmd.txPower, resp.id)
    notify({
        'event': 'downlink_enqueued',
        'dev_eui': dev_eui,
        'dr': cmd.dr,
        'tx_power': cmd.txPower,
        'calculated_snr': cmd.calculatedSNR
    })


async def flush_queue(device_client, dev_eui, auth):
    req = device_pb2.FlushDeviceQueueRequest(dev_eui=dev_eui)
    await device_client.FlushQueue(req, metadata=auth)
    logger.info("Flushed queue for %s", dev_eui)


async def main() -> None:
    auth = _auth_metadata()
    channel = grpc.aio.insecure_channel(CHIRPSTACK_HOST)
    device_client = device_pb2_grpc.DeviceServiceStub(channel)

    topic = f"application/{APPLICATION_ID}/device/+/event/up"

    async with aiomqtt.Client(MQTT_HOST, MQTT_PORT) as mqtt:
        for dev_eui in DEV_EUIS:
            await flush_queue(device_client, dev_eui, auth)

        await mqtt.subscribe(topic)
        logger.info("Subscribed to %s", topic)

        async for msg in mqtt.messages:
            try:
                # Topic: application/{id}/device/{dev_eui}/event/up
                dev_eui = str(msg.topic).split("/")[3]
                if dev_eui not in DEV_EUIS:
                    continue

                event = json.loads(msg.payload)
                lora = event["txInfo"]["modulation"]["lora"]
                rssi: int = event["rxInfo"][0]["rssi"]
                sf, bw = lora["spreadingFactor"], lora["bandwidth"]
                dr = SF_TO_DR.get(sf, 0)

                logger.info("Device %s — SF%d (DR%d), BW: %d, RSSI: %d",
                            dev_eui, sf, dr, bw, rssi)

                uplink = Uplink(
                    dr=dr,
                    bw=float(bw),
                    rxPower=rssi,
                    nodeId=dev_eui_to_u64(dev_eui),
                )
                cmd: Command = lib.pid(ctypes.byref(uplink))

                logger.info("PID → DR%d (SF%d), TX %d dBm for %s",
                            cmd.dr, DR_TO_SF.get(cmd.dr, 0), cmd.txPower, dev_eui)

                await flush_queue(device_client, dev_eui, auth)
                await send_downlink(device_client, dev_eui, cmd, auth)
                _write_tx_state(dev_eui, cmd.txPower)

            except Exception:
                logger.exception("Error processing message on %s", msg.topic)


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        exit(0)