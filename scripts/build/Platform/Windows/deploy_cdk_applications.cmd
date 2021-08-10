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
CALL .env\Scripts\activate.bat

ECHO [cdk_installation] Install the latest version of CDK
CALL npm uninstall -g aws-cdk
CALL npm install -g aws-cdk@latest

REM Set temporary AWS credentials from the assume role
FOR /f "tokens=1,2,3" %%a IN ('CALL aws sts assume-role --query Credentials.[SecretAccessKey^,SessionToken^,AccessKeyId] --output text --role-arn %ASSUME_ROLE_ARN% --role-session-name o3de-Automation-session') DO (
    SET AWS_SECRET_ACCESS_KEY=%%a
    SET AWS_SESSION_TOKEN=%%b
    SET AWS_ACCESS_KEY_ID=%%c
)
FOR /F "tokens=4 delims=:" %%a IN ("%ASSUME_ROLE_ARN%") DO SET O3DE_AWS_DEPLOY_ACCOUNT=%%a

REM Deploy the CDK applications
ECHO [cdk_bootstrap] Bootstrap CDK
CALL cdk bootstrap aws://%O3DE_AWS_DEPLOY_ACCOUNT%/%O3DE_AWS_DEPLOY_REGION%
IF ERRORLEVEL 1 (
    ECHO [cdk_bootstrap] Failed to bootstrap CDK
    exit /b 1
)

ECHO [cdk_deployment] Deploy the CDK application for the AWS Core gem
PUSHD %GEM_DIRECTORY%\AWSCore\cdk
CALL git checkout %COMMIT_ID% -- .
CALL python -m pip install -r requirements.txt
CALL cdk deploy --all --require-approval never
IF ERRORLEVEL 1 (
    ECHO [cdk_deployment] Failed to deploy the CDK application for the AWS Core gem
    POPD
    exit /b 1
)
POPD

ECHO [cdk_deployment] Deploy the CDK application for the AWS Client Auth gem
PUSHD %GEM_DIRECTORY%\AWSClientAuth\cdk
CALL git checkout %COMMIT_ID% -- .
CALL python -m pip install -r requirements.txt
CALL cdk deploy --require-approval never
IF ERRORLEVEL 1 (
    ECHO [cdk_deployment] Failed to deploy the CDK application for the AWS Client Auth gem
    POPD
    exit /b 1
)
POPD

ECHO [cdk_deployment] Deploy the CDK application for the AWS Metrics gem
PUSHD %GEM_DIRECTORY%\AWSMetrics\cdk
CALL git checkout %COMMIT_ID% -- .
CALL python -m pip install -r requirements.txt
CALL cdk deploy -c batch_processing=true --require-approval never
IF ERRORLEVEL 1 (
    ECHO [cdk_deployment] Failed to deploy the CDK application for the AWS Metrics gem
    POPD
    exit /b 1
)
POPD

CALL deactivate
EXIT /b 0
