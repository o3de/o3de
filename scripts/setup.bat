@echo off
rem
rem All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
rem its licensors.
rem
rem For complete copyright and license terms please see the LICENSE at the root of this
rem distribution (the "License"). All use of this software is governed by the License,
rem or, if provided, by the license below or the license accompanying this file. Do not
rem remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
rem WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
rem

pushd %~dp0%

pushd %~dp0..
set ENGINE_ROOT=%CD%
popd

set cmake_version=3.19.1

if not "%1"=="" (
    set LY_3RDPARTY_PATH=%1
)
if "%LY_3RDPARTY_PATH%"=="" goto no_3rd_party

if not exist %LY_3RDPARTY_PATH% mkdir %LY_3RDPARTY_PATH%
goto install_cmake

:no_3rd_party
echo A path to where the 3rd party folder is required for setup.
echo Either supply one through the LY_3RDPARTY_PATH environment
echo variable or as an argument to this script
goto fail


:install_cmake
set cmake_install_path=%LY_3RDPARTY_PATH%\CMake\%cmake_version%\Windows
set cmake_archive_name=cmake-%cmake_version%-win64-x64
set cmake_archive_path="%ENGINE_ROOT%\Tools\Redistributables\CMake\%cmake_archive_name%.zip"
if exist "%cmake_install_path%\bin\cmake.exe" goto install_python

echo Installing CMake %cmake_version% to %cmake_install_path%
if not exist %cmake_install_path% mkdir %cmake_install_path%
powershell.exe -nologo -noprofile -command^
    "& { Add-Type -A 'System.IO.Compression.FileSystem'; [IO.Compression.ZipFile]::ExtractToDirectory('%cmake_archive_path%', '%cmake_install_path%'); }"
if ERRORLEVEL 1 goto cmake_failed

set cmake_extracted_path=%cmake_install_path%\%cmake_archive_name%
for /d %%a in ("%cmake_extracted_path%\*") do move "%%a" "%cmake_install_path%\"
rmdir %cmake_extracted_path%

goto success

if ERRORLEVEL 1 goto cmake_failed
set LY_CMAKE_PATH="%cmake_install_path%\bin"
goto install_python

:cmake_failed
echo Failed to extract cmake to path %cmake_install_path%
goto fail


:install_python
echo Installing python...
call %ENGINE_ROOT%\python\get_python.bat
if ERRORLEVEL 1 goto python_failed
goto register_engine

:python_failed
echo Failed to acquire python
goto fail


:register_engine
echo Registering engine...
call %ENGINE_ROOT%\scripts\o3de.bat register --this-engine
if ERRORLEVEL 1 goto registration_failed
goto success

:registration_failed
echo Failed to register the engine
goto fail


:fail
echo O3DE setup failed
popd
exit /b 1

:success
echo O3DE setup complete
popd
exit /b %ERRORLEVEL%
