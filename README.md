# FPGA Digit Classifier on NEORV32: Build and Run Procedure

Omar · Digilent Arty A7-35 · 15 November 2026

## 1. Overview

A camera reads a handwritten digit. Python preprocesses it to 28×28 pixels and sends it over UART to a NEORV32 RISC-V CPU on the FPGA. The CPU passes the image to the `digit_mlp` HLS accelerator over AXI-Lite. The accelerator returns the digit and a cycle count. Python shows both on screen.

The network is a quantized MLP: 784 → 32 → 10, int8 weights, int32 accumulators, `SHIFT=12`, ReLU, clamp at 127, argmax.

## 2. Final results

| Metric | Start | Final | Gain |
|---|---:|---:|---:|
| FPGA cycles | 26,222 | 854 | 30.7× |
| FPGA time at 100 MHz | 262 µs | 8.5 µs | 30.7× |
| UART baud (app) | 19,200 | 1,000,000 | 52× |
| Round trip per image | 406 ms | 15 ms | 27× |

Trip history: 406 ms at 19200 baud, 77 ms at 115200 baud, 15 ms at 1 Mbaud.

## 3. Data flow

1. Camera frame, 300×300 center box.
2. `preprocess()` in `live_class.py`: blur, Otsu threshold, remove small blobs, crop, scale to 20 px, center in 28×28.
3. UART: 784 raw bytes to the board.
4. NEORV32 `main.c`: packs 4 bytes into 196 words, writes them to the IP at `0x44A00400`.
5. Start: write 1 to `0x44A00000`. Poll bit 1 for done.
6. Read the digit at `0x44A00010`. Cycle count comes from the CPU cycle counter.
7. UART back: 5 bytes. Byte 0 is the digit. Bytes 1 to 4 are the cycle count, little-endian.

On the first image only, the firmware also sends markers `1 2 3 4`. The handshake in `boot()` uses them to confirm the app runs.

## 4. Files

| File | Role |
|---|---|
| `hls/digitclassifier/digit_mlp.cpp`, `hls/digitclassifier/digit_mlp.h`, `hls/digitclassifier/weights.h` | HLS accelerator and weights |
| `neorv32-main/sw/example/digit_classifier/main.c` | NEORV32 firmware |
| `neorv32-main/sw/example/digit_classifier/build.sh` | Builds the bin and copies it to `test\` |
| `build.ps1` | Runs `build.sh` from PowerShell |
| `test/live_class.py` | Uploads the bin, then runs the live camera app |
| `test/deprecated/upload.py` | Camera-free upload and classification test |
| `test/test_set.bin` | 1,000 test images followed by 1,000 labels |

## 5. One-time setup

1. Install MSYS2 (UCRT64) and the xPack RISC-V toolchain.
2. Toolchain path used here: `/c/riscv/xpack-riscv-none-elf-gcc-15.2.0-1-win32-x64/xpack-riscv-none-elf-gcc-15.2.0-1/bin`. It is set inside `build.sh`.
3. Copy `build.sh` into `neorv32-main/sw/example/digit_classifier/`.
4. Copy `build.ps1` to `C:\HLS\digitClassifierNeorv32\`.
5. Unblock the script once:

   ```powershell
   Unblock-File .\build.ps1
   ```

6. Install Python packages:

   ```powershell
   pip install numpy opencv-python pyserial
   ```

7. Put `live_class.py` and `neorv32_exe.bin` in `C:\HLS\digitClassifierNeorv32\test\`.
8. Optional: write the bitstream to the Arty QSPI flash. In Vivado Hardware Manager, add configuration memory device `s25fl128sxxxxxx0-spi-x1_x2_x4`, program the `.bit`, and set jumper JP1 to QSPI. The board then configures itself at power-up.
9. Optional: set the FTDI latency timer to 1 ms: Device Manager → Ports → COM7 → Properties → Port Settings → Advanced.

## 6. Daily run

1. Power the Arty and program the FPGA, unless the bitstream is in QSPI flash.
2. Run:

   ```powershell
   cd C:\HLS\digitClassifierNeorv32\test
   python .\live_class.py
   ```

3. Press the Arty reset button (C2) when asked.
4. Expected output:

   ```
   handshake: 31 32 33 34 xx 56 03 00 00
   app running
   ```

In the window, `[i]` toggles dark ink on light and light on dark. `[q]` quits.

The bitstream is lost at power-off. Laptop sleep can cut USB power. If the bootloader prompt never appears, program the FPGA again.

## 7. What to run after a change

| Changed | Run |
|---|---|
| `digit_mlp.cpp` or the block design | HLS export, Vivado bitstream, program FPGA, then `live_class.py` |
| `main.c` | `.\build.ps1`, then `live_class.py` |
| `live_class.py` or `upload.py` | `live_class.py` |
| Nothing | `live_class.py` |

`live_class.py` uploads the bin into RAM on every run. Firmware changes therefore need no Vivado rebuild.

## 8. Firmware build

The build command that `build.sh` runs:

```sh
make RISCV_PREFIX=riscv-none-elf- \
  USER_FLAGS+="-Wl,--defsym,__neorv32_rom_size=32k -Wl,--defsym,__neorv32_rom_base=0x00000000 -Wl,--defsym,__neorv32_ram_size=32k -Wl,--defsym,__neorv32_ram_base=0x80000000" \
  clean_all exe
```

From PowerShell:

```powershell
.\build.ps1
```

From MSYS:

```sh
./build.sh
```

The baud setting is `#define BAUD 1000000` in `main.c`. At 100 MHz the divisor is exactly 100. The bootloader always runs at 19200; only the app runs at `BAUD`.

## 9. HLS optimization

### Diagnosis

26,222 cycles decode as 784 (input copy) + 32×784 (layer 1) + 10×32 (layer 2) + about 30 overhead. Layer 1 is 96% of the time.

### Changes in `digit_mlp.cpp`

1. All 32 hidden neurons run in parallel. The loop over the 784 inputs runs once with `PIPELINE II=1`; the inner loop over neurons is fully unrolled.
2. `W1` is partitioned `block factor=32`. Each bank holds one neuron's 784 weights.
3. `W2` is partitioned `cyclic factor=32`. All 32 products of one output are read in one cycle.
4. `B1`, `acc`, and `h8` are fully partitioned.
5. The 784-cycle input copy is removed.
6. Layer 2 does one output per cycle.

The network, weights, arithmetic, and outputs are unchanged. All math is integer, so the summation order does not change the result.

### Result

A handshake showed `56 03 00 00` = 0x0356 = 854 cycles = 8.5 µs. The estimate was about 830.

### Notes

- The pragmas assume `N_HID = 32`. If it differs, change both `factor` values.
- Initial resource estimate: about 64 DSP and 32 BRAM18. The current synthesis report is summarized in the comparison section below. The A7-35T has 90 DSP; if DSP runs out after a future change, use `UNROLL factor=8` in layer 2.
- Floor: `img` is one memory-mapped read port, so 784 cycles is the minimum for layer 1 with this interface.
- After a rebuild, check in the Address Editor that `0x44A00000` and the image offset `0x400` did not move. Check that `AP_RETURN` is still at `0x010`.

### Flow

1. Vitis HLS: C synthesis, then export the IP.
2. Vivado: update the IP and generate the bitstream.
3. Program the FPGA.
4. Run `live_class.py`.

## 10. Serial and baud lessons

1. **Opening COM7 resets the board.** Windows toggles DTR and RTS. The uploaded app is lost because it lives in RAM. Open with `dtr=False` and `rts=False`, and upload and run in the same session. `live_class.py` does both.
2. **The bootloader is fixed at 19200 baud.** Upload at 19200. Switch to the app baud only after the app starts.
3. **Flush and wait before the baud switch.** After sending `e`, call `flush()` and wait 1.5 s while the bootloader banner finishes at 19200. Then change the baud. Without this, the `e` leaves at the new baud, the bootloader never starts the app, and replies are garbage. This was the real cause of the 115200 failures.
4. **Garbage signature.** Repeating `e0` and `00` bytes mean a baud mismatch or a restart into the bootloader.
5. **Stale bin.** `main.c` must have the new `BAUD`. The bin must be rebuilt and copied to `test\`. `build.sh` does both.
6. **First-image markers.** `main.c` sends `1 2 3 4` on the first image only. `boot()` sends a dummy image to consume them and checks the handshake.
7. **Cycle count to time.** Cycles come from the CPU counter, so divide by the CPU clock (100 MHz here).

## 11. Troubleshooting

| Symptom | Likely cause | Action |
|---|---|---|
| `no bootloader prompt` | FPGA blank, wrong COM port, reset not pressed | Program the FPGA, check `PORT`, press C2 |
| Digit 0 and millions of µs | Baud mismatch or board restarted | Match `BAUD` in `main.c` and `APP_BAUD` in Python; rebuild the bin |
| `app not answering at APP_BAUD` | Old bin or wrong baud | Run `build.ps1`; check the `test\` bin timestamp |
| `upload failed` or `upload timeout` | Bootloader not ready | Press C2 again and rerun |
| Digit wrong, preview image clean | Preprocessing or lighting | Press `i`, improve contrast, and center one digit |
| `riscv-none-elf-gcc: No such file` | Toolchain not on `PATH` | Check the `export PATH` line in `build.sh` |
| `build.ps1` not signed | Downloaded file blocked | Run `Unblock-File .\build.ps1` |
| `timeout: re-run live_class.py` on screen | Board reset or UART lost | Rerun `live_class.py` and press C2 |

## 12. Where the time goes now

| Stage | Time |
|---|---:|
| UART TX, 784 B at 1 Mbaud | 7.8 ms |
| Host, FTDI latency, Python | about 7 ms |
| FPGA compute | 0.0085 ms |
| Trip total | 15 ms |

The FPGA is 0.06% of the trip. Further gains:

1. Set the FTDI latency timer to 1 ms.
2. A 30 fps webcam gives a frame every 33 ms.
3. Send 1 bit per pixel: 98 B, about 1 ms. This loses grayscale, so test accuracy on `test_set.bin` first.

## 13. Verification checklist

1. `python .\upload.py .\neorv32_exe.bin .\test_set.bin` returns `31 32 33 34` plus the digit and cycle bytes.
2. The first test image returns digit 0, as before the optimization.
3. `live_class.py` prints `app running`. A handwritten 5 shows as 5.

## 14. Comparison with the MicroBlaze project

Repo: `github.com/alibiomar/digit-classifier-mb-fpga`.

| Metric | MicroBlaze repo | NEORV32 build |
|---|---|---|
| CPU | MicroBlaze, Vitis app | NEORV32 RISC-V, `main.c` |
| Memory path | Image goes to DDR at `MIG base + 0x100000`, then the IP reads it over `m_axi` | Image goes straight into IP registers at `0x44A00400` over AXI-Lite; no DDR |
| HLS image port | `m_axi`, depth 784 | `s_axilite` |
| Clock | 81.25 MHz | 100 MHz |
| HLS schedule | Sequential MACs | 32 neurons in parallel (final) |
| FPGA cycles | about 26,242 | 26,222, then 854 |
| FPGA time | 324 µs | 262 µs, then 8.5 µs |
| UART | 115200 | 115200, then 1 Mbaud |
| Round trip | 77 ms | 77 ms at 115200, then 15 ms |
| Firmware loading | Vitis build, JTAG ELF launch | Bin over the NEORV32 UART bootloader; no Vitis |
| Rebuild time | Full Vitis flow | `build.ps1`, then `live_class.py` |

### Same in both

- Network: 784 → 32 → 10, int8 weights, `SHIFT=12`.
- Notebook accuracy: 96.18% (from the MicroBlaze README; not re-run here).
- Camera preprocessing and serial protocol: 784 bytes in, 5 bytes out.

### What improved

- **FPGA time:** 324 µs to 8.5 µs, about 38× faster.
- **Trip:** 77 ms to 15 ms at the final 1 Mbaud setting, about 5× faster.
- **Iteration:** firmware changes now take seconds. No JTAG or Vitis is needed for `main.c`.
- **Python:** `live_class.py` uploads the bin and checks the handshake itself.
- **Clock constant:** Python uses `CLK_HZ = 100e6`, rather than 81.25 MHz.

### Resources and checks

The current Vitis HLS synthesis report for the NEORV32 accelerator estimates:

| Resource | Used | Available | Utilization |
|---|---:|---:|---:|
| BRAM18 | 32 | 100 | 32% |
| DSP | 47 | 90 | 52% |
| FF | 2,245 | 41,600 | 5% |
| LUT | 3,427 | 20,800 | 16% |

The report is `hls/digitclassifier/solution1/syn/report/digit_mlp_csynth.rpt`. The parallel core costs more resources than the sequential design, but it fits the Arty A7-35T. Recheck the post-synthesis and implementation utilization reports after future HLS or block-design changes.

The same parallel schedule could be applied to the MicroBlaze version, but the gain over `m_axi` would be smaller because burst reads from DDR add cycles.

### Accuracy status

The new core has not yet been run through all 1,000 images in `test_set.bin`. The arithmetic and weights are unchanged, so it is expected to match the 96.18% notebook result. Confirm this with one loop over the full test set before treating that accuracy as verified for the NEORV32 build.

## 15. Routed implementation comparison

The routed reports show that the NEORV32 system with the parallel core fits the Arty A7-35T with room left:

| Resource | MicroBlaze repo (routed) | NEORV32 + parallel core | Change |
|---|---:|---:|---:|
| Slice LUTs | 11,225 (54%) | 3,012 (14.5%) | 3.7× fewer |
| Registers | 12,740 (30.6%) | 3,083 (7.4%) | 4.1× fewer |
| Slices | 4,177 (51%) | 1,177 (14.4%) | 3.5× fewer |
| Block RAM tiles | 25 (50%) | 34.5 (69%) | +9.5 |
| RAMB36 / RAMB18 | 16 / 18 | 19 / 31 | — |
| DSP48E1 | 33 (36.7%) | 62 (68.9%) | +29 |

The routed NEORV32 netlist lists no MIG, DDR, or MicroBlaze. Only NEORV32, `digit_mlp`, and clocking remain, so the logic is about 3.7× smaller than the MicroBlaze system.

The parallel design uses 62 of 90 DSP48E1 sites and leaves 28 DSP sites free. It uses 69% of the block RAM tiles, leaving about 15.5 tiles. Most of the BRAM increase comes from the 32 `W1` weight banks.

The following is an approximate split derived from the routed report and the HLS estimate. These figures are estimates only: HLS is pre-implementation, and the two reports use different resource-counting conventions.

| Resource | Total (routed) | `digit_mlp` (HLS) | Rest of system (estimated) |
|---|---:|---:|---:|
| DSP48E1 / DSP | 62 | 47 | about 15 |
| Block RAM tiles / BRAM18 | 34.5 | 16 (32 BRAM18) | about 18.5 |

The routed report does not provide a per-module split, so these values should not be treated as implementation-level attribution.

The trade-off is 29 additional DSPs and 9.5 additional BRAM tiles for roughly 31× faster inference, while total logic use falls to about one quarter of the MicroBlaze system.

### Remaining implementation checks

- **Timing:** run `report_timing_summary` and verify WNS ≥ 0 at 100 MHz. Thirty-two parallel MACs can make timing closure harder; successful board operation is not a substitute for the timing report.
- **Power:** power has not been measured or estimated in the routed comparison.

