#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

O3DE_PATH=${EnginePath}
O3DE_PROJECT_PATH=${ProjectPath}
O3DE_PROJECT_SEEDLIST=${O3DE_PROJECT_PATH}/AssetBundling/SeedLists/Example.seed
OUTPUT_PATH=${O3DE_PROJECT_PATH}/ProjectPackages

${O3DE_PATH}/scripts/o3de.sh export-project -es ExportScripts/export_source_built_project.py --project-path ${O3DE_PROJECT_PATH} --log-level INFO -assets --build-tools --config release --archive-output zip --seedlist ${O3DE_PROJECT_SEEDLIST} -out ${OUTPUT_PATH}