#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

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

file(COPY ${_bootstrap_output_file}
    DESTINATION ${CPACK_PACKAGE_DIRECTORY}
)

message(STATUS "Bootstrap installer generated to ${CPACK_PACKAGE_DIRECTORY}/${_bootstrap_filename}")

# use the internal default path if somehow not specified from cpack_configure_downloads
if(NOT CPACK_UPLOAD_DIRECTORY)
    set(CPACK_UPLOAD_DIRECTORY ${CPACK_PACKAGE_DIRECTORY}/CPackUploads)
endif()

# copy the artifacts intended to be uploaded to a remote server into the folder specified
# through cpack_configure_downloads.  this mimics the same process cpack does natively for
# some other frameworks that have built-in online installer support.
message(STATUS "Copying installer artifacts to upload directory...")
file(REMOVE_RECURSE ${CPACK_UPLOAD_DIRECTORY})
file(GLOB _artifacts "${_cpack_wix_out_dir}/*.msi" "${_cpack_wix_out_dir}/*.cab")
file(COPY ${_artifacts}
    DESTINATION ${CPACK_UPLOAD_DIRECTORY}
)
message(STATUS "Artifacts copied to ${CPACK_UPLOAD_DIRECTORY}")

if(NOT CPACK_UPLOAD_URL)
    return()
endif()

file(REAL_PATH "${CPACK_SOURCE_DIR}/.." _root_path)

file(TO_NATIVE_PATH "${_root_path}/python/python.cmd" _python_cmd)
file(TO_NATIVE_PATH "${_root_path}/scripts/build/tools/upload_to_s3.py" _upload_script)
file(TO_NATIVE_PATH "${_cpack_wix_out_dir}" _cpack_wix_out_dir)

# strip the scheme and extract the bucket/key prefix from the URL
string(REPLACE "s3://" "" _stripped_url ${CPACK_UPLOAD_URL})
string(REPLACE "/" ";" _tokens ${_stripped_url})

list(POP_FRONT _tokens _bucket)
string(JOIN "/" _prefix ${_tokens})

set(_file_regex ".*(cab|exe|msi)$")
set(_extra_args [[{"ACL":"bucket-owner-full-control"}]])

set(_upload_command
    ${_python_cmd} -s
    -u ${_upload_script}
    --base_dir ${_cpack_wix_out_dir}
    --file_regex="${_file_regex}"
    --bucket ${_bucket}
    --key_prefix ${_prefix}
    --extra_args ${_extra_args}
)

if(CPACK_AWS_PROFILE)
    list(APPEND _upload_command --profile ${CPACK_AWS_PROFILE})
endif()

message(STATUS "Uploading artifacts to ${CPACK_UPLOAD_URL}")
execute_process(
    COMMAND ${_upload_command}
    RESULT_VARIABLE _upload_result
    ERROR_VARIABLE _upload_errors
)

if (${_upload_result} EQUAL 0)
    message(STATUS "Artifact uploading complete!")
else()
    message(FATAL_ERROR "An error occurred uploading artifacts.  ${_upload_errors}")
endif()
