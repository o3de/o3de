#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

file(REAL_PATH "${CPACK_SOURCE_DIR}/.." LY_ROOT_FOLDER)
include(${LY_ROOT_FOLDER}/cmake/Platform/Common/PackagingPostBuild_common.cmake)

if(CPACK_UPLOAD_URL)

    # use the internal default path if somehow not specified from cpack_configure_downloads
    if(NOT CPACK_UPLOAD_DIRECTORY)
        set(CPACK_UPLOAD_DIRECTORY ${CPACK_PACKAGE_DIRECTORY}/CPackUploads)
    endif()

    # Copy the artifacts intended to be uploaded to a remote server into the folder specified
    # through CPACK_UPLOAD_DIRECTORY. This mimics the same process cpack does natively for
    # some other frameworks that have built-in online installer support.
    message(STATUS "Copying packaging artifacts to upload directory...")
    file(REMOVE_RECURSE ${CPACK_UPLOAD_DIRECTORY})
    file(GLOB _artifacts 
        "${CPACK_TOPLEVEL_DIRECTORY}/*.deb" 
        "${CPACK_TOPLEVEL_DIRECTORY}/*.sha256"
    )
    file(COPY ${_artifacts}
        DESTINATION ${CPACK_UPLOAD_DIRECTORY}
    )
    message(STATUS "Artifacts copied to ${CPACK_UPLOAD_DIRECTORY}")

    # TODO: copy gpg file to CPACK_UPLOAD_DIRECTORY

    ly_upload_to_url(
        ${CPACK_UPLOAD_URL}
        ${CPACK_UPLOAD_DIRECTORY}
        ".*(.deb|.gpg|.sha256)$"
    )

    # for auto tagged builds, we will also upload a second copy of just the boostrapper
    # to a special "Latest" folder under the branch in place of the commit date/hash
    if(CPACK_AUTO_GEN_TAG)

        set(latest_deb_package "${CPACK_UPLOAD_DIRECTORY}/${CPACK_PACKAGE_NAME}_latest.deb")
        file(COPY_FILE 
            ${CPACK_UPLOAD_DIRECTORY}/${CPACK_PACKAGE_FILE_NAME}.deb
            ${latest_deb_package}
        )
        ly_upload_to_latest(${CPACK_UPLOAD_URL} ${latest_deb_package})

        # TODO: upload gpg file to latest
        
        # replace the name fo the binary inside the generated checksum
        file(READ "${CPACK_UPLOAD_DIRECTORY}/${CPACK_PACKAGE_FILE_NAME}.deb.sha256" _checksum_contents)
        string(REPLACE "_${CPACK_PACKAGE_VERSION}" "_latest" non_versioned_checksum_contents "${_checksum_contents}")
        set(latest_hash_file "${CPACK_UPLOAD_DIRECTORY}/${CPACK_PACKAGE_NAME}_latest.deb.sha256")
        file(WRITE "${latest_hash_file}" "${non_versioned_checksum_contents}")
        ly_upload_to_latest(${CPACK_UPLOAD_URL} "${latest_hash_file}")
    endif()
endif()
