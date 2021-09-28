#
#  Copyright (c) Contributors to the Open 3D Engine Project
# 
#  SPDX-License-Identifier: Apache-2.0 OR MIT
# 
# 

param (
    [String[]] $basePath,
    [String[]] $bootstrapPath
)

$tempPath = "C:\\temp"

$certThumbprint = Get-ChildItem -Path Cert:LocalMachine\MY | Select-Object -ExpandProperty Thumbprint

Try { 
    $signtoolPath =  Resolve-Path  "C:\Program Files*\Windows Kits\10\bin\*\x64\signtool.exe" -ErrorAction Stop | Select-Object -Last 1 -ExpandProperty Path
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

if ($basePath) {
    Write-Output "### Signing CAB files ###"
    $files = @(Get-ChildItem $basePath -Recurse *.cab | % { $_.FullName })
    foreach ($file in $files) {
        Write-Signiture -signtool $signtoolPath -thumbprint $certThumbprint -filename $file
    }

    Write-Output "### Signing MSI files ###"
    $files = @(Get-ChildItem $basePath -Recurse *.msi | % { $_.FullName })
    foreach ($file in $files) {
        & $insignia_path -im $files
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