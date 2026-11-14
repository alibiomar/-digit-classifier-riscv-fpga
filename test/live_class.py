import collections
import threading
import time

import cv2
import numpy as np
import serial

PORT = "COM7"
BOOT_BAUD = 19200     # bootloader, fixed
APP_BAUD = 1000000     # must match BAUD in main.c
CLK_HZ = 100e6        # ui_clk
BOX = 300             # capture box size in pixels
EXE = "neorv32_exe.bin"

state = {"frame": None, "prev": None, "digit": None, "us": 0.0, "rt": 0.0,
         "status": "", "dark": True, "run": True}


def preprocess(roi, dark_ink):
    g = cv2.GaussianBlur(cv2.cvtColor(roi, cv2.COLOR_BGR2GRAY), (5, 5), 0)
    mode = cv2.THRESH_BINARY_INV if dark_ink else cv2.THRESH_BINARY
    _, b = cv2.threshold(g, 0, 255, mode + cv2.THRESH_OTSU)

    # drop small noise blobs
    n, lab, stats, _ = cv2.connectedComponentsWithStats(b)
    keep = np.zeros_like(b)
    for i in range(1, n):
        if stats[i, cv2.CC_STAT_AREA] >= 60:
            keep[lab == i] = 255
    ys, xs = np.where(keep > 0)
    if len(ys) < 100 or len(ys) > 0.4 * keep.size:
        return None

    k = keep[ys.min():ys.max() + 1, xs.min():xs.max() + 1]
    side = max(k.shape)

    area = cv2.countNonZero(k)
    edge = area - cv2.countNonZero(cv2.erode(k, np.ones((3, 3), np.uint8)))
    t = 2 * area / max(edge, 1)
    grow = int(0.12 * side - t)
    if grow >= 2:
        d = grow | 1
        p = d // 2 + 1
        k = cv2.copyMakeBorder(k, p, p, p, p, cv2.BORDER_CONSTANT, value=0)
        k = cv2.dilate(k, cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (d, d)))
    h, w = k.shape
    s = 20.0 / max(h, w)
    nh, nw = max(1, round(h * s)), max(1, round(w * s))
    k = cv2.resize(k, (nw, nh), interpolation=cv2.INTER_AREA)

    out = np.zeros((28, 28), dtype=np.uint8)
    y0, x0 = (28 - nh) // 2, (28 - nw) // 2
    out[y0:y0 + nh, x0:x0 + nw] = k
    return (out.astype(np.float32) * 255 / max(out.max(), 1)).astype(np.uint8)


def worker(ser):
    hist = collections.deque(maxlen=5)
    while state["run"]:
        roi = state["frame"]
        if roi is None:
            time.sleep(0.01)
            continue

        x = preprocess(roi, state["dark"])
        state["prev"] = x
        if x is None:
            state["digit"] = None
            hist.clear()
            time.sleep(0.05)
            continue

        t0 = time.time()
        ser.write(x.tobytes())
        r = ser.read(5)
        if len(r) != 5:
            state["status"] = "timeout: re-run live_class.py"
            ser.reset_input_buffer()
            continue

        state["status"] = ""
        state["rt"] = time.time() - t0
        state["us"] = int.from_bytes(r[1:], "little") / CLK_HZ * 1e6
        hist.append(r[0])
        state["digit"] = collections.Counter(hist).most_common(1)[0][0]


def boot(ser, exe=EXE):
    print("Press the Arty reset button (C2) now...")
    ser.baudrate = BOOT_BAUD
    ser.timeout = 0.3
    buf, t0 = b"", time.time()
    while b"CMD:>" not in buf:
        if time.time() - t0 > 30:
            raise SystemExit("no bootloader prompt")
        ser.write(b" ")
        buf += ser.read(256)
    ser.reset_input_buffer()
    ser.write(b"u")
    buf = b""
    while b"Awaiting" not in buf:
        buf += ser.read(64)
    ser.write(open(exe, "rb").read())
    buf, t0 = b"", time.time()
    while b"OK" not in buf and b"ERR" not in buf:
        if time.time() - t0 > 30:
            raise SystemExit("upload timeout")
        buf += ser.read(64)
    if b"ERR" in buf:
        raise SystemExit("upload failed")
    ser.reset_input_buffer()
    ser.write(b"e")
    ser.flush()
    time.sleep(1.5)             # banner finishes at BOOT_BAUD
    ser.reset_input_buffer()
    ser.baudrate = APP_BAUD
    ser.timeout = 5
    ser.write(bytes(784))       # dummy image eats the 1234 markers
    r = ser.read(9)             # 4 markers + digit + 4 cycle bytes
    print("handshake:", r.hex(" "))
    if r[:4] != b"1234":
        raise SystemExit("app not answering at APP_BAUD")
    ser.reset_input_buffer()
    print("app running")


def main():
    ser = serial.Serial()
    ser.port = PORT
    ser.baudrate = BOOT_BAUD
    ser.timeout = 5
    ser.dtr = False
    ser.rts = False
    ser.open()
    boot(ser)

    cap = cv2.VideoCapture(0, cv2.CAP_DSHOW)
    if not cap.isOpened():
        raise SystemExit("camera not found")

    th = threading.Thread(target=worker, args=(ser,), daemon=True)
    th.start()

    while True:
        ok, frame = cap.read()
        if not ok:
            break
        h, w = frame.shape[:2]
        x0, y0 = (w - BOX) // 2, (h - BOX) // 2
        state["frame"] = frame[y0:y0 + BOX, x0:x0 + BOX].copy()

        cv2.rectangle(frame, (x0, y0), (x0 + BOX, y0 + BOX), (0, 255, 0), 2)

        p = state["prev"]
        if p is not None:
            small = cv2.resize(p, (112, 112), interpolation=cv2.INTER_NEAREST)
            frame[8:120, 8:120] = cv2.cvtColor(small, cv2.COLOR_GRAY2BGR)

        d = state["digit"]
        label = "-" if d is None else str(d)
        cv2.putText(frame, label, (x0 + BOX + 30, y0 + 140),
                    cv2.FONT_HERSHEY_SIMPLEX, 5, (0, 255, 0), 8)
        cv2.putText(frame, f"FPGA {state['us']:.0f} us | trip {state['rt'] * 1000:.0f} ms",
                    (10, h - 40), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
        mode = "dark ink on light" if state["dark"] else "light on dark"
        cv2.putText(frame, f"{mode}  [i] toggle  [q] quit  {state['status']}",
                    (10, h - 12), cv2.FONT_HERSHEY_SIMPLEX, 0.55, (0, 255, 255), 1)

        cv2.imshow("FPGA digit recognition", frame)
        key = cv2.waitKey(1) & 0xFF
        if key == ord("q"):
            break
        if key == ord("i"):
            state["dark"] = not state["dark"]

    state["run"] = False
    th.join()
    ser.close()
    cap.release()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()