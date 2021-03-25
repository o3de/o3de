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
    NAME StartingPointInput.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        startingpointinput_files.cmake
        startingpointinput_autogen_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
            .
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
            CryCommon
            Gem::ScriptCanvas
    AUTOGEN_RULES
        *.ScriptCanvasNode.xml,ScriptCanvasNode_Header.jinja,$path/$fileprefix.generated.h
        *.ScriptCanvasNode.xml,ScriptCanvasNode_Source.jinja,$path/$fileprefix.generated.cpp
        *.ScriptCanvasNodeable.xml,ScriptCanvasNodeable_Header.jinja,$path/$fileprefix.generated.h
        *.ScriptCanvasNodeable.xml,ScriptCanvasNodeable_Source.jinja,$path/$fileprefix.generated.cpp
)

ly_add_target(
    NAME StartingPointInput ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.StartingPointInput.09f4bedeee614358bc36788e77f97e51.v0.1.0
    FILES_CMAKE
        startingpointinput_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
            .
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzFramework
            Gem::StartingPointInput.Static
)
ly_add_source_properties(
    SOURCES Source/StartingPointInputGem.cpp
    PROPERTY COMPILE_DEFINITIONS 
    VALUES ${LY_PAL_TOOLS_DEFINES}
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME StartingPointInput.Editor MODULE
        NAMESPACE Gem
        OUTPUT_NAME Gem.StartingPointInput.Editor.09f4bedeee614358bc36788e77f97e51.v0.1.0
        FILES_CMAKE
            startingpointinput_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                .
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzFramework
                Gem::StartingPointInput.Static
    )
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME StartingPointInput.Tests MODULE
        NAMESPACE Gem
        FILES_CMAKE
            startingpointinput_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Gem::StartingPointInput.Static
    )
    ly_add_googletest(
        NAME Gem::StartingPointInput.Tests
    )
endif()
