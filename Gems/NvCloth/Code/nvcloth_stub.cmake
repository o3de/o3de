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

ly_add_target(
    NAME NvCloth.Stub ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        nvcloth_stub_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
)
add_library(Gem::NvCloth ALIAS NvCloth.Stub) 

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME NvCloth.Editor.Stub GEM_MODULE

        NAMESPACE Gem
        FILES_CMAKE
            nvcloth_stub_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
        COMPILE_DEFINITIONS
            PRIVATE
                NVCLOTH_EDITOR
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
    )
    add_library(Gem::NvCloth.Editor ALIAS NvCloth.Editor.Stub) 
endif()
