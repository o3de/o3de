#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_get_list_relative_pal_filename(core_pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})

ly_get_list_relative_pal_filename(editor_pal_dir ${CMAKE_CURRENT_LIST_DIR}/Editor/Platform/${PAL_PLATFORM_NAME})
ly_get_list_relative_pal_filename(editor_common_dir ${CMAKE_CURRENT_LIST_DIR}/Editor/Platform/Common)

ly_add_target(
    NAME EMotionFXStaticLib STATIC
    NAMESPACE Gem
    FILES_CMAKE
        emotionfx_shared_files.cmake
        MCore/mcore_files.cmake
        EMotionFX/emotionfx_files.cmake
        EMotionFX/CommandSystem/commandsystem_files.cmake
        EMotionFX/Exporters/ExporterLib/exporterlib_files.cmake
        ${core_pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${core_pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
            Include
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
            Legacy::CryCommon
        PUBLIC
            AZ::AtomCore
            Gem::Atom_RPI.Public
            Gem::AtomLyIntegration_CommonFeatures.Static
            Gem::LmbrCentral
    COMPILE_DEFINITIONS
        PUBLIC
            EMFX_DEVELOPMENT_BUILD
)

ly_add_target(
    NAME EMotionFX.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        emotionfx_files.cmake
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::EMotionFXStaticLib
)

ly_add_target(
    NAME EMotionFX ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        Source/Integration/System/emotionfx_module_files.cmake
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::EMotionFX.Static
    RUNTIME_DEPENDENCIES
        Gem::LmbrCentral
)

# Clients and servers use the above EMotionFX module
ly_create_alias(NAME EMotionFX.Clients NAMESPACE Gem TARGETS EMotionFX)
ly_create_alias(NAME EMotionFX.Servers NAMESPACE Gem TARGETS EMotionFX)

if (PAL_TRAIT_BUILD_HOST_TOOLS)
    
    ly_add_target(
        NAME EMotionFX.Editor.Static STATIC
        NAMESPACE Gem
        AUTOMOC
        AUTORCC
        FILES_CMAKE
            emotionfx_files.cmake
            emotionfx_editor_files.cmake
            MysticQt/mysticqt_files.cmake
            EMotionFX/Tools/EMotionStudio/EMStudioSDK/emstudiosdk_files.cmake
            EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/standardplugins_files.cmake
            EMotionFX/Tools/EMotionStudio/Plugins/RenderPlugins/renderplugins_files.cmake
            EMotionFX/Rendering/rendering_files.cmake
            EMotionFX/Pipeline/SceneAPIExt/sceneapi_ext_files.cmake
            EMotionFX/Pipeline/RCExt/rc_ext_files.cmake
            EMotionFX/Pipeline/EMotionFXBuilder/emotionfxbuilder_files.cmake
            ${editor_pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        PLATFORM_INCLUDE_FILES
            ${editor_pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                EMotionFX/Tools
                EMotionFX/Pipeline
                ${editor_pal_dir}
        BUILD_DEPENDENCIES
            PUBLIC
                EMotionFXStaticLib
                3rdParty::Qt::OpenGL
                3rdParty::Qt::Widgets
                AZ::AssetBuilderSDK
                AZ::AzQtComponents
                AZ::GFxFramework
                AZ::SceneCore
                AZ::SceneData
                AZ::SceneUI
                AZ::AzToolsFramework
                Legacy::Editor.Headers
                3rdParty::OpenGLInterface
                Gem::AtomToolsFramework.Static
                Gem::AtomToolsFramework.Editor
        COMPILE_DEFINITIONS
            PUBLIC
                EMFX_EMSTUDIOLYEMBEDDED
                EMOTIONFXANIMATION_EDITOR
    )

    ly_add_target(
        NAME EMotionFX.Editor GEM_MODULE

        NAMESPACE Gem
        FILES_CMAKE
            Source/Integration/System/emotionfx_module_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                EMotionFX/Tools
                EMotionFX/Pipeline
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::EMotionFX.Editor.Static
        RUNTIME_DEPENDENCIES
            Gem::LmbrCentral.Editor
    )

    # builders and tools use the above EMotionFX.Editor module
    ly_create_alias(NAME EMotionFX.Builders NAMESPACE Gem TARGETS EMotionFX.Editor)
    ly_create_alias(NAME EMotionFX.Tools NAMESPACE Gem TARGETS EMotionFX.Editor)

endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_add_target(
        NAME EMotionFX.Tests.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            emotionfx_shared_tests_files.cmake
        INCLUDE_DIRECTORIES
            PUBLIC
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::EMotionFX.Static
    )

    ly_add_target(
        NAME EMotionFX.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            emotionfx_tests_files.cmake
            Source/Integration/System/emotionfx_module_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                ${core_pal_dir}
        BUILD_DEPENDENCIES
            PRIVATE
                Legacy::CryCommon
                AZ::AzTest
                AZ::AzFramework
                Gem::EMotionFX.Static
                Gem::EMotionFX.Tests.Static
    )
    ly_add_googletest(
        NAME Gem::EMotionFX.Tests
    )

    list(APPEND testTargets EMotionFX.Tests)

    if (PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME EMotionFX.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            AUTOMOC
            FILES_CMAKE
                emotionfx_shared_tests_files.cmake
                emotionfx_editor_tests_files.cmake
                Source/Integration/System/emotionfx_module_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
                    EMotionFX/Tools
                    EMotionFX/Pipeline
                    ${editor_pal_dir}
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    AZ::AzToolsFramework.Tests
                    3rdParty::Qt::Test
                    3rdParty::Qt::Concurrent
                    Gem::EMotionFX.Editor.Static
                    Gem::EMotionFX.Tests.Static
        )
        ly_add_googletest(
            NAME Gem::EMotionFX.Editor.Tests
        )

        list(APPEND testTargets EMotionFX.Editor.Tests)
    endif()

    ly_add_target_files(
        TARGETS
            ${testTargets}
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/EMotionFXBuilderTestAssets/AnimGraphExample.animgraph
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/EMotionFXBuilderTestAssets/AnimGraphExampleNoDependency.animgraph
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/EMotionFXBuilderTestAssets/EmptyAnimGraphExample.animgraph
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/EMotionFXBuilderTestAssets/EmptyMotionSetExample.motionset
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/EMotionFXBuilderTestAssets/MotionSetExample.motionset
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/EMotionFXBuilderTestAssets/MotionSetExampleNoDependency.motionset
        OUTPUT_SUBDIRECTORY
            Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/EMotionFXBuilderTestAssets
    )
    ly_add_target_files(
        TARGETS
            ${testTargets}
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Rin/rin.actor
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Rin/rin.animgraph
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Rin/rin.emfxrecording
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Rin/rin.motionset
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Rin/rin_come_to_stop.motion
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Rin/rin_forward_dive_roll.motion
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Rin/rin_idle.motion
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Rin/rin_jump.motion
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Rin/rin_readyattack_idle.motion
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Rin/rin_run.motion
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Rin/rin_run_turn_left.motion
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Rin/rin_run_turn_right.motion
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Rin/rin_shuffle_turn_left.motion
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Rin/rin_shuffle_turn_right.motion
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Rin/rin_stand_kick_punch_01.motion
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Rin/rin_stand_kick_punch_02.motion
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Rin/rin_stand_kick_punch_03.motion
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Rin/rin_stand_kick_punch_04.motion
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Rin/rin_stand_kick_punch_05.motion
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Rin/rin_stand_kick_punch_06.motion
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Rin/rin_turn_180.motion
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Rin/rin_turn_180_clockwise.motion
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Rin/rin_walk.motion
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Rin/rin_walk_kick_01.motion
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Rin/rin_walk_kick_02.motion
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Rin/rin_walk_kick_03.motion
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Rin/rin_walk_kick_04.motion
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Rin/rin_walk_turn_left.motion
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Rin/rin_walk_turn_right.motion
        OUTPUT_SUBDIRECTORY
            Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin
    )
    ly_add_target_files(
        TARGETS
            ${testTargets}
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Pendulum/pendulum.actor
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Pendulum/pendulum.animgraph
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Pendulum/pendulum.emfxrecording
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Pendulum/pendulum.motion
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestAssets/Pendulum/pendulum.motionset
        OUTPUT_SUBDIRECTORY
            Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Pendulum
    )
endif()
