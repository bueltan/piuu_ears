import importlib.util
import pathlib
import tempfile
import unittest
from unittest.mock import patch
import subprocess
import sys
import types
spec = importlib.util.spec_from_file_location("bridge", pathlib.Path(__file__).parents[2] / "tools/core_bridge.py")
bridge = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bridge)

class Tests(unittest.TestCase):
    def test_binary_fragmentation_and_cpp_wire(self):
        raw = bytes(range(256))
        packet = bridge.encode(bridge.DATA, raw)
        self.assertEqual(packet, pathlib.Path('/tmp/piuu-bridge-golden.bin').read_bytes())
        decoder = bridge.Decoder()
        replies = []
        for b in packet + bridge.encode(bridge.ACK, bytes([bridge.BOOT])):
            replies += decoder.feed(bytes([b]))
        self.assertEqual(replies, [(bridge.DATA, raw), (bridge.ACK, bytes([bridge.BOOT]))])
    def test_reject_bad_crc_length_and_firmware(self):
        for bad in [b'logs!', b'EB\x04\x01\x01', bridge.encode(bridge.DATA, b'\xc0\xdb')[:-1] + b'\x00']:
            with self.assertRaises(RuntimeError): bridge.Decoder().feed(bad)
        with self.assertRaises(RuntimeError):bridge.Decoder().feed(bridge.encode(bridge.ERROR,b'\x00'))
        with self.assertRaises(ValueError):bridge.encode(bridge.DATA, bytes(257))
    def test_local_socket_tunnel_and_flow_control(self):
        class FakeBridge:
            pending = False
            chunks = []
            def send(self, kind, raw):
                self.assert_ready = not self.pending
                if self.pending: raise AssertionError("Unacknowledged data overwritten")
                assert kind == bridge.DATA and len(raw) <= 256
                self.chunks.append(raw)
                self.pending = True
                self.raw = raw
            def receive(self):
                if not self.pending:return []
                self.pending = False
                return [(bridge.ACK, bytes([bridge.DATA])), (bridge.DATA, self.raw)]
        fake = FakeBridge()
        real_popen = subprocess.Popen
        def peer(cmd):
            self.assertIn("no-reset", cmd)
            self.assertIn("115200", cmd)
            port = int(cmd[cmd.index("--port")+1].rsplit(":",1)[1])
            code = """import socket,sys
s=socket.create_connection(('127.0.0.1',int(sys.argv[1])))
s.settimeout(5)
raw=bytes(range(256))*4
s.sendall(raw)
received=b''
while len(received)<len(raw): received+=s.recv(1024)
assert received==raw
s.close()
"""
            return real_popen([sys.executable, "-c", code, str(port)])
        with patch.object(bridge.subprocess, "Popen", side_effect=peer):
            bridge.run_esptool(fake, ["chip-id"])
        self.assertEqual(b''.join(fake.chunks),bytes(range(256))*4)

    def test_command_preserves_uart_data_around_ack(self):
        for packets in [[(bridge.DATA,b'before'),(bridge.ACK,bytes([bridge.BOOT])),(bridge.DATA,b'after')],
                        [(bridge.ACK,bytes([bridge.BOOT])),(bridge.DATA,b'boot log')]]:
            instance=bridge.Bridge.__new__(bridge.Bridge)
            instance.backlog=[]
            instance.send=lambda *args: None
            with patch.object(instance, 'receive', return_value=packets):
                instance.command(bridge.BOOT)
            self.assertEqual(instance.receive(),[packet for packet in packets if packet[0]==bridge.DATA])

    def test_integrated_entry_capabilities_and_exit_before_logs(self):
        class Usb:
            def __init__(self, **kwargs):self.rx=bytearray();self.writes=[]
            def open(self):pass
            def close(self):pass
            def reset_input_buffer(self):self.rx.clear()
            @property
            def in_waiting(self):return len(self.rx)
            def read(self,n):
                data=bytes(self.rx[:n]);del self.rx[:n];return data
            def write(self,data):
                self.writes.append(data)
                if data==b"\nPIUU BRIDGE\n":self.rx.extend(b"[EARS] seq=1\nPIUU BRIDGE READY\r\n")
                elif data==bridge.encode(bridge.PING):self.rx.extend(bridge.encode(bridge.ACK,bytes([bridge.PING,1])))
                elif data==bridge.encode(bridge.EXIT):self.rx.extend(bridge.encode(bridge.ACK,bytes([bridge.EXIT]))+b"[EARS] Normal mode restored\n")
                return len(data)
        with patch.dict(sys.modules,{'serial':types.SimpleNamespace(Serial=Usb)}), patch.object(bridge.time,'sleep'):
            instance=bridge.Bridge('fake')
            instance.command(bridge.PING)
            self.assertTrue(instance.integrated)
            instance.command(bridge.EXIT)
            self.assertTrue(instance.serial.rx.startswith(b'[EARS]'))
            self.assertFalse(instance.serial.dtr)
            self.assertFalse(instance.serial.rts)

    def test_flash_artifacts(self):
        with tempfile.TemporaryDirectory() as directory:
            root=pathlib.Path(directory)
            with self.assertRaises(ValueError):bridge.flash_files(root)
            for name in ['bootloader.bin','partitions.bin','boot_app0.bin','firmware.bin']:(root/name).write_bytes(b'test')
            files=bridge.flash_files(root)
            self.assertEqual(files[::2],['0x1000','0x8000','0xe000','0x10000'])

if __name__=='__main__':unittest.main()
