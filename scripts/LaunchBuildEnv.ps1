# Copyright (c) 2026 Randall Rosas (Slategray).
# All rights reserved.

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# BUILD ENVIRONMENT SETUP
# Locates eWDK/SDK and sets up standard environment variables for NMAKE/MSBuild.
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

$ErrorActionPreference = "Stop"

$ewdkRoot = $env:LEYLINE_EWDK_ROOT
if (-not $ewdkRoot) {
    $ewdkRoot = "D:\eWDK_28000"
    if (-not (Test-Path $ewdkRoot)) { $ewdkRoot = "C:\eWDK_28000" }
    if (-not (Test-Path $ewdkRoot)) { $ewdkRoot = "D:\eWDK" }
    if (-not (Test-Path $ewdkRoot)) { $ewdkRoot = "C:\eWDK" }
}

$sdkVersion = $env:LEYLINE_SDK_VERSION
if (-not $sdkVersion) { $sdkVersion = "10.0.28000.0" }

if (Test-Path $ewdkRoot) {
    Write-Host "[*] Using eWDK at: $ewdkRoot (SDK: $sdkVersion)" -ForegroundColor Gray
    $env:WDK_ROOT = "$ewdkRoot\Program Files\Windows Kits\10"
    $env:WDKContentRoot = $env:WDK_ROOT
    $env:eWDK_ROOT_DIR = $ewdkRoot

    # Initialize environment via SetupBuildEnv.cmd.
    $cmd = "`"$ewdkRoot\BuildEnv\SetupBuildEnv.cmd`" amd64 $sdkVersion && set"
    $envVars = cmd.exe /c $cmd
    foreach ($line in $envVars) {
        if ($line -match "^([^=]+)=(.*)$") {
            $name = $matches[1]; $value = $matches[2]
            if ($name -eq "PATH") { $env:PATH = $value + ";" + $env:PATH }
            else { [System.Environment]::SetEnvironmentVariable($name, $value, "Process") }
        }
    }

    # Add UM/UCRT lib paths for any host-mode compilation (Inf2Cat, etc.).
    $umLibPath = "$ewdkRoot\Program Files\Windows Kits\10\Lib\$sdkVersion\um\x64"
    $ucrtLibPath = "$ewdkRoot\Program Files\Windows Kits\10\Lib\$sdkVersion\ucrt\x64"
    $currentLib = [System.Environment]::GetEnvironmentVariable("LIB", "Process")
    $newLib = $currentLib
    if (Test-Path $umLibPath) { $newLib = "$newLib;$umLibPath" }
    if (Test-Path $ucrtLibPath) { $newLib = "$newLib;$ucrtLibPath" }
    [System.Environment]::SetEnvironmentVariable("LIB", $newLib, "Process")
}
else {
    Write-Warning "eWDK not found at '$ewdkRoot', falling back to system paths."
    $env:WDK_ROOT = "C:\Program Files (x86)\Windows Kits\10"
}

# Resolve tool paths.
$kitsRoot = if ($env:eWDK_ROOT_DIR) { "$env:eWDK_ROOT_DIR\Program Files\Windows Kits\10" } else { "C:\Program Files (x86)\Windows Kits\10" }
$vsRoot = if ($env:eWDK_ROOT_DIR) { "$env:eWDK_ROOT_DIR\Program Files\Microsoft Visual Studio" } else { "" }

$st = Get-ChildItem -Path "$kitsRoot\bin" -Filter signtool.exe  -Recurse | Where-Object { $_.FullName -match "x64" } | Select-Object -First 1
$ic = Get-ChildItem -Path "$kitsRoot\bin" -Filter Inf2Cat.exe   -Recurse | Select-Object -First 1

# MSBuild is in VS folder.
$mb = $null
if ($vsRoot -and (Test-Path $vsRoot)) {
    $mb = Get-ChildItem -Path "$vsRoot" -Filter MSBuild.exe -Recurse | Where-Object { $_.FullName -match "Bin\\amd64\\MSBuild.exe" } | Select-Object -First 1
}

if ($st) { $env:SIGNTOOL_EXE = $st.FullName }
if ($ic) { $env:INF2CAT_EXE = $ic.FullName }
if ($mb) { $env:MSBUILD_EXE = $mb.FullName }

Write-Host "[SUCCESS] Environment Set. SignTool=$env:SIGNTOOL_EXE  MSBuild=$env:MSBUILD_EXE" -ForegroundColor Green
