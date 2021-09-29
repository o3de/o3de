#
#  Copyright (c) Contributors to the Open 3D Engine Project
# 
#  SPDX-License-Identifier: Apache-2.0 OR MIT
# 
# 

param (
    [String[]] $exePath,
    [String[]] $packagePath,
    [String[]] $bootstrapPath,
    [String[]] $certificate
)

# Get prerequisites, certs, and paths ready
$tempPath = "C:\\temp"
$certThumbprint = Get-ChildItem -Path Cert:LocalMachine\MY -ErrorAction Stop | Select-Object -ExpandProperty Thumbprint # Grab first certificate from local machine store

if ($certificate) {
    Write-Output "Checking certificate thumbprint $certificate"
    Get-ChildItem -Path Cert:LocalMachine\MY -ErrorAction SilentlyContinue | Where-Object {$_.Thumbprint -eq $certificate} # Prints certificate Thumbprint and Subject if found
    if($?) {
        $certThumbprint = $certificate
    }
    else {
        Write-Error "$certificate thumbprint not found, using $certThumbprint thumbprint instead"
    }
}

Try { 
    $signtoolPath = Resolve-Path "C:\Program Files*\Windows Kits\10\bin\*\x64\signtool.exe" -ErrorAction Stop | Select-Object -Last 1 -ExpandProperty Path
    $insigniaPath = Resolve-Path "C:\Program Files*\WiX*\bin\insignia.exe" -ErrorAction Stop | Select-Object -Last 1 -ExpandProperty Path
}
Catch {
    Write-Error "Signtool or Wix insignia not found! Exiting."
}

function Write-Signiture {
    param (
        $signtool,
        $thumbprint,
        $filename
    )
    Try {
        & $signtool sign /tr http://timestamp.digicert.com /td sha256 /fd sha256 /sha1 $thumbprint /sm $filename
        & $signtool verify /pa /v $filename
        }
    Catch {
        Write-Error "Failed to sign $filename!"
    }
}

# Looping through each path insteaad of globbing to prevent hitting maximum command string length limit
if ($exePath) {
    Write-Output "### Signing EXE files ###"
    $files = @(Get-ChildItem $exePath -Recurse *.exe | % { $_.FullName })
    foreach ($file in $files) {
        Write-Signiture -signtool $signtoolPath -thumbprint $certThumbprint -filename $file
    }
}

if ($packagePath) {
    Write-Output "### Signing CAB files ###"
    $files = @(Get-ChildItem $packagePath -Recurse *.cab | % { $_.FullName })
    foreach ($file in $files) {
        Write-Signiture -signtool $signtoolPath -thumbprint $certThumbprint -filename $file
    }

    Write-Output "### Signing MSI files ###"
    $files = @(Get-ChildItem $packagePath -Recurse *.msi | % { $_.FullName })
    foreach ($file in $files) {
        & $insigniaPath -im $files
        Write-Signiture -signtool $signtoolPath -thumbprint $certThumbprint -filename $file
    }
}

if ($bootstrapPath) {
    Write-Output "### Signing bootstrapper EXE ###"
    $files = @(Get-ChildItem $bootstrapPath -Recurse *.exe | % { $_.FullName })
    foreach ($file in $files) {
        & $insigniaPath -ib $file -o $tempPath\engine.exe
        Write-Signiture -signtool $signtoolPath -thumbprint $certThumbprint -filename $tempPath\engine.exe
        & $insigniaPath -ab $tempPath\engine.exe $file -o $file
        Write-Signiture -signtool $signtoolPath -thumbprint $certThumbprint -filename $file
        del -Force $tempPath\engine.exe
    }
}