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

if(NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})
ly_get_list_relative_pal_filename(common_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/Common)

ly_add_target(
    NAME ResourceCompilerABC MODULE
    NAMESPACE Legacy
    OUTPUT_SUBDIRECTORY rc_plugins
    FILES_CMAKE
        resourcecompilerabc_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        ${common_dir}/${PAL_TRAIT_COMPILER_ID}/resourcecompilerabc_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::lz4
            3rdParty::mikkelsen
            3rdParty::zlib
            3rdParty::zstd
            Legacy::CryCommonTools
            Legacy::Cry3DEngine.MeshCompiler.Static
            Legacy::Cry3DEngine.CGF.Static
        PUBLIC
            3rdParty::alembic
            Legacy::CryCommon
            Legacy::ResourceCompiler.Static
    RUNTIME_DEPENDENCIES
        Legacy::CryXML
)
ly_add_dependencies(RC Legacy::ResourceCompilerABC)
