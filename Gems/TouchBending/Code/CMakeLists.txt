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

ly_get_list_relative_pal_filename(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME})

include(${pal_source_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake) # for PAL_TRAIT_PHYSX_SUPPORTED

if(PAL_TRAIT_PHYSX_SUPPORTED)
    set(touchbending_files touchbending_files.cmake)
    set(touchbending_shared_files touchbending_shared_files.cmake)
    set(touchbending_editor_files touchbending_editor_files.cmake)
else()
    set(touchbending_files touchbending_unsupported_files.cmake)
    set(touchbending_shared_files touchbending_unsupported_shared_files.cmake)
    set(touchbending_editor_files touchbending_unsupported_files.cmake)
endif()

ly_add_target(
    NAME TouchBending.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        ${touchbending_files}
    COMPILE_DEFINITIONS
        PUBLIC
            TOUCHBENDING_VISUALIZE_ENABLED
    INCLUDE_DIRECTORIES
        PUBLIC
            Source
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            3rdParty::PhysX
            AZ::AzFramework
            Gem::PhysX
            Legacy::CryCommon
)

ly_add_target(
    NAME TouchBending ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.TouchBending.c58d2057f3724b22ae0df0be68a4e316.v0.1.0
    FILES_CMAKE
        ${touchbending_shared_files}
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::TouchBending.Static
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME TouchBending.Editor MODULE
        NAMESPACE Gem
        OUTPUT_NAME Gem.TouchBending.Editor.c58d2057f3724b22ae0df0be68a4e316.v0.1.0
        FILES_CMAKE
            ${touchbending_editor_files}
        COMPILE_DEFINITIONS
            PRIVATE
                TOUCHBENDING_EDITOR
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::SceneCore
                AZ::SceneData
                Gem::TouchBending.Static
                Gem::SceneProcessing
    )
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_TOUCHBENDING_SUPPORT_UNIT_TEST)
    ly_add_target(
        NAME TouchBending.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            touchbending_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::AzPhysicsTests
                Gem::TouchBending.Static
        RUNTIME_DEPENDENCIES
            Gem::PhysX
    )
    ly_add_googletest(
        NAME Gem::TouchBending.Tests
    )
endif()
