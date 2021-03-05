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

:: Launches Wing IDE and the DccScriptingInterface Project Files

:: version Major
SET PYCHARM_VERSION_YEAR=2020
echo     PYCHARM_VERSION_YEAR = %PYCHARM_VERSION_YEAR%

:: version Major
SET PYCHARM_VERSION_MAJOR=2
echo     PYCHARM_VERSION_MAJOR = %PYCHARM_VERSION_MAJOR%

@echo off



:: Set up window
TITLE Lumberyard DCC Scripting Interface GEM PyCharm CE %PYCHARM_VERSION_YEAR%.%PYCHARM_VERSION_MAJOR%.%PYCHARM_VERSION_MINOR%
:: Use obvious color to prevent confusion (Grey with Yellow Text)
COLOR 8E

echo.
echo _____________________________________________________________________
echo.
echo ~    Setting up LY DCC SIG PyCharm Dev Env...
echo _____________________________________________________________________
echo.

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

SET ABS_PATH=%~dp0
echo ~    Current Dir, %ABS_PATH%

:: Keep changes local
SETLOCAL
CALL %~dp0\Env.bat

echo.
echo _____________________________________________________________________
echo.
echo ~    Setting up Env for PyCharm CE %PYCHARM_VERSION_YEAR%.%PYCHARM_VERSION_MAJOR%.%PYCHARM_VERSION_MINOR%
echo _____________________________________________________________________
echo.


::"C:\Program Files\JetBrains\PyCharm 2019.1.3\bin"
::"C:\Program Files\JetBrains\PyCharm Community Edition 2018.3.5\bin\pycharm64.exe"

:: put project env variables/paths here
set PYCHARM_HOME=%PROGRAMFILES%\JetBrains\PyCharm %PYCHARM_VERSION_YEAR%.%PYCHARM_VERSION_MAJOR%
echo     PYCHARM_HOME = %PYCHARM_HOME%

SET PYCHARM_PROJ=%DCCSIG_PATH%\Solutions
echo     PYCHARM_PROJ = %PYCHARM_PROJ%

echo.
echo _____________________________________________________________________
echo.
echo ~ Launching DCCsi Project in PyCharm %PYCHARM_VERSION_YEAR%.%PYCHARM_VERSION_MAJOR% ...
echo _____________________________________________________________________
echo.

IF EXIST "%PYCHARM_HOME%\bin\pycharm64.exe" (
   start "" "%PYCHARM_HOME%\bin\pycharm64.exe" "%PYCHARM_PROJ%"
) ELSE (
   Where pycharm64.exe 2> NUL
   IF ERRORLEVEL 1 (
      echo pycharm64.exe could not be found
         pause
   ) ELSE (
      start "" pycharm64.exe "%PYCHARM_PROJ%"
   )
)

ENDLOCAL

:: Return to starting directory
POPD

:END_OF_FILE
