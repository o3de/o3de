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

# Set common variables by package type
set(file_name "${CPACK_PACKAGE_FILE_NAME}")
set(checksum ${CPACK_PACKAGE_CHECKSUM})
set(pack_file "${CPACK_TOPLEVEL_DIRECTORY}/${file_name}.${ext}")
if("$ENV{O3DE_PACKAGE_TYPE}" STREQUAL "SNAP")
    if(CPACK_SNAP_DISTRO)
        set(file_name "${CPACK_PACKAGE_FILE_NAME}_${CPACK_SNAP_DISTRO}")
    endif()
    set(ext "snap")
    set(checksum SHA384) # Snap asserts use SHA384
elseif("$ENV{O3DE_PACKAGE_TYPE}" STREQUAL "DEB")
    set(ext "deb")
else()
     message(FATAL_ERROR "No valid packaging type defined in O3DE_PACKAGE_TYPE. Stopping build")
endif()

# Generate checksum file
set(hash_file "${pack_file}.${checksum}")
file(${checksum} ${pack_file} file_checksum) 
file(WRITE ${hash_file} "${file_checksum} ${file_name}.${ext}")

if(CPACK_UPLOAD_URL)
    # use the internal default path if somehow not specified from cpack_configure_downloads
    if(NOT CPACK_UPLOAD_DIRECTORY)
        set(CPACK_UPLOAD_DIRECTORY ${CPACK_PACKAGE_DIRECTORY}/CPackUploads)
    endif()

    if("$ENV{O3DE_PACKAGE_TYPE}" STREQUAL "DEB")
        # Sign and regenerate checksum
        ly_sign_binaries("${pack_file}" "")
        file(${checksum} ${pack_file} file_checksum)
        file(WRITE ${hash_file} "${file_checksum}  ${pack_file}")
    endif()

    # Copy the artifacts intended to be uploaded to a remote server into the folder specified
    # through CPACK_UPLOAD_DIRECTORY. This mimics the same process cpack does natively for
    # some other frameworks that have built-in online installer support.
    message(STATUS "Copying packaging artifacts to upload directory...")
    file(REMOVE_RECURSE ${CPACK_UPLOAD_DIRECTORY})
    file(GLOB _artifacts 
        "${CPACK_TOPLEVEL_DIRECTORY}/*.${ext}" 
        "${CPACK_TOPLEVEL_DIRECTORY}/*.${checksum}"
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
        ".*(.${ext}|.${checksum}|.gpg|.txt|.json)$"
    )

    # for auto tagged builds, we will also upload a second copy of the package
    # to a special "Latest" folder under the branch in place of the commit date/hash
    if(CPACK_AUTO_GEN_TAG)       
        set(latest_pack_file "${CPACK_UPLOAD_DIRECTORY}/${file_name}_latest.${ext}")
        file(COPY_FILE
            ${pack_file}
            ${latest_pack_file}
        )
        ly_upload_to_latest(${CPACK_UPLOAD_URL} "${latest_pack_file}")

        # Generate a checksum file for latest and upload it
        set(latest_hash_file "${latest_pack_file}.${checksum}")
        file(WRITE "${latest_hash_file}" "${file_checksum}  ${latest_pack_file}")
        ly_upload_to_latest(${CPACK_UPLOAD_URL} "${latest_hash_file}")
    endif()
endif()
