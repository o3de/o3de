#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

file(REAL_PATH "${CPACK_SOURCE_DIR}/.." _root_path)
file(TO_NATIVE_PATH "${_root_path}/python/python.cmd" _python_cmd)
file(TO_NATIVE_PATH "${_root_path}/scripts/build/tools/upload_to_s3.py" _upload_script)

function(ly_upload_to_url in_url in_local_path in_file_regex)

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

    if (${_upload_result} EQUAL 0)
        message(STATUS "Artifact uploading complete!")
    else()
        message(FATAL_ERROR "An error occurred uploading to s3.\nOutput:\n${_upload_output}")
    endif()
endfunction()
