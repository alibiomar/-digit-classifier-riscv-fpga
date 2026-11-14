import sys, time
import serial

PORT = "COM7"
EXE = sys.argv[1] if len(sys.argv) > 1 else "neorv32_exe.bin"

s = serial.Serial()
s.port = PORT
s.baudrate = 19200
s.timeout = 0.3
s.dtr = False
s.rts = False
s.open()

print("Press the Arty reset button (C2) now...")
buf, t0 = b"", time.time()
while b"CMD:>" not in buf:
    if time.time() - t0 > 30:
        sys.exit("no bootloader prompt")
    s.write(b" ")
    buf += s.read(256)

s.reset_input_buffer()
s.write(b"u")
buf = b""
while b"Awaiting" not in buf:
    buf += s.read(64)
s.write(open(EXE, "rb").read())
buf, t0 = b"", time.time()
while b"OK" not in buf and b"ERR" not in buf:
    if time.time() - t0 > 30:
        sys.exit("upload timeout")
    buf += s.read(64)
if b"ERR" in buf:
    sys.exit("upload failed")
s.reset_input_buffer()
s.write(b"e")
s.flush()
time.sleep(1.5)

# app must stream 'U' (0x55) forever
for baud in (19200, 38400, 57600, 115200, 230400):
    s.baudrate = baud
    time.sleep(0.2)
    s.reset_input_buffer()
    s.timeout = 1
    r = s.read(12)
    print(f"{baud:>7}: {r.hex(' ')}")
s.close()