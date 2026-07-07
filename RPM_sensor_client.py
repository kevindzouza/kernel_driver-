#!/usr/bin/env python3

import os
import fcntl
import struct

DEVICE = "/dev/RPM_sensor"

SET_TIME_INTERVAL = 0x40046B01
GET_TIME_INTERVAL = 0x80046B02


def main():

    fd = os.open(DEVICE, os.O_RDONLY)
    
    try:
        value = int(input("Enter time interval in ms: "))

        data = struct.pack("i", value)

        fcntl.ioctl(
            fd,
            SET_TIME_INTERVAL,
            data
        )

        print("Interval set to ",value)

        buffer = bytearray(4)

        fcntl.ioctl(
            fd,
            GET_TIME_INTERVAL,
            buffer,
            True
        )

        interval = struct.unpack("i", buffer)[0]

        print("Current interval:", interval)

        while True:
            data =os.read(fd, 16)

            sensor_val = data.decode().strip()

            print("Sensor valuue:", sensor_val)

    finally:
        os.close(fd)


if __name__ == "__main__":
    main()