#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

file(REAL_PATH "${CPACK_SOURCE_DIR}/.." LY_ROOT_FOLDER)
include(${LY_ROOT_FOLDER}/cmake/Platform/Common/PackagingPostBuild_common.cmake)

# Generate checksum file
message(STATUS "Generating checksum file...")
execute_process(
    COMMAND sha256sum "${CPACK_PACKAGE_FILE_NAME}.deb"
    WORKING_DIRECTORY "${CPACK_TOPLEVEL_DIRECTORY}"
    RESULT_VARIABLE _checksum_result
    ERROR_VARIABLE _checksum_errors
    OUTPUT_VARIABLE _checksum_output
    ECHO_OUTPUT_VARIABLE
)
if(NOT ${_checksum_result} EQUAL 0)
    message(WARNING "An error occurred during checksum creation, checksum wont be uploaded. ${_checksum_errors}")
else()
    file(WRITE "${CPACK_TOPLEVEL_DIRECTORY}/checksum.sha256" "${_checksum_output}")
endif()

# TODO: copy public key to ${CPACK_TOPLEVEL_DIRECTORY}

if(CPACK_UPLOAD_URL)

    ly_upload_to_url(
        ${CPACK_UPLOAD_URL}
        ${CPACK_TOPLEVEL_DIRECTORY}
        ".*(deb|asc|.sha256)$"
    )

    # for auto tagged builds, we will also upload a second copy of just the boostrapper
    # to a special "Latest" folder under the branch in place of the commit date/hash
    if(CPACK_AUTO_GEN_TAG)
        ly_upload_to_latest(${CPACK_UPLOAD_URL} ${_bootstrap_output_file} "latest")
        if(NOT ${_checksum_result} EQUAL 0)
            # replace the name fo the binary inside
            string(REPLACE "${CPACK_PACKAGE_VERSION}" "latest" non_versioned_checksum_output ${_checksum_output})
            file(WRITE "${CPACK_BINARY_DIR}/checksum.sha256" "${_checksum_output}")
            ly_upload_to_latest(${CPACK_UPLOAD_URL} "${CPACK_BINARY_DIR}/checksum.sha256" "latest")
        endif()
    endif()
endif()
