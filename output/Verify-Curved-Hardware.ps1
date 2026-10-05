param([int]$Port=61235)
$ErrorActionPreference='Stop'
$projectRoot=Split-Path -Parent $PSScriptRoot
$plugins='C:\ST\STM32CubeIDE_2.2.0\STM32CubeIDE\plugins'
function ToolFolder([string]$pattern) {
    $folder=Get-ChildItem -LiteralPath $plugins -Directory | Where-Object Name -Like $pattern | Sort-Object Name -Descending | Select-Object -First 1
    if (!$folder) { throw "Tool missing: $pattern" }
    Join-Path $folder.FullName 'tools\bin'
}
if (Get-NetTCPConnection -State Listen -LocalPort $Port -ErrorAction SilentlyContinue) { throw 'Debug port is in use' }
$gnu=ToolFolder 'com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.*'
$programmer=ToolFolder 'com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_*'
$server=Join-Path (ToolFolder 'com.st.stm32cube.ide.mcu.externaltools.stlink-gdb-server.win32_*') 'ST-LINK_gdbserver.exe'
$firmwareSize=(Get-Item -LiteralPath (Join-Path $projectRoot 'Firmware\Smartwatch.bin')).Length
$gdbSource=(Get-Content (Join-Path $PSScriptRoot 'verify-curved-hardware.gdb') -Raw).Replace('localhost:61235',"localhost:$Port")
$gdbSource='set $firmware_size='+$firmwareSize+[Environment]::NewLine+$gdbSource
Set-Content -LiteralPath (Join-Path $projectRoot 'tmp\curved-hardware-active.gdb') -Value $gdbSource -Encoding ascii
$process=Start-Process -FilePath $server -ArgumentList @('-d','-g','-e','-i','001D000E4D4B500620373831','-p',"$Port",'-cp',('"'+$programmer+'"')) -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $projectRoot 'tmp\curved-server.log') -RedirectStandardError (Join-Path $projectRoot 'tmp\curved-server-error.log')
try {
    $ready=$false
    for($i=0;$i -lt 40;++$i) {
        if($process.HasExited) { throw 'ST-LINK server exited' }
        if(Get-NetTCPConnection -State Listen -LocalPort $Port -ErrorAction SilentlyContinue) { $ready=$true; break }
        Start-Sleep -Milliseconds 250
    }
    if(!$ready) { throw 'ST-LINK server timed out' }
    $gdb=Start-Process -FilePath (Join-Path $gnu 'arm-none-eabi-gdb.exe') -ArgumentList @('--nx','--batch','-x','../tmp/curved-hardware-active.gdb','Smartwatch.elf') -WorkingDirectory (Join-Path $projectRoot 'Debug') -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $PSScriptRoot 'verify-curved-hardware.log') -RedirectStandardError (Join-Path $projectRoot 'tmp\curved-gdb-error.log')
    try {
        if(!$gdb.WaitForExit(60000)) { Stop-Process -Id $gdb.Id; throw 'Hardware verification timed out' }
        $gdb.Refresh()
        Get-Content (Join-Path $PSScriptRoot 'verify-curved-hardware.log')
        Get-Content (Join-Path $projectRoot 'tmp\curved-gdb-error.log')
        if($gdb.ExitCode -ne 0) { throw 'Hardware verification failed' }
        if ((Get-Content (Join-Path $PSScriptRoot 'verify-curved-hardware.log') -Raw) -notmatch 'HARDWARE FAILURES: 0') { throw 'Hardware checks did not all pass' }
    } finally { if(!$gdb.HasExited) { Stop-Process -Id $gdb.Id } }
} finally { if(!$process.HasExited) { Stop-Process -Id $process.Id } }
