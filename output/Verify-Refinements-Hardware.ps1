$ErrorActionPreference='Stop'
$projectRoot=Split-Path -Parent $PSScriptRoot
$plugins='C:\ST\STM32CubeIDE_2.2.0\STM32CubeIDE\plugins'
function ToolFolder([string]$pattern) {
    $folder=Get-ChildItem -LiteralPath $plugins -Directory | Where-Object Name -Like $pattern | Sort-Object Name -Descending | Select-Object -First 1
    if (!$folder) { throw "Tool missing: $pattern" }
    Join-Path $folder.FullName 'tools\bin'
}
$gnu=ToolFolder 'com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.*'
$programmer=ToolFolder 'com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_*'
$server=Join-Path (ToolFolder 'com.st.stm32cube.ide.mcu.externaltools.stlink-gdb-server.win32_*') 'ST-LINK_gdbserver.exe'
if(Get-NetTCPConnection -State Listen -LocalPort 61239 -ErrorAction SilentlyContinue) { throw 'Verification port occupied' }
$process=Start-Process -FilePath $server -ArgumentList @('-d','-g','-e','-i','001D000E4D4B500620373831','-p','61239','-cp',('"'+$programmer+'"')) -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $PSScriptRoot 'update-server.log') -RedirectStandardError (Join-Path $PSScriptRoot 'update-server-error.log')
try {
    $ready=$false
    for($attempt=0;$attempt -lt 40;++$attempt) {
        if($process.HasExited) { throw 'ST-LINK server exited' }
        if((Get-Content (Join-Path $PSScriptRoot 'update-server.log') -Raw -ErrorAction SilentlyContinue) -match 'Waiting for debugger connection') { $ready=$true; break }
        Start-Sleep -Milliseconds 250
    }
    if(!$ready) { throw 'ST-LINK server timed out' }
    $log=Join-Path $projectRoot 'output\verify-refinements-hardware.log'
    $gdb=Start-Process -FilePath (Join-Path $gnu 'arm-none-eabi-gdb.exe') -ArgumentList @('--nx','--batch','-x','../output/verify-refinements-hardware.gdb','Smartwatch.elf') -WorkingDirectory (Join-Path $projectRoot 'Debug') -WindowStyle Hidden -PassThru -RedirectStandardOutput $log -RedirectStandardError (Join-Path $PSScriptRoot 'update-gdb-error.log')
    try {
        if(!$gdb.WaitForExit(45000)) { Stop-Process -Id $gdb.Id; throw 'Verification timed out' }
        $gdb.Refresh()
        Get-Content $log
        Get-Content (Join-Path $PSScriptRoot 'update-gdb-error.log')
        if($gdb.ExitCode -ne 0 -or (Get-Content $log -Raw) -notmatch 'REFINEMENTS_HARDWARE_PASSED') { throw 'Runtime verification failed' }
    } finally { if(!$gdb.HasExited) { Stop-Process -Id $gdb.Id } }
} finally { if(!$process.HasExited) { Stop-Process -Id $process.Id } }
