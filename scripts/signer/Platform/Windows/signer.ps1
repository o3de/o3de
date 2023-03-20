#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

param (
    [String[]] $exePath,
    [String[]] $packagePath,
    [String[]] $bootstrapPath,
    [String[]] $certificate
)

# Get prerequisites, certs, and paths ready
$tempPath = [System.IO.Path]::GetTempPath() # Order of operations defined here: https://docs.microsoft.com/en-us/dotnet/api/system.io.path.gettemppath?view=net-5.0&tabs=windows#remarks
$certThumbprint = Get-ChildItem -Path Cert:LocalMachine\MY -CodeSigningCert -ErrorAction Stop | Select-Object -ExpandProperty Thumbprint # Grab first certificate from local machine store

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

function Write-Signature {
    param (
        $signtool,
        $thumbprint,
        $filename
    )

    $attempts = 2
    $sleepSec = 5

    Do {
        $attempts--
        Try {
            & $signtool sign /tr http://timestamp.digicert.com /td sha256 /fd sha256 /sha1 $thumbprint /sm $filename
            & $signtool verify /pa /v $filename
            return
        }
        Catch {
            Write-Error $_.Exception.InnerException.Message -ErrorAction Continue
            Start-Sleep -Seconds $sleepSec
        }
    } while ($attempts -lt 0)

    throw "Failed to sign $filename" # Bypassed in try block if the command is successful
}

# Looping through each path insteaad of globbing to prevent hitting maximum command string length limit
if ($exePath) {
    Write-Output "### Signing EXE files ###"
    $files = @(Get-ChildItem $exePath -Recurse *.exe | % { $_.FullName })
    foreach ($file in $files) {
        Write-Signature -signtool $signtoolPath -thumbprint $certThumbprint -filename $file
    }
}

if ($packagePath) {
    Write-Output "### Signing CAB files ###"
    $files = @(Get-ChildItem $packagePath -Recurse *.cab | % { $_.FullName })
    foreach ($file in $files) {
        Write-Signature -signtool $signtoolPath -thumbprint $certThumbprint -filename $file
    }

    Write-Output "### Signing MSI files ###"
    $files = @(Get-ChildItem $packagePath -Recurse *.msi | % { $_.FullName })
    foreach ($file in $files) {
        & $insigniaPath -im $files
        Write-Signature -signtool $signtoolPath -thumbprint $certThumbprint -filename $file
    }
}

if ($bootstrapPath) {
    Write-Output "### Signing bootstrapper EXE ###"
    $files = @(Get-ChildItem $bootstrapPath -Recurse *.exe | % { $_.FullName })
    foreach ($file in $files) {
        & $insigniaPath -ib $file -o $tempPath\engine.exe
        Write-Signature -signtool $signtoolPath -thumbprint $certThumbprint -filename $tempPath\engine.exe
        & $insigniaPath -ab $tempPath\engine.exe $file -o $file
        Write-Signature -signtool $signtoolPath -thumbprint $certThumbprint -filename $file
        Remove-Item -Force $tempPath\engine.exe
    }
}