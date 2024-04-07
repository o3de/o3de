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
    ECHO [ci_build] Warning: LY_3RDPARTY_PATH %LY_3RDPARTY_PATH% does not exist. The 3rd party files will be downloaded.
    mkdir %LY_3RDPARTY_PATH%
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
) ELSE (
    ECHO Clearing and reseting existing build folder
    DEL /S /F /Q %OUTPUT_DIRECTORY%
    RMDIR /S /Q %OUTPUT_DIRECTORY%
    mkdir %OUTPUT_DIRECTORY%
)

REM Jenkins does not defined TMP
IF "%TMP%"=="" (
    IF "%WORKSPACE%"=="" (
        SET TMP=%APPDATA%\Local\Temp
        SET TEMP=%APPDATA%\Local\Temp
    ) ELSE (
        SET TMP=%WORKSPACE%\Temp
        SET TEMP=%WORKSPACE%\Temp
        REM This folder may not be created in the workspace
        IF NOT EXIST "!TMP!" (
            MKDIR "!TMP!"
        )
    )
)

REM Create a minimal project for the native build process
IF EXIST "%TMP%\o3de_gradle_ar" (
    DEL /S /F /Q "%TMP%\o3de_gradle_ar"
    RMDIR /S /Q "%TMP%\o3de_gradle_ar"
)
ECHO Creating a minimal project for the native build process
ECHO %PYTHON% scripts\o3de.py create-project -pp "%TMP%\o3de_gradle_ar" -pn GradleTest -tn MinimalProject
CALL %PYTHON% scripts\o3de.py create-project -pp "%TMP%\o3de_gradle_ar" -pn GradleTest -tn MinimalProject

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

REM First look in the windowsregistry
FOR /F "skip=2 tokens=1,2*" %%A IN ('REG QUERY "HKEY_LOCAL_MACHINE\SOFTWARE\JavaSoft\Java Development Kit\1.8" /v "JavaHome" 2^>nul') DO (
    SET JDK_REG_VALUE=%%C
)
IF EXIST "%JDK_REG_VALUE%" (
    SET JAVA_HOME=!JDK_REG_VALUE!
    ECHO JAVA_HOME found in registry: !JAVA_HOME!
    GOTO JDK_FOUND
) ELSE (
    REM Next, look for the JDK HOME in the environment variable
    IF EXIST "%JAVA_HOME%" (
        ECHO JDK Home found in Environment: !JAVA_HOME!
        GOTO JDK_FOUND
    )
)

ECHO Unable to locate JAVA_HOME
GOTO error

:JDK_FOUND

SET JAVA_BIN=%JAVA_HOME%\bin
IF NOT EXIST "%JAVA_BIN%" (
    ECHO The environment variable JAVA_HOME is not set to a valid JDK 1.8 folder %JAVA_BIN%
    ECHO Make sure the variable is set to your local JDK 1.8 installation
    GOTO error
)

SET KEYTOOL_PATH=%JAVA_BIN%\keytool.exe
IF NOT EXIST "%KEYTOOL_PATH%" (
    ECHO The environment variable JAVA_HOME is not set to a valid JDK 1.8 folder. Cannot find keytool at %JAVA_BIN%\keytool.exe
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

    ECHO [ci_build] %PYTHON% cmake\Tools\Platform\Android\generate_android_project.py --engine-root=. --build-dir=%OUTPUT_DIRECTORY% -g "%TMP%\o3de_gradle_ar" --gradle-install-path=%GRADLE_BUILD_HOME% --third-party-path=%LY_3RDPARTY_PATH% --enable-unity-build --android-sdk-path=%ANDROID_HOME% %ANDROID_GRADLE_PLUGIN_OPTION% --signconfig-store-file %CI_ANDROID_KEYSTORE_FILE_ABS% --signconfig-store-password %CI_ANDROID_KEYSTORE_PASSWORD% --signconfig-key-alias %CI_ANDROID_KEYSTORE_ALIAS% --signconfig-key-password %CI_ANDROID_KEYSTORE_PASSWORD% %ADDITIONAL_GENERATE_ARGS% --overwrite-existing
    CALL %PYTHON% cmake\Tools\Platform\Android\generate_android_project.py --engine-root=. --build-dir=%OUTPUT_DIRECTORY% -g "%TMP%\o3de_gradle_ar" --gradle-install-path=%GRADLE_BUILD_HOME% --third-party-path=%LY_3RDPARTY_PATH% --enable-unity-build --android-sdk-path=%ANDROID_HOME% %ANDROID_GRADLE_PLUGIN_OPTION% --signconfig-store-file %CI_ANDROID_KEYSTORE_FILE_ABS% --signconfig-store-password %CI_ANDROID_KEYSTORE_PASSWORD% --signconfig-key-alias %CI_ANDROID_KEYSTORE_ALIAS% --signconfig-key-password %CI_ANDROID_KEYSTORE_PASSWORD% %ADDITIONAL_GENERATE_ARGS% --overwrite-existing
) ELSE (
    ECHO [ci_build] %PYTHON% cmake\Tools\Platform\Android\generate_android_project.py --engine-root=. --build-dir=%OUTPUT_DIRECTORY% -g "%TMP%\o3de_gradle_ar" %GRADLE_OVERRIDE_OPTION% --third-party-path=%LY_3RDPARTY_PATH% --enable-unity-build %ANDROID_GRADLE_PLUGIN_OPTION% --android-sdk-path=%ANDROID_HOME% %ADDITIONAL_GENERATE_ARGS% --overwrite-existing
    CALL %PYTHON% cmake\Tools\Platform\Android\generate_android_project.py --engine-root=. --build-dir=%OUTPUT_DIRECTORY% -g "%TMP%\o3de_gradle_ar" --gradle-install-path=%GRADLE_BUILD_HOME% --third-party-path=%LY_3RDPARTY_PATH% --enable-unity-build %ANDROID_GRADLE_PLUGIN_OPTION% --android-sdk-path=%ANDROID_HOME% %ADDITIONAL_GENERATE_ARGS% --overwrite-existing
)

SET CMAKE_BUILD_PARALLEL_LEVEL=!NUMBER_OF_PROCESSORS!

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

ECHO [ci_build] gradlew --info --no-daemon %GRADLE_BUILD_CMD%%CONFIGURATION% 
CALL gradlew --info --no-daemon %GRADLE_BUILD_CMD%%CONFIGURATION%

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
