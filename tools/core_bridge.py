#!/usr/bin/env python3
"""Flash ESP32 CORE through the integrated or dedicated EARS USB bridge (fixed 115200 baud)."""
import argparse
import binascii
import pathlib
import os
import select
import socket
import struct
import subprocess
import sys
import time

PING, BOOT, RUN, DATA, ACK, ERROR, EXIT = range(1, 8)
MAX_PAYLOAD = 256


def encode(kind, payload=b""):
    if len(payload) > MAX_PAYLOAD:
        raise ValueError("Bridge payload too large")
    body = struct.pack("<BH", kind, len(payload)) + payload
    return b"EB" + body + struct.pack("<H", binascii.crc_hqx(body, 0xffff))


class Decoder:
    def __init__(self):
        self.buffer = bytearray()

    def feed(self, data):
        self.buffer.extend(data)
        packets = []
        while len(self.buffer) >= 5:
            if self.buffer[:2] != b"EB":
                raise RuntimeError("Invalid bridge header; install the EARS bridge firmware first")
            kind, size = struct.unpack_from("<BH", self.buffer, 2)
            if size > MAX_PAYLOAD:
                raise RuntimeError("Invalid bridge length")
            if len(self.buffer) < size + 7:
                break
            packet = self.buffer[:size + 7]
            del self.buffer[:size + 7]
            if binascii.crc_hqx(packet[2:-2], 0xffff) != struct.unpack_from("<H", packet, size + 5)[0]:
                raise RuntimeError("Bridge CRC error; upload aborted")
            if kind == ERROR:
                raise RuntimeError("EARS rejected a bridge packet")
            packets.append((kind, bytes(packet[5:-2])))
        return packets


class Bridge:
    def __init__(self, port):
        import serial
        # Set both lines before open: automatic USB reset must never target EARS.
        self.serial = serial.Serial(port=None, baudrate=115200, timeout=0.02, write_timeout=5)
        self.serial.dtr = False
        self.serial.rts = False
        self.serial.port = port
        self.serial.open()
        time.sleep(0.3)
        self.serial.reset_input_buffer()
        self.decoder = Decoder()
        self.backlog = []
        self.integrated = False
        # Normal firmware consumes this exact line only on its USB console.
        # The dedicated/already-active bridge ignores it and answers PING below.
        self.serial.write(b"\nPIUU BRIDGE\n")
        deadline = time.monotonic() + 4
        line = bytearray()
        while time.monotonic() < deadline:
            b = self.serial.read(1)
            if b == b"\n":
                if bytes(line).strip() == b"PIUU BRIDGE READY":break
                if b"Maintenance rejected" in line:
                    self.serial.close()
                    raise RuntimeError("EARS could not pause audio; maintenance was not entered")
                line.clear()
            elif b:
                line.extend(b)
                if len(line)>512:line.clear()
        self.serial.reset_input_buffer()

    def send(self, kind, payload=b""):
        packet = encode(kind, payload)
        if self.serial.write(packet) != len(packet):
            raise RuntimeError("Incomplete USB write")

    def receive(self, one_byte=False):
        if self.backlog:
            packets, self.backlog = self.backlog, []
            return packets
        return self.decoder.feed(self.serial.read(1 if one_byte else min(max(self.serial.in_waiting, 1), 4096)))

    def command(self, kind):
        self.send(kind)
        deadline = time.monotonic() + 5
        saved = []
        while time.monotonic() < deadline:
            acknowledged = False
            for reply, payload in (self.receive(one_byte=True) if kind == EXIT else self.receive()):
                if reply == ACK and payload[:1] == bytes([kind]):
                    acknowledged = True
                    if kind == PING:self.integrated = len(payload)>1 and payload[1]==1
                elif reply == DATA:
                    saved.append((reply, payload))
            if acknowledged:
                self.backlog.extend(saved)
                return
        raise RuntimeError("No EARS bridge acknowledgement (close serial monitors and check firmware)")

    def close(self):
        self.serial.close()


def flash_files(directory, boot_app0=None):
    directory = pathlib.Path(directory)
    if boot_app0 is None:
        boot_app0 = directory / "boot_app0.bin"
        if not boot_app0.is_file():
            pio_home = pathlib.Path(os.environ.get("PLATFORMIO_CORE_DIR", pathlib.Path.home() / ".platformio"))
            boot_app0 = pio_home / "packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin"
    files = [("0x1000", directory / "bootloader.bin"),
             ("0x8000", directory / "partitions.bin"),
             ("0xe000", pathlib.Path(boot_app0)),
             ("0x10000", directory / "firmware.bin")]
    for _, path in files:
        if not path.is_file():
            raise ValueError(f"Missing build artifact: {path}")
    return [value for address, path in files for value in (address, str(path.resolve()))]


def run_esptool(bridge, arguments):
    # esptool talks raw UART over localhost; only this process owns the USB port.
    with socket.socket() as listener:
        listener.bind(("127.0.0.1", 0))
        listener.listen(1)
        listener.settimeout(15)
        port = listener.getsockname()[1]
        cmd = [sys.executable, "-m", "esptool", "--chip", "esp32", "--port",
               f"socket://127.0.0.1:{port}", "--baud", "115200",
               "--before", "no-reset", "--after", "no-reset"] + arguments
        process = subprocess.Popen(cmd)
        sent = received = acknowledgements = 0
        try:
            connection, _ = listener.accept()
            with connection:
                connection.settimeout(5)
                pending = False
                last_activity = time.monotonic()
                while process.poll() is None:
                    ready, _, _ = select.select([connection], [], [], 0)
                    if ready and not pending:
                        raw = connection.recv(MAX_PAYLOAD)
                        if not raw:
                            break
                        bridge.send(DATA, raw)
                        sent += len(raw)
                        pending = True
                        last_activity = time.monotonic()
                    for kind, payload in bridge.receive():
                        if kind == ACK and payload == bytes([DATA]) and pending:
                            pending = False
                            acknowledgements += 1
                            last_activity = time.monotonic()
                            continue
                        if kind != DATA:
                            raise RuntimeError("Unexpected bridge response during upload")
                        received += len(payload)
                        connection.sendall(payload)
                        last_activity = time.monotonic()
                    if time.monotonic() - last_activity > 90:
                        raise RuntimeError("CORE upload stalled")
            result = process.wait(timeout=15)
            if result:
                raise RuntimeError(f"esptool failed ({result}); CORE download mode was requested but not confirmed")
        finally:
            print(f"[BRIDGE] host_to_uart_bytes={sent} data_acks={acknowledgements} uart_to_host_bytes={received}", flush=True)
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()


def diagnose(bridge):
    bridge.command(BOOT)
    print("[BRIDGE] EARS acknowledged BOOT; capturing CORE UART at 115200...", flush=True)
    raw = bytearray()
    deadline = time.monotonic() + 2
    while time.monotonic() < deadline:
        for kind, payload in bridge.receive():
            if kind == DATA:
                raw.extend(payload[:4096-len(raw)])
    print(f"[BRIDGE] captured_bytes={len(raw)} (maximum 4096)")
    if raw:
        print(raw.decode("utf-8", errors="backslashreplace"))
        print("[BRIDGE] Look for DOWNLOAD_BOOT / waiting for download; SPI_FAST_FLASH_BOOT means normal application boot.")
    else:
        print("[BRIDGE] No CORE boot bytes: check CORE power, EN/BOOT and CORE TX1 -> EARS RX7. A silent ROM is also possible.")
    print("[BRIDGE] No flash written. CORE has not been reset into its application.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", default="/dev/esp32-ears")
    sub = parser.add_subparsers(dest="action", required=True)
    sub.add_parser("diagnose", help="Reset CORE to download mode and capture its ROM boot output")
    sub.add_parser("info", help="Reset CORE to ROM loader and read its chip ID (no flash write)")
    sub.add_parser("run", help="Release BOOT and reset CORE into its application")
    flash = sub.add_parser("flash", help="Write a complete PlatformIO CORE build, then reset CORE")
    flash.add_argument("--build-dir", required=True)
    flash.add_argument("--boot-app0", help="Override the Arduino boot_app0.bin artifact")
    args = parser.parse_args()
    files = flash_files(args.build_dir, args.boot_app0) if args.action == "flash" else None
    if args.action in ("info", "flash"):
        import esptool  # Fail before resetting CORE if dependencies are missing.
    bridge = Bridge(args.port)
    try:
        bridge.command(PING)
        if args.action == "diagnose":
            diagnose(bridge)
            return
        if args.action == "run":
            bridge.command(RUN)
        else:
            bridge.command(BOOT)
            run_esptool(bridge, ["chip-id"] if args.action == "info" else ["write-flash"] + files)
            bridge.command(RUN)
        if bridge.integrated:
            bridge.command(EXIT)
            print("CORE reset requested; EARS acknowledged return to normal mode.")
        else:
            print("CORE reset requested. Dedicated EARS bridge remains in maintenance mode.")
    finally:
        bridge.close()


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, ValueError, subprocess.SubprocessError, ImportError) as error:
        print(f"Error: {error}", file=sys.stderr)
        sys.exit(1)
