#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

#! ly_include_cmake_file_list: includes a cmake file list
#
# Includes a .cmake file that contains a variable "FILE" with the list of files
# The function appends the list of files to the variable ALLFILES. Callers of this
# function can include multiple cmake file list files and then use ALLFILES as the
# source for libraries/binaries/etc. Files that need to be excluded from unity builds
# due to possible ODR violations will need to be set in the variable 'SKIP_UNITY_BUILD_INCLUSION_FILES'
# in the same .cmake file (regardless of whether unity builds are enabled or not)
#
# The function takes care of appending the path relative to the cmake list file
#
# \arg:file cmake file list to include
#
function(ly_include_cmake_file_list file)

    set(UNITY_AUTO_EXCLUSIONS)

    include(${file})
    get_filename_component(file_path "${file}" PATH)
    if(file_path)
        list(TRANSFORM FILES PREPEND ${file_path}/)
    endif()
    foreach(f ${FILES})
        get_filename_component(absolute_path ${f} ABSOLUTE)
        if(NOT EXISTS ${absolute_path})
            message(SEND_ERROR "File ${absolute_path} referenced in ${file} not found")
        endif()

        # Automatically exclude any extensions marked for the current platform
        if (PAL_TRAIT_BUILD_UNITY_EXCLUDE_EXTENSIONS)
            get_filename_component(file_extension ${f} LAST_EXT)
            if (${file_extension} IN_LIST PAL_TRAIT_BUILD_UNITY_EXCLUDE_EXTENSIONS)
                list(APPEND UNITY_AUTO_EXCLUSIONS ${f})
            endif()
        endif()

    endforeach()
    list(APPEND FILES ${file}) # Add the _files.cmake to the list so it shows in the IDE

    if(file_path)
        list(TRANSFORM SKIP_UNITY_BUILD_INCLUSION_FILES PREPEND ${file_path}/)
    endif()

    # Check if there are any files to exclude from unity groupings
    foreach(f ${SKIP_UNITY_BUILD_INCLUSION_FILES})
        get_filename_component(absolute_path ${f} ABSOLUTE)
        if(NOT EXISTS ${absolute_path})
            message(FATAL_ERROR "File ${absolute_path} for SKIP_UNITY_BUILD_INCLUSION_FILES referenced in ${file} not found")
        endif()
        if(NOT f IN_LIST FILES)
            list(APPEND FILES ${f})
        endif()
    endforeach()

    # Mark files tagged for unity build group exclusions for unity builds
    if (LY_UNITY_BUILD)
        set_source_files_properties(
            ${UNITY_AUTO_EXCLUSIONS}
            ${SKIP_UNITY_BUILD_INCLUSION_FILES}
            PROPERTIES 
                SKIP_UNITY_BUILD_INCLUSION ON
        )
    endif()

    set(ALLFILES ${ALLFILES} ${FILES} PARENT_SCOPE)

endfunction()

#! ly_source_groups_from_folders: creates source_group for files
#
# Uses the path to the files being passed to create source_gropups for them.
#
# \arg:files list of files to create source_group for
#
function(ly_source_groups_from_folders files)
    foreach(file IN LISTS files)
        if(IS_ABSOLUTE ${file})
            file(RELATIVE_PATH file ${CMAKE_CURRENT_SOURCE_DIR} ${file})
        endif()
        get_filename_component(file_path "${file}" PATH)
        string(REPLACE "/" "\\" file_filters "${file_path}")
        source_group("${file_filters}" FILES "${file}")
    endforeach()
endfunction()

#! ly_update_platform_settings: Function to generate a config-parseable file for cmake-project generation settings
#
# This generate a platform-specific, parseable config file that will live in the build directory specified during
# the cmake project generation. This will provide a bridge to non-cmake tools to read the platform-specific cmake
# project generation settings.
#
get_property(LY_PROJECTS_TARGET_NAME GLOBAL PROPERTY LY_PROJECTS_TARGET_NAME)
function(ly_update_platform_settings)
    # Update the <platform>.last file to keep track of the recent build_dir
    set(ly_platform_last_path "${CMAKE_BINARY_DIR}/platform.settings")
    string(TIMESTAMP ly_last_generation_timestamp "%Y-%m-%dT%H:%M:%S")

    file(WRITE ${ly_platform_last_path} "# Auto Generated from last cmake project generation (${ly_last_generation_timestamp})

[settings]
platform=${PAL_PLATFORM_NAME}
game_projects=${LY_PROJECTS_TARGET_NAME}
asset_deploy_mode=${LY_ASSET_DEPLOY_MODE}
asset_deploy_type=${LY_ASSET_DEPLOY_ASSET_TYPE}
override_pak_root=${LY_OVERRIDE_PAK_FOLDER_ROOT}
")

endfunction()

#! ly_file_read: wrap to file(READ) that adds the file to configuration tracking
#
# file(READ) does not add file tracking. So changes to the file being read will not cause a cmake regeneration
#
function(ly_file_read path content)
    unset(file_content)
    file(READ ${path} file_content)
    set(${content} ${file_content} PARENT_SCOPE)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${path})
endfunction()
