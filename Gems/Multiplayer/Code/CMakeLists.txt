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
    NAME Multiplayer.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        multiplayer_files.cmake
        multiplayer_autogen_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            ${pal_source_dir}
            AZ::AzNetworking
            Source
            .
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
            AZ::AzNetworking
    AUTOGEN_RULES
        *.AutoPackets.xml,AutoPackets_Header.jinja,$path/$fileprefix.AutoPackets.h
        *.AutoPackets.xml,AutoPackets_Inline.jinja,$path/$fileprefix.AutoPackets.inl
        *.AutoPackets.xml,AutoPackets_Source.jinja,$path/$fileprefix.AutoPackets.cpp
        *.AutoPackets.xml,AutoPacketDispatcher_Header.jinja,$path/$fileprefix.AutoPacketDispatcher.h
        *.AutoPackets.xml,AutoPacketDispatcher_Inline.jinja,$path/$fileprefix.AutoPacketDispatcher.inl
        *.AutoComponent.xml,AutoComponent_Header.jinja,$path/$fileprefix.AutoComponent.h
        *.AutoComponent.xml,AutoComponent_Source.jinja,$path/$fileprefix.AutoComponent.cpp
        *.AutoComponent.xml,AutoComponentTypes_Header.jinja,$path/AutoComponentTypes.h
        *.AutoComponent.xml,AutoComponentTypes_Source.jinja,$path/AutoComponentTypes.cpp
)

ly_add_target(
    NAME Multiplayer ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        multiplayer_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
            .
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::Multiplayer.Static
    RUNTIME_DEPENDENCIES
        Gem::CertificateManager
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)

    ly_add_target(
        NAME Multiplayer.Tools MODULE
        NAMESPACE Gem
        OUTPUT_NAME Gem.Multiplayer.Tools
        FILES_CMAKE
            multiplayer_tools_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                .
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzToolsFramework
                Gem::Multiplayer.Static
    )
    
    ly_add_target(
        NAME Multiplayer.Editor.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            multiplayer_editor_files.cmake
        COMPILE_DEFINITIONS
            PUBLIC
                MULTIPLAYER_EDITOR
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Source
                ${pal_source_dir}
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                Legacy::CryCommon
                Legacy::Editor.Headers
                AZ::AzCore
                AZ::AzFramework
                AZ::AzNetworking
                AZ::AzToolsFramework
                AZ::GFxFramework
                Gem::Multiplayer.Static
    )

    ly_add_target(
        NAME Multiplayer.Editor GEM_MODULE
        NAMESPACE Gem
        FILES_CMAKE
            multiplayer_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Source
                ${pal_source_dir}
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::Multiplayer.Editor.Static
        RUNTIME_DEPENDENCIES
            Gem::LmbrCentral.Editor
    )

endif()

if (PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME Multiplayer.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            multiplayer_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                ${pal_source_dir}
                Source
                .
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Gem::Multiplayer.Static
    )
    ly_add_googletest(
        NAME Gem::Multiplayer.Tests
    )
endif()

ly_add_target(
    NAME Multiplayer.Debug ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        multiplayer_debug_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
            .
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AtomCore
            AZ::AzFramework
            AZ::AzNetworking
            Gem::Atom_Feature_Common.Static
            Gem::Multiplayer.Static
            Gem::ImGui.Static
)
