#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME ${gem_name}.Static STATIC
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
            Gem::ScriptCanvas.API
            Gem::ScriptCanvas.Extensions
    AUTOGEN_RULES
        *.ScriptCanvasGrammar.xml,ScriptCanvasGrammar_Header.jinja,$path/$fileprefix.generated.h
        *.ScriptCanvasGrammar.xml,ScriptCanvasGrammar_Source.jinja,$path/$fileprefix.generated.cpp
        *.ScriptCanvasNodeable.xml,ScriptCanvasNodeable_Header.jinja,$path/$fileprefix.generated.h
        *.ScriptCanvasNodeable.xml,ScriptCanvasNodeable_Source.jinja,$path/$fileprefix.generated.cpp
)

ly_add_target(
    NAME ${gem_name} ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
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
            Gem::${gem_name}.Static
)

ly_add_source_properties(
    SOURCES Source/StartingPointInputGem.cpp
    PROPERTY COMPILE_DEFINITIONS 
    VALUES
        ${LY_PAL_TOOLS_DEFINES}
        O3DE_GEM_NAME=${gem_name}
        O3DE_GEM_VERSION=${gem_version}
)

# the above module is for use in clients and servers
ly_create_alias(NAME ${gem_name}.Servers  NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Unified  NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Clients  NAMESPACE Gem TARGETS Gem::${gem_name})

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME ${gem_name}.Editor GEM_MODULE

        NAMESPACE Gem
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
                Gem::${gem_name}.Static
    )

    # by default, activate the ab ove module in builders and tools:
    ly_create_alias(NAME ${gem_name}.Builders NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)
    ly_create_alias(NAME ${gem_name}.Tools    NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)

endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME ${gem_name}.Tests MODULE
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
                Gem::${gem_name}.Static
    )
    ly_add_googletest(
        NAME Gem::${gem_name}.Tests
    )
endif()
