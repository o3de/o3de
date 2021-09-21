#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

#! ly_install_code_function_override: Linux-specific copy function to handle RPATH fixes
set(ly_copy_template [[
function(ly_copy source_file target_directory)
    file(COPY "${source_file}" DESTINATION "${target_directory}" FILE_PERMISSIONS @LY_COPY_PERMISSIONS@ FOLLOW_SYMLINK_CHAIN)
    get_filename_component(target_filename_ext "${source_file}" LAST_EXT)
    if("${source_file}" MATCHES "qt/plugins" AND "${target_filename_ext}" STREQUAL ".so")
        get_filename_component(target_filename "${source_file}" NAME)
        file(RPATH_CHANGE FILE "${target_directory}/${target_filename}" OLD_RPATH "\$ORIGIN/../../lib" NEW_RPATH "\$ORIGIN/..")
    elseif("${source_file}" MATCHES "lrelease")
        get_filename_component(target_filename "${source_file}" NAME)
        file(RPATH_CHANGE FILE "${target_directory}/${target_filename}" OLD_RPATH "\$ORIGIN/../lib" NEW_RPATH "\$ORIGIN")
    endif()
endfunction()]])

function(ly_install_code_function_override)
    string(CONFIGURE "${ly_copy_template}" ly_copy_function_linux @ONLY)
    install(CODE "${ly_copy_function_linux}" 
        COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}
    )
endfunction()

include(cmake/Platform/Common/Install_common.cmake)
