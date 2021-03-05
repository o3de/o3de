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

ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})

include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake) # For PAL_TRAIT_ATOM_CRYRENDEROTHER_SUPPORTED

if(NOT PAL_TRAIT_ATOM_CRYRENDEROTHER_SUPPORTED)
    return()
endif()

ly_add_target(
    NAME CryRenderOther ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Legacy
    FILES_CMAKE
        atom_shim_renderer_files.cmake
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            PCH
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            Legacy::CryCommon
            Legacy::CryRender.Headers
            Legacy::CryRenderNULL.Static
            AZ::AtomCore
            Gem::Atom_RHI.Reflect
            Gem::Atom_RPI.Public
)

# Atom_AtomBridge.Static is the one that drives loading CryRenderOther, however, CryRenderOther
# is not enabled in every platform, so we define the dependency here
ly_add_dependencies(Atom_AtomBridge.Static CryRenderOther)
