#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# This file is copied during engine registration. Edits to this file will be lost next
# time a registration happens.

include_guard()

set(LY_EXTERNAL_SUBDIRS "" CACHE STRING "List of subdirectories to recurse into when running cmake against the engine's CMakeLists.txt")

#! read_engine_external_subdirs
# Read the external subdirectories from the engine.json file
# External subdirectories are any folders with CMakeLists.txt in them
# This could be regular subdirectories, Gems(contains an additional gem.json),
# Restricted folders(contains an additional restricted.json), etc...
# \arg:output_external_subdirs name of output variable to store external subdirectories into
function(read_engine_external_subdirs output_external_subdirs)
    ly_file_read(${LY_ROOT_FOLDER}/engine.json engine_json_data)
    string(JSON external_subdirs_count ERROR_VARIABLE engine_json_error
        LENGTH ${engine_json_data} "external_subdirectories")
    if(engine_json_error)
        message(FATAL_ERROR "Error querying number of elements in JSON array \"external_subdirectories\": ${engine_json_error}")
    endif()

    if(external_subdirs_count GREATER 0)
        math(EXPR external_subdir_range "${external_subdirs_count}-1")
        # Convert the paths the relative paths to absolute paths using the engine root
        # as the base directory
        foreach(external_subdir_index RANGE ${external_subdir_range})
            string(JSON external_subdir ERROR_VARIABLE engine_json_error
                GET ${engine_json_data} "external_subdirectories" "${external_subdir_index}")
            if(engine_json_error)
                message(FATAL_ERROR "Error reading field at index ${external_subdir_index} in \"external_subdirectories\" JSON array: ${engine_json_error}")
            endif()
            file(REAL_PATH ${external_subdir} real_external_subdir BASE_DIRECTORY ${CMAKE_SOURCE_DIR})
            list(APPEND external_subdirs ${real_external_subdir})
        endforeach()
    endif()
    set(${output_external_subdirs} ${external_subdirs} PARENT_SCOPE)
endfunction()
