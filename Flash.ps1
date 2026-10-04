param(
    [string]$IdeRoot = 'C:\ST\STM32CubeIDE_2.2.0\STM32CubeIDE',
    [Parameter(Mandatory=$true)][string]$SerialNumber
)
$ErrorActionPreference = 'Stop'
$pluginPath = Join-Path $IdeRoot 'plugins'
$programmer = Get-ChildItem -LiteralPath $pluginPath -Directory |
    Where-Object Name -Like 'com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_*' |
    Sort-Object Name -Descending | Select-Object -First 1
if (!$programmer) { throw 'STM32CubeProgrammer araci bulunamadi.' }
$cli = Join-Path $programmer.FullName 'tools\bin\STM32_Programmer_CLI.exe'
$firmware = Join-Path $PSScriptRoot 'Debug\Smartwatch.elf'
if (!(Test-Path -LiteralPath $firmware)) { throw 'Once Build.ps1 ile projeyi derleyin.' }
& $cli -c port=SWD "sn=$SerialNumber" mode=UR -d $firmware -v -g 0x08000000
if ($LASTEXITCODE -ne 0) { throw 'Karta yukleme basarisiz.' }

