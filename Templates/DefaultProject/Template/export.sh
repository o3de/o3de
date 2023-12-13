#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# This packaging script simplifies the project export command by defaulting the required and important
# arguments to the values based on:
#     --project-path %O3DE_PROJECT_PATH%      <- The path to this project for exporting
#     --config release                        <- Specify a release build for the exported project
#     --archive-output zip                    <- The output format of the compressed archive for the exported project
#     --seedlist %O3DE_PROJECT_SEEDLIST%      <- The seedlist to use to create the pak files
#     --tools-build-path %TOOLS_BUILD_PATH%   <- The location of the tools/editor build to make sure all the tools necessary 
#                                                for packaging are built and available (if applicable)
#     -out %OUTPUT_PATH%                      <- The output location for the exported project
#
# Feel free to adjust any of the arguments as necessary. For more information about the project export command, type in the 
# following command from the engine root
#
# scripts\o3de.sh export-project -es ExportScripts\export_source_built_project.py --script-help
#


# Resolve the current folder in order to resolve the project path
SOURCE="${BASH_SOURCE[0]}"
# While $SOURCE is a symlink, resolve it
while [[ -h "$SOURCE" ]]; do
    DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
    SOURCE="$( readlink "$SOURCE" )"
    # If $SOURCE was a relative symlink (so no "/" as prefix, need to resolve it relative to the symlink base directory
    [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
done
DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"

O3DE_PROJECT_PATH=$DIR
echo Using project path at $O3DE_PROJECT_PATH

# Query the EngineFinder.cmake to locate the path to the engine
ENGINE_PATH_QUERY=$(cmake -P $DIR/cmake/EngineFinder.cmake)
if [ $? -ne 0 ]; then
    echo "Unable to determine path to the engine."
    exit 1
fi

O3DE_PATH=$(echo ${ENGINE_PATH_QUERY:20} | sed "s/'//g")
echo Using engine path at $O3DE_PATH

O3DE_PROJECT_SEEDLIST=${O3DE_PROJECT_PATH}/AssetBundling/SeedLists/*.seed
OUTPUT_PATH=${O3DE_PROJECT_PATH}/ProjectPackages

if [[ "$OSTYPE" == *"darwin"* ]]; then
	TOOLS_BUILD_PATH=${O3DE_PROJECT_PATH}/build/mac
elif [[ "$OSTYPE" == "msys" ]]; then
	TOOLS_BUILD_PATH=${O3DE_PROJECT_PATH}/build/windows
else
	TOOLS_BUILD_PATH=${O3DE_PROJECT_PATH}/build/linux
fi

echo ${O3DE_PATH}/scripts/o3de.sh export-project -es ExportScripts/export_source_built_project.py --project-path ${O3DE_PROJECT_PATH} --log-level INFO -assets --build-tools --config release --archive-output zip --seedlist ${O3DE_PROJECT_SEEDLIST} -out ${OUTPUT_PATH}