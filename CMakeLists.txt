#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Cmake version 3.20 is the minimum version needed for all of Open 3D Engine's supported platforms
cmake_minimum_required(VERSION 3.20)

include(cmake/LySet.cmake)
include(cmake/Version.cmake)
include(cmake/OutputDirectory.cmake)

if(NOT PROJECT_NAME)
    project(O3DE
        LANGUAGES C CXX
        VERSION ${LY_VERSION_STRING}
    )
endif()

################################################################################
# Initialize
################################################################################
include(cmake/GeneralSettings.cmake)
include(cmake/FileUtil.cmake)
include(cmake/PAL.cmake)
include(cmake/PALTools.cmake)
include(cmake/RuntimeDependencies.cmake)
include(cmake/Configurations.cmake) # Requires to be after PAL so we get platform variable definitions
include(cmake/Dependencies.cmake)
include(cmake/Deployment.cmake)
include(cmake/3rdParty.cmake)
include(cmake/LYPython.cmake)
include(cmake/Install.cmake)
include(cmake/LYWrappers.cmake)
include(cmake/Gems.cmake)
include(cmake/UnitTest.cmake)
include(cmake/LYTestWrappers.cmake)
include(cmake/Monolithic.cmake)
include(cmake/SettingsRegistry.cmake)
include(cmake/TestImpactFramework/LYTestImpactFramework.cmake)
include(cmake/CMakeFiles.cmake)
include(cmake/O3DEJson.cmake)

################################################################################
# Subdirectory processing
################################################################################

function(add_engine_json_external_subdirectories)
    read_json_external_subdirs(external_subdis ${LY_ROOT_FOLDER}/engine.json)
    foreach(external_subdir ${external_subdis})
        file(REAL_PATH ${external_subdir} real_external_subdir BASE_DIRECTORY ${LY_ROOT_FOLDER})
        list(APPEND engine_external_subdirs ${real_external_subdir})
    endforeach()

    set_property(GLOBAL APPEND PROPERTY LY_EXTERNAL_SUBDIRS ${engine_external_subdirs})
endfunction()

# Add the projects first so the Launcher can find them
include(cmake/Projects.cmake)

if(NOT INSTALLED_ENGINE)
    
    # Add the rest of the targets
    add_subdirectory(Assets)
    add_subdirectory(Code)
    add_subdirectory(python)
    add_subdirectory(Registry)
    add_subdirectory(scripts)
    add_subdirectory(Templates)
    add_subdirectory(Tools)

    # Add external subdirectories listed in the engine.json. LY_EXTERNAL_SUBDIRS is a cache variable so the user can add extra
    # external subdirectories
    add_engine_json_external_subdirectories()
else()
    ly_find_o3de_packages()
endif()

get_property(external_subdirs GLOBAL PROPERTY LY_EXTERNAL_SUBDIRS)
list(APPEND LY_EXTERNAL_SUBDIRS ${external_subdirs})

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
    add_subdirectory(${external_directory} ${CMAKE_BINARY_DIR}/External/${directory_name}-${full_directory_hash})
endforeach()

################################################################################
# Post-processing
################################################################################
# The following steps have to be done after all targets are registered:

# 1. Add any dependencies registered via ly_enable_gems
ly_enable_gems_delayed()

# 2. Defer generation of the StaticModules.inl file which is needed to create the AZ::Module derived class in monolithic
#    builds until after all the targets are known and all the gems are enabled
ly_delayed_generate_static_modules_inl()

# 3. generate a settings registry .setreg file for all ly_add_project_dependencies() and ly_add_target_dependencies() calls
#    to provide applications with the filenames of gem modules to load
#    This must be done before ly_delayed_target_link_libraries() as that inserts BUILD_DEPENDENCIES as MANUALLY_ADDED_DEPENDENCIES
#    if the build dependency is a MODULE_LIBRARY. That would cause a false load dependency to be generated
ly_delayed_generate_settings_registry()

# 4. link targets where the dependency was yet not declared, we need to have the declaration so we do different
#    linking logic depending on the type of target
ly_delayed_target_link_libraries()

# 5. generate a registry file for unit testing for platforms that support unit testing
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_delayed_generate_unit_test_module_registry()
endif()

# 5. inject runtime dependencies to the targets. We need to do this after (1) since we are going to walk through
#    the dependencies
ly_delayed_generate_runtime_dependencies()

# 6. Perform test impact framework post steps once all of the targets have been enumerated
ly_test_impact_post_step()

# 7. Generate the O3DE find file and setup install locations for scripts, tools, assets etc., required by the engine
if(LY_INSTALL_ENABLED)
    # 8. Generate the O3DE find file and setup install locations for scripts, tools, assets etc., required by the engine
    ly_setup_o3de_install()
    # 9. CPack information (to be included after install)
    include(cmake/Packaging.cmake)
endif()
