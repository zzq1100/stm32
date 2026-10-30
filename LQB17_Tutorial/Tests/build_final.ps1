param([string]$UV4Path = 'D:\Tools\Keil\UV4\UV4.exe',
      [string]$PythonPath = 'D:\Tools\Deepseek Harness\resources\runtime\primary-runtime\dependencies\python\python.exe')
$ErrorActionPreference = 'Stop'
$project = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\Firmware\MDK-ARM\LQB17_Tutorial.uvprojx')).Path
$logPath = Join-Path $PSScriptRoot 'build\keil_final.log'
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $logPath) | Out-Null
$uvArguments = @('-r', ('"' + $project + '"'), '-t', 'LQB17_Final', '-o', ('"' + $logPath + '"'))
$buildProcess = Start-Process -FilePath $UV4Path -ArgumentList $uvArguments -WindowStyle Hidden -PassThru
$buildProcess.WaitForExit()
if (Test-Path -LiteralPath $logPath) { Get-Content -LiteralPath $logPath }
if ($buildProcess.ExitCode -ne 0) { throw "Keil build returned $($buildProcess.ExitCode); inspect $logPath" }

& $PythonPath -X utf8 (Join-Path $PSScriptRoot 'package_submission.py') --record-build
if ($LASTEXITCODE -ne 0) { throw "Build input recording failed: $LASTEXITCODE" }
