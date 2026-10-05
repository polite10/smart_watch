param([string]$IdeRoot = 'C:\ST\STM32CubeIDE_2.2.0\STM32CubeIDE', [switch]$Clean)
$ErrorActionPreference = 'Stop'
$projectPath = $PSScriptRoot
$buildTime = [System.TimeZoneInfo]::ConvertTimeBySystemTimeZoneId([DateTime]::UtcNow, 'Turkey Standard Time')
$buildWeekday = if ([int]$buildTime.DayOfWeek -eq 0) { 7 } else { [int]$buildTime.DayOfWeek }
$timeHeader = "#define BUILD_YEAR $($buildTime.Year)`n#define BUILD_MONTH $($buildTime.Month)`n#define BUILD_DAY $($buildTime.Day)`n#define BUILD_HOUR $($buildTime.Hour)`n#define BUILD_MINUTE $($buildTime.Minute)`n#define BUILD_SECOND $($buildTime.Second)`n#define BUILD_WEEKDAY $buildWeekday`n"
[System.IO.File]::WriteAllText((Join-Path $PSScriptRoot 'Core\Inc\build_time.h'), $timeHeader, [System.Text.Encoding]::ASCII)
if ($projectPath -match '[^\x00-\x7F]') {
    if (-not ('Smartwatch.NativePath' -as [type])) {
        Add-Type -Name NativePath -Namespace Smartwatch -MemberDefinition '[System.Runtime.InteropServices.DllImport("kernel32.dll", CharSet=System.Runtime.InteropServices.CharSet.Unicode)] public static extern uint GetShortPathName(string path, System.Text.StringBuilder result, uint size);'
    }
    $shortPath = New-Object System.Text.StringBuilder 1024
    if ([Smartwatch.NativePath]::GetShortPathName($projectPath, $shortPath, 1024)) { $projectPath = $shortPath.ToString() }
}
# Eclipse requires its workspace to be outside the imported project.
$buildRoot = Join-Path ([System.IO.Path]::GetTempPath()) 'Smartwatch-CubeIDE'
$workspacePath = Join-Path $buildRoot 'workspace'
$configurationPath = Join-Path $buildRoot 'configuration'
# Launch Java directly so Turkish characters survive the console launcher's
# legacy ANSI argument conversion on Windows.
$pluginPath = Join-Path $IdeRoot 'plugins'
$jre = Get-ChildItem -LiteralPath $pluginPath -Directory | Where-Object Name -Like 'com.st.stm32cube.ide.jre.win64_*' | Sort-Object Name -Descending | Select-Object -First 1
$launcher = Get-ChildItem -LiteralPath $pluginPath -File -Filter 'org.eclipse.equinox.launcher_*.jar' | Sort-Object Name -Descending | Select-Object -First 1
if (!$jre -or !$launcher) { throw 'CubeIDE Java/launcher bulunamadi.' }
$buildAction = if ($Clean) { '-cleanBuild' } else { '-build' }
& (Join-Path $jre.FullName 'jre\bin\java.exe') '-Dosgi.requiredJavaVersion=1.8' -jar $launcher.FullName -nosplash -configuration $configurationPath -application org.eclipse.cdt.managedbuilder.core.headlessbuild -data $workspacePath -import $projectPath $buildAction Smartwatch/Debug
if ($LASTEXITCODE -ne 0) { throw 'CubeIDE derlemesi basarisiz.' }
# Keep the distributable binary in sync with the ELF produced by this build.
$toolchain = Get-ChildItem -LiteralPath $pluginPath -Directory |
    Where-Object Name -Like 'com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.*' |
    Sort-Object Name -Descending | Select-Object -First 1
if (!$toolchain) { throw 'ARM objcopy araci bulunamadi.' }
[System.IO.Directory]::CreateDirectory((Join-Path $PSScriptRoot 'Firmware')) | Out-Null
Push-Location -LiteralPath $PSScriptRoot
try {
    & (Join-Path $toolchain.FullName 'tools\bin\arm-none-eabi-objcopy.exe') -O binary 'Debug/Smartwatch.elf' 'Firmware/Smartwatch.bin'
    if ($LASTEXITCODE -ne 0) { throw 'Firmware binary olusturulamadi.' }
} finally { Pop-Location }
