#!/bin/bash
set -e
cd "$(dirname "$0")"
export PATH="/c/riscv/xpack-riscv-none-elf-gcc-15.2.0-1-win32-x64/xpack-riscv-none-elf-gcc-15.2.0-1/bin:$PATH"

DEST="/c/HLS/digitClassifierNeorv32/test"

make RISCV_PREFIX=riscv-none-elf- \
  USER_FLAGS+="-Wl,--defsym,__neorv32_rom_size=32k -Wl,--defsym,__neorv32_rom_base=0x00000000 -Wl,--defsym,__neorv32_ram_size=32k -Wl,--defsym,__neorv32_ram_base=0x80000000" \
  clean_all exe

cp -v neorv32_exe.bin "$DEST/"
echo "OK: bin copied to $DEST"