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

set(CPACK_EXTERNAL_BUILT_PACKAGES "${snap_file}" "${assertion_file}")
