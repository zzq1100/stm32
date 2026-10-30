param([string]$GccPath = 'D:\Downloads\mingw64\bin\gcc.exe')
$ErrorActionPreference = 'Stop'
$firmwareRoot = Join-Path $PSScriptRoot '..\Firmware'
$buildRoot = Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Force -Path $buildRoot | Out-Null
$exePath = Join-Path $buildRoot 'test_phase4.exe'
$arguments = @('-std=c99', '-Wall', '-Wextra', '-Werror', '-pedantic',
    '-I', (Join-Path $PSScriptRoot 'host4_stubs'),
    '-I', (Join-Path $firmwareRoot 'BSP_Drivers\Inc'),
    (Join-Path $PSScriptRoot 'test_phase4.c'))
foreach ($module in @('settings','control','totalizer','alarms','display','app','measurement','bsp_time')) {
    $arguments += Join-Path $firmwareRoot "BSP_Drivers\Src\$module.c"
}
$arguments += @('-lm', '-o', $exePath)
& $GccPath @arguments
if ($LASTEXITCODE -ne 0) { throw "Host compilation failed: $LASTEXITCODE" }
& $exePath | Tee-Object -FilePath (Join-Path $buildRoot 'host_phase4_results.txt')
if ($LASTEXITCODE -ne 0) { throw "Host tests failed: $LASTEXITCODE" }
