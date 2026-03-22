$ErrorActionPreference = "Stop"
$VMName = "TestVM"
$Credential = New-Object System.Management.Automation.PSCredential ("User", (ConvertTo-SecureString "rd" -AsPlainText -Force))

Write-Host "[1] Running Install.ps1 with -fast to push the new driver and avoid restoring the stale snapshot..." -ForegroundColor Cyan
& .\Install.ps1 -fast

Write-Host "`n[2] Forcefully rebooting the VM to flush the locked kernel driver..." -ForegroundColor Cyan
Invoke-Command -VMName $VMName -Credential $Credential -ScriptBlock {
    Restart-Computer -Force
} -ErrorAction SilentlyContinue

Write-Host "[3] Waiting for VM to reboot and Audio Endpoint Builder to initialize (approx 45s)..." -ForegroundColor Yellow
$timer = [System.Diagnostics.Stopwatch]::StartNew()
Start-Sleep -Seconds 20

while ($timer.Elapsed.TotalSeconds -lt 90) {
    try {
        $test = Invoke-Command -VMName $VMName -Credential $Credential -ScriptBlock { return $true } -ErrorAction Stop
        if ($test) { break }
    } catch {
        Start-Sleep -Seconds 5
        Write-Host "." -NoNewline
    }
}
Write-Host "`n[+] VM is back online." -ForegroundColor Green

# Give audio endpoint builder a little extra time to enumerate the new paths
Start-Sleep -Seconds 15

Write-Host "`n[4] Running the test battery natively!" -ForegroundColor Cyan
& .\RunTestsOnVM.ps1
