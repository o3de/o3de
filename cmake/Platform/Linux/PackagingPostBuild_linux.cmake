#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

file(REAL_PATH "${CPACK_SOURCE_DIR}/.." LY_ROOT_FOLDER)
include(${LY_ROOT_FOLDER}/cmake/Platform/Common/PackagingPostBuild_common.cmake)
include(${CPACK_CODESIGN_SCRIPT})

if("$ENV{O3DE_PACKAGE_TYPE}" STREQUAL "SNAP")
    set(snap_file "${CPACK_TOPLEVEL_DIRECTORY}/${CPACK_PACKAGE_FILE_NAME}_amd64.snap")
    set(hash_file "${CPACK_TOPLEVEL_DIRECTORY}/${CPACK_PACKAGE_FILE_NAME}_amd64.snap.sha384")
    file(SHA384 ${snap_file} file_checksum) # Snap asserts use SHA384
    file(WRITE ${hash_file} "${file_checksum}  ${CPACK_PACKAGE_FILE_NAME}_amd64.snap")
elseif("$ENV{O3DE_PACKAGE_TYPE}" STREQUAL "DEB")
    set(deb_file "${CPACK_TOPLEVEL_DIRECTORY}/${CPACK_PACKAGE_FILE_NAME}.deb")
    set(hash_file "${CPACK_TOPLEVEL_DIRECTORY}/${CPACK_PACKAGE_FILE_NAME}.deb.sha256")
    file(${CPACK_PACKAGE_CHECKSUM} ${deb_file} file_checksum)
    file(WRITE ${hash_file} "${file_checksum}  ${CPACK_PACKAGE_FILE_NAME}.deb")
endif()

if(CPACK_UPLOAD_URL)

    # use the internal default path if somehow not specified from cpack_configure_downloads
    if(NOT CPACK_UPLOAD_DIRECTORY)
        set(CPACK_UPLOAD_DIRECTORY ${CPACK_PACKAGE_DIRECTORY}/CPackUploads)
    endif()

    if("$ENV{O3DE_PACKAGE_TYPE}" STREQUAL "DEB")
        # Sign and regenerate checksum
        ly_sign_binaries("${deb_file}" "")
        file(${CPACK_PACKAGE_CHECKSUM} ${deb_file} file_checksum)
        file(WRITE ${hash_file} "${file_checksum}  ${deb_file}")
    endif()

    # Copy the artifacts intended to be uploaded to a remote server into the folder specified
    # through CPACK_UPLOAD_DIRECTORY. This mimics the same process cpack does natively for
    # some other frameworks that have built-in online installer support.
    message(STATUS "Copying packaging artifacts to upload directory...")
    file(REMOVE_RECURSE ${CPACK_UPLOAD_DIRECTORY})
    file(GLOB _artifacts 
        "${CPACK_TOPLEVEL_DIRECTORY}/*.deb" 
        "${CPACK_TOPLEVEL_DIRECTORY}/*.sha256"
        "${CPACK_TOPLEVEL_DIRECTORY}/*.sha384"
        "${CPACK_TOPLEVEL_DIRECTORY}/*.snap"
        "${LY_ROOT_FOLDER}/scripts/signer/Platform/Linux/*.gpg"
        "${CPACK_3P_LICENSE_FILE}"
        "${CPACK_3P_MANIFEST_FILE}"
    )
    file(COPY ${_artifacts}
        DESTINATION ${CPACK_UPLOAD_DIRECTORY}
    )
    message(STATUS "Artifacts copied to ${CPACK_UPLOAD_DIRECTORY}")

    ly_upload_to_url(
        ${CPACK_UPLOAD_URL}
        ${CPACK_UPLOAD_DIRECTORY}
        ".*(.deb|.gpg|.sha256|.sha384|.snap|.txt|.json)$"
    )

    # for auto tagged builds, we will also upload a second copy of the package
    # to a special "Latest" folder under the branch in place of the commit date/hash
    if(CPACK_AUTO_GEN_TAG)
        if("$ENV{O3DE_PACKAGE_TYPE}" STREQUAL "SNAP")
            set(latest_snap_file "${CPACK_UPLOAD_DIRECTORY}/${CPACK_PACKAGE_NAME}_amd64_latest.snap")
            file(COPY_FILE
                ${snap_file}
                ${latest_snap_file}
            )
            ly_upload_to_latest(${CPACK_UPLOAD_URL} "${latest_snap_package}")

            # Generate a checksum file for latest and upload it
            set(latest_hash_file "${CPACK_UPLOAD_DIRECTORY}/${CPACK_PACKAGE_NAME}_latest.snap.sha384")
            file(WRITE "${latest_hash_file}" "${file_checksum}  ${latest_snap_package}")
            ly_upload_to_latest(${CPACK_UPLOAD_URL} "${latest_hash_file}")
        elseif("$ENV{O3DE_PACKAGE_TYPE}" STREQUAL "DEB")
            set(latest_deb_package "${CPACK_UPLOAD_DIRECTORY}/${CPACK_PACKAGE_NAME}_latest.deb")
            file(COPY_FILE
                ${CPACK_UPLOAD_DIRECTORY}/${CPACK_PACKAGE_FILE_NAME}.deb
                ${latest_deb_package}
            )
            ly_upload_to_latest(${CPACK_UPLOAD_URL} "${latest_deb_package}")

            # Generate a checksum file for latest and upload it
            set(latest_hash_file "${CPACK_UPLOAD_DIRECTORY}/${CPACK_PACKAGE_NAME}_latest.deb.sha256")
            file(WRITE "${latest_hash_file}" "${file_checksum}  ${latest_deb_package}")
            ly_upload_to_latest(${CPACK_UPLOAD_URL} "${latest_hash_file}")
        endif()
    endif()
endif()
