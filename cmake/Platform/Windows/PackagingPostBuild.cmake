#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

file(REAL_PATH "${CPACK_SOURCE_DIR}/.." _root_path)
file(TO_NATIVE_PATH "${_root_path}/scripts/signer/Platform/Windows/signer.ps1" _sign_script)

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

set(_signing_command
    psexec.exe
    -accepteula 
    -nobanner 
    -s
    powershell.exe
    -NoLogo
    -ExecutionPolicy Bypass 
    -File ${_sign_script}
)

message(STATUS "Signing package files in ${_cpack_wix_out_dir}")
execute_process(
    COMMAND ${_signing_command} -packagePath ${_cpack_wix_out_dir}
    RESULT_VARIABLE _signing_result
    ERROR_VARIABLE _signing_errors
    OUTPUT_VARIABLE _signing_output
    ECHO_OUTPUT_VARIABLE
)

if(NOT ${_signing_result} EQUAL 0)
    message(FATAL_ERROR "An error occurred during signing package files.  ${_signing_errors}")
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

file(COPY ${_bootstrap_output_file}
    DESTINATION ${CPACK_PACKAGE_DIRECTORY}
)

message(STATUS "Bootstrap installer generated to ${CPACK_PACKAGE_DIRECTORY}/${_bootstrap_filename}")

message(STATUS "Signing bootstrap installer in ${CPACK_PACKAGE_DIRECTORY}")
execute_process(
    COMMAND ${_signing_command} -bootstrapPath ${CPACK_PACKAGE_DIRECTORY}/${_bootstrap_filename}
    RESULT_VARIABLE _signing_result
    ERROR_VARIABLE _signing_errors
    OUTPUT_VARIABLE _signing_output
    ECHO_OUTPUT_VARIABLE
)

if(NOT ${_signing_result} EQUAL 0)
    message(FATAL_ERROR "An error occurred during signing bootstrap installer.  ${_signing_errors}")
endif()

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

file(TO_NATIVE_PATH "${_cpack_wix_out_dir}" _cpack_wix_out_dir)
file(TO_NATIVE_PATH "${_root_path}/python/python.cmd" _python_cmd)
file(TO_NATIVE_PATH "${_root_path}/scripts/build/tools/upload_to_s3.py" _upload_script)

function(upload_to_s3 in_url in_local_path in_file_regex)

    # strip the scheme and extract the bucket/key prefix from the URL
    string(REPLACE "s3://" "" _stripped_url ${in_url})
    string(REPLACE "/" ";" _tokens ${_stripped_url})

    list(POP_FRONT _tokens _bucket)
    string(JOIN "/" _prefix ${_tokens})

    set(_extra_args [[{"ACL":"bucket-owner-full-control"}]])

    set(_upload_command
        ${_python_cmd} -s
        -u ${_upload_script}
        --base_dir ${in_local_path}
        --file_regex="${in_file_regex}"
        --bucket ${_bucket}
        --key_prefix ${_prefix}
        --extra_args ${_extra_args}
    )

    if(CPACK_AWS_PROFILE)
        list(APPEND _upload_command --profile ${CPACK_AWS_PROFILE})
    endif()

    execute_process(
        COMMAND ${_upload_command}
        RESULT_VARIABLE _upload_result
        OUTPUT_VARIABLE _upload_output
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if (NOT ${_upload_result} EQUAL 0)
        message(FATAL_ERROR "An error occurred uploading to s3.\nOutput:\n${_upload_output}")
    endif()
endfunction()

message(STATUS "Uploading artifacts to ${CPACK_UPLOAD_URL}")
upload_to_s3(
    ${CPACK_UPLOAD_URL}
    ${_cpack_wix_out_dir}
    ".*(cab|exe|msi)$"
)
message(STATUS "Artifact uploading complete!")

# for auto tagged builds, we will also upload a second copy of just the boostrapper
# to a special "Latest" folder under the branch in place of the commit date/hash
if(CPACK_AUTO_GEN_TAG)
    message(STATUS "Updating latest tagged build")

    # make sure we can extra the commit info from the URL first
    string(REGEX MATCH "([0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]-[0-9a-zA-Z]+)"
        _commit_info ${CPACK_UPLOAD_URL}
    )
    if(NOT _commit_info)
        message(FATAL_ERROR "Failed to extract the build tag")
    endif()

    set(_temp_dir ${_cpack_wix_out_dir}/temp)
    if(NOT EXISTS ${_temp_dir})
        file(MAKE_DIRECTORY ${_temp_dir})
    endif()

    # strip the version number form the exe name in the one uploaded to latest
    string(TOLOWER "${CPACK_PACKAGE_NAME}_installer.exe" _non_versioned_exe)
    set(_temp_exe_copy ${_temp_dir}/${_non_versioned_exe})

    file(COPY ${_bootstrap_output_file} DESTINATION ${_temp_dir})
    file(RENAME "${_temp_dir}/${_bootstrap_filename}" ${_temp_exe_copy})

    # include the commit info in a text file that will live next to the exe
    set(_temp_info_file ${_temp_dir}/build_tag.txt)
    file(WRITE ${_temp_info_file} ${_commit_info})

    # update the URL and upload
    string(REPLACE
        ${_commit_info} "Latest"
        _latest_upload_url ${CPACK_UPLOAD_URL}
    )

    upload_to_s3(
        ${_latest_upload_url}
        ${_temp_dir}
        ".*(${_non_versioned_exe}|build_tag.txt)$"
    )

    # cleanup the temp files
    file(REMOVE_RECURSE ${_temp_dir})

    message(STATUS "Latest build update complete!")
endif()
