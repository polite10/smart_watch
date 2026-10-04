param([string]$IdeRoot = 'C:\ST\STM32CubeIDE_2.2.0\STM32CubeIDE')
$ErrorActionPreference = 'Stop'
$workspacePath = Join-Path $PSScriptRoot '.local\workspace'
$configurationPath = Join-Path $PSScriptRoot '.local\configuration'
& (Join-Path $IdeRoot 'stm32cubeidec.exe') --launcher.suppressErrors -nosplash -configuration $configurationPath -application org.eclipse.cdt.managedbuilder.core.headlessbuild -data $workspacePath -import $PSScriptRoot -build Smartwatch/Debug
if ($LASTEXITCODE -ne 0) { throw 'CubeIDE build failed.' }
