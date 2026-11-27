#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Shared helpers for make_portable_zip.ps1 and make_windows_packages.ps1. Dot-source this file.

# Use the Windows bsdtar, which can write zip files (a tar from Git or MSYS on PATH cannot)
$script:WindowsTar = Join-Path $env:SystemRoot 'System32\tar.exe'

function Assert-WindowsTar {
    if (-not (Test-Path $script:WindowsTar)) {
        throw "tar.exe not found at $script:WindowsTar (requires Windows 10 1803 or newer)"
    }
}

function Write-TextFile([string]$Path, [string]$Content) {
    # No BOM, so checksum files also work with sha256sum
    [System.IO.File]::WriteAllText($Path, $Content, (New-Object System.Text.UTF8Encoding $false))
}

function Get-EngineVersion([string]$EngineJsonPath) {
    # The display version is what the installer is named after; fall back to the engine version
    $EngineInfo = Get-Content -Raw $EngineJsonPath | ConvertFrom-Json
    $Version = $null
    if ($EngineInfo.PSObject.Properties['display_version']) {
        $Version = $EngineInfo.display_version
    }
    if ([string]::IsNullOrEmpty($Version) -or $Version -eq '00.00') {
        $Version = $EngineInfo.version
    }
    return $Version
}

function Get-SevenZip {
    $Command = Get-Command 7z.exe -ErrorAction SilentlyContinue
    if ($Command) {
        return $Command.Source
    }
    if (Test-Path "$env:ProgramFiles\7-Zip\7z.exe") {
        return "$env:ProgramFiles\7-Zip\7z.exe"
    }
    return $null
}

# Zips <SourceRoot>\<TopDir> into <Archive> with <TopDir> as the single top-level folder.
# -Store skips compression, for contents that are already compressed (MSI and .cab files).
function New-ZipArchive([string]$SourceRoot, [string]$TopDir, [string]$Archive, [switch]$Store) {
    if (Test-Path $Archive) {
        Remove-Item -Force $Archive
    }
    $SevenZip = Get-SevenZip
    if ($SevenZip) {
        # 7-Zip compresses with all cores, much faster than tar.exe for archives this size
        $Level = if ($Store) { '-mx=0' } else { '-mx=5' }
        Push-Location $SourceRoot
        try {
            & $SevenZip a -tzip $Level -mmt=on -bso0 -bsp0 $Archive $TopDir
            if ($LASTEXITCODE -ne 0) { throw "7-Zip failed with exit code $LASTEXITCODE" }
        }
        finally {
            Pop-Location
        }
    }
    else {
        Assert-WindowsTar
        $Options = if ($Store) { @('--options', 'zip:compression=store') } else { @() }
        & $script:WindowsTar -a -c -f $Archive @Options -C $SourceRoot $TopDir
        if ($LASTEXITCODE -ne 0) { throw "tar.exe failed with exit code $LASTEXITCODE" }
    }
}

# Writes <Archive>.sha256 in the "<hash>  <file name>" format that sha256sum -c accepts
function Write-Sha256File([string]$Archive) {
    $Hash = (Get-FileHash -Algorithm SHA256 $Archive).Hash.ToLowerInvariant()
    Write-TextFile "$Archive.sha256" "$Hash  $(Split-Path -Leaf $Archive)`n"
}

# Writes a detached, armored GPG signature to <Archive>.asc
function New-GpgSignature([string]$Archive, [string]$Key) {
    & gpg --batch --yes --local-user $Key --armor --detach-sign --output "$Archive.asc" $Archive
    if ($LASTEXITCODE -ne 0) { throw "gpg failed with exit code $LASTEXITCODE" }
}

# Returns the names of the external .cab files an MSI's Media table references
function Get-MsiExternalCabinets([string]$MsiPath) {
    $Installer = New-Object -ComObject WindowsInstaller.Installer
    $Database = $null
    $View = $null
    try {
        # 0 = msiOpenDatabaseModeReadOnly
        $Database = $Installer.GetType().InvokeMember('OpenDatabase', 'InvokeMethod', $null, $Installer, @($MsiPath, 0))
        $View = $Database.GetType().InvokeMember('OpenView', 'InvokeMethod', $null, $Database, @('SELECT `Cabinet` FROM `Media`'))
        $View.GetType().InvokeMember('Execute', 'InvokeMethod', $null, $View, $null) | Out-Null
        $Cabinets = @()
        while ($true) {
            $Record = $View.GetType().InvokeMember('Fetch', 'InvokeMethod', $null, $View, $null)
            if ($null -eq $Record) {
                break
            }
            $Cabinet = $Record.GetType().InvokeMember('StringData', 'GetProperty', $null, $Record, @(1))
            # Names starting with '#' are embedded in the MSI itself
            if ($Cabinet -and -not $Cabinet.StartsWith('#')) {
                $Cabinets += $Cabinet
            }
        }
        return $Cabinets
    }
    finally {
        # Release the COM objects so the .msi is not left locked
        if ($View) {
            $View.GetType().InvokeMember('Close', 'InvokeMethod', $null, $View, $null) | Out-Null
            [System.Runtime.InteropServices.Marshal]::ReleaseComObject($View) | Out-Null
        }
        if ($Database) {
            [System.Runtime.InteropServices.Marshal]::ReleaseComObject($Database) | Out-Null
        }
        [System.Runtime.InteropServices.Marshal]::ReleaseComObject($Installer) | Out-Null
    }
}
