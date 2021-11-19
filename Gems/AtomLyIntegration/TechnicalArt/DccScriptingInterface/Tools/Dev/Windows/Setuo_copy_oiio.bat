@echo off
REM 
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

:: This maps up to the \Dev folder
set O3DE_DEV=..\..\..\..\..\..

:: shared location for default O3DE python location
set O3DE_PYTHON_INSTALL=%O3DE_DEV%\Python

set PY_SITE=%O3DE_PYTHON_INSTALL%\runtime\python-3.7.10-rev2-windows\python\Lib\site-packages

set PACKAGE_LOC=C:\Depot\3rdParty\packages\openimageio-2.1.16.0-rev1-windows\OpenImageIO\2.1.16.0\win_x64\bin

copy %PACKAGE_LOC%\OpenImageIO.pyd %PY_SITE%\OpenImageIO.pyd
