# Copyright (c) 2026 Randall Rosas (Slategray).
# All rights reserved.

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# VM UNINSTALLER
# Delegates to Install.ps1 with the -Uninstall flag.
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

param (
    [string]$VMName = $(if ($env:LEYLINE_VM_NAME) { $env:LEYLINE_VM_NAME } else { "TestVM" })
)

Write-Host "[*] Triggering Consolidated VM Uninstall..." -ForegroundColor Cyan
& "$PSScriptRoot\Install.ps1" -Uninstall -VMName $VMName
