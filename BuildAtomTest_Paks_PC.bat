@echo off

REM 
REM All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
REM its licensors.
REM
REM For complete copyright and license terms please see the LICENSE at the root of this
REM distribution (the "License"). All use of this software is governed by the License,
REM or, if provided, by the license below or the license accompanying this file. Do not
REM remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
REM WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
REM

setlocal enabledelayedexpansion 

set ORIGINALDIRECTORY=%cd%
set MYBATCHFILEDIRECTORY=%~dp0
cd "%MYBATCHFILEDIRECTORY%"

REM Attempt to determine the best BinFolder for rc.exe and AssetProcessorBatch.exe
call "%MYBATCHFILEDIRECTORY%\DetermineRCandAP.bat" SILENT

REM If a bin folder was registered, validate the presence of the binfolder/rc.exe
IF ERRORLEVEL 1 (
    ECHO unable to determine the locations of AssetProcessor and rc.exe.  Make sure that they are available or rebuild from source
    EXIT /b 1
)
ECHO Detected binary folder at %MYBATCHFILEDIRECTORY%%BINFOLDER%

echo ----- Processing Assets Using Asset Processor Batch ----
.\%BINFOLDER%\AssetProcessorBatch.exe /gamefolder=AtomTest /platforms=pc
IF ERRORLEVEL 1 GOTO AssetProcessingFailed

echo ----- Creating Packages ----
rem lowercase is intentional, since cache folders are lowercase on some platforms
.\%BINFOLDER%\rc.exe /job=%MYBATCHFILEDIRECTORY%Code\Tools\RC\Config\rc\RCJob_Generic_MakePaks.xml /p=pc /game=atomtest
IF ERRORLEVEL 1 GOTO RCFailed

echo ----- Done -----
cd "%ORIGINALDIRECTORY%"
exit /b 0

:RCFailed
echo ---- RC PAK failed ----
cd "%ORIGINALDIRECTORY%"
exit /b 1

:AssetProcessingFailed
echo ---- ASSET PROCESSING FAILED ----
cd "%ORIGINALDIRECTORY%"
exit /b 1




