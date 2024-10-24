#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# This packaging script simplifies the project export command by defaulting the required and important
# arguments to the values based on:
#     --project-path $O3DE_PROJECT_PATH       <- The path to this project for exporting
#     --tools-build-path $TOOLS_BUILD_PATH    <- The location of the tools/editor build to make sure all the tools necessary 
#                                                for packaging are built and available (if applicable)
#     -out $OUTPUT_PATH                       <- The output location for the exported project. This can be set by passing in
#                                                a path to the arguments to this script. If not provided, it will default
#                                                to $O3DE_PROJECT_PATH/ProjectPackages
#
# Feel free to adjust any of the arguments as necessary. For more information about the project export command, type in the 
# following command from the engine root
#
# scripts\o3de.sh export-project -es ExportScripts\export_source_built_project.py --script-help
#
# To see the default options the export-project command will use other than the above required override arguments, you can view/edit
# the parameters with the export-project-configure command
#
# $O3DE_PATH\scripts\o3de.bat export-project-configure --help
#
# To view the settings for the current project:
#
# $O3DE_PATH\scripts\o3de.bat export-project-configure -p $O3DE_PROJECT_PATH --list
#
#
# Note: The location of the engine (O3DE_PATH) is hardcoded to the location of the engine that was used to generate 
#       this project. The engine path must reflect the path to the engine on the local machine.

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

O3DE_PATH=${EnginePath}
if [ ! -f ${O3DE_PATH}/scripts/o3de.sh ]
then
    echo Engine path $O3DE_PATH is invalid in this script. Make sure to install the engine to $O3DE_PATH or update this script''s $O3DE_PATH to point to the installed engine path on this system.
    exit 1
fi
echo Using engine path at $O3DE_PATH

if [[ $1 == "-h" ]]
then
    echo
    echo "Usage: $0 EXPORT_PATH"
    echo "where:"
    echo "    EXPORT_PATH     The optional path to export the project package to. "
    echo "                    Default: ${O3DE_PROJECT_PATH}/ProjectPackages"
    echo
    exit 0
elif [[ $1 == "" ]]
then
    OUTPUT_PATH=${O3DE_PROJECT_PATH}/build/export
else
    OUTPUT_PATH=$1
fi

echo Exporting project to $OUTPUT_PATH

# The default path where the tools are built is platform dependent
if [[ "$OSTYPE" == *"darwin"* ]]; then
    TOOLS_BUILD_PATH=${O3DE_PROJECT_PATH}/build/mac
elif [[ "$OSTYPE" == "msys" ]]; then
    TOOLS_BUILD_PATH=${O3DE_PROJECT_PATH}/build/windows
else
    TOOLS_BUILD_PATH=${O3DE_PROJECT_PATH}/build/linux
fi

${O3DE_PATH}/scripts/o3de.sh export-project -es ExportScripts/export_source_built_project.py --project-path ${O3DE_PROJECT_PATH} --log-level INFO -out ${OUTPUT_PATH}