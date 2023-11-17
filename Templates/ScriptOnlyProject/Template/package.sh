#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# This script is meant to export the project into a standalone shippable project that others can run.
# However, the project developer is expected to modify it to add steps or change it to their needs.

# To get more information about the possible tweakable parameters, run 
# (engine folder)/scripts/o3de.sh export-project -es ExportScripts/export_source_built_project.py --script-help

O3DE_PATH=${EnginePath}
O3DE_PROJECT_PATH=${ProjectPath}
O3DE_PROJECT_SEEDLIST=${O3DE_PROJECT_PATH}/AssetBundling/SeedLists/Example.seed
OUTPUT_PATH=${O3DE_PROJECT_PATH}/ProjectPackages

# change this to release or debug if you want it to make a release or debug package
# (Only works if the installer you have actually includes release and debug binaries)
OUTPUT_CONFIGURATION=profile

${O3DE_PATH}/scripts/o3de.sh export-project -es ExportScripts/export_source_built_project.py --project-path ${O3DE_PROJECT_PATH} --no-monolithic-build  --log-level INFO -assets --config ${OUTPUT_CONFIGURATION} --archive-output gztar --seedlist ${O3DE_PROJECT_SEEDLIST} -out ${OUTPUT_PATH}
