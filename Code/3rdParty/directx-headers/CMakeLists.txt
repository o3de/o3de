#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(directx-headers
    VERSION "1.620.2"
    LICENSE "MIT"
    URL "https://github.com/microsoft/DirectX-Headers/archive/refs/tags/v1.620.2.tar.gz"
    URL_HASH "2b916ef1be5db35313f56d60499274fed39db37b54a1f0befa8aa49041c14855"
    GIT "https://github.com/microsoft/DirectX-Headers.git"
    GIT_HASH "61c7718954ec73dbdd43c723240fe1a0452a3724"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(directx-headers)

file(
    COPY "${directx-headers_SOURCE_DIR}/include/"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

o3de_disable_warnings()

ly_add_target(
    NAME directx-headers STATIC
    NAMESPACE 3rdParty
    FILES_CMAKE files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            "${CMAKE_CURRENT_BINARY_DIR}/Include"
            "${CMAKE_CURRENT_BINARY_DIR}/Include/directx"
)

if(NOT WIN32)
    target_include_directories(directx-headers PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Include/wsl/stubs")
endif()

set_target_properties(directx-headers PROPERTIES SYSTEM TRUE)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/directx-headers
)
