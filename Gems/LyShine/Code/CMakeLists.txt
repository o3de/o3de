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
ly_get_list_relative_pal_filename(common_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/Common)

ly_add_target(
    NAME LyShine.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        lyshine_static_files.cmake
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::CryCommon
        PUBLIC
            Gem::Atom_RPI.Public
            Gem::Atom_Utils.Static
            Gem::Atom_Bootstrap.Headers
            Gem::AtomFont
            Gem::LmbrCentral.Static
            Gem::TextureAtlas
)

ly_add_target(
    NAME LyShine ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        lyshine_common_module_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::LyShine.Static
            Legacy::CryCommon
    RUNTIME_DEPENDENCIES
        Gem::LmbrCentral
        Gem::TextureAtlas
)

if (PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME LyShine.Editor.Static STATIC
        NAMESPACE Gem
        AUTOMOC
        AUTOUIC
        AUTORCC
        FILES_CMAKE
            lyshine_uicanvaseditor_files.cmake
            lyshine_editor_builder_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Source
                Editor
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                3rdParty::Qt::Core
                3rdParty::Qt::Gui
                3rdParty::Qt::Widgets
                AZ::AzCore
                AZ::AzToolsFramework
                AZ::AssetBuilderSDK
                Legacy::EditorCommon
                Legacy::EditorCore
                Gem::LyShine.Static
                Legacy::CryCommon
                Gem::LmbrCentral.Editor.Static
                Gem::TextureAtlas
                Gem::AtomToolsFramework.Static
                Gem::AtomToolsFramework.Editor
                ${additional_dependencies}
            PUBLIC
                Gem::Atom_RPI.Public
                Gem::Atom_Utils.Static
                Gem::Atom_Bootstrap.Headers
        RUNTIME_DEPENDENCIES
            Gem::TextureAtlas
    )

    ly_add_target(
        NAME LyShine.Editor GEM_MODULE

        NAMESPACE Gem
        FILES_CMAKE
            lyshine_common_module_files.cmake
        COMPILE_DEFINITIONS
            PRIVATE
                LYSHINE_EDITOR
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Source
                Editor
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                Legacy::CryCommon
                AZ::AssetBuilderSDK
                Gem::LyShine.Editor.Static
                Gem::LmbrCentral.Editor
                Gem::TextureAtlas
        RUNTIME_DEPENDENCIES
            Gem::LmbrCentral.Editor
            Gem::TextureAtlas
)
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_add_target(
        NAME LyShine.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            lyshine_common_module_files.cmake
            lyshine_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Gem::LyShine.Static
                Legacy::CryCommon
        RUNTIME_DEPENDENCIES
            Gem::LmbrCentral
            Gem::TextureAtlas
    )
    ly_add_googletest(
        NAME Gem::LyShine.Tests
    )

    if (PAL_TRAIT_BUILD_HOST_TOOLS)

        ly_add_target(
            NAME LyShine.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                lyshine_common_module_files.cmake
                lyshine_editor_tests_files.cmake
            COMPILE_DEFINITIONS
                PRIVATE
                    LYSHINE_EDITOR
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
                    Editor
                    Pipeline
                    .
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    Legacy::CryCommon
                    AZ::AssetBuilderSDK
                    Gem::LyShine.Editor.Static
            RUNTIME_DEPENDENCIES
                Gem::LmbrCentral.Editor
                Gem::TextureAtlas
        )
        ly_add_googletest(
            NAME Gem::LyShine.Editor.Tests
        )
    endif()

endif()
