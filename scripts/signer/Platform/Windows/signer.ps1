#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

param (
    [String[]] $exePath,
    [String[]] $packagePath,
    [String[]] $bootstrapPath,
    [String[]] $certificate,
    [String] $MSDlibPath,
    [String] $MetadataFilePath,
    [switch] $Verbose
)

# Get prerequisites, certs, and paths ready
$tempPath = [System.IO.Path]::GetTempPath() # Order of operations defined here: https://docs.microsoft.com/en-us/dotnet/api/system.io.path.gettemppath?view=net-5.0&tabs=windows#remarks

# Function to detect MS Trusted Signing availability
function Test-TrustedSigningAvailable {
    $dlibFound = $false
    $metadataFound = $false
    
    # Check for MS DLib - prioritize environment variable, then parameter
    if ($env:O3DE_MS_DLIB_PATH -and (Test-Path $env:O3DE_MS_DLIB_PATH)) {
        # Verify the DLib file contains the expected name pattern
        if ($env:O3DE_MS_DLIB_PATH -match "CodeSigning\.Dlib\.dll") {
            $script:MSDlibPath = $env:O3DE_MS_DLIB_PATH
            $dlibFound = $true
        } else {
            Write-Warning "DLib path does not contain 'CodeSigning.Dlib.dll'. Please check the path."
        }
    } elseif ($MSDlibPath -and (Test-Path $MSDlibPath)) {
        # Verify the DLib file contains the expected name pattern
        if ($MSDlibPath -match "CodeSigning\.Dlib\.dll") {
            $dlibFound = $true
        } else {
            Write-Warning "DLib path does not contain 'CodeSigning.Dlib.dll'. Please check the path."
        }
    }
    
    # Check for metadata file - prioritize environment variable, then parameter
    if ($env:O3DE_MS_METADATA_PATH -and (Test-Path $env:O3DE_MS_METADATA_PATH)) {
        $script:MetadataFilePath = $env:O3DE_MS_METADATA_PATH
        $metadataFound = $true
    } elseif ($MetadataFilePath -and (Test-Path $MetadataFilePath)) {
        $metadataFound = $true
    }
    
    return ($dlibFound -and $metadataFound)
}

# Function to check signtool version
function Test-SignToolVersion {
    param($signtoolPath)
    
    try {
        Write-Output "SignTool path: $signtoolPath"
        if (-not (Test-Path $signtoolPath)) {
            Write-Warning "SignTool path does not exist"
            return $false
        }
        
        # Get the file version directly from the executable
        $fileVersion = (Get-Item $signtoolPath).VersionInfo.FileVersion
        if ($fileVersion) {
            $requiredVersion = [Version]"10.0.22621.0"
            
            # Try to extract version numbers from the file version string
            if ($fileVersion -match "(\d+\.\d+\.\d+\.\d+)") {
                $version = [Version]$matches[1]
                
                if ($version -lt $requiredVersion) {
                    Write-Warning "SignTool version $version is below the required version $requiredVersion for Trusted Signing"
                    return $false
                }
                return $true
            }
        }
        
        # If we got here, we couldn't determine the version, but we'll assume it's OK if the path contains the required version
        if ($signtoolPath -match "10\.0\.22621\.0|10\.0\.2[3-9]|10\.0\.[3-9]\d") {
            return $true
        } else {
            Write-Warning "Could not determine SignTool version. Trusted Signing may not work correctly."
            return $false
        }
    } catch {
        Write-Warning "Error checking SignTool version: $($_.Exception.Message)"
        return $false
    }
}
function Write-Signature {
    param (
        $signtool,
        $filename,
        [switch]$UseTrustedSigning,
        $thumbprint = $null,
        $dlibPath = $null,
        $metadataPath = $null,
        [switch]$Verbose
    )

    $attempts = 2
    $sleepSec = 5

    Do {
        $attempts--
        Try {
            if ($UseTrustedSigning) {
                # MS Trusted Signing command
                if ($Verbose) {
                    # Verbose output for trusted signing
                    Write-Host "Signing: $filename"
                    & $signtool sign /v /debug /fd SHA256 /tr "http://timestamp.acs.microsoft.com" /td SHA256 /dlib $dlibPath /dmdf $metadataPath $filename
                } else {
                    # Quiet output for trusted signing (default)
                    $result = & $signtool sign /fd SHA256 /tr "http://timestamp.acs.microsoft.com" /td SHA256 /dlib $dlibPath /dmdf $metadataPath $filename 2>&1
                }
            } else {
                # Traditional certificate signing
                if ($Verbose) {
                    # Verbose output for certificate signing
                    Write-Host "Signing: $filename"
                    & $signtool sign /tr http://timestamp.digicert.com /td sha256 /fd sha256 /sha1 $thumbprint /sm $filename
                } else {
                    # Quiet output for certificate signing (default)
                    $result = & $signtool sign /tr http://timestamp.digicert.com /td sha256 /fd sha256 /sha1 $thumbprint /sm $filename 2>&1
                }
            }
            
            # Check if signing was successful
            if ($LASTEXITCODE -ne 0) {
                Write-Error "Signed: $filename - Failed with exit code $LASTEXITCODE"
                Start-Sleep -Seconds $sleepSec
                continue
            }
            
            # Show success message in a single line
            Write-Host "Signed: $filename - Success"
            
            # Don't verify immediately - we'll do that later
            return $true
        }
        Catch {
            Write-Error $_.Exception.InnerException.Message -ErrorAction Continue
            Start-Sleep -Seconds $sleepSec
        }
    } while ($attempts -gt 0)

    Write-Error "Signed: $filename - Failed after multiple attempts"
    return $false
}

function Verify-Signature {
    param (
        $signtool,
        $filename,
        [switch]$Verbose
    )
    
    if ($Verbose) {
        # Verbose verification output
        Write-Host "`nVerifying: $filename"
        & $signtool verify /pa /v $filename
    } else {
        # Quiet verification output - still capture the result
        $result = & $signtool verify /pa $filename 2>&1
        if ($LASTEXITCODE -eq 0) {
            Write-Host "Verified: $filename - Success"
        } else {
            Write-Error "Verified: $filename - Failed"
        }
    }
}

# Main Function

# Find required tools first
Try {
    $signtoolPath = Resolve-Path "C:\Program Files*\Windows Kits\10\bin\*\x64\signtool.exe" -ErrorAction Stop | Select-Object -Last 1 -ExpandProperty Path
    $insigniaPath = Resolve-Path "C:\Program Files*\WiX*\bin\insignia.exe" -ErrorAction Stop | Select-Object -Last 1 -ExpandProperty Path
}
Catch {
    Write-Error "Signtool or Wix insignia not found! Exiting."
    exit 1
}

# Determine signing method - use Trusted Signing if dlib and metadata are available
$useTrustedSigning = $false
if (Test-TrustedSigningAvailable) {
    $useTrustedSigning = $true
    Write-Host "### Using Trusted Signing ###"
    Write-Host "SignTool Path: $signtoolPath"
    Write-Host "DLib Path: $script:MSDlibPath"
    Write-Host "Metadata Path: $script:MetadataFilePath"
    
    # Check signtool version if we're planning to use Trusted Signing
    if ($useTrustedSigning) {
        $null = Test-SignToolVersion -signtoolPath $signtoolPath
    }
} else {
    Write-Host "### Using certificate-based signing ###"
    Write-Host "SignTool Path: $signtoolPath"
}

# Set up certificate-based signing if not using Trusted Signing
$certThumbprint = $null
if (-not $useTrustedSigning) {
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
}

# Looping through each path instead of globbing to prevent hitting maximum command string length limit
$allSignedFiles = @()

if ($exePath) {
    Write-Host "### Signing EXE files ###"
    $files = @(Get-ChildItem $exePath -Recurse *.exe | % { $_.FullName })
    foreach ($file in $files) {
        $success = Write-Signature -signtool $signtoolPath -filename $file -UseTrustedSigning:$useTrustedSigning -thumbprint $certThumbprint -dlibPath $script:MSDlibPath -metadataPath $script:MetadataFilePath -Verbose:$Verbose
        if ($success) {
            $allSignedFiles += $file
        }
    }
}

if ($packagePath) {
    Write-Host "### Signing CAB files ###"
    $files = @(Get-ChildItem $packagePath -Recurse *.cab | % { $_.FullName })
    foreach ($file in $files) {
        $success = Write-Signature -signtool $signtoolPath -filename $file -UseTrustedSigning:$useTrustedSigning -thumbprint $certThumbprint -dlibPath $script:MSDlibPath -metadataPath $script:MetadataFilePath -Verbose:$Verbose
        if ($success) {
            $allSignedFiles += $file
        }
    }

    Write-Host "### Signing MSI files ###"
    $files = @(Get-ChildItem $packagePath -Recurse *.msi | % { $_.FullName })
    foreach ($file in $files) {
        & $insigniaPath -im $file
        $success = Write-Signature -signtool $signtoolPath -filename $file -UseTrustedSigning:$useTrustedSigning -thumbprint $certThumbprint -dlibPath $script:MSDlibPath -metadataPath $script:MetadataFilePath -Verbose:$Verbose
        if ($success) {
            $allSignedFiles += $file
        }
    }
}

if ($bootstrapPath) {
    Write-Host "### Signing bootstrapper EXE ###"
    $files = @(Get-ChildItem $bootstrapPath -Recurse *.exe | % { $_.FullName })
    foreach ($file in $files) {
        & $insigniaPath -ib $file -o $tempPath\engine.exe
        $success = Write-Signature -signtool $signtoolPath -filename $tempPath\engine.exe -UseTrustedSigning:$useTrustedSigning -thumbprint $certThumbprint -dlibPath $script:MSDlibPath -metadataPath $script:MetadataFilePath -Verbose:$Verbose
        
        & $insigniaPath -ab $tempPath\engine.exe $file -o $file
        
        $success = Write-Signature -signtool $signtoolPath -filename $file -UseTrustedSigning:$useTrustedSigning -thumbprint $certThumbprint -dlibPath $script:MSDlibPath -metadataPath $script:MetadataFilePath -Verbose:$Verbose
        if ($success) {
            $allSignedFiles += $file
        }
        Remove-Item -Force $tempPath\engine.exe
    }
}

# Verify all signed files at the end
if ($allSignedFiles.Count -gt 0) {
    Write-Host "`n### Verifying signatures ###"
    foreach ($file in $allSignedFiles) {
        Verify-Signature -signtool $signtoolPath -filename $file -Verbose:$Verbose
    }
    
    Write-Host "`nSummary: Successfully signed $($allSignedFiles.Count) files"
}