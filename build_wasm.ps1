$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$emsdk = Join-Path $root "build\emsdk"
if (-not (Test-Path (Join-Path $emsdk "emsdk_env.ps1"))) {
  Write-Error "emsdk is not installed at $emsdk"
  exit 1
}
. (Join-Path $emsdk "emsdk_env.ps1")
emcmake cmake -S $root -B (Join-Path $root "build\wasm") -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
cmake --build (Join-Path $root "build\wasm")
exit $LASTEXITCODE
