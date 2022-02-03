#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

message(STATUS "Executing packaging postbuild...")

# ly_is_s3_url
# if the given URL is a s3 url of thr form "s3://(stuff)" then sets
# the output_variable_name to TRUE otherwise unsets it.
function (ly_is_s3_url download_url output_variable_name)
    if ("${download_url}" MATCHES "s3://.*")
        set(${output_variable_name} TRUE PARENT_SCOPE)
    else()
        unset(${output_variable_name} PARENT_SCOPE)
    endif()
endfunction()

function(ly_upload_to_url in_url in_local_path in_file_regex)

    message(STATUS "Uploading ${in_local_path}/${in_file_regex} artifacts to ${CPACK_UPLOAD_URL}")
    ly_is_s3_url(${in_url} _is_s3_bucket)
    if(NOT _is_s3_bucket)
        message(FATAL_ERROR "Only S3 installer uploading is supported at this time")
    endif()

    # strip the scheme and extract the bucket/key prefix from the URL
    string(REPLACE "s3://" "" _stripped_url ${in_url})
    string(REPLACE "/" ";" _tokens ${_stripped_url})

    list(POP_FRONT _tokens _bucket)
    string(JOIN "/" _prefix ${_tokens})

    set(_extra_args [[{"ACL":"bucket-owner-full-control"}]])

    file(TO_NATIVE_PATH "${LY_ROOT_FOLDER}/scripts/build/tools/upload_to_s3.py" _upload_script)

    set(_upload_command
        ${CPACK_LY_PYTHON_CMD} -s
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
        ERROR_VARIABLE _upload_error
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if (${_upload_result} EQUAL 0)
        message(STATUS "Artifact uploading complete!")
    else()
        message(FATAL_ERROR "An error occurred uploading to s3.\n  Output: ${_upload_output}\n\  Error: ${_upload_error}")
    endif()
endfunction()

function(ly_upload_to_latest in_url in_path)

    message(STATUS "Updating latest tagged build")

    # make sure we can extra the commit info from the URL first
    string(REGEX MATCH "([0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]-[0-9a-zA-Z]+)"
        commit_info ${in_url}
    )
    if(NOT commit_info)
        message(FATAL_ERROR "Failed to extract the build tag")
    endif()

    # Create a temp directory where we are going to rename the file to take out the version
    # and then upload it
    set(temp_dir ${CPACK_BINARY_DIR}/temp)
    if(NOT EXISTS ${temp_dir})
        file(MAKE_DIRECTORY ${temp_dir})
    endif()
    file(COPY ${in_path} DESTINATION ${temp_dir})

    cmake_path(GET in_path FILENAME in_path_filename)
    string(REPLACE "_${CPACK_PACKAGE_VERSION}" "" non_versioned_in_path_filename ${in_path_filename})
    file(RENAME "${temp_dir}/${in_path_filename}" "${temp_dir}/${non_versioned_in_path_filename}")

    # include the commit info in a text file that will live next to the exe
    set(_temp_info_file ${temp_dir}/build_tag.txt)
    file(WRITE ${_temp_info_file} ${commit_info})

    # update the URL and upload
    string(REPLACE
        ${commit_info} "Latest"
        latest_upload_url ${in_url}
    )

    ly_upload_to_url(
        ${latest_upload_url}
        ${temp_dir}
        ".*(${non_versioned_in_path_filename}|build_tag.txt)$"
    )

    # cleanup the temp files
    file(REMOVE_RECURSE ${temp_dir})
    message(STATUS "Latest build update complete!")

endfunction()