@ECHO OFF
REM
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

SETLOCAL EnableDelayedExpansion

ECHO [ci_build] aws s3 cp --recursive %CSV_DIRECTORY% %O3DE_METRICS_S3_BUCKET% --acl bucket-owner-full-control
aws s3 cp --recursive %CSV_DIRECTORY% %O3DE_METRICS_S3_BUCKET% --acl bucket-owner-full-control

EXIT /b %ERRORLEVEL%