#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

<#
.SYNOPSIS
    Creates the offline installer zip and the portable zip from the O3DE WiX bootstrapper.

.DESCRIPTION
    1. Runs the bootstrapper with /layout to gather the bootstrapper, the .msi and all .cab files
       into one folder (the offline installer layout). Burn uses payloads next to the bootstrapper
       when present and downloads the rest from the download URL built into it.
    2. Checks that every .cab the .msi references is in the layout.
    3. Zips the layout as <package>_windows_offline.zip.
    4. Runs make_portable_zip.ps1 on the layout's .msi to create <package>_windows_portable.zip.

    Each zip gets a .sha256 file and, with -GpgKey, a detached .asc signature.

.PARAMETER Installer
    The bootstrapper, e.g. o3de_26.10.0_installer.exe.

.PARAMETER LayoutDir
    An existing offline layout folder to use instead of running the bootstrapper.

.PARAMETER OutputDir
    Directory to write the zips to. Keep the path short: staging happens inside it. Default: .\dist

.PARAMETER GpgKey
    Optionally sign each zip with this GPG key.

.PARAMETER SkipOffline
    Only create the portable zip.

.PARAMETER SkipPortable
    Only create the offline installer zip.

.PARAMETER Keep
    Keep the layout and staging directories after the zips are created.

.EXAMPLE
    .\make_windows_packages.ps1 -OutputDir C:\dist C:\build\CPackUploads\o3de_26.10.0_installer.exe

.EXAMPLE
    .\make_windows_packages.ps1 -OutputDir C:\dist -LayoutDir D:\o3de_layout
#>

[CmdletBinding(DefaultParameterSetName = 'Installer')]
param(
    [Parameter(Mandatory = $true, Position = 0, ParameterSetName = 'Installer')]
    [string]$Installer,
    [Parameter(Mandatory = $true, ParameterSetName = 'Layout')]
    [string]$LayoutDir,
    [string]$OutputDir = (Join-Path (Get-Location) 'dist'),
    [string]$GpgKey,
    [switch]$SkipOffline,
    [switch]$SkipPortable,
    [switch]$Keep
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$ScriptDir = $PSScriptRoot
. (Join-Path $ScriptDir 'package_helpers.ps1')

function Write-Step([string]$Message) {
    Write-Host "[packages] $Message"
}

if ($SkipOffline -and $SkipPortable) {
    throw "Nothing to do: both -SkipOffline and -SkipPortable were given"
}

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
$OutputDir = (Resolve-Path $OutputDir).Path
$StagingRoot = $null

try {
    # 1. Get the offline layout
    if ($PSCmdlet.ParameterSetName -eq 'Installer') {
        $InstallerPath = (Resolve-Path $Installer).Path
        $StagingRoot = Join-Path $OutputDir ('.layout-' + [System.IO.Path]::GetRandomFileName().Substring(0, 8))
        $Layout = Join-Path $StagingRoot 'layout'
        New-Item -ItemType Directory -Path $Layout | Out-Null
        $LayoutLog = Join-Path $OutputDir 'bootstrapper_layout.log'

        Write-Step "Creating offline layout from $InstallerPath (log: $LayoutLog)"
        $Burn = Start-Process -FilePath $InstallerPath -Wait -PassThru -ArgumentList @(
            '/layout', "`"$Layout`"", '/quiet', '/log', "`"$LayoutLog`"")
        if ($Burn.ExitCode -ne 0 -and $Burn.ExitCode -ne 3010) {
            throw "Bootstrapper /layout failed with exit code $($Burn.ExitCode), see $LayoutLog"
        }
    }
    else {
        $Layout = (Resolve-Path $LayoutDir).Path
        Write-Step "Using existing offline layout $Layout"
    }

    # 2. Check the layout is complete
    $Msi = @(Get-ChildItem -Path $Layout -Filter *.msi -File)
    if ($Msi.Count -ne 1) {
        throw "Expected exactly one .msi in $Layout, found $($Msi.Count)"
    }
    $Msi = $Msi[0]
    $PackageName = $Msi.BaseName
    if (-not (Get-ChildItem -Path $Layout -Filter *.exe -File)) {
        Write-Warning "No bootstrapper .exe in $Layout; the offline zip will only contain the .msi and .cab files"
    }

    try {
        $Cabinets = @(Get-MsiExternalCabinets $Msi.FullName)
    }
    catch {
        $Cabinets = $null
        Write-Warning "Could not read the .cab list from $($Msi.Name), skipping the completeness check: $_"
    }
    if ($null -ne $Cabinets) {
        $Missing = @($Cabinets | Where-Object { -not (Test-Path (Join-Path $Layout $_)) })
        if ($Missing.Count -gt 0) {
            throw "The layout is missing $($Missing.Count) of $($Cabinets.Count) .cab files: $($Missing -join ', ')"
        }
        Write-Step "Layout complete: $($Msi.Name) and $($Cabinets.Count) .cab files"
    }

    # 3. Offline installer zip
    if (-not $SkipOffline) {
        $OfflineName = "${PackageName}_windows_offline"
        if ($PSCmdlet.ParameterSetName -eq 'Installer') {
            # Give the layout folder the name users see when they extract the zip
            $OfflineDir = Join-Path $StagingRoot $OfflineName
            Rename-Item -Path $Layout -NewName $OfflineName
            $Layout = $OfflineDir
            $Msi = Get-Item (Join-Path $Layout $Msi.Name)
            $ReadmePath = Join-Path $Layout 'README-offline.txt'
            $AddedReadme = $true
        }
        else {
            # A layout folder that was passed in keeps its own name; the README is only added
            # for zipping and removed again afterwards
            $ReadmePath = Join-Path $Layout 'README-offline.txt'
            $AddedReadme = -not (Test-Path $ReadmePath)
        }

        if ($AddedReadme) {
            $Bootstrapper = Get-ChildItem -Path $Layout -Filter *.exe -File | Select-Object -First 1
            $BootstrapperName = if ($Bootstrapper) { $Bootstrapper.Name } else { $Msi.Name }
            $Readme = Get-Content -Raw (Join-Path $ScriptDir 'README-offline.txt')
            $Readme = $Readme.Replace('@PACKAGE_NAME@', $PackageName).Replace('@OFFLINE_NAME@', $OfflineName).Replace('@BOOTSTRAPPER@', $BootstrapperName)
            $Readme = ($Readme -replace "`r`n", "`n") -replace "`n", "`r`n"
            Write-TextFile $ReadmePath $Readme
        }

        $OfflineArchive = Join-Path $OutputDir "$OfflineName.zip"
        Write-Step "Creating $OfflineArchive"
        try {
            # The .msi and .cab files are already compressed, so store them as is
            New-ZipArchive (Split-Path -Parent $Layout) (Split-Path -Leaf $Layout) $OfflineArchive -Store
        }
        finally {
            if ($AddedReadme -and $PSCmdlet.ParameterSetName -eq 'Layout') {
                Remove-Item -Force $ReadmePath -ErrorAction SilentlyContinue
            }
        }
        Write-Sha256File $OfflineArchive
        if ($GpgKey) {
            Write-Step "Signing $OfflineArchive with key $GpgKey"
            New-GpgSignature $OfflineArchive $GpgKey
        }
    }

    # 4. Portable zip, extracted from the layout's .msi
    if (-not $SkipPortable) {
        Write-Step "Creating the portable zip from $($Msi.Name)"
        $PortableArgs = @{ Source = $Msi.FullName; OutputDir = $OutputDir }
        if ($GpgKey) { $PortableArgs.GpgKey = $GpgKey }
        if ($Keep) { $PortableArgs.Keep = $true }
        & (Join-Path $ScriptDir 'make_portable_zip.ps1') @PortableArgs
    }

    if ($Keep -and $StagingRoot) {
        Write-Step "Layout kept at $Layout"
    }
    Write-Step "Done:"
    Get-ChildItem -Path $OutputDir -File -Filter "$PackageName*" |
        Format-Table Name, @{ Label = 'Size (MB)'; Expression = { [math]::Round($_.Length / 1MB) } } -AutoSize
}
finally {
    if (-not $Keep -and $StagingRoot -and (Test-Path $StagingRoot)) {
        Remove-Item -Recurse -Force $StagingRoot -ErrorAction SilentlyContinue
    }
}
