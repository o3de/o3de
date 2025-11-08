#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Always start by checking if the target already exists.
# This prevents repeated calls but also allows the user to substitute their own 3rd party library
# if they wish to do so.

if (TARGET 3rdParty::OpenMesh)
    return()
endif()

# Variables inside a local function are scoped to the function body.
# Putting all of this inside a function lets us basically ensure that any variables set by the 
# external 3rdParty CMake file do not have any effect on the outside world.
# and allows us not to have to save and restore anything.

function(GetOpenMesh)
    # Part 1:  Where do you get the library from?  Make sure to inform the user of the source of the library and any patches applied.
    include(FetchContent)

    set(OPENMESH_GIT_REPO "https://github.com/o3de/openmesh.git")
    set(OPENMESH_GIT_TAG "8078b5bd01b568e61c6b8dd84af8673e7d9e6cae")

    # Note that the upstream branch is o3de_openmesh_changes and is only the change of the submodule URL from the original
    # and is the following diff in full:
    #     diff --git a/.gitmodules b/.gitmodules
    #     index 6681eef9..18700afb 100644
    #     --- a/.gitmodules
    #     +++ b/.gitmodules
    #     @@ -1,3 +1,3 @@
    #     [submodule "cmake-library"]
    #             path = cmake-library
    #     -       url = ../../cmake/cmake-library
    #     +       url = ../openmesh-cmake-library

    # this is because we need to mirror openmesh due to firewall issues in some regions, specifically, port 9000
    # is blocked by some university and corporate firewalls, and the original gitlab server uses that port.
    # However, the original source requires a submodule located at a path relative to the original repo at
    # ../../cmake/cmake-library, ie the actual repository name is "cmake/cmake-library"
    # github does not support using slashes in the name of a repository.  So we have to mirror it at "openmesh-cmake-library"
    # and update the url in the .gitmodules file to point to the new location.

    # Note that our actual usage of OpenMesh in O3DE is only inside one specific editor shared library (.so, .dll).
    # It is not used or exposed in any other way.  Because it is not used in runtime game engine code, and it is not
    # exposed via any public API, it is not necessary to include its library or header files in the installer version
    # of O3DE.  (Shared Libraries always package all the code they use inside themselves).
    # As such, we add EXCLUDE_FROM_ALL to prevent it from trying to copy its outputs into the install layout.  If
    # we needed files from it, we would have still used EXCLUDE_FROM_ALL since its install locations would be incorrect
    # for O3De (we build multiple different flavors into different folders).
    FetchContent_Declare(
            OpenMesh
            GIT_REPOSITORY ${OPENMESH_GIT_REPO}
            GIT_TAG ${OPENMESH_GIT_TAG}
            GIT_SHALLOW
            EXCLUDE_FROM_ALL # Prevent it from executing 'install' ops, it doesn't need to be included in installer
    )

    # please always be really clear about what third parties your gem uses.
    message(STATUS "WhiteBox Gem uses OpenMesh 11.0 (BSD-3-Clause) ${OPENMESH_GIT_REPO}")
    message(STATUS "    - This is a mirror of: https://gitlab.vci.rwth-aachen.de:9000/OpenMesh/OpenMesh.git")

    # Part 2: Set the build settings and trigger the actual execution of the downloaded CMakeLists.txt file

    # Note that CMAKE_ARGS does NOT WORK for FetchContent_*, only ExternalProject.
    # Thus, you must set any configuration settings here, in the scope in which you call FetchContent_MakeAvailable.

    # These settings will be applied only to the current CMake scope - so it is only worth saving and restoring values from settings
    # that may affect other targets in the same CMake scope, most likely anything "CMAKE_xxxxxxxx".
    set(OLD_LOG_LEVEL ${CMAKE_MESSAGE_LOG_LEVEL}) # save the old CMAKE_MESSAGE_LOG_LEVEL
    set(CMAKE_MESSAGE_LOG_LEVEL ${O3DE_FETCHCONTENT_MESSAGE_LEVEL})
    set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)

    # Backup and set the developer warning suppression for OpenMesh to suppress a expected warning
    set(ORIGINAL_CMAKE_SUPPRESS_DEVELOPER_WARNINGS ${CMAKE_SUPPRESS_DEVELOPER_WARNINGS})
    set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON CACHE BOOL "" FORCE)

    # The rest of these are all specific settings that come from OpenMesh's CMakeLists.txt files.
    set(OPENMESH_BUILD_SHARED OFF)
    set(OPENMESH_DOCS OFF)
    set(OPENMESH_BUILD_UNIT_TESTS OFF)
    set(DISABLE_QMAKE_BUILD ON)
    set(BUILD_APPS OFF)
    set(VCI_NO_LIBRARY_INSTALL ON)
    set(VCI_COMMON_DO_NOT_COPY_POST_BUILD ON)

    # the below line is what actualy runs its CMakeList.txt file and executes targets and so on:
    FetchContent_MakeAvailable(OpenMesh)

    set(CMAKE_MESSAGE_LOG_LEVEL ${OLD_LOG_LEVEL})
    set(CMAKE_WARN_DEPRECATED ON CACHE BOOL "" FORCE)

    # Part 3: alias any targets to 3rdParty::xxx, make sure any O3DE-specific fixups are done on the target compile options
    # generated by the above FetchContent_MakeAvailable call.  Generally, disable things like warnings-as-errors since we 
    # can't really guarantee that the 3rd party library will compile cleanly with our settings.
    set(OPENMESH_TARGETS OpenMeshCore OpenMeshTools OpenMeshCoreStatic OpenMeshToolsStatic)
    foreach(OpenMesh_Target ${OPENMESH_TARGETS})
        # OpenMesh Specific: on non-Windows platforms, it creates both a shared and a static library, and assigns them targetname
        # xxxxxxxx and xxxxxxxxStatic.  On Windows platforms, it only makes a static library, and it is called just the targetname
        # without appending Static on it.  We need to handle both cases here, so we have a "IF (NOT TARGET)" check.
        if (NOT TARGET ${OpenMesh_Target})
            continue()
        endif()
        # customize the compile options for the target, note that these defines already contain the 
        # appropriate level of visibility (PUBLIC / INTERFACE / PRIVATE) and may be blank for a given compiler.
        target_compile_options(${OpenMesh_Target}  
            ${O3DE_COMPILE_OPTION_DISABLE_WARNINGS}     # OpenMesh does not pass Warning Level 4
            ${O3DE_COMPILE_OPTION_EXPORT_SYMBOLS}       # OpenMesh requires symbols to be exported.
            ${O3DE_COMPILE_OPTION_ENABLE_EXCEPTIONS})   # OpenMesh requires Exceptions enabled in it, and things that use it

        target_compile_definitions(${OpenMesh_Target}
            PRIVATE WIN32_LEAN_AND_MEAN)                # Fix compile errors in Windows kit header files
                
        # this block makes sure that in IDEs like Visual Studio, they show up in the "External" subfolder under the gem.
        get_property(this_gem_root GLOBAL PROPERTY "@GEMROOT:${gem_name}@")
        ly_get_engine_relative_source_dir(${this_gem_root} relative_this_gem_root)
        set_property(TARGET ${OpenMesh_Target} PROPERTY FOLDER "${relative_this_gem_root}/External")
    endforeach()

    # OpenMesh Specific: If the xxxxxxxStatic library exists, prefer using them over the xxxxxxx (without Static) library.
    set(OpenMesh_Primary_Target      OpenMeshCore)
    set(OpenMeshTools_Primary_Target OpenMeshTools)
    if (TARGET OpenMeshCoreStatic)
        set(OpenMesh_Primary_Target OpenMeshCoreStatic)
        set(OpenMeshTools_Primary_Target OpenMeshToolsStatic)
    endif()

    # ly_create_alias is a helper function that creates an alias target with the given name and namespace
    # However, it creates both the 3rdParty::xxxxxx and the xxxxxxx target.  
    # Most importantly, it registers the alias with the installer generator, which will try to recreate the
    # alias and target automatically for us, in the installer, which we don't want to do, since we're providing our
    # own FindOpenMesh.cmake for the installer to use.  So avoid using ly_create_alias when you have a custom find file.
    add_library(3rdParty::OpenMesh ALIAS ${OpenMesh_Primary_Target})
    add_library(3rdParty::OpenMeshTools ALIAS OpenMeshTools)

    # Part 4: Make sure things work in the Installer version of O3DE. 
    # To make it simple, we just have a premade FindOpenMesh.cmake for the installer specifically
    # that we put in a folder (cmake/3rdParty) that is already part of the search path for find_package calls in installers.
    ly_install(FILES ${CMAKE_CURRENT_LIST_DIR}/Installer/FindOpenMesh.cmake DESTINATION cmake/3rdParty)
    
    # signal that find_package(OpenMesh) has succeeded.
    # we have to set it on the PARENT_SCOPE since we're in a function 
    set(OpenMesh_FOUND TRUE PARENT_SCOPE)

    # Restore the original suppression flags that were forced as part of OpenMesh
    set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ${ORIGINAL_CMAKE_SUPPRESS_DEVELOPER_WARNINGS} CACHE BOOL "" FORCE)

endfunction()

GetOpenMesh()

# for extra safety, we'll remove the function from the global scope, so that it can't be called again.
unset(GetOpenMesh)
