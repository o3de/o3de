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

# convert the path to a windows style path using string replace because TO_NATIVE_PATH
# only works on real paths
string(REPLACE "/" "\\" _fixed_package_install_dir ${CPACK_PACKAGE_INSTALL_DIRECTORY})

# directory where the auto generated files live e.g <build>/_CPack_Package/win64/WIX
set(_cpack_wix_out_dir ${CPACK_TOPLEVEL_DIRECTORY})
set(_bootstrap_out_dir "${CPACK_TOPLEVEL_DIRECTORY}/bootstrap")

set(_bootstrap_filename "${CPACK_PACKAGE_FILE_NAME}_installer.exe")
set(_bootstrap_output_file ${_cpack_wix_out_dir}/${_bootstrap_filename})

set(_ext_flags
    -ext WixBalExtension
)

set(_addtional_defines
    -dCPACK_BOOTSTRAP_THEME_FILE=${CPACK_BINARY_DIR}/BootstrapperTheme
    -dCPACK_BOOTSTRAP_UPGRADE_GUID=${CPACK_WIX_BOOTSTRAP_UPGRADE_GUID}
    -dCPACK_DOWNLOAD_SITE=${CPACK_DOWNLOAD_SITE}
    -dCPACK_LOCAL_INSTALLER_DIR=${_cpack_wix_out_dir}
    -dCPACK_PACKAGE_FILE_NAME=${CPACK_PACKAGE_FILE_NAME}
    -dCPACK_PACKAGE_INSTALL_DIRECTORY=${_fixed_package_install_dir}
    -dCPACK_WIX_PRODUCT_LOGO=${CPACK_WIX_PRODUCT_LOGO}
    -dCPACK_RESOURCE_PATH=${CPACK_SOURCE_DIR}/Platform/Windows/Packaging
)

if(CPACK_LICENSE_URL)
    list(APPEND _addtional_defines -dCPACK_LICENSE_URL=${CPACK_LICENSE_URL})
endif()

set(_candle_command
    ${CPACK_WIX_CANDLE_EXECUTABLE}
    -nologo
    -arch x64
    "-I${_cpack_wix_out_dir}" # to include cpack_variables.wxi
    ${_addtional_defines}
    ${_ext_flags}
    "${CPACK_SOURCE_DIR}/Platform/Windows/Packaging/Bootstrapper.wxs"
    -o "${_bootstrap_out_dir}/"
)

set(_light_command
    ${CPACK_WIX_LIGHT_EXECUTABLE}
    -nologo
    ${_ext_flags}
    ${_bootstrap_out_dir}/*.wixobj
    -o "${_bootstrap_output_file}"
)

if(CPACK_UPLOAD_URL) # Skip signing if we are not uploading the package
    ly_sign_binaries("${_cpack_wix_out_dir}" "packagePath")
endif()

message(STATUS "Creating Bootstrap Installer...")
execute_process(
    COMMAND ${_candle_command}
    RESULT_VARIABLE _candle_result
    ERROR_VARIABLE _candle_errors
)
if(NOT ${_candle_result} EQUAL 0)
    message(FATAL_ERROR "An error occurred invoking candle.exe.  ${_candle_errors}")
endif()

execute_process(
    COMMAND ${_light_command}
    RESULT_VARIABLE _light_result
    ERROR_VARIABLE _light_errors
)
if(NOT ${_light_result} EQUAL 0)
    message(FATAL_ERROR "An error occurred invoking light.exe.  ${_light_errors}")
endif()

message(STATUS "Bootstrap installer generated to ${_bootstrap_output_file}")

if(CPACK_UPLOAD_URL) # Skip signing if we are not uploading the package
    ly_sign_binaries("${_bootstrap_output_file}" "bootstrapPath")
endif()

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
    "${_cpack_wix_out_dir}/*.msi" 
    "${_cpack_wix_out_dir}/*.cab"
    "${_cpack_wix_out_dir}/*.exe"
    "${CPACK_3P_LICENSE_FILE}"
    "${CPACK_3P_MANIFEST_FILE}"
)
file(COPY ${_artifacts}
    DESTINATION ${CPACK_UPLOAD_DIRECTORY}
)
message(STATUS "Artifacts copied to ${CPACK_UPLOAD_DIRECTORY}")

if(CPACK_UPLOAD_URL)
    ly_upload_to_url(
        ${CPACK_UPLOAD_URL}
        ${CPACK_UPLOAD_DIRECTORY}
        ".*(.cab|.exe|.msi|.txt|.json)$"
    )

    # for auto tagged builds, we will also upload a second copy of just the boostrapper
    # to a special "Latest" folder under the branch in place of the commit date/hash
    if(CPACK_AUTO_GEN_TAG)
        ly_upload_to_latest(${CPACK_UPLOAD_URL} ${_bootstrap_output_file})
    endif()
endif()
