#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

function(ly_sign_binaries in_path)
    message(STATUS "Executing package signing...")
    file(REAL_PATH "${CPACK_SOURCE_DIR}/.." _root_path)
    unset(_signing_command)

    cmake_path(SET _sign_script "${_root_path}/scripts/signer/Platform/Linux/signer.sh")

    list(APPEND _signing_command
        ${_sign_script}
    )
    message(STATUS "Signing package files in ${in_path}")
    execute_process(
        COMMAND ${_signing_command} ${in_path}
        RESULT_VARIABLE _signing_result
        ERROR_VARIABLE _signing_errors
        OUTPUT_VARIABLE _signing_output
        ECHO_OUTPUT_VARIABLE
    )

    if(NOT ${_signing_result} EQUAL 0)
        message(FATAL_ERROR "An error occurred during signing files.  ${_signing_errors}")
    else()
        message(STATUS "Signing complete!")
    endif()
endfunction()
