# Automates building and running of the Leyline testing suite

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "LEYLINE VIRTUAL AUDIO DEVICE TEST RUNNER" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

# Ensure the driver is installed (requires admin)
If (!([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Host "WARNING: Tests might fail or skip if the driver is not currently installed or if you aren't running as Administrator!" -ForegroundColor Yellow
}

# 1. Compile Fuzzer (requires MSVC)
Write-Host "`n[1/3] Compiling Fuzzer..." -ForegroundColor Yellow
$fuzzerExe = ".\Fuzzer\Fuzzer.exe"
if (Get-Command cl.exe -ErrorAction SilentlyContinue) {
    cl.exe /nologo /O2 /EHsc .\Fuzzer\Fuzzer.cpp /Fe:$fuzzerExe
} else {
    Write-Host "WARNING: 'cl.exe' (MSVC) not in PATH. Skipping Fuzzer compilation." -ForegroundColor Red
}

# 2. Compile EndpointTester (requires .NET Core / MSBuild)
Write-Host "`n[2/3] Compiling EndpointTester..." -ForegroundColor Yellow
if (Get-Command dotnet -ErrorAction SilentlyContinue) {
    dotnet build .\EndpointTester\EndpointTester.csproj -c Release
} else {
    Write-Host "WARNING: 'dotnet' not in PATH. Please run this in a Developer Command Prompt." -ForegroundColor Red
}

# 3. RUN TESTS
Write-Host "`n[3/3] RUNNING TEST SUITE" -ForegroundColor Yellow

# A. Endpoint Performance & Quality Tests
$endpointTesterDll = ".\EndpointTester\bin\Release\netcoreapp3.1\EndpointTester.dll" # Adjust target framework if needed
if (Test-Path $endpointTesterDll) {
    Write-Host "`n--- Running Audio Quality & Latency Benchmarks ---" -ForegroundColor Green
    dotnet $endpointTesterDll
} else {
    Write-Host "EndpointTester build missing! Run 'dotnet build' inside the EndpointTester directory." -ForegroundColor Red
}

# B. CDO Fuzzing
if (Test-Path $fuzzerExe) {
    Write-Host "`n--- Running CDO Kernel Fuzzer ---" -ForegroundColor Green
    & $fuzzerExe
} else {
    Write-Host "Fuzzer executable not found." -ForegroundColor DarkGray
}

Write-Host "`n=========================================" -ForegroundColor Cyan
Write-Host "TEST RUN COMPLETE" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
