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

# The output of this script will be placed in the "ProjectPackages" folder and will include
# * A standalone version of the project game client, shippable, with all the assets and binaries
# * A standalone version of the project server (if the project has one), shippable, with all the assets and binaries
# * A standalone version of the project's "Unified" (Client + 'listen' Server), with assets and binaries
# * zip files of each of the above (based on the --archive-output zip) option below - xz, bz2, gzip and 'none' 
#   are also available.

O3DE_PATH=${EnginePath}
O3DE_PROJECT_PATH=${ProjectPath}

# The seedlist is a list of assets that will be included in the package (it automatically computes dependencies
# based on these assets and will include dependencies recusively on anything in the seed list.  Modify it
# to include your own 'root' assets that you want to include in the package using the AssetBundler tool
O3DE_PROJECT_SEEDLIST=${O3DE_PROJECT_PATH}/AssetBundling/SeedLists/Example.seed
OUTPUT_PATH=${O3DE_PROJECT_PATH}/ProjectPackages

# change this to release, profile, or debug 
# (Only works if the installer you have actually includes release, profile or debug binaries)

# note that script-only-projects cannot support monolithic configurations and the default installer
# only includes monolithic release configuration (along with non-monolithic debug and profile).
# you can build your own installer that has non-monolithic release, and then use release, in that case,
# but otherwise, stick to either profile or debug.
OUTPUT_CONFIGURATION=profile

${O3DE_PATH}/scripts/o3de.sh export-project -es ExportScripts/export_source_built_project.py --project-path ${O3DE_PROJECT_PATH} -nomono  --log-level INFO -assets --config ${OUTPUT_CONFIGURATION} --archive-output zip --seedlist ${O3DE_PROJECT_SEEDLIST} -out ${OUTPUT_PATH}
