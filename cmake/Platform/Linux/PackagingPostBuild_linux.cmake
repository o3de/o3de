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
    add_custom_command(OUTPUT ${CPACK_TOPLEVEL_DIRECTORY}/O3DE_${O3DE_INSTALL_VERSION_STRING}_amd64.snap.assert
        COMMAND snapcraft sign-build ${CPACK_TOPLEVEL_DIRECTORY}/O3DE_${O3DE_INSTALL_VERSION_STRING}_amd64.snap > ${CPACK_TOPLEVEL_DIRECTORY}/O3DE_${O3DE_INSTALL_VERSION_STRING}_amd64.snap.assert
        VERBATIM
    )
elseif("$ENV{O3DE_PACKAGE_TYPE}" STREQUAL "DEB")
    file(${CPACK_PACKAGE_CHECKSUM} ${CPACK_TOPLEVEL_DIRECTORY}/${CPACK_PACKAGE_FILE_NAME}.deb file_checksum)
    file(WRITE ${CPACK_TOPLEVEL_DIRECTORY}/${CPACK_PACKAGE_FILE_NAME}.deb.sha256 "${file_checksum}  ${CPACK_PACKAGE_FILE_NAME}.deb")
endif()

if(CPACK_UPLOAD_URL)

    # use the internal default path if somehow not specified from cpack_configure_downloads
    if(NOT CPACK_UPLOAD_DIRECTORY)
        set(CPACK_UPLOAD_DIRECTORY ${CPACK_PACKAGE_DIRECTORY}/CPackUploads)
    endif()

    if("$ENV{O3DE_PACKAGE_TYPE}" STREQUAL "DEB")
        # Sign and regenerate checksum
        ly_sign_binaries("${CPACK_TOPLEVEL_DIRECTORY}/*.deb" "")
        file(${CPACK_PACKAGE_CHECKSUM} ${CPACK_TOPLEVEL_DIRECTORY}/${CPACK_PACKAGE_FILE_NAME}.deb file_checksum)
        file(WRITE ${CPACK_TOPLEVEL_DIRECTORY}/${CPACK_PACKAGE_FILE_NAME}.deb.sha256 "${file_checksum}  ${CPACK_PACKAGE_FILE_NAME}.deb")
    endif()

    # Copy the artifacts intended to be uploaded to a remote server into the folder specified
    # through CPACK_UPLOAD_DIRECTORY. This mimics the same process cpack does natively for
    # some other frameworks that have built-in online installer support.
    message(STATUS "Copying packaging artifacts to upload directory...")
    file(REMOVE_RECURSE ${CPACK_UPLOAD_DIRECTORY})
    file(GLOB _artifacts 
        "${CPACK_TOPLEVEL_DIRECTORY}/*.deb" 
        "${CPACK_TOPLEVEL_DIRECTORY}/*.sha256"
        "${CPACK_TOPLEVEL_DIRECTORY}/*.snap"
        "${CPACK_TOPLEVEL_DIRECTORY}/*.assert"
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
        ".*(.deb|.gpg|.sha256|.snap|.assert|.txt|.json)$"
    )

    # for auto tagged builds, we will also upload a second copy of just the boostrapper
    # to a special "Latest" folder under the branch in place of the commit date/hash
    if(CPACK_AUTO_GEN_TAG)
        if("$ENV{O3DE_PACKAGE_TYPE}" STREQUAL "SNAP")
            set(latest_assertion_file "${CPACK_UPLOAD_DIRECTORY}/O3DE_${O3DE_INSTALL_VERSION_STRING}_amd64_latest.snap.assert")
            file(COPY_FILE
                O3DE_${O3DE_INSTALL_VERSION_STRING}_amd64.snap.assert
                ${latest_assertion_file}
            )
            ly_upload_to_latest(${CPACK_UPLOAD_URL} "${latest_assertion_file}")
        else()
            set(latest_deb_package "${CPACK_UPLOAD_DIRECTORY}/${CPACK_PACKAGE_NAME}_latest.deb")
            file(COPY_FILE
                ${CPACK_UPLOAD_DIRECTORY}/${CPACK_PACKAGE_FILE_NAME}.deb
                ${latest_deb_package}
            )
            ly_upload_to_latest(${CPACK_UPLOAD_URL} ${latest_deb_package})

            # Generate a checksum file for latest and upload it
            set(latest_hash_file "${CPACK_UPLOAD_DIRECTORY}/${CPACK_PACKAGE_NAME}_latest.deb.sha256")
            file(WRITE "${latest_hash_file}" "${file_checksum}  ${CPACK_PACKAGE_NAME}_latest.deb")
            ly_upload_to_latest(${CPACK_UPLOAD_URL} "${latest_hash_file}")
        endif()
    endif()
endif()
