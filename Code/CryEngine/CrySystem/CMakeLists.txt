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

add_subdirectory(XML)

ly_add_target(
    NAME CrySystem.Static STATIC
    NAMESPACE Legacy
    FILES_CMAKE
        crysystem_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::expat
            3rdParty::lz4
            3rdParty::md5
            3rdParty::tiff
            3rdParty::zlib
            3rdParty::zstd
            Legacy::CryCommon
            Legacy::CrySystem.XMLBinary
            Legacy::RemoteConsoleCore
            AZ::AzFramework
)

ly_add_source_properties(
    SOURCES SystemInit.cpp
    PROPERTY COMPILE_DEFINITIONS 
    VALUES ${LY_PAL_TOOLS_DEFINES}
)

ly_add_target(
    NAME CrySystem ${PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE}
    NAMESPACE Legacy
    FILES_CMAKE
        crysystem_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::CrySystem.Static
            AZ::AzCore
            Legacy::CryCommon
)
