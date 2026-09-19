#!/usr/bin/env python3

import sys
import time
import struct
import zlib
import argparse

import serial


PACKET_SIZE = 1024
PROTOCOL_VERSION = 1

MAGIC = b"W25U"

DEFAULT_ADDRESS = 0x010000


# ============================================================
# Serial helpers
# ============================================================

def read_line(ser, timeout=10):
    old_timeout = ser.timeout
    ser.timeout = timeout

    try:
        line = ser.readline()

        if not line:
            raise TimeoutError("Timeout waiting for ESP32")

        return line.decode(
            "ascii",
            errors="replace"
        ).strip()

    finally:
        ser.timeout = old_timeout


def wait_for_line(ser, expected, timeout=10):
    deadline = time.monotonic() + timeout

    while time.monotonic() < deadline:
        remaining = deadline - time.monotonic()

        if remaining <= 0:
            break

        ser.timeout = min(1.0, remaining)

        line = ser.readline()

        if not line:
            continue

        text = line.decode(
            "ascii",
            errors="replace"
        ).strip()

        print(f"< {text}")

        if text == expected:
            return True

        if text.startswith("ERROR:"):
            raise RuntimeError(text)

    raise TimeoutError(
        f"Timeout waiting for '{expected}'"
    )


# ============================================================
# Main uploader
# ============================================================

def upload(port, filename, address):
    print()
    print("========================================")
    print(" W25Q128 Linux Flash Uploader")
    print("========================================")
    print()

    # --------------------------------------------------------
    # Read entire binary
    # --------------------------------------------------------

    with open(filename, "rb") as f:
        data = f.read()


    size = len(data)

    if size == 0:
        raise RuntimeError("Binary file is empty")


    crc = zlib.crc32(data) & 0xFFFFFFFF


    print(f"File    : {filename}")
    print(f"Size    : {size:,} bytes")
    print(f"Address : 0x{address:06X}")
    print(f"End     : 0x{address + size - 1:06X}")
    print(f"CRC32   : {crc:08X}")
    print()


    if address < 0:
        raise RuntimeError("Invalid flash address")


    if address + size > 16 * 1024 * 1024:
        raise RuntimeError(
            "Image does not fit in W25Q128"
        )


    # --------------------------------------------------------
    # Open serial
    # --------------------------------------------------------

    print(f"Opening {port}...")

    ser = serial.Serial(
        port=port,
        baudrate=115200,
        timeout=1,
        write_timeout=5
    )


    try:
        # ----------------------------------------------------
        # Give ESP32 a little time after opening USB.
        # ----------------------------------------------------

        time.sleep(0.5)

        ser.reset_input_buffer()
        ser.reset_output_buffer()


        print("Connected to ESP32 W25Q128 uploader")

		# The ESP32 may have printed READY:W25Q128:EF4018
		# before the Python program opened the USB port.
		#
		# The ESP32 is designed to wait for the upload header,
		# so we do not require the READY message here.

        time.sleep(0.2)

        # ----------------------------------------------------
        # Construct upload header.
        #
        # < = little endian
        #
        # 4 bytes magic
        # 1 byte version
        # 1 byte reserved
        # 4 bytes address
        # 4 bytes size
        # 4 bytes CRC32
        # 2 bytes reserved
        # ----------------------------------------------------

        header = struct.pack(
            "<4sBBIIIH",
            MAGIC,
            PROTOCOL_VERSION,
            0,
            address,
            size,
            crc,
            0
        )


        assert len(header) == 20


        print()
        print("Sending upload header...")

        ser.write(header)
        ser.flush()


        # ----------------------------------------------------
        # Wait for HEADER_OK
        # ----------------------------------------------------

        wait_for_line(
            ser,
            "HEADER_OK",
            timeout=10
        )


        # ----------------------------------------------------
        # Wait for erase to finish.
        # ----------------------------------------------------

        print()
        print("Erasing flash...")


        deadline = time.monotonic() + 120

        erase_done = False

        while time.monotonic() < deadline:
            ser.timeout = 2

            line = ser.readline()

            if not line:
                continue

            text = line.decode(
                "ascii",
                errors="replace"
            ).strip()

            print(f"< {text}")


            if text == "ERASE_DONE":
                erase_done = True
                break


            if text.startswith("ERROR:"):
                raise RuntimeError(text)


        if not erase_done:
            raise TimeoutError(
                "Flash erase timeout"
            )


        # ----------------------------------------------------
        # Send packets
        # ----------------------------------------------------

        print()
        print("Programming flash...")


        total_sent = 0
        sequence = 0

        start_time = time.monotonic()


        while total_sent < size:

            chunk = data[
                total_sent:
                total_sent + PACKET_SIZE
            ]


            packet_header = struct.pack(
                "<IH",
                sequence,
                len(chunk)
            )


            packet_crc = (
                zlib.crc32(chunk) &
                0xFFFFFFFF
            )


            packet = (
                packet_header +
                chunk +
                struct.pack("<I", packet_crc)
            )


            ser.write(packet)
            ser.flush()


            # ------------------------------------------------
            # Wait for ACK for this packet.
            # ------------------------------------------------

            expected_ack = (
                f"ACK:{sequence}:"
                f"{total_sent + len(chunk)}"
            )


            deadline = (
                time.monotonic() + 15
            )


            acknowledged = False


            while time.monotonic() < deadline:

                ser.timeout = 2

                line = ser.readline()

                if not line:
                    continue


                text = line.decode(
                    "ascii",
                    errors="replace"
                ).strip()


                if text.startswith("ERROR:"):
                    raise RuntimeError(text)


                if text == expected_ack:
                    acknowledged = True
                    break


                # Ignore other diagnostic lines.


            if not acknowledged:
                raise TimeoutError(
                    f"Timeout waiting for ACK "
                    f"packet {sequence}"
                )


            total_sent += len(chunk)
            sequence += 1


            elapsed = (
                time.monotonic() -
                start_time
            )


            speed = (
                total_sent / elapsed
                if elapsed > 0
                else 0
            )


            percent = (
                total_sent * 100 /
                size
            )


            print(
                f"\r"
                f"{percent:6.2f}%  "
                f"{total_sent:,}/{size:,} bytes  "
                f"{speed / 1024:7.1f} KiB/s",
                end="",
                flush=True
            )


        print()
        print()
        print("Programming complete.")


        # ----------------------------------------------------
        # Verify
        # ----------------------------------------------------

        print("Waiting for ESP32 verification...")


        deadline = time.monotonic() + 120

        verified = False


        while time.monotonic() < deadline:

            ser.timeout = 2

            line = ser.readline()

            if not line:
                continue


            text = line.decode(
                "ascii",
                errors="replace"
            ).strip()


            print(f"< {text}")


            if text == "VERIFY_OK":
                verified = True
                break


            if text.startswith("ERROR:"):
                raise RuntimeError(text)


        if not verified:
            raise RuntimeError(
                "Flash verification failed"
            )


        # ----------------------------------------------------
        # Done
        # ----------------------------------------------------

        print()
        print("========================================")
        print(" W25Q128 UPLOAD SUCCESS")
        print("========================================")
        print(f"Address : 0x{address:06X}")
        print(f"Size    : {size:,} bytes")
        print(f"CRC32   : {crc:08X}")
        print("Verify  : OK")
        print()


    finally:
        ser.close()


# ============================================================
# Command line
# ============================================================

def main():

    parser = argparse.ArgumentParser(
        description=
        "Upload a binary file to W25Q128 "
        "through ESP32-S3"
    )


    parser.add_argument(
        "filename",
        help="Binary file to upload"
    )


    parser.add_argument(
        "-p",
        "--port",
        default="/dev/ttyACM0",
        help="ESP32 USB serial device "
             "(default: /dev/ttyACM0)"
    )


    parser.add_argument(
        "-a",
        "--address",
        default="0x010000",
        help="Flash address "
             "(default: 0x010000)"
    )


    args = parser.parse_args()


    try:
        address = int(
            args.address,
            0
        )

    except ValueError:
        print(
            f"Invalid address: {args.address}",
            file=sys.stderr
        )

        sys.exit(1)


    try:
        upload(
            args.port,
            args.filename,
            address
        )

    except KeyboardInterrupt:
        print("\nInterrupted.")
        sys.exit(1)

    except Exception as e:
        print(
            f"\nERROR: {e}",
            file=sys.stderr
        )

        sys.exit(1)


if __name__ == "__main__":
    main()
