param(
    [string]$IdeRoot = 'C:\ST\STM32CubeIDE_2.2.0\STM32CubeIDE',
    [string]$SerialNumber = '001D000E4D4B500620373831',
    [int]$Port = 61236,
    [switch]$WriteCommandOnly
)
$ErrorActionPreference = 'Stop'
$tempPath = Join-Path $PSScriptRoot 'tmp'
if ($WriteCommandOnly) {
    # Invoked by GDB after stopping at the UI task boundary, not before connecting.
    $now = [TimeZoneInfo]::ConvertTimeBySystemTimeZoneId([DateTime]::UtcNow, 'Turkey Standard Time')
    $command = @"
set `$clock_ok = (int)watch_clock_set_datetime($($now.Year),$($now.Month),$($now.Day),$($now.Hour),$($now.Minute),$($now.Second))
if !`$clock_ok
  echo CLOCK_SYNC_FAILED\n
  detach
  quit 1
end
printf "CLOCK_SYNC_OK: $($now.ToString('yyyy-MM-dd HH:mm:ss')) Europe/Istanbul\n"
call lv_screen_load(screens[HOME])
"@
    [IO.File]::WriteAllText((Join-Path $tempPath 'current-time.gdb'), $command, [Text.Encoding]::ASCII)
    exit 0
}
if (!(Test-Path -LiteralPath (Join-Path $PSScriptRoot 'Debug\Smartwatch.elf'))) {
    throw 'Debug/Smartwatch.elf gerekli. Karta yuklenen surumle ayni ELF dosyasini kullanin.'
}
$plugins = Join-Path $IdeRoot 'plugins'
function Find-Tools([string]$pattern) {
    $folder = Get-ChildItem -LiteralPath $plugins -Directory | Where-Object Name -Like $pattern |
        Sort-Object Name -Descending | Select-Object -First 1
    if (!$folder) { throw "Arac bulunamadi: $pattern" }
    return (Join-Path $folder.FullName 'tools\bin')
}
$gnu = Find-Tools 'com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.*'
$programmer = Find-Tools 'com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_*'
$server = Join-Path (Find-Tools 'com.st.stm32cube.ide.mcu.externaltools.stlink-gdb-server.win32_*') 'ST-LINK_gdbserver.exe'
if (Get-NetTCPConnection -State Listen -LocalPort $Port -ErrorAction SilentlyContinue) {
    throw "Port $Port kullanimda. Debug oturumunu kapatin veya -Port ile baska port secin."
}
New-Item -ItemType Directory -Force -Path $tempPath | Out-Null
$gdbScript = @"
set pagination off
set confirm off
set remotetimeout 20
target remote localhost:$Port
tbreak refresh_rtc
continue
shell powershell.exe -NoProfile -ExecutionPolicy Bypass -File ../Sync-Time.ps1 -WriteCommandOnly
source ../tmp/current-time.gdb
detach
quit
"@
[IO.File]::WriteAllText((Join-Path $tempPath 'sync-time.gdb'), $gdbScript, [Text.Encoding]::ASCII)
$serverLog = Join-Path $tempPath 'sync-server.log'
$serverError = Join-Path $tempPath 'sync-server-error.log'
$process = Start-Process -FilePath $server -ArgumentList @('-d','-g','-e','-i',$SerialNumber,'-p',"$Port",'-cp',('"'+$programmer+'"')) -WindowStyle Hidden -PassThru -RedirectStandardOutput $serverLog -RedirectStandardError $serverError
try {
    $ready = $false
    for ($attempt=0; $attempt -lt 40; ++$attempt) {
        if ($process.HasExited) { throw "ST-LINK sunucusu baslatilamadi. $serverError dosyasini inceleyin." }
        if (Get-NetTCPConnection -State Listen -LocalPort $Port -ErrorAction SilentlyContinue) { $ready=$true; break }
        Start-Sleep -Milliseconds 250
    }
    if (!$ready) { throw 'ST-LINK sunucusu zamaninda hazir olmadi.' }
    Push-Location -LiteralPath (Join-Path $PSScriptRoot 'Debug')
    try {
        $gdbLog = Join-Path $tempPath 'sync-gdb.log'
        $gdbError = Join-Path $tempPath 'sync-gdb-error.log'
        $debugger = Start-Process -FilePath (Join-Path $gnu 'arm-none-eabi-gdb.exe') -ArgumentList @('--nx','--batch','-x','../tmp/sync-time.gdb','Smartwatch.elf') -WorkingDirectory (Get-Location).Path -WindowStyle Hidden -PassThru -RedirectStandardOutput $gdbLog -RedirectStandardError $gdbError
        try {
            if (!$debugger.WaitForExit(45000)) {
                Stop-Process -Id $debugger.Id
                throw 'Saat esitleme zaman asimina ugradi; karta yuklenen ELF surumunu kontrol edin.'
            }
            $debugger.Refresh()
            $exitCode = $debugger.ExitCode
            $result = @(Get-Content -LiteralPath $gdbLog) + @(Get-Content -LiteralPath $gdbError)
        } finally {
            if (!$debugger.HasExited) { Stop-Process -Id $debugger.Id }
        }
        $result | ForEach-Object { Write-Host $_ }
        if ($exitCode -ne 0 -or !($result -match 'CLOCK_SYNC_OK:')) { throw 'Saat esitleme basarisiz. Karta yuklenen ELF surumunu ve ST-LINK baglantisini kontrol edin.' }
    } finally { Pop-Location }
} finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id }
}
