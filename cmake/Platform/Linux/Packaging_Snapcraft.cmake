#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Copy snapcraft file
configure_file("${LY_ROOT_FOLDER}/cmake/Platform/Linux/Packaging/snapcraft.yaml.in"
    "${CPACK_TEMPORARY_DIRECTORY}/snapcraft.yaml"
)

execute_process (COMMAND lsb_release -a)

execute_process (COMMAND bash ${CPACK_TEMPORARY_DIRECTORY}/O3DE/${CPACK_PACKAGE_VERSION}/python/get_python.sh)

# make sure that all files have the correct permissions
execute_process (COMMAND chmod -R 755 O3DE
                 WORKING_DIRECTORY ${CPACK_TEMPORARY_DIRECTORY}
)

# clean then build snap
execute_process (COMMAND snapcraft clean
                 WORKING_DIRECTORY ${CPACK_TEMPORARY_DIRECTORY}
)

execute_process (COMMAND snapcraft --verbose
                 WORKING_DIRECTORY ${CPACK_TEMPORARY_DIRECTORY}
)

set(snap_file "${CPACK_TEMPORARY_DIRECTORY}/o3de_${CPACK_PACKAGE_VERSION}_amd64.snap")

# Manually copy the files, the CPACK_EXTERNAL_BUILT_PACKAGES process runs after our packaging post build script
# which is too late to be uploaded.
file(COPY_FILE
                ${snap_file}
                "${CPACK_TOPLEVEL_DIRECTORY}/o3de_${CPACK_PACKAGE_VERSION}_amd64.snap"
            )
