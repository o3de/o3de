#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")
set(common_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/Common)

include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake) #for PAL_TRAIT_AUDIO_ENGINE_WWISE Traits

set(AUDIOENGINEWWISE_COMPILEDEFINITIONS
    $<IF:$<CONFIG:Release>,WWISE_RELEASE,ENABLE_AUDIO_LOGGING>
)

find_package(Wwise MODULE)

################################################################################
# Servers
# (and situations where Wwise SDK is not found or otherwise unavailable)
################################################################################
if(NOT PAL_TRAIT_AUDIO_ENGINE_WWISE_SUPPORTED OR NOT Wwise_FOUND)
    # Don't create any Gem targets and aliases.  Nothing should depend on this
    # Gem directly, because if it doesn't define targets it will cause an error.
    return()
endif()


################################################################################
# Clients
################################################################################
ly_add_target(
    NAME ${gem_name}.Private.Object STATIC
    NAMESPACE Gem
    FILES_CMAKE
        audioenginewwise_files.cmake
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    TARGET_PROPERTIES
        O3DE_PRIVATE_TARGET TRUE
    INCLUDE_DIRECTORIES
        PRIVATE
            Source/Engine
            ${pal_dir}
            ${common_dir}
    COMPILE_DEFINITIONS
        PUBLIC
            ${AUDIOENGINEWWISE_COMPILEDEFINITIONS}
    BUILD_DEPENDENCIES
        PUBLIC
            3rdParty::Wwise
            Legacy::CryCommon
            Gem::AudioSystem.API
)

ly_add_target(
    NAME ${gem_name} ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        audioenginewwise_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source/Engine
            ${pal_dir}
            ${common_dir}
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::${gem_name}.Private.Object
)

# Inject the gem name into the Module source file
ly_add_source_properties(
    SOURCES
        Source/AudioEngineWwiseModule.cpp
    PROPERTY COMPILE_DEFINITIONS
        VALUES
            O3DE_GEM_NAME=${gem_name}
            O3DE_GEM_VERSION=${gem_version})

ly_create_alias(NAME ${gem_name}.Clients NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Unified NAMESPACE Gem TARGETS Gem::${gem_name})

################################################################################
# Tests
################################################################################
if (PAL_TRAIT_BUILD_TESTS_SUPPORTED)
   ly_add_target(
        NAME ${gem_name}.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            audioenginewwise_tests_files.cmake
            audioenginewwise_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                Source/Engine
                ${pal_dir}
                ${common_dir}
        BUILD_DEPENDENCIES
            PUBLIC
                Gem::AudioSystem.API
            PRIVATE
                AZ::AzTest
                Gem::${gem_name}.Private.Object
    )

    ly_add_googletest(
        NAME Gem::${gem_name}.Tests
    )

    list(APPEND testTargets ${gem_name}.Tests)

    ly_add_target_files(
        TARGETS
            ${testTargets}
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/Sounds/wwise/soundbanks/init.bnk
        OUTPUT_SUBDIRECTORY
            Test.Assets/Gems/AudioEngineWwise/sounds/wwise/soundbanks
    )

endif() # PAL_TRAIT_BUILD_TESTS_SUPPORTED


################################################################################
# Tools / Builders
################################################################################
if (PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME ${gem_name}.Editor.Private.Object STATIC
        NAMESPACE Gem
        AUTORCC
        FILES_CMAKE
            audioenginewwise_editor_files.cmake
        TARGET_PROPERTIES
            O3DE_PRIVATE_TARGET TRUE
        COMPILE_DEFINITIONS
            PUBLIC
                ${AUDIOENGINEWWISE_COMPILEDEFINITIONS}
        INCLUDE_DIRECTORIES
            PRIVATE
                Source/Engine
                Source/Editor
                ${pal_dir}
        BUILD_DEPENDENCIES
            PUBLIC
                3rdParty::Wwise
                Gem::AudioSystem.Editor.API
                Legacy::CryCommon
            PRIVATE
                Legacy::EditorCore
                Gem::${gem_name}.Private.Object
        RUNTIME_DEPENDENCIES
            Gem::AudioSystem.Editor
    )

    ly_add_target(
        NAME ${gem_name}.Editor GEM_MODULE
        NAMESPACE Gem
        FILES_CMAKE
            audioenginewwise_editor_shared_files.cmake
        COMPILE_DEFINITIONS
            PRIVATE
                AUDIO_ENGINE_WWISE_EDITOR
        INCLUDE_DIRECTORIES
            PRIVATE
                Source/Engine
                Source/Editor
                ${pal_dir}
        BUILD_DEPENDENCIES
            PUBLIC
                Gem::AudioSystem.API
            PRIVATE
                Gem::${gem_name}.Editor.Private.Object
        RUNTIME_DEPENDENCIES
            Gem::AudioSystem.Editor
    )

    ly_add_target(
        NAME ${gem_name}.Builder.Private.Object STATIC
        NAMESPACE Gem
        FILES_CMAKE
            audioenginewwise_builder_files.cmake
        TARGET_PROPERTIES
            O3DE_PRIVATE_TARGET TRUE
        COMPILE_DEFINITIONS
            PUBLIC
                ${AUDIOENGINEWWISE_COMPILEDEFINITIONS}
        INCLUDE_DIRECTORIES
            PRIVATE
                Source/Builder
                Source/Engine
                ${pal_dir}
        BUILD_DEPENDENCIES
            PUBLIC
                3rdParty::Wwise
                AZ::AssetBuilderSDK
                Gem::AudioSystem.API
            PRIVATE
                Legacy::EditorCore
    )

    ly_add_target(
        NAME ${gem_name}.Builder GEM_MODULE
        NAMESPACE Gem
        FILES_CMAKE
            audioenginewwise_builder_shared_files.cmake
        COMPILE_DEFINITIONS
            PRIVATE
                AUDIO_ENGINE_WWISE_BUILDER
        INCLUDE_DIRECTORIES
            PRIVATE
                Source/Builder
                Source/Editor
                Source/Engine
                ${pal_dir}
        BUILD_DEPENDENCIES
            PUBLIC
                Gem::AudioSystem.API
                Gem::AudioSystem.Editor.API
                Legacy::CryCommon
            PRIVATE
                Gem::${gem_name}.Builder.Private.Object
                Gem::${gem_name}.Private.Object
    )

    ly_create_alias(NAME ${gem_name}.Builders   NAMESPACE Gem TARGETS Gem::${gem_name}.Builder)
    ly_create_alias(NAME ${gem_name}.Tools      NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)

    if (PAL_TRAIT_BUILD_TESTS_SUPPORTED)
        ly_add_target(
            NAME ${gem_name}.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                audioenginewwise_editor_tests_files.cmake
                audioenginewwise_editor_shared_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source/Builder
                    Source/Editor
                    Source/Engine
                    ${pal_dir}
            BUILD_DEPENDENCIES
                PUBLIC
                    Gem::AudioSystem.API
                PRIVATE
                    AZ::AzTest
                    Gem::${gem_name}.Editor.Private.Object
                    Gem::${gem_name}.Builder.Private.Object
                    Legacy::CryCommon
        )

        ly_add_googletest(
            NAME Gem::${gem_name}.Editor.Tests
        )

        list(APPEND editorTestTargets ${gem_name}.Editor.Tests)

        ly_add_target_files(
            TARGETS
                ${editorTestTargets}
            FILES
                ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/EmptyControl.xml
                ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/MissingPreloads.xml
                ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/MissingWwiseFileNode.xml
                ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/MultiplePreloadsMultipleBanks.xml
                ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/MultiplePreloadsOneBank.xml
                ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/OnePreloadMultipleBanks.xml
                ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/OnePreloadOneBank.xml
            OUTPUT_SUBDIRECTORY
                Test.Assets/Gems/AudioEngineWwise/AudioControls
        )

        ly_add_target_files(
            TARGETS
                ${editorTestTargets}
            FILES
                ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/Legacy/MissingAtlPlatformsNode.xml
                ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/Legacy/MissingConfigGroupNameAttribute.xml
                ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/Legacy/MissingPlatformNameAttributeOnePreload.xml
                ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/Legacy/MissingPlatformNode.xml
                ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/Legacy/MissingWwiseFileNode.xml
                ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/Legacy/MultiplePreloadsMultipleBanks.xml
                ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/Legacy/MultiplePreloadsOneBank.xml
                ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/Legacy/NoConfigGroups.xml
                ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/Legacy/OnePreloadMultipleBanks.xml
                ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/Legacy/OnePreloadOneBank.xml
                ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/Legacy/WrongConfigGroup.xml
            OUTPUT_SUBDIRECTORY
                Test.Assets/Gems/AudioEngineWwise/AudioControls/Legacy
        )

        ly_add_target_files(
            TARGETS
                ${editorTestTargets}
            FILES
                ${CMAKE_CURRENT_SOURCE_DIR}/Tests/Sounds/wwise/test_bank1.bankdeps
                ${CMAKE_CURRENT_SOURCE_DIR}/Tests/Sounds/wwise/test_bank2.bankdeps
                ${CMAKE_CURRENT_SOURCE_DIR}/Tests/Sounds/wwise/test_bank3.bankdeps
                ${CMAKE_CURRENT_SOURCE_DIR}/Tests/Sounds/wwise/test_bank4.bankdeps
                ${CMAKE_CURRENT_SOURCE_DIR}/Tests/Sounds/wwise/test_bank5.bankdeps
                ${CMAKE_CURRENT_SOURCE_DIR}/Tests/Sounds/wwise/test_bank6.bankdeps
                ${CMAKE_CURRENT_SOURCE_DIR}/Tests/Sounds/wwise/test_bank7.bankdeps
                ${CMAKE_CURRENT_SOURCE_DIR}/Tests/Sounds/wwise/test_bank8.bankdeps
                ${CMAKE_CURRENT_SOURCE_DIR}/Tests/Sounds/wwise/test_bank9.bankdeps
            OUTPUT_SUBDIRECTORY
                Test.Assets/Gems/AudioEngineWwise/sounds/wwise
        )

    endif() # PAL_TRAIT_BUILD_TESTS_SUPPORTED
endif() # PAL_TRAIT_BUILD_HOST_TOOLS
