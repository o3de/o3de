#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(_target_name ${CMAKE_HOST_SYSTEM_NAME})
if(${_target_name} STREQUAL Darwin)
    set(_target_name Mac)
endif()

if(CPACK_AUTO_GEN_TAG)
    set(_python_script python.sh)
    if(${_target_name} STREQUAL Windows)
        set(_python_script python.cmd)
    endif()

    file(REAL_PATH "${CPACK_SOURCE_DIR}/.." _root_path)
    file(TO_NATIVE_PATH "${_root_path}/python/${_python_script}" _python_cmd)
    file(TO_NATIVE_PATH "${_root_path}/scripts/build/tools/generate_build_tag.py" _gen_tag_script)

    execute_process(
        COMMAND ${_python_cmd} -s -u ${_gen_tag_script}
        RESULT_VARIABLE _gen_tag_result
        OUTPUT_VARIABLE _gen_tag_output
        ERROR_VARIABLE _gen_tag_errors
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_STRIP_TRAILING_WHITESPACE
    )

    if (NOT ${_gen_tag_result} EQUAL 0)
        message(FATAL_ERROR "Failed to generate build tag!  Errors: ${_gen_tag_errors}")
    endif()

    set(_url_tag ${_gen_tag_output})
else()
    set(_url_tag ${CPACK_PACKAGE_VERSION})
endif()

set(_full_tag ${_url_tag}/${_target_name})

if(CPACK_DOWNLOAD_SITE)
    set(CPACK_DOWNLOAD_SITE ${CPACK_DOWNLOAD_SITE}/${_full_tag})
endif()
if(CPACK_UPLOAD_URL)
    set(CPACK_UPLOAD_URL ${CPACK_UPLOAD_URL}/${_full_tag})
endif()
