#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME EditorModeFeedback.Editor.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            editormodefeedback_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzCore
                AZ::AzToolsFramework
                Gem::Atom_RPI.Public
                Gem::Atom_Feature_Common.Static
                Gem::AtomLyIntegration_CommonFeatures.Static
            PRIVATE
    )

    ly_add_target(
        NAME EditorModeFeedback.Editor GEM_MODULE
        NAMESPACE Gem
        AUTORCC
        FILES_CMAKE
            editormodefeedback_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::EditorModeFeedback.Editor.Static
    )
endif()
