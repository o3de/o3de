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
    NAME GraphCanvas ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        GraphCanvas_game_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
            .
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
)

if (PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME GraphCanvasWidgets STATIC
        NAMESPACE Gem
        AUTOMOC
        AUTOUIC
        AUTORCC
        FILES_CMAKE
            graphcanvas_staticlib_files.cmake
        INCLUDE_DIRECTORIES
            PUBLIC
                .
                StaticLib
        COMPILE_DEFINITIONS
            PUBLIC
                GRAPH_CANVAS_ENABLE_DETAILED_PROFILING=0
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AzFramework
                AZ::AzToolsFramework
                3rdParty::Qt::Core
                3rdParty::Qt::Gui
                3rdParty::Qt::Widgets
    )

    ly_add_target(
        NAME GraphCanvas.Editor GEM_MODULE

        NAMESPACE Gem
        AUTOMOC
        AUTORCC
        FILES_CMAKE
            graphcanvas_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Include
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AzFramework
                AZ::AzToolsFramework
                AZ::AssetBuilderSDK
                3rdParty::Qt::Core
                3rdParty::Qt::Gui
                3rdParty::Qt::Widgets
                Gem::GraphCanvasWidgets
        RUNTIME_DEPENDENCIES
            3rdParty::Qt::Xml
            AZ::AzQtComponents
    )
endif ()
