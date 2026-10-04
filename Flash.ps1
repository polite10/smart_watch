param(
    [string]$IdeRoot = 'C:\ST\STM32CubeIDE_2.2.0\STM32CubeIDE',
    [string]$SerialNumber = '001D000E4D4B500620373831'
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
Push-Location -LiteralPath (Join-Path $PSScriptRoot 'Debug')
try {
    & $cli -c port=SWD "sn=$SerialNumber" mode=UR -d Smartwatch.elf -v -g 0x08000000
    if ($LASTEXITCODE -ne 0) { throw 'Karta yukleme basarisiz.' }
} finally { Pop-Location }
