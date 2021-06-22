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

ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} ${o3de_gem_restricted_path} ${o3de_gem_path} ${o3de_gem_name})
include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

if(PAL_TRAIT_PYTHONCOVERAGE_SUPPORTED)
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME PythonCoverage.Editor.Static STATIC
            NAMESPACE Gem
            FILES_CMAKE
                pythoncoverage_editor_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Source
                PUBLIC
                    Include
            COMPILE_DEFINITIONS
                PUBLIC
                    PYTHON_COVERAGE_EDITOR
                PRIVATE
                    LY_TEST_IMPACT_DEFAULT_CONFIG_FILE=\"\"
            BUILD_DEPENDENCIES
                PUBLIC
                    AZ::AzToolsFramework
        )

        ly_add_target(
            NAME PythonCoverage.Editor MODULE
            NAMESPACE Gem
            AUTOMOC
            OUTPUT_NAME Gem.PythonCoverage.Editor
            FILES_CMAKE
                pythoncoverage_editor_shared_files.cmake
            COMPILE_DEFINITIONS
                PRIVATE
                    PYTHON_COVERAGE_EDITOR
            INCLUDE_DIRECTORIES
                PRIVATE
                    Source
                PUBLIC
                    Include
            BUILD_DEPENDENCIES
                PUBLIC
                    Gem::PythonCoverage.Editor.Static
        )

        # By default, we will specify that the above target PythonCoverage would be used by
        # Tool and Builder type targets when this gem is enabled.  If you don't want it
        # active in Tools or Builders by default, delete one of both of the following lines:
        ly_create_alias(NAME PythonCoverage.Tools    NAMESPACE Gem TARGETS Gem::PythonCoverage.Editor)
        ly_create_alias(NAME PythonCoverage.Builders NAMESPACE Gem TARGETS Gem::PythonCoverage.Editor)
    endif()
endif()

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME PythonCoverage.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                pythoncoverage_editor_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    Gem::PythonCoverage.Editor
        )

        ly_add_googletest(
            NAME Gem::PythonCoverage.Editor.Tests
        )
    endif()
endif()