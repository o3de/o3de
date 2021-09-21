#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_set(LY_INSTALL_ENABLED TRUE)
if(INSTALLED_ENGINE)
    ly_set(LY_INSTALL_ENABLED FALSE)
endif()

if(LY_INSTALL_ENABLED)
    ly_get_absolute_pal_filename(pal_dir ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Platform/${PAL_PLATFORM_NAME})
    include(${pal_dir}/Install_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)
endif()

#! ly_install_directory: specifies a directory to be copied to the install layout at install time
#
# \arg:DIRECTORIES directories to install
# \arg:DESTINATION (optional) destination to install the directory to (relative to CMAKE_PREFIX_PATH)
# \arg:EXCLUDE_PATTERNS (optional) patterns to exclude
# \arg:VERBATIM (optional) copies the directories as they are, this excludes the default exclude patterns
#
# \notes: 
#  - refer to cmake's install(DIRECTORY documentation for more information
#  - If the directory contains programs/scripts, exclude them from this call and add a specific ly_install_files with 
#      PROGRAMS set. This is necessary to set the proper execution permissions.
#  - This function will automatically filter out __pycache__, *.egg-info, CMakeLists.txt, *.cmake files. If those files
#      need to be installed, use ly_install_files. Use VERBATIM to exclude such filters.
#
function(ly_install_directory)

    if(NOT LY_INSTALL_ENABLED)
        return()
    endif()

    set(options VERBATIM)
    set(oneValueArgs DESTINATION)
    set(multiValueArgs DIRECTORIES EXCLUDE_PATTERNS)

    cmake_parse_arguments(ly_install_directory "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ly_install_directory_DIRECTORIES)
        message(FATAL_ERROR "You must provide at least a directory to install")
    endif()

    foreach(directory ${ly_install_directory_DIRECTORIES})

        cmake_path(ABSOLUTE_PATH directory)

        if(NOT ly_install_directory_DESTINATION)
            # maintain the same structure relative to LY_ROOT_FOLDER
            set(ly_install_directory_DESTINATION ${directory})
            if(${ly_install_directory_DESTINATION} STREQUAL ".")
                set(ly_install_directory_DESTINATION ${CMAKE_CURRENT_LIST_DIR})
            else()
                cmake_path(ABSOLUTE_PATH ly_install_directory_DESTINATION BASE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
            endif()
            # take out the last directory since install asks for the destination of the folder, without including the fodler itself
            cmake_path(GET ly_install_directory_DESTINATION PARENT_PATH ly_install_directory_DESTINATION)
            cmake_path(RELATIVE_PATH ly_install_directory_DESTINATION BASE_DIRECTORY ${LY_ROOT_FOLDER})       
        endif()

        unset(exclude_patterns)
        if(ly_install_directory_EXCLUDE_PATTERNS)
            foreach(exclude_pattern ${ly_install_directory_EXCLUDE_PATTERNS})
                list(APPEND exclude_patterns PATTERN ${exclude_pattern} EXCLUDE)
            endforeach()
        endif()
        
        if(NOT ly_install_directory_VERBATIM)
            # Exclude cmake since that has to be generated
            list(APPEND exclude_patterns PATTERN CMakeLists.txt EXCLUDE)
            list(APPEND exclude_patterns PATTERN *.cmake EXCLUDE)

            # Exclude python-related things that dont need to be installed
            list(APPEND exclude_patterns PATTERN __pycache__ EXCLUDE)
            list(APPEND exclude_patterns PATTERN *.egg-info EXCLUDE)
        endif()

        install(DIRECTORY ${directory}
            DESTINATION ${ly_install_directory_DESTINATION}
            COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME} # use the deafult for the time being
            ${exclude_patterns}
        )
    endforeach()

endfunction()

#! ly_install_files: specifies files to be copied to the install layout at install time
#
# \arg:FILES files to install
# \arg:DESTINATION destination to install the directory to (relative to CMAKE_PREFIX_PATH)
# \arg:PROGRAMS (optional) indicates if the files are programs that should be installed with EXECUTE permissions
#
# \notes: 
#  - refer to cmake's install(FILES/PROGRAMS documentation for more information
#
function(ly_install_files)

    if(NOT LY_INSTALL_ENABLED)
        return()
    endif()

    set(options PROGRAMS)
    set(oneValueArgs DESTINATION)
    set(multiValueArgs FILES)

    cmake_parse_arguments(ly_install_files "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ly_install_files_FILES)
        message(FATAL_ERROR "You must provide a list of files to install")
    endif()
    if(NOT ly_install_files_DESTINATION)
        message(FATAL_ERROR "You must provide a destination to install files to")
    endif()

    unset(files)
    foreach(file ${ly_install_files_FILES})
        cmake_path(ABSOLUTE_PATH file)
        list(APPEND files ${file})
    endforeach()

    if(ly_install_files_PROGRAMS)
        set(install_type PROGRAMS)
    else()
        set(install_type FILES)
    endif()

    install(${install_type} ${files}
        DESTINATION ${ly_install_files_DESTINATION}
        COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME} # use the default for the time being
    )

endfunction()

#! ly_install_run_code: specifies code to be added to the install process (will run at install time)
#
# \notes: 
#  - refer to cmake's install(CODE documentation for more information
#
function(ly_install_run_code CODE)

    if(NOT LY_INSTALL_ENABLED)
        return()
    endif()

    install(CODE ${CODE}
        COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME} # use the default for the time being
    )

endfunction()

#! ly_install_run_script: specifies path to script to be added to the install process (will run at install time)
#
# \notes: 
#  - refer to cmake's install(SCRIPT documentation for more information
#
function(ly_install_run_script SCRIPT)

    if(NOT LY_INSTALL_ENABLED)
        return()
    endif()

    install(SCRIPT ${SCRIPT}
        COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME} # use the default for the time being
    )

endfunction()