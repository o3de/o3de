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

# Cmake version 3.17 is the minimum version needed for all of Open 3D Engine's supported platforms
cmake_minimum_required(VERSION 3.19)

# CMP0111 introduced in 3.19 has a bug that produces the policy to warn every time there is an
# INTERFACE IMPORTED library. We use this type of libraries for handling 3rdParty. The rest of
# the documentation states that INTERFACE IMPORTED libraries do not require to set locations, but
# the policy still warns about it. Issue: https://gitlab.kitware.com/cmake/cmake/-/issues/21470
# The issue was fixed in 3.19.1 so we just disable the policy for 3.19
if(CMAKE_VERSION VERSION_EQUAL 3.19)
    cmake_policy(SET CMP0111 OLD)
endif()

include(cmake/LySet.cmake)
include(cmake/Version.cmake)
include(cmake/OutputDirectory.cmake)

# Set the engine_path and engine_json
set(o3de_engine_path ${CMAKE_CURRENT_LIST_DIR})
set(o3de_engine_json ${o3de_engine_path}/engine.json)

if(NOT PROJECT_NAME)
    project(O3DE
        LANGUAGES C CXX
        VERSION ${LY_VERSION_STRING}
    )

    # o3de manifest
    include(cmake/o3de_manifest.cmake)
endif()

################################################################################
# Resolve this engines name and restricted path
################################################################################
o3de_engine_name(${o3de_engine_json} o3de_engine_name)
o3de_restricted_path(${o3de_engine_json} o3de_engine_restricted_path)
message(STATUS "O3DE Engine Name: ${o3de_engine_name}")
message(STATUS "O3DE Engine Path: ${o3de_engine_path}")
if(o3de_engine_restricted_path)
    message(STATUS "O3DE Engine Restricted Path: ${o3de_engine_restricted_path}")
endif()

# add the engines cmake folder to the CMAKE_MODULE_PATH
list(APPEND CMAKE_MODULE_PATH "${o3de_engine_path}/cmake")

################################################################################
# Initialize
################################################################################
include(cmake/GeneralSettings.cmake)
include(cmake/FileUtil.cmake)
include(cmake/PAL.cmake)
include(cmake/PALTools.cmake)
include(cmake/Install.cmake)
include(cmake/Configurations.cmake) # Requires to be after PAL so we get platform variable definitions
include(cmake/Dependencies.cmake)
include(cmake/Deployment.cmake)
include(cmake/3rdParty.cmake)
include(cmake/LYPython.cmake)
include(cmake/LYWrappers.cmake)
include(cmake/UnitTest.cmake)
include(cmake/LYTestWrappers.cmake)
include(cmake/Monolithic.cmake)
include(cmake/SettingsRegistry.cmake)
include(cmake/TestImpactFramework/LYTestImpactFramework.cmake)
include(cmake/CMakeFiles.cmake)

################################################################################
# Subdirectory processing
################################################################################

# Add the projects first so the Launcher can find them
include(cmake/Projects.cmake)

if(NOT INSTALLED_ENGINE)
    # Add the rest of the targets
    add_subdirectory(Code)
else()
    ly_find_o3de_packages()
endif()

# Add external subdirectories listed in the manifest
list(APPEND LY_EXTERNAL_SUBDIRS ${o3de_engine_external_subdirectories})

set(enabled_platforms
    ${PAL_PLATFORM_NAME}
    ${LY_PAL_TOOLS_ENABLED})

# Add any engine restricted platforms as external subdirs
o3de_add_engine_restricted_platform_external_subdirs()

add_subdirectory(scripts)

# SPEC-1417 will investigate and fix this
if(NOT PAL_PLATFORM_NAME STREQUAL "Mac")
    add_subdirectory(Tools/LyTestTools/tests/)
    add_subdirectory(Tools/RemoteConsole/ly_remote_console/tests/)
endif()

################################################################################
# Post-processing
################################################################################

# Loop over the additional external subdirectories and invoke add_subdirectory on them
foreach(external_directory ${LY_EXTERNAL_SUBDIRS})
    # Hash the extenal_directory name and append it to the Binary Directory section of add_subdirectory
    # This is to deal with potential situations where multiple external directories has the same last directory name
    # For example if D:/Company1/RayTracingGem and F:/Company2/Path/RayTracingGem were both added as a subdirectory
    file(REAL_PATH ${external_directory} full_directory_path)
    string(SHA256 full_directory_hash ${full_directory_path})
    # Truncate the full_directory_hash down to 8 characters to avoid hitting the Windows 260 character path limit
    # when the external subdirectory contains relative paths of significant length
    string(SUBSTRING ${full_directory_hash} 0 8 full_directory_hash)
    # Use the last directory as the suffix path to use for the Binary Directory
    get_filename_component(directory_name ${external_directory} NAME)
    add_subdirectory(${external_directory} ${CMAKE_BINARY_DIR}/${directory_name}-${full_directory_hash})
endforeach()

# The following steps have to be done after all targets are registered:
# 1. generate a settings registry .setreg file for all ly_add_project_dependencies() and ly_add_target_dependencies() calls
#    to provide applications with the filenames of gem modules to load
#    This must be done before ly_delayed_target_link_libraries() as that inserts BUILD_DEPENDENCIE as MANUALLY_ADDED_DEPENDENCIES
#    if the build dependency is a MODULE_LIBRARY. That would cause a false load dependency to be generated
ly_delayed_generate_settings_registry()
# 2. link targets where the dependency was yet not declared, we need to have the declaration so we do different
#    linking logic depending on the type of target
ly_delayed_target_link_libraries()
# 3. generate a registry file for unit testing for platforms that support unit testing
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_delayed_generate_unit_test_module_registry()
endif()
# 4. inject runtime dependencies to the targets. We need to do this after (1) since we are going to walk through
#    the dependencies
include(cmake/RuntimeDependencies.cmake)
# 5. Perform test impact framework post steps once all of the targets have been enumerated
ly_test_impact_post_step()
# 6. Generate the O3DE find file and setup install locations for scripts, tools, assets etc., required by the engine
if(NOT INSTALLED_ENGINE)
    ly_setup_o3de_install()

    # IMPORTANT: must be included last
    include(cmake/CPack.cmake)
endif()
