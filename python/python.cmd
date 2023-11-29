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
SET PYTHON_VENV=%USERPROFILE%\.o3de\3rdParty\venv\%ENGINE_ID%
SET PYTHON_VENV_ACTIVATE=%PYTHON_VENV%\Scripts\activate.bat
SET PYTHON_VENV_DEACTIVATE=%PYTHON_VENV%\Scripts\deactivate.bat
IF [%1] EQU [debug] (
    SET PYTHON_VENV_PYTHON=%PYTHON_VENV%\Scripts\python_d.exe
    SET PYTHON_ARGS=%*:~6%
) ELSE (
    SET PYTHON_VENV_PYTHON=%PYTHON_VENV%\Scripts\python.exe
    SET PYTHON_ARGS=%*
)
IF EXIST %PYTHON_VENV_PYTHON% GOTO PYTHON_VENV_EXISTS
ECHO Python has not been setup completely for O3DE. 
ECHO Try running %CMD_DIR%\get_python.bat to setup Python for O3DE.
exit /b 1


:PYTHON_VENV_EXISTS
REM Execute the python call from the arguments within the python venv environment

call %PYTHON_VENV_ACTIVATE%

call "%PYTHON_VENV_PYTHON%" %PYTHON_ARGS%
SET PYTHON_RESULT=%ERRORLEVEL%

call %PYTHON_VENV_DEACTIVATE%

exit /B %PYTHON_RESULT%
