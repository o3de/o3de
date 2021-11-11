#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

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

# Scan the engine and 3rd Party folders for licenses

file(TO_NATIVE_PATH "${_root_path}/scripts/license_scanner/license_scanner.py" _license_script)
file(TO_NATIVE_PATH "${_root_path}/scripts/license_scanner/scanner_config.json" _license_config_script)
set(_license_scan_path "${_root_path} ${LY_3RDPARTY_PATH}")

set(_license_command
    ${_python_cmd} -s
    -u ${_license_script}
    --config-file ${_license_config_script}
    --license-file-path "${_cpack_wix_out_dir}/NOTICES.txt"
    --package-file-path "${_cpack_wix_out_dir}/SPDX-License.json"
)

message(STATUS "Scanning for license files in ${_license_scan_path}")
execute_process(
    COMMAND ${_license_command} --scan-path ${_license_scan_path}
    RESULT_VARIABLE _license_result
    ERROR_VARIABLE _license_errors
    OUTPUT_VARIABLE _license_output
    ECHO_OUTPUT_VARIABLE
)

if(NOT ${_license_result} EQUAL 0)
    message(FATAL_ERROR "An error occurred during license scan.  ${_license_errors}")
endif()
