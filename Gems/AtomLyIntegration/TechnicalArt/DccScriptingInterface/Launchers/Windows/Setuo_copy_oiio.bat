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

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

:: This maps up to the \Dev folder
set LY_DEV=..\..\..\..\..\..

:: shared location for default O3DE python location
set DCCSI_PYTHON_INSTALL=%LY_DEV%\Python

set PY_SITE=%DCCSI_PYTHON_INSTALL%\runtime\python-3.7.10-rev1-windows\python\Lib\site-packages

set PACKAGE_LOC=C:\Depot\3rdParty\packages\openimageio-2.1.16.0-rev1-windows\OpenImageIO\2.1.16.0\win_x64\bin

copy %PACKAGE_LOC%\OpenImageIO.pyd %PY_SITE%\OpenImageIO.pyd
