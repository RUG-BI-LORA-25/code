import asyncio
import ctypes
import json
import logging
import os
import struct

import aiomqtt
import grpc.aio
from chirpstack_api.api import device_pb2, device_pb2_grpc

logging.basicConfig(
    level=logging.DEBUG if os.environ.get("DEBUG") else logging.INFO,
)
logger = logging.getLogger(__name__)

lib = ctypes.CDLL("./libalgo.so")


class State(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("sf", ctypes.c_uint8),
        ("bw", ctypes.c_float),
        ("pwr", ctypes.c_int8),
    ]


lib.pid.argtypes = [ctypes.POINTER(State)]
lib.pid.restype = None

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


async def send_downlink(
    device_client: device_pb2_grpc.DeviceServiceStub,
    dev_eui: str,
    sf: int,
    bw: float,
    pwr: int,
    auth: list[tuple[str, str]],
) -> None:
    payload = struct.pack("<Bfb", sf, bw, pwr)
    req = device_pb2.EnqueueDeviceQueueItemRequest(
        queue_item=device_pb2.DeviceQueueItem(
            dev_eui=dev_eui,
            confirmed=False,
            f_port=2,
            data=payload,
        ),
    )
    resp = await device_client.Enqueue(req, metadata=auth)
    logger.info("Enqueued downlink for %s: id=%s", dev_eui, resp.id)

    # Immediately check what's in the queue
    queue_req = device_pb2.GetDeviceQueueItemsRequest(dev_eui=dev_eui)
    queue_resp = await device_client.GetQueue(queue_req, metadata=auth)
    logger.info("Queue for %s has %d items:", dev_eui, len(queue_resp.result))
    for item in queue_resp.result:
        logger.info("  port=%d data=%s pending=%s", item.f_port, item.data.hex(), item.is_pending)

async def flush_queue(device_client, dev_eui, auth):
    req = device_pb2.FlushDeviceQueueRequest(dev_eui=dev_eui)
    await device_client.FlushQueue(req, metadata=auth)
    logger.info("Flushed queue for %s", dev_eui)

async def main() -> None:
    auth = _auth_metadata()
    channel = grpc.aio.insecure_channel(CHIRPSTACK_HOST)
    device_client = device_pb2_grpc.DeviceServiceStub(channel)

    # Single wildcard topic covers all devices in the application.
    topic = f"application/{APPLICATION_ID}/device/+/event/up"

    async with aiomqtt.Client(MQTT_HOST, MQTT_PORT) as mqtt:
        # flush qeueues for all devices at startup
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

                logger.debug("Device %s — SF: %d, BW: %d, RSSI: %d", dev_eui, sf, bw, rssi)

                state = State(sf=sf, bw=float(bw), pwr=rssi)
                lib.pid(ctypes.byref(state))

                await send_downlink(device_client, dev_eui, state.sf, state.bw, state.pwr, auth)

            except Exception:
                logger.exception("Error processing message on %s", msg.topic)


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        ...
        