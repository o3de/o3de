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

# make sure that all executables have the correct permissions
execute_process (COMMAND find ./O3DE/${CPACK_PACKAGE_VERSION}/bin/Linux -type f -executable -exec chmod \+x {} +
                 WORKING_DIRECTORY ${CPACK_TEMPORARY_DIRECTORY}
)

# setup the correct elf interpreter
execute_process (COMMAND find ./O3DE/${CPACK_PACKAGE_VERSION}/bin/Linux -type f -executable -exec patchelf --set-interpreter /snap/core22/current/lib64/ld-linux-x86-64.so.2 {} +
                 WORKING_DIRECTORY ${CPACK_TEMPORARY_DIRECTORY}
)

# build snap
execute_process (COMMAND snapcraft --verbose
                 WORKING_DIRECTORY ${CPACK_TEMPORARY_DIRECTORY}
)

set(snap_file "${CPACK_TEMPORARY_DIRECTORY}/o3de_${CPACK_PACKAGE_VERSION}_amd64.snap")
set(assertion_file "${CPACK_TEMPORARY_DIRECTORY}/o3de_${CPACK_PACKAGE_VERSION}_amd64.snap.assert")
execute_process(
    COMMAND snapcraft sign-build ${snap_file}
    OUTPUT_FILE ${assertion_file}
)

# Manually copy the files, the CPACK_EXTERNAL_BUILT_PACKAGES process runs after our packaging post build script
# which is too late to be uploaded.
file(COPY_FILE
                ${snap_file}
                "${CPACK_TOPLEVEL_DIRECTORY}/o3de_${CPACK_PACKAGE_VERSION}_amd64.snap"
            )
file(COPY_FILE
                ${assertion_file}
                "${CPACK_TOPLEVEL_DIRECTORY}/o3de_${CPACK_PACKAGE_VERSION}_amd64.snap.assert"
            )
