@echo off
REM
REM  All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
REM  its licensors.
REM
REM  REM  For complete copyright and license terms please see the LICENSE at the root of this
REM  distribution (the "License"). All use of this software is governed by the License,
REM  or, if provided, by the license below or the license accompanying this file. Do not
REM  remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
REM  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
REM
REM

REM This batch file's job is to determine the best Bin folder that has both the AssetProcessorBatch and rc.exe
REM The resulting value if successful will be set in BINFOLDER env variable

set ORIGINALDIRECTORY=%cd%
set MYBATCHFILEDIRECTORY=%~dp0
cd "%MYBATCHFILEDIRECTORY%"

IF [%1]==[SILENT] (
    SET SILENTMODE=1
    SET VERBOSE=0
) ELSE IF [%1]==[VERBOSE] (
    SET SILENTMODE=0
    SET VERBOSE=1
) ELSE (
    SET SILENTMODE=0
    SET VERBOSE=0
)

REM Initialize the BINFOLDER_HINT from the 3rd parameter
IF NOT "%~2"=="" (
    SET BINFOLDER_HINT=%~2
) ELSE (
    SET BINFOLDER_HINT=
)

SET BINFOLDER=

REM Highest Priorty: Determine registry key 'HKCU\Software\Amazon\Lumberyard\Settings'
IF [%SILENTMODE%]==[0] ECHO Looking up BinFolder in the registry
SET BINFOLDER_REG_KEY="HKCU\Software\Amazon\Lumberyard\Settings"
IF %VERBOSE%==1 (
    IF [%SILENTMODE%]==[0] (
        ECHO REG.EXE QUERY %BINFOLDER_REG_KEY%
    )
)
REG.EXE QUERY %BINFOLDER_REG_KEY% > NUL
IF ERRORLEVEL 1 (
    SET ERROR_MSG="Unable to read the bin folder from the registry"
    GOTO determineBestBinFolder
)

REM Parse out the folder value
FOR /F "tokens=2*" %%A IN ('REG.EXE QUERY %BINFOLDER_REG_KEY% /v "ENG_BinFolder" ') DO SET BINFOLDER=%%B

IF [%BINFOLDER%]==[] (
    IF %SILENTMODE%==0 echo Binfolder not Found in registry
) ELSE (
    IF %SILENTMODE%==0 echo Binfolder Found in registry: %BINFOLDER%
)


:determineBestBinFolder

REM If a bin folder was registered, validate the presence of the binfolder/AssetProcessorBatch.exe
IF NOT [%BINFOLDER%]==[] (
    IF EXIST "%MYBATCHFILEDIRECTORY%\%BINFOLDER%\rc.exe" (
        IF EXIST "%MYBATCHFILEDIRECTORY%\%BINFOLDER%\AssetProcessorBatch.exe" GOTO BinFolderFound
        IF %SILENTMODE%==0 echo Cannot find AssetProcessorBatch.exe at current registered BinFolder %MYBATCHFILEDIRECTORY%\%BINFOLDER%.
        IF %SILENTMODE%==0 echo Looking for all locations
    ) ELSE (
        IF %SILENTMODE%==0 echo Cannot find rc.exe at current registered BinFolder %MYBATCHFILEDIRECTORY%\%BINFOLDER%\rc.
        IF %SILENTMODE%==0 echo Looking for all locations
    )
)

REM Next search using the %BINFOLDER_HINT% as a directory if it is non-empty
IF NOT "%BINFOLDER_HINT%"=="" (
    IF EXIST "%MYBATCHFILEDIRECTORY%\%BINFOLDER_HINT%\AssetProcessorBatch.exe" (
        IF EXIST "%MYBATCHFILEDIRECTORY%\%BINFOLDER_HINT%\rc.exe" (
            SET BINFOLDER=%BINFOLDER_HINT%
            GOTO BinFolderFound
        )
    )
)
REM There was an error determining the folder from the registry, try from the most recent to oldest vc version
IF EXIST "%MYBATCHFILEDIRECTORY%\Bin64vc142\AssetProcessorBatch.exe" (
    IF EXIST "%MYBATCHFILEDIRECTORY%\Bin64vc142\rc.exe" (
        SET BINFOLDER=Bin64vc142
        GOTO BinFolderFound
    )
)
IF EXIST "%MYBATCHFILEDIRECTORY%\Bin64vc141\AssetProcessorBatch.exe" (
    IF EXIST "%MYBATCHFILEDIRECTORY%\Bin64vc141\rc.exe" (
        SET BINFOLDER=Bin64vc141
        GOTO BinFolderFound
    )
)


REM Unable to determine or find a binfolder/AssetProcessorBatch.exe
IF %SILENTMODE%==0 echo unable to locate AssetProcessorBatch.exe.  Make sure it is available or re-build it from the source
cd "%ORIGINALDIRECTORY%"
EXIT /b 1

:BinFolderFound
IF %SILENTMODE%==0 echo Detected AssetProcessorBatch.exe at %MYBATCHFILEDIRECTORY%%BINFOLDER%\ and rc.exe at %MYBATCHFILEDIRECTORY%%BINFOLDER%\rc

EXIT /b 0
