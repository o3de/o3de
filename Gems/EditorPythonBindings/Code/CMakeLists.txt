#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

# This will set python_package_name to whatever the package 'Python' is associated with
ly_get_package_association(Python python_package_name)
if (NOT python_package_name)
    set(python_package_name "python-no-package-assocation-found")
    message(WARNING "Python was not found in the package assocation list.  Did someone call ly_associate_package(xxxxxxx Python) ?")
endif()
    
ly_add_target(
    NAME EditorPythonBindings.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        editorpythonbindings_common_files.cmake
        Source/Platform/${PAL_PLATFORM_NAME}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        Source/Platform/${PAL_PLATFORM_NAME}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        Source/Platform/Common/${PAL_TRAIT_COMPILER_ID}/editorpythonbindings_static_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake
    COMPILE_DEFINITIONS
        PRIVATE
            PY_PACKAGE="${python_package_name}" 
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
    NAME EditorPythonBindings.Editor GEM_MODULE

    NAMESPACE Gem
    FILES_CMAKE
        editorpythonbindings_editor_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::EditorPythonBindings.Static
)

# builders and tools use EditorPythonBindings.Editor
ly_create_alias(NAME EditorPythonBindings.Builders NAMESPACE Gem TARGETS EditorPythonBindings.Editor)
ly_create_alias(NAME EditorPythonBindings.Tools    NAMESPACE Gem TARGETS EditorPythonBindings.Editor)

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME EditorPythonBindings.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
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
                Gem::EditorPythonBindings.Static
    )
    ly_add_googletest(
        NAME Gem::EditorPythonBindings.Tests
        COMPONENT TestTools
    )
endif()
