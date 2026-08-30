#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

$grammarDirectory = $PSScriptRoot
$antlrJar = Join-Path $grammarDirectory "antlr4.jar"
$antlrJarDownload = "$antlrJar.download"
$antlrJarUri = "https://www.antlr.org/download/antlr-4.13.2-complete.jar"
$antlrJarSha256 = "EAE2DFA119A64327444672AFF63E9EC35A20180DC5B8090B7A6AB85125DF4D76"
$lexerGrammar = Join-Path $grammarDirectory "azslLexer.g4"
$parserGrammar = Join-Path $grammarDirectory "azslParser.g4"
$generatedSourceFiles = @(
    "azslLexer.cpp"
    "azslLexer.h"
    "azslParser.cpp"
    "azslParser.h"
    "azslParserBaseListener.cpp"
    "azslParserBaseListener.h"
    "azslParserListener.cpp"
    "azslParserListener.h"
)

$o3deCopyrightHeader = @(
    "// Copyright (c) Contributors to the Open 3D Engine Project."
    "// For complete copyright and license terms please see the LICENSE at the root of this distribution."
    "//"
    "// SPDX-License-Identifier: Apache-2.0 OR MIT"
    "//"
    ""
) -join "`n"
$utf8WithoutBom = New-Object System.Text.UTF8Encoding($false)

$java = Get-Command java -CommandType Application -ErrorAction SilentlyContinue
if (-not $java)
{
    throw "Java was not found on PATH. Install a Java runtime before regenerating the grammar."
}

function Test-AntlrJar
{
    if (-not (Test-Path -LiteralPath $antlrJar -PathType Leaf))
    {
        return $false
    }

    $actualHash = (Get-FileHash -LiteralPath $antlrJar -Algorithm SHA256).Hash
    if ($actualHash -eq $antlrJarSha256)
    {
        return $true
    }

    Write-Warning "Removing ANTLR JAR with unexpected SHA-256: $actualHash"
    Remove-Item -LiteralPath $antlrJar -Force
    return $false
}

if (-not (Test-AntlrJar))
{
    try
    {
        Write-Host "Downloading ANTLR 4.13.2..."
        Invoke-WebRequest -Uri $antlrJarUri -OutFile $antlrJarDownload

        $actualHash = (Get-FileHash -LiteralPath $antlrJarDownload -Algorithm SHA256).Hash
        if ($actualHash -ne $antlrJarSha256)
        {
            throw "Downloaded ANTLR JAR has SHA-256 '$actualHash'; expected '$antlrJarSha256'."
        }

        Move-Item -LiteralPath $antlrJarDownload -Destination $antlrJar
    }
    finally
    {
        if (Test-Path -LiteralPath $antlrJarDownload)
        {
            Remove-Item -LiteralPath $antlrJarDownload -Force
        }
    }
}

function Invoke-Antlr
{
    param(
        [Parameter(Mandatory)]
        [string] $Grammar,

        [Parameter(ValueFromRemainingArguments)]
        [string[]] $AdditionalArguments = @()
    )

    $antlrArguments = @(
        "-jar"
        $antlrJar
        "-Dlanguage=Cpp"
        "-o"
        $grammarDirectory
        "-listener"
        "-no-visitor"
    ) + $AdditionalArguments + @($Grammar)

    & $java.Source @antlrArguments
    if ($LASTEXITCODE -ne 0)
    {
        throw "ANTLR failed while generating sources from '$Grammar' (exit code $LASTEXITCODE)."
    }
}

function Format-GeneratedSource
{
    param(
        [Parameter(Mandatory)]
        [string] $SourceFile
    )

    $sourcePath = Join-Path $grammarDirectory $SourceFile
    $sourceContent = [System.IO.File]::ReadAllText($sourcePath).Replace("`t", "    ")
    [System.IO.File]::WriteAllText($sourcePath, $o3deCopyrightHeader + $sourceContent, $utf8WithoutBom)
}

Push-Location $grammarDirectory
try
{
    Invoke-Antlr -Grammar (Split-Path $lexerGrammar -Leaf)
    Invoke-Antlr -Grammar (Split-Path $parserGrammar -Leaf) -lib $grammarDirectory
    $generatedSourceFiles | ForEach-Object { Format-GeneratedSource -SourceFile $_ }
    # Trailing pipes, not leading ones: a line-leading '|' is PowerShell 7+ syntax and is a parse error in
    # Windows PowerShell 5.1, which fails the whole script before a single line of it runs.
    Get-ChildItem -LiteralPath $grammarDirectory -File |
        Where-Object Extension -In ".interp", ".tokens" |
        Remove-Item -Force
}
finally
{
    Pop-Location
}

Write-Host "ANTLR C++ sources regenerated in: $grammarDirectory"
