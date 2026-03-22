$VMName = "TestVM"
$Credential = New-Object System.Management.Automation.PSCredential ("User", (ConvertTo-SecureString "rd" -AsPlainText -Force))
$vmsess = New-PSSession -VMName $VMName -Credential $Credential

Invoke-Command -Session $vmsess -ScriptBlock {
    $ErrorActionPreference = "SilentlyContinue"

    # We enforce 16-bit 48000Hz PCM as the default format.
    # The magical Windows Audio Engine byte structure for this is standard WAVEFORMATEX.
    $wfx16_48 = [byte[]](1, 0, 2, 0, 128, 187, 0, 0, 0, 238, 2, 0, 4, 0, 16, 0, 0, 0)
    
    $paths = @("HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Render",
               "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Capture")

    $foundCount = 0
    foreach ($basePath in $paths) {
        $keys = Get-ChildItem $basePath
        foreach ($key in $keys) {
            $propPath = Join-Path $key.PSPath "Properties"
            if (Test-Path $propPath) {
                # {a45c254e-df1c-4efd-8020-67d146a850e0},2 is the device description property
                $descVal = (Get-ItemProperty $propPath)."{a45c254e-df1c-4efd-8020-67d146a850e0},2"
                if ($descVal -like "*Leyline*") {
                    Write-Host "Found Leyline Endpoint: $($key.Name)" -ForegroundColor Green

                    # Set Default Format (PKEY_AudioEngine_DeviceFormat)
                    Set-ItemProperty -Path $propPath -Name "{e4870e26-3cc5-4cd2-ba46-ca0a9a70ed04},0" -Value $wfx16_48 -Type Binary -Force
                    
                    # Set Device State to Active (1)
                    Set-ItemProperty -Path $key.PSPath -Name "DeviceState" -Value 1 -Type DWord -Force
                    $foundCount++
                }
            }
        }
    }
    
    if ($foundCount -gt 0) {
        Write-Host "Restarting Audio Services to enforce format..." -ForegroundColor Cyan
        Restart-Service AudioEndpointBuilder -Force
        Restart-Service Audiosrv -Force
    }
}
Remove-PSSession $vmsess
