@ECHO OFF
REM
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

REM Deploy the CDK applcations for AWS gems (Windows only)
REM Prerequisites:
REM 1) Node.js is installed
REM 2) Node.js version >= 10.13.0, except for versions 13.0.0 - 13.6.0. A version in active long-term support is recommended.
SETLOCAL EnableDelayedExpansion

SET SOURCE_DIRECTORY=%CD%
SET PATH=%SOURCE_DIRECTORY%\python;%PATH%
SET GEM_DIRECTORY=%SOURCE_DIRECTORY%\Gems

REM Create and activate a virtualenv for the CDK deployment
CALL python -m venv .env
IF ERRORLEVEL 1 (
    ECHO [cdk_bootstrap] Failed to create a virtualenv for the CDK deployment
    exit /b 1
)
CALL .env\Scripts\activate.bat
IF ERRORLEVEL 1 (
    ECHO [cdk_bootstrap] Failed to activate the virtualenv for the CDK deployment
    exit /b 1
)

ECHO [cdk_installation] Install the latest version of CDK
CALL npm uninstall -g aws-cdk
IF ERRORLEVEL 1 (
    ECHO [cdk_bootstrap] Failed to uninstall the current version of CDK
    exit /b 1
)
CALL npm install -g aws-cdk@latest
IF ERRORLEVEL 1 (
    ECHO [cdk_bootstrap] Failed to install the latest version of CDK
    exit /b 1
)

REM Set temporary AWS credentials from the assume role
FOR /f "tokens=1,2,3" %%a IN ('CALL aws sts assume-role --query Credentials.[SecretAccessKey^,SessionToken^,AccessKeyId] --output text --role-arn %ASSUME_ROLE_ARN% --role-session-name o3de-Automation-session') DO (
    SET AWS_SECRET_ACCESS_KEY=%%a
    SET AWS_SESSION_TOKEN=%%b
    SET AWS_ACCESS_KEY_ID=%%c
)
FOR /F "tokens=4 delims=:" %%a IN ("%ASSUME_ROLE_ARN%") DO SET O3DE_AWS_DEPLOY_ACCOUNT=%%a

REM Bootstrap and deploy the CDK applications
ECHO [cdk_bootstrap] Bootstrap CDK
CALL cdk bootstrap aws://%O3DE_AWS_DEPLOY_ACCOUNT%/%O3DE_AWS_DEPLOY_REGION%
IF ERRORLEVEL 1 (
    ECHO [cdk_bootstrap] Failed to bootstrap CDK
    exit /b 1
)

CALL :DeployCDKApplication AWSCore "-c disable_access_log=true --all"
IF ERRORLEVEL 1 (
    exit /b 1
)
CALL :DeployCDKApplication AWSClientAuth
IF ERRORLEVEL 1 (
    exit /b 1
)
CALL :DeployCDKApplication AWSMetrics "-c batch_processing=true"
IF ERRORLEVEL 1 (
    exit /b 1
)

EXIT /b 0

:DeployCDKApplication
REM Deploy the CDK application for a specific AWS gem
SET GEM_NAME=%~1
SET ADDITIONAL_ARGUMENTS=%~2
ECHO [cdk_deployment] Deploy the CDK application for the %GEM_NAME% gem
PUSHD %GEM_DIRECTORY%\%GEM_NAME%\cdk

REM Revert the CDK application code to a stable state using the provided commit ID
CALL git checkout %COMMIT_ID% -- .
IF ERRORLEVEL 1 (
    ECHO [git_checkout] Failed to checkout the CDK application for the %GEM_NAME% gem using commit ID %COMMIT_ID%
    POPD
    exit /b 1
)

REM Install required packages for the CDK application
CALL python -m pip install -r requirements.txt
IF ERRORLEVEL 1 (
    ECHO [cdk_deployment] Failed to install required packages for the %GEM_NAME% gem
    POPD
    exit /b 1
)

REM Deploy the CDK application
CALL cdk deploy %ADDITIONAL_ARGUMENTS% --require-approval never
IF ERRORLEVEL 1 (
    ECHO [cdk_deployment] Failed to deploy the CDK application for the %GEM_NAME% gem
    POPD
    exit /b 1
)
POPD
