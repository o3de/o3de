@ECHO OFF
REM 
REM Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM 
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

SETLOCAL
PUSHD %~dp0%

rem Set base CMD_DIR to use with the script.
SET CMD_DIR=%~dp0
SET TOOLS_DIR=%CMD_DIR:~0,-18%
GOTO TESTRAIL_IMPORTER_EXISTS

rem Check if the main testrail_importer.py script exists.
:TESTRAIL_IMPORTER_EXISTS
SET TESTRAIL_IMPORTER=%TOOLS_DIR%\TestRailImporter\lib\testrail_importer\testrail_importer.py
IF EXIST %TESTRAIL_IMPORTER% GOTO UNIT_TESTS_EXIST

ECHO Could not find the TestRailImporter tool in %TESTRAIL_IMPORTER%
GOTO FAILED

rem Make sure the TestRailImporter unit tests also exist.
:UNIT_TESTS_EXIST
SET TESTRAIL_IMPORTER_UNIT_TESTS=%TOOLS_DIR%\TestRailImporter\tests\unit
IF EXIST "%TESTRAIL_IMPORTER_UNIT_TESTS%" GOTO PYTHON_PATH_SET

ECHO Could not find the TestRailImporter tool unit test directory in %TESTRAIL_IMPORTER_UNIT_TESTS%

rem Check for the Lumberyard packaged Python.
:PYTHON_PATH_SET
SET PYTHON_DIR=%TOOLS_DIR%\..\python
IF EXIST "%PYTHON_DIR%" GOTO PYTHON_DIR_EXISTS

ECHO Could not find Python in %TOOLS_DIR%
GOTO FAILED

:PYTHON_DIR_EXISTS
SET PYTHON=%PYTHON_DIR%\python.cmd
ECHO Found Lumberyard packaged Python at %PYTHON%
IF EXIST "%PYTHON%" GOTO PYTHON_EXISTS

ECHO Could not find python.cmd in %PYTHON_DIR%
GOTO FAILED

:PYTHON_EXISTS
rem Run unit tests and exit the script if the --unit-tests arg is passed.
FOR %%x IN (%*) DO (
IF %%x == --unit-tests GOTO RUN_UNIT_TESTS
)

ECHO --unit-tests arg not found, running testrail_importer.py script directly.
GOTO RUN_TESTRAIL_IMPORTER

:RUN_UNIT_TESTS
ECHO Calling TestRailImporter unit tests: %PYTHON% -m pytest -s -v %TESTRAIL_IMPORTER_UNIT_TESTS%
CALL %PYTHON% -m pytest -s -v %TESTRAIL_IMPORTER_UNIT_TESTS%
IF ERRORLEVEL 1 GOTO FAILED
GOTO SUCCESS

:RUN_TESTRAIL_IMPORTER
rem No --unit-tests arg, so just run the normal testrail_importer.py script.
ECHO Calling TestRailImporter Python script: %PYTHON% %TESTRAIL_IMPORTER% (CLI args redacted for security)
CALL %PYTHON% %TESTRAIL_IMPORTER% %*
IF ERRORLEVEL 1 GOTO FAILED
GOTO SUCCESS

:SUCCESS
POPD
EXIT /b 0

:FAILED
POPD
EXIT /b 1
