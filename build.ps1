# Build neorv32_exe.bin from PowerShell (no MSYS window needed)
# Usage: .\build.ps1

$msys = "C:\msys64"   # adjust if MSYS2 is installed elsewhere
$script = "/c/HLS/digitClassifierNeorv32/neorv32-main/sw/example/digit_classifier/build.sh"

$env:MSYSTEM = "UCRT64"
$env:CHERE_INVOKING = "1"
& "$msys\usr\bin\bash.exe" -lc "bash $script"
if ($LASTEXITCODE -ne 0) { Write-Error "build failed"; exit 1 }
