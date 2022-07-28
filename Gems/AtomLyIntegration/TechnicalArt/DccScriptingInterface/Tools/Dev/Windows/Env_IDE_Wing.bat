@echo off
REM 
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

:: Launches Wing IDE and the O3DE DccScriptingInterface Project Files
:: Status: Prototype
:: Version: 0.0.1
:: Support: Wing Pro 8+
:: Readme.md:  https://github.com/o3de/o3de/tree/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/IDE/WingIDE/readme.md
:: Notes:
:: - Wing 7.x was previously supported, but it was python2.7 based and we are deprecating support for py2.7
:: - py2.7 deprecation includes apps that are pre-py3
:: - Previous versions may still work, however you will need to configure the env yourself
:: - Try overriding envars and paths in your Env_Dev.bat
:: - Wing Pro 8 does not use the version minor in the name of it's folder structure

:: Skip initialization if already completed
IF "%DCCSI_ENV_WINGIDE_INIT%"=="1" GOTO :END_OF_FILE

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

:: WingIDE version Major
IF "%DCCSI_WING_VERSION_MAJOR%"=="" (set DCCSI_WING_VERSION_MAJOR=8)

:: Wing Pro 8 does not use the version minor in the name of it's folder structure
:: Will deprecate these lines in a future iteration
REM :: WingIDE version Minor
REM IF "%DCCSI_WING_VERSION_MINOR%"=="" (set DCCSI_WING_VERSION_MINOR=2)

:: Initialize env
CALL %~dp0\Env_O3DE_Core.bat
CALL %~dp0\Env_O3DE_Python.bat

:: This can now only be added late, in the launcher
:: it conflicts with other Qt apps like Wing Pro 8+
::CALL %~dp0\Env_O3DE_Qt.bat
:: this could interfer with standalone python apps/tools/utils that use O3DE Qt
:: and trying to run them from the IDE
:: We may have to find a work around in the next iteration?

echo.
echo _____________________________________________________________________
echo.
echo ~    O3DE DCCsi IDE Env WingIDE %DCCSI_WING_VERSION_MAJOR%.%DCCSI_WING_VERSION_MINOR% ...
echo _____________________________________________________________________
echo.

:: Wing Pro 8 does not use the version minor in the name of it's folder structure
:: Will deprecate these lines in a future iteration
REM IF "%WINGHOME%"=="" (set "WINGHOME=%PROGRAMFILES(X86)%\Wing Pro %DCCSI_WING_VERSION_MAJOR%.%DCCSI_WING_VERSION_MINOR%")
REM IF "%WING_PROJ%"=="" (set "WING_PROJ=%PATH_DCCSIG%\Tools\Dev\Windows\Solutions\.wing\DCCsi_%DCCSI_WING_VERSION_MAJOR%x.wpr")

:: put project env variables/paths here
IF "%WINGHOME%"=="" (set "WINGHOME=%PROGRAMFILES(X86)%\Wing Pro %DCCSI_WING_VERSION_MAJOR%")
IF "%WING_PROJ%"=="" (set "WING_PROJ=%PATH_DCCSIG%\Tools\IDE\Wing\.solutions\DCCsi_%DCCSI_WING_VERSION_MAJOR%x.wpr")

echo     DCCSI_WING_VERSION_MAJOR = %DCCSI_WING_VERSION_MAJOR%
REM echo     DCCSI_WING_VERSION_MINOR = %DCCSI_WING_VERSION_MINOR%
echo     WINGHOME = %WINGHOME%
echo     WING_PROJ = %WING_PROJ%

:: Set flag so we don't initialize dccsi environment twice
SET DCCSI_ENV_WINGIDE_INIT=1
GOTO END_OF_FILE

:: Return to starting directory
POPD

:END_OF_FILE
