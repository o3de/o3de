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

IF NOT EXIST "%LY_3RDPARTY_PATH%" (
    ECHO [ci_build] LY_3RDPARTY_PATH is invalid or not set
    GOTO :error
)

IF NOT EXIST "%GRADLE_HOME%" (
    REM This is the default for developers
    SET GRADLE_HOME=C:\Gradle\gradle-5.6.4
)
IF NOT EXIST "%GRADLE_HOME%" (
    ECHO [ci_build] FAIL: GRADLE_HOME=%GRADLE_HOME%
    GOTO :error
)

IF NOT EXIST "%CMAKE_HOME%" (
    SET CMAKE_HOME=%LY_3RDPARTY_PATH%/CMake/3.19.1/Windows/
)
IF NOT EXIST "%CMAKE_HOME%" (
    ECHO [ci_build] FAIL: CMAKE_HOME=%CMAKE_HOME%
    GOTO :error
)

IF NOT EXIST "%LY_NINJA_PATH%" (
    SET LY_NINJA_PATH=%LY_3RDPARTY_PATH%/ninja/1.10.1/Windows
)
IF NOT EXIST "%LY_NINJA_PATH%" (
    ECHO [ci_build] FAIL: LY_NINJA_PATH=%LY_NINJA_PATH%
    GOTO :error
)

IF NOT EXIST "%LY_ANDROID_SDK%" (
    SET LY_ANDROID_SDK=%LY_3RDPARTY_PATH%/android-sdk/platform-29
)
IF NOT EXIST "%LY_ANDROID_SDK%" (
    ECHO [ci_build] FAIL: LY_ANDROID_SDK=%LY_ANDROID_SDK%
    GOTO :error
)

IF NOT EXIST "%LY_ANDROID_NDK%" (
    set LY_ANDROID_NDK=%LY_3RDPARTY_PATH%/android-ndk/r21d
)
IF NOT EXIST "%LY_ANDROID_NDK%" (
    ECHO [ci_build] LY_ANDROID_NDK=%LY_ANDROID_NDK%
    GOTO :error
)

REM Delete output directory if CLEAN_OUTPUT_DIRECTORY env variable is set
IF "%CLEAN_OUTPUT_DIRECTORY%"=="true" (
    IF EXIST %OUTPUT_DIRECTORY% (
        ECHO [ci_build] CLEAN_OUTPUT_DIRECTORY option set with value "%CLEAN_OUTPUT_DIRECTORY%"
        ECHO [ci_build] Deleting "%OUTPUT_DIRECTORY%"
        DEL /s /q /f %OUTPUT_DIRECTORY%
    )
)

IF NOT EXIST %OUTPUT_DIRECTORY% (
    mkdir %OUTPUT_DIRECTORY%
)

REM Jenkins reports MSB8029 when TMP/TEMP is not defined, define a dummy folder
SET TMP=%cd%/temp
SET TEMP=%cd%/temp
IF NOT EXIST %TMP% (
    mkdir temp
)
SET PYTHON=python\python.cmd
ECHO [ci_build] %PYTHON% cmake\Tools\Platform\Android\generate_android_project.py --project-path=%GAME_PROJECT% --engine-root=. --build-dir=%OUTPUT_DIRECTORY% --gradle-install-path=%GRADLE_HOME% --cmake-install-path=%CMAKE_HOME% --ninja-install-path=%LY_NINJA_PATH% --third-party-path=%LY_3RDPARTY_PATH% --android-ndk-path=%LY_ANDROID_NDK% --android-sdk-path=%LY_ANDROID_SDK% --android-ndk-version=%ANDROID_NDK_PLATFORM% --android-sdk-version=%ANDROID_SDK_PLATFORM%
CALL %PYTHON% cmake\Tools\Platform\Android\generate_android_project.py --project-path="%GAME_PROJECT%" --engine-root=. --build-dir="%OUTPUT_DIRECTORY%" --gradle-install-path="%GRADLE_HOME%" --cmake-install-path="%CMAKE_HOME%" --ninja-install-path="%LY_NINJA_PATH%" --third-party-path="%LY_3RDPARTY_PATH%" --android-ndk-path="%LY_ANDROID_NDK%" --android-sdk-path="%LY_ANDROID_SDK%" --android-ndk-version=%ANDROID_NDK_PLATFORM% --android-sdk-version=%ANDROID_SDK_PLATFORM%
IF NOT %ERRORLEVEL%==0 GOTO :error

PUSHD %OUTPUT_DIRECTORY%

REM Stop any running or orphaned gradle daemon 
ECHO [ci_build] gradlew --stop
gradlew --stop

ECHO [ci_build] gradlew --no-daemon -build%CONFIGURATION%
gradlew --no-daemon build%CONFIGURATION%
IF NOT %ERRORLEVEL%==0 GOTO :popd_error

POPD

ECHO [ci_build] gradlew --stop
gradlew --stop

EXIT /b 0

:popd_error
POPD

:error

ECHO [ci_build] gradlew --stop
gradlew --stop

EXIT /b 1