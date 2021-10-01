#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

file(REAL_PATH "${CPACK_SOURCE_DIR}/.." _root_path)
set(_cpack_wix_out_dir ${CPACK_TOPLEVEL_DIRECTORY})
file(TO_NATIVE_PATH "${_root_path}/scripts/signer/Platform/Windows/signer.ps1" _sign_script)

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

message(STATUS "Signing executable files in ${_cpack_wix_out_dir}")
execute_process(
    COMMAND ${_signing_command} -exePath ${_cpack_wix_out_dir}
    RESULT_VARIABLE _signing_result
    ERROR_VARIABLE _signing_errors
    OUTPUT_VARIABLE _signing_output
    ECHO_OUTPUT_VARIABLE
)

if(NOT ${_signing_result} EQUAL 0)
    message(FATAL_ERROR "An error occurred during signing executable files.  ${_signing_errors}")
endif()

message(STATUS "Signing exes complete!")
