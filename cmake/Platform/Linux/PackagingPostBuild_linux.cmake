#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

file(REAL_PATH "${CPACK_SOURCE_DIR}/.." LY_ROOT_FOLDER)
include(${LY_ROOT_FOLDER}/cmake/Platform/Common/PackagingPostBuild_common.cmake)

# TODO: copy public key to ${CPACK_TOPLEVEL_DIRECTORY}

if(CPACK_UPLOAD_URL)

    ly_upload_to_url(
        ${CPACK_UPLOAD_URL}
        ${CPACK_TOPLEVEL_DIRECTORY}
        ".*(deb|asc|.md5)$"
    )

    # for auto tagged builds, we will also upload a second copy of just the boostrapper
    # to a special "Latest" folder under the branch in place of the commit date/hash
    if(CPACK_AUTO_GEN_TAG)
        ly_upload_to_latest(${CPACK_UPLOAD_URL} ${_bootstrap_output_file})
        
        # replace the name fo the binary inside the generated checksum
        file(READ "${CPACK_TOPLEVEL_DIRECTORY}/${CPACK_PACKAGE_FILE_NAME}.md5" _checksum_contents)
        string(REPLACE "_${CPACK_PACKAGE_VERSION}" "" non_versioned_checksum_contents ${_checksum_contents})
        file(WRITE "${CPACK_BINARY_DIR}/${CPACK_PACKAGE_NAME}.md5" "${non_versioned_checksum_contents}")
        ly_upload_to_latest(${CPACK_UPLOAD_URL} "${CPACK_BINARY_DIR}/${CPACK_PACKAGE_NAME}.md5")
    endif()
endif()
