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

    # use the internal default path if somehow not specified from cpack_configure_downloads
    if(NOT CPACK_UPLOAD_DIRECTORY)
        set(CPACK_UPLOAD_DIRECTORY ${CPACK_PACKAGE_DIRECTORY}/CPackUploads)
    endif()

    ly_upload_to_url(
        ${CPACK_UPLOAD_URL}
        ${CPACK_UPLOAD_DIRECTORY}
        ".*(deb|asc|.sha256)$"
    )

    # for auto tagged builds, we will also upload a second copy of just the boostrapper
    # to a special "Latest" folder under the branch in place of the commit date/hash
    if(CPACK_AUTO_GEN_TAG)

        set(latest_deb_package "${CPACK_UPLOAD_DIRECTORY}/CPackUploads/${CPACK_PACKAGE_NAME}_latest.deb")
        file(COPY_FILE ${CPACK_UPLOAD_DIRECTORY}/CPackUploads/${CPACK_PACKAGE_FILE_NAME}.deb
            DESTINATION ${latest_deb_package}
        )
        ly_upload_to_latest(${CPACK_UPLOAD_URL} ${latest_deb_package})
        
        # replace the name fo the binary inside the generated checksum
        file(READ "${CPACK_UPLOAD_DIRECTORY}/CPackUploads/${CPACK_PACKAGE_FILE_NAME}.deb.sha256" _checksum_contents)
        string(REPLACE "_${CPACK_PACKAGE_VERSION}" "" non_versioned_checksum_contents ${_checksum_contents})
        set(latest_hash_file "${CPACK_UPLOAD_DIRECTORY}/CPackUploads/${CPACK_PACKAGE_NAME}_latest.deb.sha256")
        file(WRITE "${latest_hash_file}" "${non_versioned_checksum_contents}")
        ly_upload_to_latest(${CPACK_UPLOAD_URL} "${latest_hash_file}")
    endif()
endif()
