@echo off
REM --------------------------------------------------------------------------------------------------
REM 
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM 
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM 
REM --------------------------------------------------------------------------------------------------

REM This packaging script simplifies the project export command by defaulting the required and important
REM arguments to the values based on:
REM     --project-path %O3DE_PROJECT_PATH%      <- The path to this project for exporting
REM     --tools-build-path %TOOLS_BUILD_PATH%   <- The location of the tools/editor build to make sure all the tools necessary 
REM                                                for packaging are built and available (if applicable)
REM     -out %OUTPUT_PATH%                      <- The output location for the exported project. This can be set by passing in
REM                                                a path to the arguments to this script. If not provided, it will default
REM                                                to %O3DE_PROJECT_PATH%\ProjectPackages
REM
REM Feel free to adjust any of the arguments as necessary. For more information about the project export command, type in the 
REM following command from the engine root
REM
REM %O3DE_PATH%\scripts\o3de.bat export-project -es ExportScripts\export_source_built_project.py --script-help
REM
REM To see the default options the export-project command will use other than the above required override arguments, you can view/edit
REM the parameters with the export-project-configure command
REM
REM %O3DE_PATH%\scripts\o3de.bat export-project-configure --help
REM
REM To view the settings for the current project:
REM
REM %O3DE_PATH%\scripts\o3de.bat export-project-configure -p %O3DE_PROJECT_PATH% --list
REM
REM Note: The location of the engine (O3DE_PATH) is hardcoded to the location of the engine that was used to generate 
REM       this project. The engine path must reflect the path to the engine on the local machine.

set O3DE_PATH=${EnginePath}
set O3DE_PROJECT_PATH=%~dp0

IF "%1" == "-h" (
    echo Usage: %0 EXPORT_PATH
    echo where:
    echo    EXPORT_PATH     The optional path to export the project package to. 
    echo                    Default: %O3DE_PROJECT_PATH%ProjectPackages
    echo
    exit /B 0
) ELSE IF "%1" == "" (
    set OUTPUT_PATH=%O3DE_PROJECT_PATH%build\export
) ELSE (
    set OUTPUT_PATH=%1
)

set TOOLS_BUILD_PATH=%O3DE_PROJECT_PATH%build\windows

echo Using project path at %O3DE_PROJECT_PATH%
IF NOT EXIST %O3DE_PATH%\scripts\o3de.bat (
    echo Engine path %O3DE_PATH% is invalid in this script. Make sure to install the engine to %O3DE_PATH% or update this script's 'O3DE_PATH' to point to the installed engine path on this system.
    exit /B 1
)

echo Using engine path at %O3DE_PATH%
echo Exporting project to %OUTPUT_PATH%

call %O3DE_PATH%\scripts\o3de.bat export-project -es ExportScripts\export_source_built_project.py --project-path %O3DE_PROJECT_PATH% --log-level INFO --tools-build-path %TOOLS_BUILD_PATH% -out %OUTPUT_PATH%
