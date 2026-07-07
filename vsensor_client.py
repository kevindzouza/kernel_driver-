#!/usr/bin/env python3
"""
vsensor_client.py - Userspace CLI for the vsensor character device driver.

Talks to /dev/vsensor0 via blocking read() and ioctl(), demonstrating
kernel <-> userspace interaction from Python.

Usage:
    python3 vsensor_client.py --interval-ms 200 --csv readings.csv --count 20
    python3 vsensor_client.py            # stream forever at default rate
"""

import fcntl
import os
import struct
import sys

DEVICE_PATH = "/dev/vsensor0"

# --- Reproduce the kernel's _IOW/_IOR macro encoding so these ioctl numbers
#     match what vsensor.c defines via _IOW(VSENSOR_MAGIC, 1, int) etc. ---
_IOC_NRBITS, _IOC_TYPEBITS, _IOC_SIZEBITS = 8, 8, 14
_IOC_NRSHIFT = 0
_IOC_TYPESHIFT = _IOC_NRSHIFT + _IOC_NRBITS
_IOC_SIZESHIFT = _IOC_TYPESHIFT + _IOC_TYPEBITS
_IOC_DIRSHIFT = _IOC_SIZESHIFT + _IOC_SIZEBITS
_IOC_WRITE, _IOC_READ = 1, 2


def _ioc(direction, type_char, nr, size):
    return (
        (direction << _IOC_DIRSHIFT)
        | (ord(type_char) << _IOC_TYPESHIFT)
        | (nr << _IOC_NRSHIFT)
        | (size << _IOC_SIZESHIFT)
    )


VSENSOR_MAGIC = "k"
VSENSOR_SET_INTERVAL_MS = _ioc(_IOC_WRITE, VSENSOR_MAGIC, 1, 4)  # int = 4 bytes
VSENSOR_GET_INTERVAL_MS = _ioc(_IOC_READ, VSENSOR_MAGIC, 2, 4)


def set_interval(fd, ms: int) -> None:
    fcntl.ioctl(fd, VSENSOR_SET_INTERVAL_MS, struct.pack("i", ms))


def get_interval(fd) -> int:
    result = fcntl.ioctl(fd, VSENSOR_GET_INTERVAL_MS, struct.pack("i", 0))
    return struct.unpack("i", result)[0]


def open_device():

        return os.open(DEVICE_PATH, os.O_RDONLY)


def stream(interval_ms: int, count: int | None) -> None:
    fd = open_device()

    try:
        set_interval(fd, interval_ms)
        print(f"sampling interval set to {get_interval(fd)} ms, reading from {DEVICE_PATH}")

        n = 0
        while count is None or n < count:
            raw = os.read(fd, 16)  # blocks in the kernel until the timer produces new data
            if not raw:
                continue
            value = raw.decode().strip()
            print(value)
            n += 1
    except KeyboardInterrupt:
        print("\nstopped by user")
    finally:
        os.close(fd)


def main() -> None:
    interval_ms = 500      # sampling interval in ms (10-60000)
    count = None           # set to an int to stop after N readings, else runs until Ctrl+C

    stream(interval_ms, count)


if __name__ == "__main__":
    main()