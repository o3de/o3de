#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

<#
.SYNOPSIS
    Creates a portable (no installer) O3DE zip from an installed engine directory or an O3DE .msi.

.DESCRIPTION
    The zip contents match what the MSI installs, with the post-install steps (bundled CMake
    extraction, build leftovers cleanup) already applied. Python is set up by the Project Manager
    on first launch, as it is for the Linux package.

    From an installed copy (e.g. "C:\Program Files\O3DE\26.10.0") the files are copied as is.
    From an .msi, an administrative install (msiexec /a) extracts the files without running any
    install actions. Run it from the offline layout folder so the .cab files are next to the .msi.

.PARAMETER Source
    An installed engine directory or an O3DE .msi file.

.PARAMETER OutputDir
    Directory to write the zip to. Keep the path short: staging happens inside it, and some
    engine paths are long. Default: .\dist

.PARAMETER Name
    Base name of the zip. Default: o3de_<version>_windows_portable

.PARAMETER GpgKey
    Optionally create a detached, armored GPG signature (.asc) with this key.

.PARAMETER Keep
    Keep the staging directory after the zip is created.

.EXAMPLE
    .\make_portable_zip.ps1 -OutputDir C:\dist "C:\Program Files\O3DE\26.10.0"

.EXAMPLE
    .\make_portable_zip.ps1 -OutputDir C:\dist D:\layout\o3de_26.10.0.msi
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$Source,
    [string]$OutputDir = (Join-Path (Get-Location) 'dist'),
    [string]$Name,
    [string]$GpgKey,
    [switch]$Keep
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$ScriptDir = $PSScriptRoot
# Use the Windows bsdtar, which can write zip files (a tar from Git or MSYS on PATH cannot)
$Tar = Join-Path $env:SystemRoot 'System32\tar.exe'
if (-not (Test-Path $Tar)) {
    throw "tar.exe not found at $Tar (requires Windows 10 1803 or newer)"
}

function Write-Step([string]$Message) {
    Write-Host "[portable] $Message"
}

function Write-TextFile([string]$Path, [string]$Content) {
    # No BOM, so the checksum file also works with sha256sum
    [System.IO.File]::WriteAllText($Path, $Content, (New-Object System.Text.UTF8Encoding $false))
}

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
$OutputDir = (Resolve-Path $OutputDir).Path
$StagingRoot = Join-Path $OutputDir ('.staging-' + [System.IO.Path]::GetRandomFileName().Substring(0, 8))
New-Item -ItemType Directory -Path $StagingRoot | Out-Null

try {
    # 1. Get the engine files into the staging directory
    $AdminRoot = $null
    if (Test-Path $Source -PathType Container) {
        $EngineSource = (Resolve-Path $Source).Path
        Write-Step "Copying installed engine from $EngineSource"
    }
    elseif ((Test-Path $Source -PathType Leaf) -and ($Source -like '*.msi')) {
        $MsiPath = (Resolve-Path $Source).Path
        $AdminRoot = Join-Path $StagingRoot 'msi'
        $MsiLog = Join-Path $OutputDir 'msi_admin_install.log'
        Write-Step "Extracting $MsiPath (administrative install, log: $MsiLog)"
        $msiexec = Start-Process -FilePath msiexec.exe -Wait -PassThru -ArgumentList @(
            '/a', "`"$MsiPath`"", '/qn', "TARGETDIR=`"$AdminRoot`"", '/l*v', "`"$MsiLog`"")
        if ($msiexec.ExitCode -ne 0 -and $msiexec.ExitCode -ne 3010) {
            throw "msiexec /a failed with exit code $($msiexec.ExitCode), see $MsiLog. Are the .cab files next to the .msi?"
        }
        $EngineJson = Get-ChildItem -Path $AdminRoot -Filter engine.json -Recurse -Depth 4 -File |
            Sort-Object { $_.FullName.Length } | Select-Object -First 1
        if (-not $EngineJson) {
            throw "No engine.json found in the extracted MSI under $AdminRoot"
        }
        $EngineSource = $EngineJson.DirectoryName
    }
    else {
        throw "$Source is neither a directory nor an .msi file"
    }

    $EngineJsonPath = Join-Path $EngineSource 'engine.json'
    if (-not (Test-Path $EngineJsonPath)) {
        throw "$EngineSource does not contain engine.json"
    }

    # Use the display version (what the installer is named after), falling back to the engine version
    $EngineInfo = Get-Content -Raw $EngineJsonPath | ConvertFrom-Json
    $Version = $null
    if ($EngineInfo.PSObject.Properties['display_version']) {
        $Version = $EngineInfo.display_version
    }
    if ([string]::IsNullOrEmpty($Version) -or $Version -eq '00.00') {
        $Version = $EngineInfo.version
    }
    $TopDir = "o3de_$Version"
    if (-not $Name) {
        $Name = "o3de_${Version}_windows_portable"
    }
    $Stage = Join-Path $StagingRoot $TopDir

    Write-Step "Engine version $Version, staging to $Stage"
    if ($AdminRoot) {
        Move-Item -Path $EngineSource -Destination $Stage
        # The administrative install also leaves a copy of the .msi behind
        Remove-Item -Recurse -Force $AdminRoot
    }
    else {
        # robocopy copes with long paths and is much faster than Copy-Item for large trees
        & robocopy.exe $EngineSource $Stage /E /COPY:DAT /DCOPY:DAT /R:1 /W:1 /MT:16 /NFL /NDL /NJH /NJS /NP | Out-Null
        if ($LASTEXITCODE -ge 8) {
            throw "robocopy failed with exit code $LASTEXITCODE"
        }
    }

    # 2. Apply the post-install steps from cmake/Platform/Windows/Packaging/PostInstallSetup.wxs
    $CMakeRuntime = Join-Path $Stage 'cmake\runtime'
    if (-not (Test-Path (Join-Path $CMakeRuntime 'bin\cmake.exe'))) {
        $RedistDir = Join-Path $Stage 'Tools\Redistributables\CMake'
        $CMakeZip = Get-ChildItem -Path $RedistDir -Filter 'cmake-*-windows-*.zip' -File -ErrorAction SilentlyContinue |
            Select-Object -First 1
        if (-not $CMakeZip) {
            throw "Bundled CMake not found in $RedistDir"
        }
        Write-Step "Extracting bundled CMake into cmake\runtime"
        $CMakeExtract = Join-Path $StagingRoot 'cmake'
        New-Item -ItemType Directory -Path $CMakeExtract | Out-Null
        & $Tar -x -f $CMakeZip.FullName -C $CMakeExtract
        if ($LASTEXITCODE -ne 0) {
            throw "Extracting $($CMakeZip.Name) failed"
        }
        if (Test-Path $CMakeRuntime) {
            Remove-Item -Recurse -Force $CMakeRuntime
        }
        Move-Item -Path (Join-Path $CMakeExtract $CMakeZip.BaseName) -Destination $CMakeRuntime
        Remove-Item -Recurse -Force $CMakeExtract
    }

    # Leftovers from the installer build or from running the engine
    Write-Step "Removing build and runtime leftovers"
    Get-ChildItem -Path $Stage -Recurse -Directory -Force -Include '*.egg-info', '__pycache__' -ErrorAction SilentlyContinue |
        Sort-Object { $_.FullName.Length } -Descending |
        Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
    Get-ChildItem -Path (Join-Path $Stage 'bin\Windows') -Directory -ErrorAction SilentlyContinue |
        Get-ChildItem -Directory | ForEach-Object {
            $UserDir = Join-Path $_.FullName 'user'
            if (Test-Path $UserDir) {
                Remove-Item -Recurse -Force $UserDir
            }
        }

    # 3. Add the portable launcher, shortcut scripts and README
    Copy-Item (Join-Path $ScriptDir 'o3de.cmd') $Stage
    Copy-Item (Join-Path $ScriptDir 'create-shortcuts.cmd') $Stage
    Copy-Item (Join-Path $ScriptDir 'create-shortcuts.ps1') $Stage
    $Readme = Get-Content -Raw (Join-Path $ScriptDir 'README-windows.txt')
    $Readme = $Readme.Replace('@VERSION@', $Version).Replace('@TOP_DIR@', $TopDir).Replace('@PACKAGE_NAME@', $Name)
    # Windows line endings so the README reads well in any editor
    $Readme = ($Readme -replace "`r`n", "`n") -replace "`n", "`r`n"
    Write-TextFile (Join-Path $Stage 'README-windows.txt') $Readme

    # 4. Archive, checksum and optionally sign
    $Archive = Join-Path $OutputDir "$Name.zip"
    if (Test-Path $Archive) {
        Remove-Item -Force $Archive
    }
    Write-Step "Creating $Archive"
    $SevenZip = $null
    $SevenZipCommand = Get-Command 7z.exe -ErrorAction SilentlyContinue
    if ($SevenZipCommand) {
        $SevenZip = $SevenZipCommand.Source
    }
    elseif (Test-Path "$env:ProgramFiles\7-Zip\7z.exe") {
        $SevenZip = "$env:ProgramFiles\7-Zip\7z.exe"
    }
    if ($SevenZip) {
        # 7-Zip compresses with all cores, much faster than tar.exe for an archive this size
        Push-Location $StagingRoot
        try {
            & $SevenZip a -tzip -mx=5 -mmt=on -bso0 -bsp0 $Archive $TopDir
            if ($LASTEXITCODE -ne 0) { throw "7-Zip failed with exit code $LASTEXITCODE" }
        }
        finally {
            Pop-Location
        }
    }
    else {
        & $Tar -a -c -f $Archive -C $StagingRoot $TopDir
        if ($LASTEXITCODE -ne 0) { throw "tar.exe failed with exit code $LASTEXITCODE" }
    }

    $Hash = (Get-FileHash -Algorithm SHA256 $Archive).Hash.ToLowerInvariant()
    Write-TextFile "$Archive.sha256" "$Hash  $Name.zip`n"

    if ($GpgKey) {
        Write-Step "Signing with key $GpgKey"
        & gpg --batch --yes --local-user $GpgKey --armor --detach-sign --output "$Archive.asc" $Archive
        if ($LASTEXITCODE -ne 0) { throw "gpg failed with exit code $LASTEXITCODE" }
    }

    if ($Keep) {
        Write-Step "Staging directory kept at $Stage"
    }
    Write-Step "Done:"
    Get-Item "$Archive*" | Format-Table Name, @{ Label = 'Size (MB)'; Expression = { [math]::Round($_.Length / 1MB) } } -AutoSize
}
finally {
    if (-not $Keep -and (Test-Path $StagingRoot)) {
        Remove-Item -Recurse -Force $StagingRoot -ErrorAction SilentlyContinue
    }
}
