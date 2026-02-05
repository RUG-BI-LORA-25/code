import os
import json
import ctypes

import grpc
from chirpstack_api import api
from chirpstack_api.api import internal_pb2, internal_pb2_grpc
import logging

# set up logg
logging.basicConfig(level=logging.INFO if not os.environ.get("DEBUG") else logging.DEBUG)

lib = ctypes.CDLL("./libalgo.so")


class State(ctypes.Structure):
    _fields_ = [
        ("sf", ctypes.c_int),
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


def get_latest_uplink(client, dev_eui, auth):
    req = internal_pb2.StreamDeviceFramesRequest(dev_eui=dev_eui)
    for frame in client.StreamDeviceFrames(req, metadata=auth):
        body = json.loads(frame.body)
        lora = body["tx_info"]["modulation"]["lora"]
        rssi = body["rx_info"][0]["rssi"]
        return lora["spreadingFactor"], lora["bandwidth"], rssi
    return None


def main():
    auth = [("authorization", f"Bearer {CHIRPSTACK_API_KEY}")]
    channel = grpc.insecure_channel(CHIRPSTACK_HOST)
    
    device_client = api.DeviceServiceStub(channel)
    internal_client = internal_pb2_grpc.InternalServiceStub(channel)

    for dev_eui in DEV_EUIS:
        device = device_client.Get(
            api.GetDeviceRequest(dev_eui=dev_eui), metadata=auth
        ).device
        logging.debug(f"\nDevice: {device.name} ({dev_eui})")

        sf, bw, rssi = get_latest_uplink(internal_client, dev_eui, auth)
        
        if sf is not None:
            logging.debug(f" SF: {sf}, BW: {bw}, RSSI: {rssi}")
            state = State(sf=sf, bw=float(bw), pwr=rssi)
        else:
            logging.warning("No uplink, using defaults")
            state = State(sf=12, bw=125000.0, pwr=14)
        
        # call
        lib.pid(ctypes.byref(state))


if __name__ == "__main__":
    main()