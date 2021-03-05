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

# Cmake version 3.17 is the minimum version needed for all of lumberyard's supported platforms
cmake_minimum_required(VERSION 3.17)

# CMP0111 introduced in 3.19 has a bug that produces the policy to warn every time there is an
# INTERFACE IMPORTED library. We use this type of libraries for handling 3rdParty. The rest of
# the documentation states that INTERFACE IMPORTED libraries do not require to set locations, but
# the policy still warns about it. Issue: https://gitlab.kitware.com/cmake/cmake/-/issues/21470
# The issue was fixed in 3.19.1 so we just disable the policy for 3.19
if(CMAKE_VERSION VERSION_EQUAL 3.19)
    cmake_policy(SET CMP0111 OLD)
endif()

project(Lumberyard LANGUAGES C CXX)

# Turn on the ability to create folders to organize projects (.vcproj)
# It creates "CMakePredefinedTargets" folder by default and adds CMake
# defined projects like INSTALL.vcproj and ZERO_CHECK.vcproj
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_WARN_DEPRECATED ON)

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib CACHE PATH "Build directory for static libraries and import libraries")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin CACHE PATH "Build directory for shared libraries")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin CACHE PATH "Build directory for executables")
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "Installation prefix")

include(cmake/FileUtil.cmake)
include(cmake/PAL.cmake)
include(cmake/PALTools.cmake)
include(cmake/Configurations.cmake) # Requires to be after PAL so we get platform variable definitions
include(cmake/Dependencies.cmake)
include(cmake/Deployment.cmake)
include(cmake/3rdParty.cmake)
include(cmake/LYPython.cmake)
include(cmake/LYWrappers.cmake)
include(cmake/UnitTest.cmake)
include(cmake/LYTestWrappers.cmake)
include(cmake/Version.cmake)
include(cmake/Monolithic.cmake)
include(cmake/SettingsRegistry.cmake)
include(cmake/TestImpactFramework/LYTestImpactFramework.cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

# Add all cmake files in a project so they can be handled from within the IDE
ly_include_cmake_file_list(cmake/cmake_files.cmake)
add_custom_target(CMakeFiles SOURCES ${ALLFILES})
ly_source_groups_from_folders("${ALLFILES}")
unset(ALLFILES)

# Add the projects first so the Launcher can find them
include(cmake/Projects.cmake)

# Add the rest of the targets
add_subdirectory(Code)
add_subdirectory(Gems)

set(enabled_platforms
    ${PAL_PLATFORM_NAME}
    ${LY_PAL_TOOLS_ENABLED})

foreach(restricted_platform ${PAL_RESTRICTED_PLATFORMS})
    if(restricted_platform IN_LIST enabled_platforms)
        add_subdirectory(restricted/${restricted_platform})
    endif()
endforeach()

# The following steps have to be done after all targets were registered:
# 1. link targets where the dependency was yet not declared, we need to have the declaration so we do different
#    linking logic depending on the type of target
ly_delayed_target_link_libraries()
# 2. generate a settings registry .setreg file for all ly_add_project_dependencies() and ly_add_target_dependencies() calls
#    to provide applications with the filenames of gem modules to load
ly_delayed_generate_settings_registry()
# 3. generate a registry file for unit testing for platforms that support unit testing
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_delayed_generate_unit_test_module_registry()
endif()
# 4. inject runtime dependencies to the targets. We need to do this after (1) since we are going to walk through
#    the dependencies
include(cmake/RuntimeDependencies.cmake)

################################################################################
# Tests
################################################################################
# Recurse into directory of general python test scripts
# specific python test registration can be specified within the CMakeLists.txt that is associated with the test(i.e Gem/<Foo>/CMakeLists.txt)
add_subdirectory(ctest_scripts)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/scripts/CMakeLists.txt")
    add_subdirectory(scripts)
endif()

# SPEC-1417 will investigate and fix this
if(NOT PAL_PLATFORM_NAME STREQUAL "Mac")
    add_subdirectory(Tools/LyTestTools/tests/)
    add_subdirectory(Tools/RemoteConsole/ly_remote_console/tests/)
endif()

################################################################################
# Test Impact Framework
################################################################################
# Perform test impact framework post steps once all of the targets have been enumerated
ly_test_impact_post_step()