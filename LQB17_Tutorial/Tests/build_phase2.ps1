param([string]$UV4Path = 'D:\Tools\Keil\UV4\UV4.exe')
$ErrorActionPreference = 'Stop'
$project = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\Firmware\MDK-ARM\LQB17_Tutorial.uvprojx')).Path
$buildRoot = Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Force -Path $buildRoot | Out-Null
$logPath = Join-Path $buildRoot 'keil_phase2.log'
$uvArguments = @('-r', ('"' + $project + '"'), '-t', 'Phase2_SelfTest', '-o', ('"' + $logPath + '"'))
$buildProcess = Start-Process -FilePath $UV4Path -ArgumentList $uvArguments -WindowStyle Hidden -PassThru
$buildProcess.WaitForExit()
if (Test-Path -LiteralPath $logPath) { Get-Content -LiteralPath $logPath }
if ($buildProcess.ExitCode -ne 0) { throw "Keil build returned $($buildProcess.ExitCode); inspect $logPath" }
