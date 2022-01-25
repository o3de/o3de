@ECHO OFF
REM
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

SETLOCAL EnableDelayedExpansion

IF NOT EXIST "%LY_3RDPARTY_PATH%" (
    ECHO [ci_build] LY_3RDPARTY_PATH is invalid or not set
    GOTO :error
)

IF NOT EXIST "%GRADLE_BUILD_HOME%" (
    REM This is the default for developers
    SET GRADLE_BUILD_HOME=C:\Gradle\gradle-7.0
)
IF NOT EXIST "%GRADLE_BUILD_HOME%" (
    ECHO [ci_build] FAIL: GRADLE_BUILD_HOME=%GRADLE_BUILD_HOME%
    GOTO :error
)

IF NOT "%ANDROID_GRADLE_PLUGIN%" == "" (
    set ANDROID_GRADLE_PLUGIN_OPTION=--gradle-plugin-version=%ANDROID_GRADLE_PLUGIN%
)

IF NOT EXIST %OUTPUT_DIRECTORY% (
    mkdir %OUTPUT_DIRECTORY%
)

REM Jenkins reports MSB8029 when TMP/TEMP is not defined, define a dummy folder
SET TMP=%cd%/temp
SET TEMP=%cd%/temp
IF NOT EXIST %TMP% (
    mkdir %TMP%
)

REM Optionally sign the APK if we are generating an APK 
SET GENERATE_SIGNED_APK=false
IF "%SIGN_APK%"=="true" (
    IF "%GRADLE_BUILD_CMD%"=="assemble" (
        SET GENERATE_SIGNED_APK=true
    )
)

SET PYTHON=python\python.cmd

REM Regardless of whether or not we generate a signing key, apparently we must set variables outside of 
REM an IF clause otherwise it will not work.

REM First look for the JDK HOME in the environment variable
IF EXIST "%JDK_HOME%" (
    ECHO JDK Home found in Environment: !JDK_HOME!
    GOTO JDK_FOUND
)

REM Next, look in the registry
FOR /F "skip=2 tokens=1,2*" %%A IN ('REG QUERY "HKEY_LOCAL_MACHINE\SOFTWARE\JavaSoft\Java Development Kit\1.8" /v "JavaHome" 2^>nul') DO (
    SET JDK_REG_VALUE=%%C
)
IF EXIST "%JDK_REG_VALUE%" (
    SET JDK_HOME=!JDK_REG_VALUE!
    ECHO JDK Home found in registry: !JDK_HOME!
    GOTO JDK_FOUND
)

ECHO Unable to locate JDK_HOME
GOTO error

:JDK_FOUND

SET JDK_BIN=%JDK_HOME%\bin
IF NOT EXIST "%JDK_BIN%" (
    ECHO The environment variable JDK_HOME is not set to a valid JDK 1.8 folder %JDK_BIN%
    ECHO Make sure the variable is set to your local JDK 1.8 installation
    GOTO error
)

SET KEYTOOL_PATH=%JDK_BIN%\keytool.exe
IF NOT EXIST "%KEYTOOL_PATH%" (
    ECHO The environment variable JDK_HOME is not set to a valid JDK 1.8 folder. Cannot find keytool at %JDK_BIN%\keytool.exe
    ECHO Make sure the variable is set to your local JDK 1.8 installation
    GOTO error
)

SET CI_ANDROID_KEYSTORE_FILE=ly-android-dev.keystore
SET CI_ANDROID_KEYSTORE_ALIAS=ly-android
SET CI_ANDROID_KEYSTORE_PASSWORD=lumberyard
SET CI_ANDROID_KEYSTORE_DN=cn=LY Developer, ou=Lumberyard, o=Amazon, c=US
SET CI_KEYSTORE_VALIDITY_DAYS=10000
SET CI_KEYSTORE_CERT_DN=cn=LY Developer, ou=Lumberyard, o=Amazon, c=US

REM Clear out any existing keystore file since the password/alias may have changed
SET CI_ANDROID_KEYSTORE_FILE_ABS=%cd%\%OUTPUT_DIRECTORY%\%CI_ANDROID_KEYSTORE_FILE%


IF "%GENERATE_SIGNED_APK%"=="true" (

    REM Prepare a temporary keystore just for this unit test session

    REM Generate the keystore file if needed
    IF NOT EXIST "%CI_ANDROID_KEYSTORE_FILE_ABS%" (

        ECHO [ci_build] Generating keystore file %CI_ANDROID_KEYSTORE_FILE%
        ECHO [ci_build] "%KEYTOOL_PATH%" -genkeypair -v -keystore %CI_ANDROID_KEYSTORE_FILE_ABS% -storepass %CI_ANDROID_KEYSTORE_PASSWORD% -alias %CI_ANDROID_KEYSTORE_ALIAS% -keypass %CI_ANDROID_KEYSTORE_PASSWORD% -keyalg RSA -keysize 2048 -validity %CI_KEYSTORE_VALIDITY_DAYS% -dname "%CI_KEYSTORE_CERT_DN%"
        CALL "%KEYTOOL_PATH%" -genkeypair -v -keystore %CI_ANDROID_KEYSTORE_FILE_ABS% -storepass %CI_ANDROID_KEYSTORE_PASSWORD% -alias %CI_ANDROID_KEYSTORE_ALIAS% -keypass %CI_ANDROID_KEYSTORE_PASSWORD% -keyalg RSA -keysize 2048 -validity %CI_KEYSTORE_VALIDITY_DAYS% -dname "%CI_KEYSTORE_CERT_DN%" 2> nul 
        IF errorlevel 1 (
            ECHO Unable to generate keystore file "%CI_ANDROID_KEYSTORE_FILE_ABS%"
            GOTO error
        )
    ) ELSE (
        ECHO Using keystore file at %CI_ANDROID_KEYSTORE_FILE_ABS%
    )

    ECHO [ci_build] %PYTHON% cmake\Tools\Platform\Android\generate_android_project.py --engine-root=. --build-dir=%OUTPUT_DIRECTORY% -g %GAME_PROJECT% --gradle-install-path=%GRADLE_BUILD_HOME% --third-party-path=%LY_3RDPARTY_PATH% --enable-unity-build --android-sdk-path=%ANDROID_HOME% %ANDROID_GRADLE_PLUGIN_OPTION% --signconfig-store-file %CI_ANDROID_KEYSTORE_FILE_ABS% --signconfig-store-password %CI_ANDROID_KEYSTORE_PASSWORD% --signconfig-key-alias %CI_ANDROID_KEYSTORE_ALIAS% --signconfig-key-password %CI_ANDROID_KEYSTORE_PASSWORD% %ADDITIONAL_GENERATE_ARGS% --overwrite-existing
    CALL %PYTHON% cmake\Tools\Platform\Android\generate_android_project.py --engine-root=. --build-dir=%OUTPUT_DIRECTORY% -g %GAME_PROJECT% --gradle-install-path=%GRADLE_BUILD_HOME% --third-party-path=%LY_3RDPARTY_PATH% --enable-unity-build --android-sdk-path=%ANDROID_HOME% %ANDROID_GRADLE_PLUGIN_OPTION% --signconfig-store-file %CI_ANDROID_KEYSTORE_FILE_ABS% --signconfig-store-password %CI_ANDROID_KEYSTORE_PASSWORD% --signconfig-key-alias %CI_ANDROID_KEYSTORE_ALIAS% --signconfig-key-password %CI_ANDROID_KEYSTORE_PASSWORD% %ADDITIONAL_GENERATE_ARGS% --overwrite-existing
) ELSE (
    ECHO [ci_build] %PYTHON% cmake\Tools\Platform\Android\generate_android_project.py --engine-root=. --build-dir=%OUTPUT_DIRECTORY% -g %GAME_PROJECT% %GRADLE_OVERRIDE_OPTION% --third-party-path=%LY_3RDPARTY_PATH% --enable-unity-build %ANDROID_GRADLE_PLUGIN_OPTION% --android-sdk-path=%ANDROID_HOME% %ADDITIONAL_GENERATE_ARGS% --overwrite-existing
    CALL %PYTHON% cmake\Tools\Platform\Android\generate_android_project.py --engine-root=. --build-dir=%OUTPUT_DIRECTORY% -g %GAME_PROJECT% --gradle-install-path=%GRADLE_BUILD_HOME% --third-party-path=%LY_3RDPARTY_PATH% --enable-unity-build %ANDROID_GRADLE_PLUGIN_OPTION% --android-sdk-path=%ANDROID_HOME% %ADDITIONAL_GENERATE_ARGS% --overwrite-existing
)

REM Validate the android project generation
IF %ERRORLEVEL%==0 GOTO generate_project_success
ECHO Error Generating Android Project
goto error

:generate_project_success

REM Run the gradle build from the output directory
PUSHD %OUTPUT_DIRECTORY%

REM Stop any running or orphaned gradle daemon 
ECHO [ci_build] gradlew --stop
CALL gradlew --stop

ECHO [ci_build] gradlew --no-daemon %GRADLE_BUILD_CMD%%CONFIGURATION%
CALL gradlew --no-daemon %GRADLE_BUILD_CMD%%CONFIGURATION%

IF %ERRORLEVEL%==0 GOTO gradle_build_success

REM Do another build with the debug flag to try to get the failure reasons

GOTO error

ECHO Error building gradle. Rebuilding with debug information
ECHO [ci_build] gradlew --debug --full-stacktrace --no-daemon %GRADLE_BUILD_CMD%%CONFIGURATION%
CALL gradlew --debug --full-stacktrace --no-daemon %GRADLE_BUILD_CMD%%CONFIGURATION%

ECHO [ci_build] gradlew --stop
CALL gradlew --stop

POPD

GOTO error


:gradle_build_success

ECHO [ci_build] gradlew --stop
CALL gradlew --stop

POPD

EXIT /b 0

:popd_error
POPD

:error

EXIT /b 1