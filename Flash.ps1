param(
    [string]$Port = 'COM5',
    [string]$DeviceStudioRoot = (Join-Path $PSScriptRoot '..\DeviceStudio')
)
$ErrorActionPreference = 'Stop'
$uploader = Join-Path $DeviceStudioRoot 'flash_cli.py'
$firmware = Join-Path $PSScriptRoot 'Firmware\Smartwatch.dfw'
if (!(Test-Path -LiteralPath $uploader)) { throw 'DeviceStudio checkout bulunamadi. -DeviceStudioRoot ile yolunu belirtin.' }
if (!(Test-Path -LiteralPath $firmware)) { throw 'Once Build.ps1 ile DeviceBoot uyumlu firmware paketini derleyin.' }
$bundledPython = Join-Path $env:USERPROFILE '.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
$python = if (Test-Path -LiteralPath $bundledPython) { $bundledPython } else { (Get-Command python -ErrorAction Stop).Source }
& $python $uploader $firmware --port $Port
if ($LASTEXITCODE -ne 0) { throw 'Custom bootloader uzerinden yukleme basarisiz. Ilk DeviceBoot kurulumunu Device Studio ile yapin.' }
