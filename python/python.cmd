@ECHO OFF
REM 
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

REM This script provides a single entry point that you can trust is present.
REM Depending on this entry point instead of trying to find a python.exe
REM In a subfolder allows you to keep working if the version of python changes or
REM other environmental requirements change.
REM When the project switches to a new version of Python, this file will be updated.

SETLOCAL
SET CMD_DIR=%~dp0
SET CMD_DIR=%CMD_DIR:~0,-1%

REM Calculate the path to the expected python venv for the current engine located at %CMD_DIR%\.. 
REM The logic in LYPython will generate a unique ID based on the absolute path of the current engine
REM so that the venv will not collide with any other versions of O3DE installed on the current machine


REM Run the custom cmake command script to calculate the ID based on %CMD_DIR%\.. 
SET CALC_PATH=%CMD_DIR%\..\cmake\CalculateEnginePathId.cmake
FOR /F %%g IN ('cmake -P %CALC_PATH% %CMD_DIR%\..') DO SET ENGINE_ID=%%g
IF NOT "%ENGINE_ID%" == "" GOTO ENGINE_ID_CALCULATED
echo
echo Unable to calculate engine ID
exit /b 1

:ENGINE_ID_CALCULATED

REM Set the expected location of the python venv for this engine and the locations of the critical scripts/executables 
REM needed to run python within the venv properly

REM If the %LY_3RDPARTY_PATH% is not set, then default it to %USERPROFILE%/.o3de/3rdParty
IF "" == "%LY_3RDPARTY_PATH%" (
    SET LY_3RDPARTY_PATH=%USERPROFILE%\.o3de\3rdParty
)

SET PYTHON_VENV=%USERPROFILE%\.o3de\Python\venv\%ENGINE_ID%
SET PYTHON_VENV_ACTIVATE=%PYTHON_VENV%\Scripts\activate.bat
SET PYTHON_VENV_DEACTIVATE=%PYTHON_VENV%\Scripts\deactivate.bat
IF [%1] EQU [debug] (
    SET PYTHON_VENV_PYTHON=%PYTHON_VENV%\Scripts\python_d.exe
    SET PYTHON_ARGS=%*:~6%
) ELSE (
    SET PYTHON_VENV_PYTHON=%PYTHON_VENV%\Scripts\python.exe
    SET PYTHON_ARGS=%*
)
IF EXIST "%PYTHON_VENV_PYTHON%" GOTO PYTHON_VENV_EXISTS
ECHO Python has not been setup completely for O3DE. Missing Python venv %PYTHON_VENV_PYTHON%
ECHO Try running %CMD_DIR%\get_python.bat to setup Python for O3DE.
exit /b 1

:PYTHON_VENV_EXISTS

REM If python venv exists, we still need to validate that it is the current version by getting the 
REM package current package hash from 3rd Party
FOR /F %%g IN ('cmake -P %CMD_DIR%\get_python_package_hash.cmake %CMD_DIR%\.. Windows') DO SET CURRENT_PACKAGE_HASH=%%g
IF NOT "%CURRENT_PACKAGE_HASH%" == "" GOTO PACKAGE_HASH_READ
echo
echo Unable to get current python package hash
exit /b 1


:PACKAGE_HASH_READ

REM Make sure there a .hash file that serves as the marker for the source python package the venv is from
SET PYTHON_VENV_HASH=%PYTHON_VENV%\.hash

IF EXIST "%PYTHON_VENV_HASH%" GOTO PYTHON_VENV_HASH_EXISTS
ECHO Python has not been setup completely for O3DE. Missing venv hash %PYTHON_VENV_HASH%
ECHO Try running %CMD_DIR%\get_python.bat to setup Python for O3DE.
exit /b 1

:PYTHON_VENV_HASH_EXISTS

REM Read in the .hash from the venv to see if we need to update the version of python
SET /p VENV_PACKAGE_HASH=<"%PYTHON_VENV_HASH%"

IF "%VENV_PACKAGE_HASH%" == "%CURRENT_PACKAGE_HASH%" GOTO PYTHON_VENV_MATCHES
ECHO Python needs to be updated against the current version.
ECHO Try running %CMD_DIR%\get_python.bat to update Python for O3DE.
exit /b 1

:PYTHON_VENV_MATCHES
REM Execute the python call from the arguments within the python venv environment

call "%PYTHON_VENV_ACTIVATE%"

call "%PYTHON_VENV_PYTHON%" -B %PYTHON_ARGS%
SET PYTHON_RESULT=%ERRORLEVEL%

call "%PYTHON_VENV_DEACTIVATE%"

exit /B %PYTHON_RESULT%
