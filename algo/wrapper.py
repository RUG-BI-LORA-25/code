# pyright:basic

import ctypes
import json
import logging
import os
from typing import Any

from chirpstack_api.api import internal_pb2, internal_pb2_grpc
from chirpstack_api.api.internal_pb2 import StreamDeviceFramesRequest
import grpc

# set up logg
logging.basicConfig(level=logging.INFO if not os.environ.get("DEBUG") else logging.DEBUG)

lib = ctypes.CDLL("./libalgo.so")


class State(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("sf", ctypes.c_uint8),
        ("bw", ctypes.c_float),
        ("pwr", ctypes.c_int),
    ]


lib.pid.argtypes = [ctypes.POINTER(State)]
lib.pid.restype = None

CHIRPSTACK_HOST = os.environ.get("CHIRPSTACK_HOST", "localhost:8080")
if ":" not in CHIRPSTACK_HOST:
    CHIRPSTACK_HOST += ":8080"
CHIRPSTACK_API_KEY = os.environ["CHIRPSTACK_API_KEY"]
DEV_EUIS = os.environ["DEV_EUIS"].split(",")

# https://github.com/chirpstack/chirpstack/blob/master/api/proto/api/internal.proto#L43
def get_latest_uplink(client, dev_eui, auth) -> tuple[Any, Any, Any]:
    req: StreamDeviceFramesRequest = internal_pb2.StreamDeviceFramesRequest(dev_eui=dev_eui)
    for frame in client.StreamDeviceFrames(req, metadata=auth):
        body = json.loads(frame.body)
        lora = body["tx_info"]["modulation"]["lora"]
        rssi = body["rx_info"][0]["rssi"]
        return lora["spreadingFactor"], lora["bandwidth"], rssi
    raise Exception("No uplink frames found")

import struct
from chirpstack_api.api import device_pb2, device_pb2_grpc

def send_downlink(client, dev_eui, sf, bw, pwr, auth):
    # SF (1 byte) | BW (4 bytes, float32 LE) | Power (1 byte, signed)
    payload = struct.pack("<Bfi", sf, bw, pwr)
    device_client = device_pb2_grpc.DeviceServiceStub(client._channel)

    req = device_pb2.EnqueueDeviceQueueItemRequest(
        queue_item=device_pb2.DeviceQueueItem(
            dev_eui=dev_eui,
            confirmed=False,
            f_port=2,
            data=payload,
        ),
        flush_queue=True,
    )

    resp = device_client.Enqueue(req, metadata=auth)
    logging.info(f"Downlink for {dev_eui}: id={resp.id} (SF={sf}, BW={bw}, PWR={pwr})")

def main():
    auth = [("authorization", f"Bearer {CHIRPSTACK_API_KEY}")]
    channel = grpc.insecure_channel(CHIRPSTACK_HOST)
    
    internal = internal_pb2_grpc.InternalServiceStub(channel)
    current_uplinks = [(None, None, None)] * len(DEV_EUIS)
    while True:
        for i, dev_eui in enumerate(DEV_EUIS):
            sf, bw, rssi = get_latest_uplink(internal, dev_eui, auth)
            if (sf, bw, rssi) == current_uplinks[i]:
                continue
            current_uplinks[i] = (sf, bw, rssi)
            logging.debug(f" SF: {sf}, BW: {bw}, RSSI: {rssi}")
            state = State(sf=sf, bw=float(bw), pwr=rssi)
            
            # call
            lib.pid(ctypes.byref(state))
            send_downlink(internal, dev_eui, state.sf, state.bw, state.pwr, auth)

if __name__ == "__main__":
    main()