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
    set(full_target_directory "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/${target_directory}")
    file(COPY "${source_file}" DESTINATION "${full_target_directory}" FILE_PERMISSIONS @LY_COPY_PERMISSIONS@ FOLLOW_SYMLINK_CHAIN)
    cmake_path(NORMAL_PATH full_target_directory)
    get_filename_component(target_filename "${source_file}" NAME)
    get_filename_component(target_filename_ext "${source_file}" LAST_EXT)
    if("${target_filename_ext}" STREQUAL ".so")
        if("${source_file}" MATCHES "qt/plugins")
            file(RPATH_CHANGE FILE "${full_target_directory}/${target_filename}" OLD_RPATH "\$ORIGIN/../../lib" NEW_RPATH "\$ORIGIN/..")
        endif()
        execute_process(COMMAND @CMAKE_STRIP@ "${full_target_directory}/${target_filename}")
    elseif("${source_file}" MATCHES "lrelease")
        file(RPATH_CHANGE FILE "${full_target_directory}/${target_filename}" OLD_RPATH "\$ORIGIN/../lib" NEW_RPATH "\$ORIGIN")
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
