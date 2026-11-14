import sys, time, os
import serial

PORT = "COM7"
BOOT_BAUD = 19200     # bootloader, fixed
APP_BAUD = 1000000     # must match BAUD in main.c
CLK_HZ = 100e6
FILE = sys.argv[1] if len(sys.argv) > 1 else "neorv32_exe.bin"
TEST = sys.argv[2] if len(sys.argv) > 2 else "test_set.bin"

s = serial.Serial()
s.port = PORT
s.baudrate = BOOT_BAUD
s.timeout = 0.3
s.dtr = False
s.rts = False
s.open()
print("Press the Arty reset button (C2) now...")

buf = b""
t0 = time.time()
while b"CMD:>" not in buf:
    if time.time() - t0 > 30:
        sys.exit("no bootloader prompt")
    s.write(b" ")
    buf += s.read(256)
print("bootloader ready")

s.reset_input_buffer()
s.write(b"u")
buf = b""
while b"Awaiting" not in buf:
    buf += s.read(64)

s.write(open(FILE, "rb").read())
buf = b""
t0 = time.time()
while b"OK" not in buf and b"ERR" not in buf:
    if time.time() - t0 > 30:
        sys.exit("upload timeout")
    buf += s.read(64)
print(buf.decode(errors="ignore"))
if b"ERR" in buf:
    sys.exit("upload failed")

s.reset_input_buffer()
s.write(b"e")
s.flush()
time.sleep(1.5)                 # banner ends at BOOT_BAUD
s.reset_input_buffer()
s.baudrate = APP_BAUD
s.timeout = 3

if os.path.exists(TEST):
    img = open(TEST, "rb").read()[:784]
    s.write(img)
    r = s.read(9)               # markers 1 2 3 4 + digit + 4 cycle bytes
    print("reply:", r.hex(" "))
    if r[:4] == b"1234" and len(r) == 9:
        cyc = int.from_bytes(r[5:], "little")
        print(f"digit {r[4]} | {cyc} cycles | {cyc / CLK_HZ * 1e6:.0f} us")
    else:
        print("bad reply: baud mismatch or app not running")
s.close()