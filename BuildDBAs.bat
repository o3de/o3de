@ECHO OFF

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
REM Original file Copyright Crytek GMBH or its affiliates, used under license.
REM

REM  command line argument %1, if exists, will be the Game Name that we are using
REM  command line argument %2, if exists, will be the TempDir name that we are using  (will not be deleted if exists)

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

SET gameName=RPGSample
SET baseTempDir=TempRC
SET tempDir=%baseTempDir%
SET tempDefined=False
SET tempDefinedExistsAlready=False
SET /a uniqueIndex =1

REM  Allow gameName to be overwritten by command line arg name
IF NOT "%1"=="" (SET gameName=%1)
IF NOT "%2"=="" (SET tempDefined=True)

IF %tempDefined%==True GOTO alreadyDefined
GOTO makeTempDirName

:alreadyDefined
SET tempDir=%2
IF EXIST %tempDir% SET tempDefinedExistsAlready=True
GOTO tempDirDefined

:makeTempDirName
IF NOT EXIST %tempDir% GOTO tempDirDefined
SET tempDir=%baseTempDir%%uniqueIndex%
SET /a uniqueIndex = %uniqueIndex% + 1
GOTO makeTempDirName

:tempDirDefined
%BINFOLDER%\rc\RC.exe /job="%BINFOLDER%\rc\RCJob_Build_DBAs.xml" /gamesubdirectory="%gameName%" /game="%gameName%" /target="%tempDir%/%gameName%"

IF NOT %tempDefinedExistsAlready%==True (rmdir /S /Q %tempDir%)

