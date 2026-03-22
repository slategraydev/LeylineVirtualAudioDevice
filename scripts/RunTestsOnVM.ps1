# Copyright (c) 2026 Randall Rosas (Slategray).
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# DEPLOYS AND RUNS TESTS ON THE HYPER-V TEST VM
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

param (
    [string]$VMName = "TestVM",
    [string]$VMUser = "User",
    [string]$VMPassword = "rd"
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Resolve-Path "$PSScriptRoot\.."
$remoteTestPath = "C:\LeylineTest"

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "LEYLINE VM TEST DEPLOYMENT & EXECUTION" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

# 1. Compile Host Binaries
Write-Host "`n[*] Compiling Fuzzer on Host..." -ForegroundColor Yellow
$fuzzerSrc = "$ProjectRoot\test\Fuzzer\Fuzzer.cpp"
$fuzzerExe = "$ProjectRoot\test\Fuzzer\Fuzzer.exe"
if (Get-Command cl.exe -ErrorAction SilentlyContinue) {
    cl.exe /nologo /O2 /EHsc $fuzzerSrc /Fe:$fuzzerExe
} else {
    Write-Warning "cl.exe not in PATH. Please run this in a Developer Command Prompt."
}

Write-Host "[*] Compiling EndpointTester on Host..." -ForegroundColor Yellow
if (Get-Command dotnet -ErrorAction SilentlyContinue) {
    dotnet build "$ProjectRoot\test\EndpointTester\EndpointTester.csproj" -c Release
} else {
    Write-Warning "dotnet not in PATH."
}

# 2. Connect to VM
Write-Host "`n[*] Connecting to VM '$VMName'..." -ForegroundColor Cyan
$secPassword = ConvertTo-SecureString $VMPassword -AsPlainText -Force
$Credential = New-Object System.Management.Automation.PSCredential ($VMUser, $secPassword)

try {
    $vmsess = New-PSSession -VMName $VMName -Credential $Credential
    
    # Clean remote DIR
    Invoke-Command -Session $vmsess -ScriptBlock {
        param($path)
        if (Test-Path $path) { Remove-Item $path -Recurse -Force }
        New-Item -ItemType Directory -Path $path -Force | Out-Null
    } -ArgumentList $remoteTestPath
    
    # 3. Copy binaries to VM
    Write-Host "[*] Copying Fuzzer and EndpointTester binaries to VM..." -ForegroundColor Cyan
    Copy-Item "$ProjectRoot\test\Fuzzer\Fuzzer.exe" -Destination "$remoteTestPath\" -ToSession $vmsess -Force -ErrorAction SilentlyContinue
    Copy-Item "$ProjectRoot\test\EndpointTester\bin\Release\net8.0-windows\*" -Destination "$remoteTestPath\" -ToSession $vmsess -Recurse -Force -ErrorAction SilentlyContinue
    
    # 4. Execute on VM
    Write-Host "`n[*] RUNNING TESTS IN VIRTUAL MACHINE..." -ForegroundColor Green
    Invoke-Command -Session $vmsess -ScriptBlock {
        param($path)
        Set-Location $path
        
        Write-Host "`n--- VM Execution: Audio Quality & Benchmarks ---" -ForegroundColor Yellow
        if (Test-Path ".\EndpointTester.exe") {
            .\EndpointTester.exe
        } elseif (Test-Path ".\EndpointTester.dll") {
            dotnet .\EndpointTester.dll
        } else {
            Write-Host "EndpointTester not found."
        }
        
        Write-Host "`n--- VM Execution: CDO Kernel Fuzzer ---" -ForegroundColor Yellow
        if (Test-Path ".\Fuzzer.exe") {
            .\Fuzzer.exe
        } else {
            Write-Host "Fuzzer.exe not found."
        }
    } -ArgumentList $remoteTestPath

} catch {
    Write-Error "VM Deployment Failed: $_"
} finally {
    if ($vmsess) { Remove-PSSession $vmsess }
    Write-Host "`n[*] Disconnected."
}
