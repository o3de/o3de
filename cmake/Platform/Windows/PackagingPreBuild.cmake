#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

file(REAL_PATH "${CPACK_SOURCE_DIR}/.." _root_path)
file(REAL_PATH "${CPACK_BINARY_DIR}/.." _cpack_binary_dir)
file(TO_NATIVE_PATH "${_root_path}/scripts/signer/signer.ps1" _sign_script)

set(_signexe_command
    powershell.exe
    -nologo
    -ExecutionPolicy Bypass 
    -File ${_sign_script}
    -exePath ${_cpack_binary_dir}
)

message(STATUS "Signing exe files in ${_cpack_binary_dir}")
execute_process(
    COMMAND ${_signexe_command}
    RESULT_VARIABLE _signexe_result
    ERROR_VARIABLE _signexe_errors
    OUTPUT_VARIABLE _signexe_output
    ECHO_OUTPUT_VARIABLE
)

message(STATUS "Signing exes complete!")
