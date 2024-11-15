#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

ly_add_target(
    NAME ${gem_name}.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        editorpythonbindings_common_files.cmake
    PLATFORM_INCLUDE_FILES
        Source/Platform/${PAL_PLATFORM_NAME}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        Source/Platform/Common/${PAL_TRAIT_COMPILER_ID}/editorpythonbindings_static_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
        PUBLIC
            3rdParty::Python
            3rdParty::pybind11
            AZ::AzToolsFramework
)

ly_add_target(
    NAME ${gem_name}.Editor GEM_MODULE

    NAMESPACE Gem
    FILES_CMAKE
        editorpythonbindings_editor_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source/Platform/${PAL_PLATFORM_NAME}
        PRIVATE
            .
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::${gem_name}.Static
)

# Inject the gem name into the Module source file
ly_add_source_properties(
    SOURCES
        Source/EditorPythonBindingsModule.cpp
    PROPERTY COMPILE_DEFINITIONS
        VALUES
            O3DE_GEM_NAME=${gem_name}
            O3DE_GEM_VERSION=${gem_version})

# builders and tools use EditorPythonBindings.Editor
ly_create_alias(NAME ${gem_name}.Builders NAMESPACE Gem TARGETS ${gem_name}.Editor)
ly_create_alias(NAME ${gem_name}.Tools    NAMESPACE Gem TARGETS ${gem_name}.Editor)

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME ${gem_name}.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            editorpythonbindings_tests_files.cmake
        PLATFORM_INCLUDE_FILES
            Source/Platform/${PAL_PLATFORM_NAME}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
            Source/Platform/Common/${PAL_TRAIT_COMPILER_ID}/editorpythonbindings_tests_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Tests
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Gem::${gem_name}.Static
    )
    ly_add_googletest(
        NAME Gem::${gem_name}.Tests
        COMPONENT TestTools
        LABELS REQUIRES_tiaf
    )
endif()
