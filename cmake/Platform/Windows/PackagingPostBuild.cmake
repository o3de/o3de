#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

# convert the path to a windows style path
string(REPLACE "/" "\\" _install_dir ${CPACK_PACKAGE_INSTALL_DIRECTORY})

# directory where the auto generated files live e.g <build>/_CPack_Package/win64/WIX
set(_cpack_out_dir "${CPACK_TOPLEVEL_DIRECTORY}")
set(_out_dir "${CPACK_BINARY_DIR}/wixobj_bootstrap")

set(_wix_ext_flags
    -ext WixBalExtension
)

set(_candle_command
    ${CPACK_WIX_ROOT}/bin/candle.exe
    -nologo
    -arch x64
    "-I${_cpack_out_dir}"
    ${_wix_ext_flags}

    -dCPACK_DOWNLOAD_SITE=${CPACK_DOWNLOAD_SITE}
    -dCPACK_LOCAL_INSTALLER_DIR=${_cpack_out_dir}
    -dCPACK_PACKAGE_FILE_NAME=${CPACK_PACKAGE_FILE_NAME}
    -dCPACK_PACKAGE_INSTALL_DIRECTORY=${_install_dir}

    "${CPACK_SOURCE_DIR}/Platform/Windows/PackagingBootstrapper.wxs"

    -o "${_out_dir}"
)

set(_light_command
    ${CPACK_WIX_ROOT}/bin/light.exe
    -nologo
    ${_wix_ext_flags}
    ${_out_dir}/*.wixobj

    -o "${CPACK_BINARY_DIR}/installer.exe"
)

message(STATUS "Creating Installer Bootstrapper...")

execute_process(
    COMMAND
        ${_candle_command}

    COMMAND
        ${_light_command}
)
