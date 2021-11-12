#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

file(REAL_PATH "${CPACK_SOURCE_DIR}/.." LY_ROOT_FOLDER)
include(${LY_ROOT_FOLDER}/cmake/Platform/Common/PackagingPreBuild_common.cmake)

if(NOT CPACK_UPLOAD_URL) # Skip signing if we are not uploading the package
    return()
endif()

file(REAL_PATH "${CPACK_SOURCE_DIR}/.." _root_path)
set(_cpack_wix_out_dir ${CPACK_TOPLEVEL_DIRECTORY})
file(TO_NATIVE_PATH "${_root_path}/scripts/signer/Platform/Windows/signer.ps1" _sign_script)

unset(_signing_command)
find_program(_psiexec_path psexec.exe)
if(_psiexec_path)
    list(APPEND _signing_command
        ${_psiexec_path}
        -accepteula
        -nobanner
        -s
    )
endif()

find_program(_powershell_path powershell.exe REQUIRED)
list(APPEND _signing_command
    ${_powershell_path}
    -NoLogo
    -ExecutionPolicy Bypass
    -File ${_sign_script}
)

# This requires to have a valid local certificate. In continuous integration, these certificates are stored
# in the machine directly. 
# You can generate a test certificate to be able to run this in a PowerShell elevated promp with:
# New-SelfSignedCertificate -DnsName foo.o3de.com -Type CodeSigning -CertStoreLocation Cert:\CurrentUser\My
# Export-Certificate -Cert (Get-ChildItem Cert:\CurrentUser\My\<cert thumbprint>) -Filepath "c:\selfsigned.crt"
# Import-Certificate -FilePath "c:\selfsigned.crt" -Cert Cert:\CurrentUser\TrustedPublisher
# Import-Certificate -FilePath "c:\selfsigned.crt" -Cert Cert:\CurrentUser\Root

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
else()
    message(STATUS "Signing exes complete!")
endif()
