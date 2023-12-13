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
REM     --project-path %O3DE_PROJECT_PATH% 	    <- The path to this project for exporting
REM     --config release                        <- Specify a release build for the exported project
REM     --archive-output zip                    <- The output format of the compressed archive for the exported project
REM     --seedlist %O3DE_PROJECT_SEEDLIST%      <- The seedlist to use to create the pak files
REM     --tools-build-path %TOOLS_BUILD_PATH%   <- The location of the tools/editor build to make sure all the tools necessary 
REM                                                for packaging are built and available (if applicable)
REM     -out %OUTPUT_PATH%                      <- The output location for the exported project
REM
REM Feel free to adjust any of the arguments as necessary. For more information about the project export command, type in the 
REM following command from the engine root
REM
REM scripts\o3de.bat export-project -es ExportScripts\export_source_built_project.py --script-help
REM

REM Note: The location of the engine (O3DE_PATH) is hardcoded to the location of the engine that was used to generate 
REM       this project. The engine path must reflect the path to the engine on the local machine.

set O3DE_PATH=${EnginePath}
set O3DE_PROJECT_PATH=%~dp0
set O3DE_PROJECT_SEEDLIST=%O3DE_PROJECT_PATH%\AssetBundling\SeedLists\*.seed
set OUTPUT_PATH=%O3DE_PROJECT_PATH%\ProjectPackages
set TOOLS_BUILD_PATH=%O3DE_PROJECT_PATH%\build\windows

%O3DE_PATH%\scripts\o3de.bat export-project -es ExportScripts\export_source_built_project.py --project-path %O3DE_PROJECT_PATH% --log-level INFO -assets --build-tools --config release --archive-output zip --seedlist %O3DE_PROJECT_SEEDLIST% --tools-build-path %TOOLS_BUILD_PATH% -out %OUTPUT_PATH%
