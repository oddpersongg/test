# -*- coding: utf-8 -*-
"""
UDS over ISO-TP test tool (direct ZLG usbcan.dll / VCI API).

Usage:
    python uds_tool.py                    # run the built-in full test sequence
    python uds_tool.py 10 01              # single request: SID + data bytes
    python uds_tool.py 10 01 27 02 A5     # sequential requests (each waits for its response)

Features:
    - ISO 15765-2 sender: SF (<=7 bytes) / FF+CF with FC handling (>7 bytes)
    - ISO-TP receiver: reassembles SF / FF+CF responses, strips PCI
    - reports every request/response, PASS/FAIL per step
"""
import ctypes
import json
import os
import sys
import time

DLL_PATH = r"D:\02_Engineering_Tool\ZCANPRO\kerneldlls\usbcan.dll"
DEV_TYPE = 99          # ZCANPRO usbcan.dll: VCI_USBCAN2
BTR = (0x00, 0x1C)     # 500 kbps (16 MHz)
REQ_ID = 0x7E0
RESP_ID = 0x7E8

# ISO-TP PCI types
PCI_SF = 0x0
PCI_FF = 0x1
PCI_CF = 0x2
PCI_FC = 0x3


class VCI_INIT_CONFIG(ctypes.Structure):
    _fields_ = [
        ("AccCode", ctypes.c_uint32),
        ("AccMask", ctypes.c_uint32),
        ("Reserved", ctypes.c_uint32),
        ("Filter", ctypes.c_ubyte),
        ("Timing0", ctypes.c_ubyte),
        ("Timing1", ctypes.c_ubyte),
        ("Mode", ctypes.c_ubyte),
    ]


class VCI_CAN_OBJ(ctypes.Structure):
    _fields_ = [
        ("ID", ctypes.c_uint32),
        ("TimeStamp", ctypes.c_uint32),
        ("TimeFlag", ctypes.c_ubyte),
        ("SendType", ctypes.c_ubyte),
        ("RemoteFlag", ctypes.c_ubyte),
        ("ExternFlag", ctypes.c_ubyte),
        ("DataLen", ctypes.c_ubyte),
        ("Data", ctypes.c_ubyte * 8),
        ("Reserved", ctypes.c_ubyte * 3),
    ]


class VCI_ERR_INFO(ctypes.Structure):
    _fields_ = [
        ("ErrCode", ctypes.c_uint32),
        ("Passive_ErrData", ctypes.c_ubyte * 3),
        ("ArLost_ErrData", ctypes.c_ubyte),
    ]


class IsoTp(object):
    """Minimal ISO 15765-2 sender/receiver over a VCI CAN channel."""

    def __init__(self, dll, chn=0):
        self.dll = dll
        self.chn = chn

    # ---------------- send ----------------
    def send(self, data):
        """Send an SDU with automatic SF / FF+CF segmentation."""
        if len(data) <= 7:
            self._send_frame([PCI_SF | len(data)] + list(data))
            return
        # first frame
        ff = [PCI_FF << 4 | ((len(data) >> 8) & 0x0F), len(data) & 0xFF] + list(data[:6])
        self._send_frame(ff)
        # wait for flow control (CTS)
        fc = self._wait_fc()
        if fc is None:
            raise RuntimeError("no flow control frame")
        fs = fc[0] & 0x0F
        if fs == 0x02:
            raise RuntimeError("flow control overflow")
        # consecutive frames
        sn = 1
        pos = 6
        while pos < len(data):
            cf = [PCI_CF << 4 | (sn & 0x0F)] + list(data[pos:pos + 7])
            self._send_frame(cf)
            sn = (sn + 1) & 0x0F
            pos += 7

    def _send_frame(self, frame):
        tx = VCI_CAN_OBJ()
        tx.ID = REQ_ID
        tx.SendType = 0
        tx.RemoteFlag = 0
        tx.ExternFlag = 0
        tx.DataLen = len(frame)
        for i, b in enumerate(frame):
            tx.Data[i] = b
        if self.dll.VCI_Transmit(ctypes.c_uint32(DEV_TYPE), 0, self.chn,
                                 ctypes.byref(tx), 1) != 1:
            raise RuntimeError("transmit failed")

    def _wait_fc(self, timeout=1.0):
        """Wait for a flow-control frame on RESP_ID; return its data or None."""
        dl = time.time() + timeout
        rx = VCI_CAN_OBJ()
        while time.time() < dl:
            cnt = self.dll.VCI_Receive(ctypes.c_uint32(DEV_TYPE), 0, self.chn,
                                       ctypes.byref(rx), 1, 50)
            if cnt > 0 and rx.ID == RESP_ID and (rx.Data[0] >> 4) == PCI_FC:
                return [rx.Data[i] for i in range(rx.DataLen)]
        return None

    # ---------------- receive ----------------
    def receive(self, timeout=3.0):
        """Reassemble a complete response SDU (SF or FF+CF). Returns data bytes."""
        rx = VCI_CAN_OBJ()
        dl = time.time() + timeout
        while time.time() < dl:
            cnt = self.dll.VCI_Receive(ctypes.c_uint32(DEV_TYPE), 0, self.chn,
                                       ctypes.byref(rx), 1, 50)
            if cnt == 0:
                continue
            data = [rx.Data[i] for i in range(rx.DataLen)]
            pci = data[0]
            ptype = pci >> 4
            if ptype == PCI_SF:
                n = pci & 0x0F
                return data[1:1 + n]
            if ptype == PCI_FF:
                total = ((pci & 0x0F) << 8) | data[1]
                buf = list(data[2:])
                expect_sn = 1
                while len(buf) < total:
                    cnt2 = self.dll.VCI_Receive(ctypes.c_uint32(DEV_TYPE), 0,
                                                self.chn, ctypes.byref(rx), 1, 50)
                    if cnt2 == 0:
                        continue
                    d2 = [rx.Data[i] for i in range(rx.DataLen)]
                    if (d2[0] >> 4) != PCI_CF:
                        continue
                    sn = d2[0] & 0x0F
                    if sn != expect_sn:
                        raise RuntimeError("CF sequence error (got %d, want %d)" % (sn, expect_sn))
                    buf += d2[1:]
                    expect_sn = (expect_sn + 1) & 0x0F
                return buf[:total]
        return None


def hexs(data):
    return " ".join("%02X" % b for b in data)


def run_request(tp, sid, payload):
    """Send one UDS request, print trace, return (ok, response_or_None)."""
    req = [sid] + list(payload)
    tp.send(req)
    resp = tp.receive()
    if resp is None:
        print("  [TIMEOUT] req=%s" % hexs(req))
        return False, None
    print("  req=%s -> resp=%s" % (hexs(req), hexs(resp)))
    return True, resp


def expect(resp, want):
    """Check response: want is a list of exact bytes, or a 2-byte prefix."""
    if resp is None:
        return False
    return list(resp)[:len(want)] == list(want)


def main():
    dll = ctypes.WinDLL(DLL_PATH)
    if dll.VCI_OpenDevice(ctypes.c_uint32(DEV_TYPE), 0, 0) != 1:
        print("ERROR: cannot open device (ZCANPRO running?)")
        sys.exit(1)

    cfg = VCI_INIT_CONFIG()
    cfg.AccCode = 0
    cfg.AccMask = 0xFFFFFFFF
    cfg.Reserved = 0
    cfg.Filter = 0
    cfg.Timing0, cfg.Timing1 = BTR
    cfg.Mode = 0
    dll.VCI_ResetCAN(ctypes.c_uint32(DEV_TYPE), 0, 0)
    time.sleep(0.05)
    dll.VCI_InitCAN(ctypes.c_uint32(DEV_TYPE), 0, 0, ctypes.byref(cfg))
    dll.VCI_ClearBuffer(ctypes.c_uint32(DEV_TYPE), 0, 0)
    dll.VCI_StartCAN(ctypes.c_uint32(DEV_TYPE), 0, 0)

    tp = IsoTp(dll)

    args = sys.argv[1:]
    results = []

    if not args:
        # ---- built-in full sequence ----
        seq = [
            ("10 default session",     0x10, [0x01],            [0x50, 0x01]),
            ("10 programming session", 0x10, [0x02],            [0x50, 0x02]),
            ("27 request seed",        0x27, [0x01],            [0x67, 0x01]),
            ("27 send key",            0x27, [0x02, 0xA5],      [0x67, 0x02]),
            ("34 request download",    0x34, [0x00, 0x41, 0x08, 0x00, 0x80, 0x00, 0x08], [0x74, 0x20, 0x08, 0x00]),
            ("36 transfer data",       0x36, [0x01, 0x11, 0x22], [0x76, 0x01]),
            ("37 request transfer exit", 0x37, [],              [0x77]),
            ("10 back to default",     0x10, [0x01],            [0x50, 0x01]),
        ]
        for name, sid, payload, want in seq:
            print("[%s]" % name)
            ok, resp = run_request(tp, sid, payload)
            passed = ok and expect(resp, want)
            results.append((name, passed))
            print("  => %s" % ("PASS" if passed else "FAIL"))
            if resp and resp[0] == 0x7F:
                print("     (negative response, NRC=0x%02X)" % (resp[2] if len(resp) > 2 else 0))
    else:
        # ---- single/sequential requests from CLI bytes ----
        b = [int(x, 16) for x in args]
        i = 0
        while i < len(b):
            sid = b[i]
            payload = b[i + 1:i + 8]  # SID + up to 7 bytes (SF); longer handled by send()
            print("[request 0x%02X %s]" % (sid, hexs(payload)))
            ok, resp = run_request(tp, sid, payload)
            results.append(("0x%02X" % sid, ok and resp is not None))
            i += 1 + len(payload)

    dll.VCI_CloseDevice(ctypes.c_uint32(DEV_TYPE), 0)

    failed = [n for n, p in results if not p]
    print("=" * 50)
    print("TOTAL: %d/%d passed" % (len(results) - len(failed), len(results)))
    if failed:
        print("FAILED: %s" % ", ".join(failed))
        sys.exit(1)
    print("ALL TESTS PASSED")


if __name__ == "__main__":
    main()
