#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

#! ly_setup_runtime_dependencies_copy_function_override: Linux-specific copy function to handle RPATH fixes
set(ly_copy_template [[
function(ly_copy source_file target_directory)
    cmake_path(GET source_file FILENAME target_filename)
    cmake_path(APPEND full_target_directory "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}" "${target_directory}")
    cmake_path(APPEND target_file "${full_target_directory}" "${target_filename}")
    if("${source_file}" IS_NEWER_THAN "${target_file}")
        message(STATUS "Copying ${source_file} to ${full_target_directory}...")
        file(MAKE_DIRECTORY "${full_target_directory}")
        file(COPY "${source_file}" DESTINATION "${full_target_directory}" FILE_PERMISSIONS @LY_COPY_PERMISSIONS@ FOLLOW_SYMLINK_CHAIN)
        file(TOUCH_NOCREATE "${target_file}")
        
        # Special case for install
        cmake_PATH(GET source_file EXTENSION target_filename_ext)
        if("${target_filename_ext}" STREQUAL ".so")
            if("${source_file}" MATCHES "qt/plugins")
                file(RPATH_CHANGE FILE "${target_file}" OLD_RPATH "\$ORIGIN/../../lib" NEW_RPATH "\$ORIGIN/..")
            endif()
            if(CMAKE_INSTALL_DO_STRIP)
                execute_process(COMMAND @CMAKE_STRIP@ "${target_file}")
            endif()
        elseif("${source_file}" MATCHES "lrelease")
            file(RPATH_CHANGE FILE "${target_file}" OLD_RPATH "\$ORIGIN/../lib" NEW_RPATH "\$ORIGIN")
        endif()
    endif()
endfunction()]])

function(ly_setup_runtime_dependencies_copy_function_override)
    string(CONFIGURE "${ly_copy_template}" ly_copy_function_linux @ONLY)
    foreach(conf IN LISTS CMAKE_CONFIGURATION_TYPES)
        string(TOUPPER ${conf} UCONF)
        ly_install(CODE "${ly_copy_function_linux}" 
            COMPONENT  ${LY_INSTALL_PERMUTATION_COMPONENT}_${UCONF}
        )
    endforeach()
endfunction()

include(cmake/Platform/Common/Install_common.cmake)
