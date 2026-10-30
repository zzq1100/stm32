param([string]$GccPath = 'D:\Downloads\mingw64\bin\gcc.exe')
$ErrorActionPreference = 'Stop'
$firmwareRoot = Join-Path $PSScriptRoot '..\Firmware'
$buildRoot = Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Force -Path $buildRoot | Out-Null
$exePath = Join-Path $buildRoot 'test_phase3.exe'
$arguments = @('-std=c99', '-Wall', '-Wextra', '-Werror', '-pedantic',
    '-I', (Join-Path $PSScriptRoot 'host3_stubs'),
    '-I', (Join-Path $firmwareRoot 'BSP_Drivers\Inc'),
    (Join-Path $PSScriptRoot 'test_phase3.c'))
foreach ($module in @('bsp_keys','bsp_adc','measurement','bsp_capture','bsp_pwm')) {
    $arguments += Join-Path $firmwareRoot "BSP_Drivers\Src\$module.c"
}
$arguments += @('-lm', '-o', $exePath)
& $GccPath @arguments
if ($LASTEXITCODE -ne 0) { throw "Host compilation failed: $LASTEXITCODE" }
& $exePath | Tee-Object -FilePath (Join-Path $buildRoot 'host_phase3_results.txt')
if ($LASTEXITCODE -ne 0) { throw "Host tests failed: $LASTEXITCODE" }
