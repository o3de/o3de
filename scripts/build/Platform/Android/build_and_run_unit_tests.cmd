@ECHO OFF
REM
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

SETLOCAL EnableDelayedExpansion

CALL %~dp0gradle_windows.cmd
IF NOT %ERRORLEVEL%==0 GOTO :error

IF NOT EXIST "%LY_3RDPARTY_PATH%" (
    ECHO [ci_build] LY_3RDPARTY_PATH is invalid or not set
    GOTO :error
)

IF NOT EXIST "%LY_ANDROID_SDK%" (
    ECHO [ci_build] FAIL: LY_ANDROID_SDK=!LY_ANDROID_SDK!
    GOTO :error
)
SET ANDROID_SDK_ROOT=%LY_ANDROID_SDK%
ECHO "ANDROID_SDK_ROOT=!ANDROID_SDK_ROOT!"
SET PYTHON=python\python.cmd
ECHO [ci_build] %PYTHON% scripts\build\Platform\Android\run_test_on_android_simulator.py --android-sdk-path %LY_ANDROID_SDK% --build-path %OUTPUT_DIRECTORY% --build-config %CONFIGURATION%
CALL %PYTHON% scripts\build\Platform\Android\run_test_on_android_simulator.py --android-sdk-path %LY_ANDROID_SDK% --build-path %OUTPUT_DIRECTORY% --build-config %CONFIGURATION%
IF NOT %ERRORLEVEL%==0 GOTO :popd_error

EXIT /b 0

:popd_error
POPD

:error
ECHO ERROR

EXIT /b 1
