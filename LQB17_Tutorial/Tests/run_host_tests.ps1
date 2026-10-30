param([string]$GccPath = 'D:\Downloads\mingw64\bin\gcc.exe')
$ErrorActionPreference = 'Stop'
$testRoot = $PSScriptRoot
$firmwareRoot = Join-Path $testRoot '..\Firmware'
$buildRoot = Join-Path $testRoot 'build'
New-Item -ItemType Directory -Force -Path $buildRoot | Out-Null
$exePath = Join-Path $buildRoot 'test_phase2.exe'
$arguments = @(
    '-std=c99', '-Wall', '-Wextra', '-Werror', '-pedantic',
    '-I', (Join-Path $testRoot 'host_stubs'),
    '-I', (Join-Path $firmwareRoot 'BSP_Drivers\Inc'),
    (Join-Path $testRoot 'test_phase2.c'),
    (Join-Path $firmwareRoot 'BSP_Drivers\Src\bsp_led.c'),
    (Join-Path $firmwareRoot 'BSP_Drivers\Src\bsp_lcd.c'),
    (Join-Path $firmwareRoot 'BSP_Drivers\Src\bsp_time.c'),
    '-o', $exePath
)
& $GccPath @arguments
if ($LASTEXITCODE -ne 0) { throw "Host compilation failed: $LASTEXITCODE" }
& $exePath | Tee-Object -FilePath (Join-Path $buildRoot 'host_results.txt')
if ($LASTEXITCODE -ne 0) { throw "Host tests failed: $LASTEXITCODE" }
