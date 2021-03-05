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

SETLOCAL

IF [%3] == [] (
    GOTO USAGE
)

REM search for the engine root from the engine.json if possible
IF NOT EXIST engine.json GOTO noSetupConfig

pushd %~dp0%

FOR /F "tokens=1,2*" %%A in ('findstr /I /N "ExternalEnginePath" engine.json') do SET ENGINE_ROOT=%%C

REM Clear the trailing comma if any
SET ENGINE_ROOT=%ENGINE_ROOT:,=%

REM Trim the double quotes
SET ENGINE_ROOT=%ENGINE_ROOT:"=%

IF "%ENGINE_ROOT%"=="" GOTO noSetupConfig

IF NOT EXIST "%ENGINE_ROOT%" GOTO noSetupConfig

REM Set the base path to the value
SET BASE_PATH=%ENGINE_ROOT%\
ECHO Engine Root: %BASE_PATH%
GOTO pythonPathSet

:noSetupConfig
SET BASE_PATH=%~dp0
ECHO Engine Root: %BASE_PATH%

:pythonPathSet
SET TOOLS_DIR=%BASE_PATH%\Tools

SET PYTHON_DIR=%BASE_PATH%\python
IF EXIST "%PYTHON_DIR%" GOTO PYTHON_DIR_EXISTS

ECHO Could not find Python in %PYTHON_DIR%
GOTO :EOF

:PYTHON_DIR_EXISTS

SET PYTHON=%PYTHON_DIR%\python.cmd
IF EXIST "%PYTHON%" GOTO PYTHON_EXISTS

ECHO Could not find python.cmd in %PYTHON_DIR%
GOTO :EOF

:PYTHON_EXISTS

REM Attempt to determine the best BinFolder for rc.exe and AssetProcessorBatch.exe
CALL "%BASE_PATH%\DetermineRCandAP.bat" SILENT

REM If a bin folder was registered, validate the presence of the binfolder/rc.exe
IF NOT ERRORLEVEL 0 (
    ECHO Unable to determine the locations of AssetProcessor and rc.exe.  Make sure that they are available or rebuild from source
    GOTO FAILED
)

set BINFOLDERPATH=%~dp0%BINFOLDER%
ECHO Detected binary folder at %BINFOLDERPATH%

SET GAMENAME=%1
SET SHADERFLAVOR=%2
SET ASSETPLATFORM=%3
SET SOURCESHADERLIST=%4

REM Trim the trailing \ from BASE_PATH, otherwise passing "%BASE_PATH%" with a trailing '\' causes the last " to be escaped and the arg processing for -s fails
IF %BASE_PATH:~-1%==\ SET BASE_PATH=%BASE_PATH:~0,-1%

SET ARGS=%GAMENAME% %ASSETPLATFORM% %SHADERFLAVOR% -b "%BINFOLDER%" -e "%BASE_PATH%"
IF NOT [%SOURCESHADERLIST%] == [] SET ARGS=%ARGS% -s %SOURCESHADERLIST%

call "%PYTHON%" "%TOOLS_DIR%\PakShaders\gen_shaders.py" %ARGS%

IF ERRORLEVEL 1 GOTO FAILED

SET SOURCE="Cache\%GAMENAME%\%ASSETPLATFORM%\user\cache\shaders\cache"
SET OUTPUT="build\%ASSETPLATFORM%\%GAMENAME%"

call "%PYTHON%" "%TOOLS_DIR%\PakShaders\pak_shaders.py" %OUTPUT% -r %SOURCE% -s %SHADERFLAVOR%

IF ERRORLEVEL 1 GOTO FAILED
popd
EXIT /b 0

:FAILED
popd
EXIT /b 1

:USAGE
ECHO expected usage: "lmbr_pak_shaders.bat <GameName> D3D11|GLES3|METAL pc|es3|ios|osx_gl [source shader list file]"
EXIT /B 1

