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
    NAME AtomLyIntegration_CommonFeatures.Public HEADERONLY
    NAMESPACE Gem
    FILES_CMAKE
        atomlyintegration_commonfeatures_public_files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            Include
    BUILD_DEPENDENCIES
        INTERFACE
            Gem::Atom_Feature_Common.Public
)

ly_add_target(
    NAME AtomLyIntegration_CommonFeatures.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        atomlyintegration_commonfeatures_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
            Gem::AtomLyIntegration_CommonFeatures.Public
            Gem::LmbrCentral.Static
            Gem::GradientSignal.Static
            Gem::Atom_Feature_Common.Static
            Gem::Atom_Bootstrap.Headers
)

ly_add_target(
    NAME AtomLyIntegration_CommonFeatures ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.AtomLyIntegration_CommonFeatures.4e981f3b17394f5d84d674fff0f54f4f.v0.1.0
    FILES_CMAKE
        atomlyintegration_commonfeatures_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::AtomLyIntegration_CommonFeatures.Static
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME AtomLyIntegration_CommonFeatures.Editor MODULE
        NAMESPACE Gem
        OUTPUT_NAME Gem.AtomLyIntegration_CommonFeatures.Editor.4e981f3b17394f5d84d674fff0f54f4f.v0.1.0
        AUTOUIC
        AUTOMOC
        AUTORCC
        FILES_CMAKE
            atomlyintegration_commonfeatures_editor_files.cmake
            ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Source
            PUBLIC
                Include
        COMPILE_DEFINITIONS
            PRIVATE
                ATOMLYINTEGRATION_FEATURE_COMMON_EDITOR
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::AtomLyIntegration_CommonFeatures.Static
                Gem::Atom_RPI.Edit
                Gem::AtomToolsFramework.Static
                Gem::AtomToolsFramework.Editor
                AZ::SceneCore
                AZ::SceneData
                Legacy::EditorLib
                Legacy::CryCommon
    )
endif()
