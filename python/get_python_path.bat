@echo off
REM 
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

:: Retreives the path to the O3dE python executable

FOR /F "tokens=* USEBACKQ" %%F IN (`python.cmd get_python_path.py`) DO (SET O3DE_PY_EXE=%%F)
echo     O3DE_PY_EXE - is now the folder containing python executable
echo     O3DE_PY_EXE = %O3DE_PY_EXE% 