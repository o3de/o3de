#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(spirv-cross
    VERSION "1.4.363.0"
    LICENSE "Apache-2.0 OR MIT"
    URL "https://github.com/KhronosGroup/SPIRV-Cross/archive/refs/tags/vulkan-sdk-1.4.363.0.tar.gz"
    URL_HASH "f708982e88b763ef5b0394ed468fc2c3628f68cdab89f2e7a946cd628c04e721"
    GIT "https://github.com/KhronosGroup/SPIRV-Cross.git"
    GIT_HASH "f11ba9f0b21ba8fc15153d50a2a1ae31ab1cf8f7"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(spirv-cross)

o3de_disable_warnings()

ly_add_target(
    NAME spirv-cross EXECUTABLE
    NAMESPACE 3rdParty
    NO_UNITY
    OUTPUT_SUBDIRECTORY Builders/SPIRVCross
    FILES_CMAKE files.cmake
)

set_target_properties(spirv-cross PROPERTIES SYSTEM TRUE)

target_compile_options(spirv-cross PRIVATE ${O3DE_COMPILE_OPTION_ENABLE_EXCEPTIONS})

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/spirv-cross
)
