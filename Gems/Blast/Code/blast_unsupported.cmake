#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME Blast.Stub ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        blast_stub_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
)
add_library(Gem::Blast ALIAS Blast.Stub) 

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME Blast.Editor.Stub GEM_MODULE

        NAMESPACE Gem
        FILES_CMAKE
            blast_stub_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
        COMPILE_DEFINITIONS
            PRIVATE
                BLAST_EDITOR
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
    )
    add_library(Gem::Blast.Editor ALIAS Blast.Editor.Stub) 
endif()
