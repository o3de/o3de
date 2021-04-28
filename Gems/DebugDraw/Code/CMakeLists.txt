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
    NAME DebugDraw.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        debugdraw_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            Legacy::CryCommon
)

ly_add_target(
    NAME DebugDraw ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        debugdraw_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::DebugDraw.Static
)

# servers do not need debug draw components, only clients
ly_create_alias(NAME DebugDraw.Clients NAMESPACE Gem TARGETS DebugDraw)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME DebugDraw.Editor GEM_MODULE

        NAMESPACE Gem
        FILES_CMAKE
            debugdraw_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::DebugDraw.Static
                AZ::AzToolsFramework
    )

    # builders and tools use DebugDraw.Editor
    ly_create_alias(NAME DebugDraw.Builders NAMESPACE Gem TARGETS DebugDraw.Editor)
    ly_create_alias(NAME DebugDraw.Tools    NAMESPACE Gem TARGETS DebugDraw.Editor)
  
endif()
