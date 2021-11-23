#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

message(STATUS "Executing package signing...")

function(ly_sign_binaries in_path sign_type)
    file(REAL_PATH "${CPACK_SOURCE_DIR}/.." _root_path)
    unset(_signing_command)

    if(WIN32 OR CYGWIN)        
        cmake_path(SET _sign_script "${_root_path}/scripts/signer/Platform/Windows/signer.ps1" )

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
        if(sign_type MATCHES "win-executable")
            message(STATUS "Signing executable files in ${in_path}")
            execute_process(
                COMMAND ${_signing_command} -exePath ${in_path}
                RESULT_VARIABLE _signing_result
                ERROR_VARIABLE _signing_errors
                OUTPUT_VARIABLE _signing_output
                ECHO_OUTPUT_VARIABLE
            )
        elseif(sign_type MATCHES "win-package")
            message(STATUS "Signing package files in ${in_path}")
            execute_process(
                COMMAND ${_signing_command} -packagePath ${in_path}
                RESULT_VARIABLE _signing_result
                ERROR_VARIABLE _signing_errors
                OUTPUT_VARIABLE _signing_output
                ECHO_OUTPUT_VARIABLE
            )
        elseif(sign_type MATCHES "win-bootstrap")
            message(STATUS "Signing bootstrap installer in ${in_path}")
            execute_process(
                COMMAND ${_signing_command} -bootstrapPath ${in_path}
                RESULT_VARIABLE _signing_result
                ERROR_VARIABLE _signing_errors
                OUTPUT_VARIABLE _signing_output
                ECHO_OUTPUT_VARIABLE
            )
        endif()

    elseif(UNIX AND NOT CYGWIN)
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

    elseif(APPLE AND NOT CYGWIN)
        message("Mac and iOS Gatekeeper signing currently not supported through this function, use internal build signing functions instead")

    else()
        message("Unsupported platform for signing")
    endif()

    if(NOT ${_signing_result} EQUAL 0)
        message(FATAL_ERROR "An error occurred during signing files.  ${_signing_errors}")
    else()
        message(STATUS "Signing complete!")
    endif()
endfunction()
