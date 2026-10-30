param([string]$UV4Path = 'D:\Tools\Keil\UV4\UV4.exe')
$ErrorActionPreference = 'Stop'
$project = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\Firmware\MDK-ARM\LQB17_Tutorial.uvprojx')).Path
$logPath = Join-Path $PSScriptRoot 'build\keil_phase3.log'
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $logPath) | Out-Null
$uvArguments = @('-r', ('"' + $project + '"'), '-t', 'Phase3_SelfTest', '-o', ('"' + $logPath + '"'))
$buildProcess = Start-Process -FilePath $UV4Path -ArgumentList $uvArguments -WindowStyle Hidden -PassThru
$buildProcess.WaitForExit()
if (Test-Path -LiteralPath $logPath) { Get-Content -LiteralPath $logPath }
if ($buildProcess.ExitCode -ne 0) { throw "Keil build returned $($buildProcess.ExitCode); inspect $logPath" }
