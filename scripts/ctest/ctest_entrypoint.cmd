@ECHO OFF
REM
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM
REM Continuous Integration CLI entrypoint script to start CTest, triggering post-build tests
REM
SETLOCAL

SET DEV_DIR=%~dp0\..\..
SET PYTHON=%DEV_DIR%\python\python.cmd
SET CTEST_SCRIPT=%~dp0\ctest_driver.py

REM pass all args to python-based CTest script
call %PYTHON% %CTEST_SCRIPT% %*
exit /b %ERRORLEVEL%
