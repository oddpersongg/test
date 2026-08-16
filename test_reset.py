# -*- coding: utf-8 -*-
"""
0x11 ECUReset hardware verification (respond-then-reset + restart window).

Sequence:
  1. enter programming session (10 02)                  -> 50 02
  2. ECU reset (11 01)                                  -> 51 01
  3. probe 10x TesterPresent (3E 00) @ 200 ms           -> expect TIME-OUTs
     (the chip is restarting: CAN not initialized yet)
  4. session check: 34 must be rejected with 7F 34 7F   (default session
     after the reset => state was really cleared)
  5. re-enter programming session on the fresh boot     -> 50 02
"""
import ctypes
import sys
import time

sys.path.insert(0, r"D:\100_worksapce\099_test\test")
import uds_tool as ut


def main():
    dll = ctypes.WinDLL(ut.DLL_PATH)
    if dll.VCI_OpenDevice(ctypes.c_uint32(ut.DEV_TYPE), 0, 0) != 1:
        print("ERROR: cannot open device (ZCANPRO running?)")
        return 1

    cfg = ut.VCI_INIT_CONFIG()
    cfg.AccCode = 0
    cfg.AccMask = 0xFFFFFFFF
    cfg.Reserved = 0
    cfg.Filter = 0
    cfg.Timing0, cfg.Timing1 = ut.BTR
    cfg.Mode = 0
    dll.VCI_ResetCAN(ctypes.c_uint32(ut.DEV_TYPE), 0, 0)
    time.sleep(0.05)
    dll.VCI_InitCAN(ctypes.c_uint32(ut.DEV_TYPE), 0, 0, ctypes.byref(cfg))
    dll.VCI_ClearBuffer(ctypes.c_uint32(ut.DEV_TYPE), 0, 0)
    dll.VCI_StartCAN(ctypes.c_uint32(ut.DEV_TYPE), 0, 0)

    tp = ut.IsoTp(dll)
    results = []

    def step(name, sid, payload, want, timeout=3.0):
        ok, resp = ut.run_request(tp, sid, payload)
        passed = ok and ut.expect(resp, want)
        results.append((name, passed))
        print("  => %s" % ("PASS" if passed else "FAIL"))
        if resp and resp[0] == 0x7F:
            print("     (negative response, NRC=0x%02X)" % (resp[2] if len(resp) > 2 else 0))
        return resp

    print("[1. enter programming session]")
    step("10 02", 0x10, [0x02], [0x50, 0x02])

    print("[2. ECU reset 11 01]")
    step("11 01", 0x11, [0x01], [0x51, 0x01])

    print("[3. probe restart window: 10x 3E 00 @ 200 ms, measure latency]")
    misses = 0
    lat = []
    for i in range(10):
        t0 = time.time()
        tp.send([0x3E, 0x00])
        resp = tp.receive(timeout=1.0)
        dt = (time.time() - t0) * 1000.0
        if resp is None:
            misses += 1
            print("  probe %2d: TIMEOUT (chip restarting?)" % i)
        else:
            lat.append(dt)
            print("  probe %2d: %s  (%.1f ms)" % (i, ut.hexs(resp), dt))
        time.sleep(0.2)
    # restart evidence: a miss, or a response latency clearly above the
    # normal sub-ms main-loop turnaround (the first response after the
    # chip comes back takes the full restart time)
    max_lat = max(lat) if lat else 0.0
    restarted = (misses > 0) or (max_lat > 50.0)
    results.append(("restart window observed", restarted))
    print("  => %s (misses=%d, max latency=%.1f ms)" %
          ("PASS" if restarted else "FAIL", misses, max_lat))

    print("[4. session check after reset: 34 -> 7F 34 7F (default session)]")
    step("34 after reset", 0x34, [0x22, 0x00, 0x08, 0x00, 0x08], [0x7F, 0x34, 0x7F])

    print("[5. re-enter programming session on the fresh boot]")
    step("10 02 again", 0x10, [0x02], [0x50, 0x02])

    dll.VCI_CloseDevice(ctypes.c_uint32(ut.DEV_TYPE), 0)

    failed = [n for n, p in results if not p]
    print("=" * 50)
    print("TOTAL: %d/%d passed" % (len(results) - len(failed), len(results)))
    if failed:
        print("FAILED: %s" % ", ".join(failed))
        return 1
    print("ALL TESTS PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
