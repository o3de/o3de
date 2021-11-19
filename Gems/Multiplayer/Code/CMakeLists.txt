#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
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
        PRIVATE
            Gem::EMotionFXStaticLib
            Gem::PhysX.Static
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

# The  "Multiplayer" target is used by clients and servers, Debug is used only on clients.
ly_create_alias(NAME Multiplayer.Clients NAMESPACE Gem TARGETS Gem::Multiplayer Gem::Multiplayer.Debug)
ly_create_alias(NAME Multiplayer.Servers NAMESPACE Gem TARGETS Gem::Multiplayer Gem::Multiplayer.Debug)

if (PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME Multiplayer.Builders.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            multiplayer_tools_files.cmake
        COMPILE_DEFINITIONS
            PUBLIC
                MULTIPLAYER_TOOLS
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Source
                ${pal_source_dir}
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework
                Gem::Multiplayer.Static
    )

    # by naming this target Multiplayer.Builders it ensures that it is loaded
    # in any pipeline tools (Like Asset Processor, AssetBuilder, etc)
    ly_add_target(
        NAME Multiplayer.Builders GEM_MODULE
        NAMESPACE Gem
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
                Gem::Multiplayer.Builders.Static
        RUNTIME_DEPENDENCIES
            Gem::Multiplayer.Editor
    )

    ly_add_target(
        NAME Multiplayer.Editor GEM_MODULE
        NAMESPACE Gem
        FILES_CMAKE
            multiplayer_editor_shared_files.cmake
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
            PRIVATE
                Legacy::CryCommon
                Legacy::Editor.Headers
                AZ::AzCore
                AZ::AzFramework
                AZ::AzNetworking
                AZ::AzToolsFramework
                Gem::Atom_RPI.Public
                Gem::Atom_RHI.Reflect
                Gem::Multiplayer.Static
                Gem::Multiplayer.Builders
    )

    # use the Multiplayer.Editor module in tools like the Editor:  Such tools also get the visual debug view:
    ly_create_alias(NAME Multiplayer.Tools     NAMESPACE Gem TARGETS Gem::Multiplayer.Editor Gem::Multiplayer.Debug Gem::Multiplayer.Builders)
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
        AUTOGEN_RULES
            *.AutoComponent.xml,AutoComponent_Header.jinja,$path/$fileprefix.AutoComponent.h
            *.AutoComponent.xml,AutoComponent_Source.jinja,$path/$fileprefix.AutoComponent.cpp
            *.AutoComponent.xml,AutoComponentTypes_Header.jinja,$path/AutoComponentTypes.h
            *.AutoComponent.xml,AutoComponentTypes_Source.jinja,$path/AutoComponentTypes.cpp
    )
    ly_add_googletest(
        NAME Gem::Multiplayer.Tests
    )
    ly_add_googlebenchmark(
        NAME Gem::Multiplayer.Benchmarks
        TARGET Gem::Multiplayer.Tests
    )
    
    if (PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME Multiplayer.Builders.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                multiplayer_tools_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
                    .
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    AZ::AzTestShared
                    AZ::AzToolsFrameworkTestCommon
                    Gem::Multiplayer.Builders.Static
        )
        ly_add_googletest(
            NAME Gem::Multiplayer.Builders.Tests
        )
    endif()

endif()
