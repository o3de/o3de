REM --------------------------------------------------------------------------------------------------
REM 
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM 
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM 
REM --------------------------------------------------------------------------------------------------

set O3DE_PATH=${EnginePath}
set O3DE_PROJECT_PATH=${ProjectPath}
set O3DE_PROJECT_SEEDLIST=%O3DE_PROJECT_PATH%\AssetBundling\SeedLists\Example.seed
set OUTPUT_PATH=%O3DE_PROJECT_PATH%\ProjectPackages

REM change this to release or debug if you want it to make a release or debug package
REM (Only works if the installer you have actually includes release and debug binaries)
set OUTPUT_CONFIGURATION=profile

%O3DE_PATH%\scripts\o3de.bat export-project -es ExportScripts\export_script_only_project.py --project-path %O3DE_PROJECT_PATH% --log-level INFO -assets --config %OUTPUT_CONFIGURATION% --archive-output zip --seedlist %O3DE_PROJECT_SEEDLIST% -out %OUTPUT_PATH%

