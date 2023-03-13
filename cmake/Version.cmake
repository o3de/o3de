#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

include_guard()

string(TIMESTAMP current_year "%Y")
set(O3DE_COPYRIGHT_YEAR ${current_year} CACHE STRING "Open 3D Engine's copyright year")

# avoid reading engine.json multiple times 
ly_file_read("${LY_ROOT_FOLDER}/engine.json" tmp_json_data)
set_property(GLOBAL PROPERTY O3DE_ENGINE_JSON_DATA ${tmp_json_data})
unset(tmp_json_data)

#! o3de_get_name_and_version_specifier: Parse the dependency name, and optional version 
# operator and version from the supplied input string. 
# \arg:input - the input string e.g. o3de==1.0.0
# \arg:dependency_name - the name to the left of any optional version specifier
# \arg:operator - the version specifier operator e.g. == 
# \arg:version - the version specifier version e.g. 1.0.0
function(o3de_get_name_and_version_specifier input dependency_name operator version)
    if("${input}" MATCHES "^(.*)(~=|==|!=|<=|>=|<|>|===)(.*)$")
        if(${CMAKE_MATCH_COUNT} GREATER_EQUAL 1)
            string(STRIP ${CMAKE_MATCH_1} _dependency_name)
            set(${dependency_name} ${_dependency_name} PARENT_SCOPE)
        endif()
        if(${CMAKE_MATCH_COUNT} GREATER_EQUAL 2)
            set(${operator} ${CMAKE_MATCH_2} PARENT_SCOPE)
        endif()
        if(${CMAKE_MATCH_COUNT} GREATER_EQUAL 3)
            string(STRIP ${CMAKE_MATCH_3} _version)
            set(${version} ${_version} PARENT_SCOPE)
        endif()
    else()
        # format unknown, assume it's just the dependency name
        set(${dependency_name} ${input} PARENT_SCOPE)
    endif()
endfunction()

#! o3de_get_version_compatible: Check if the input version is compatible based on
# the operator and specifier version provided
# \arg:version - input version to check  e.g. 1.0.0
# \arg:op - the version specifier operator e.g. ==
# \arg:specifier_version - the version part of the version specifier  e.g. 1.2.0
# \arg:is_compatible - TRUE if version is compatible otherwise FALSE 
function(o3de_get_version_compatible version op specifier_version is_compatible)
    set(contains_version FALSE)

    if(op STREQUAL "==" AND version VERSION_EQUAL specifier_version)
        set(contains_version TRUE)
    elseif(op STREQUAL "!=" AND NOT version VERSION_EQUAL specifier_version)
        set(contains_version TRUE)
    elseif(op STREQUAL "<=" AND version VERSION_LESS_EQUAL specifier_version)
        set(contains_version TRUE)
    elseif(op STREQUAL ">=" AND version VERSION_GREATER_EQUAL specifier_version)
        set(contains_version TRUE)
    elseif(op STREQUAL "<" AND version VERSION_LESS specifier_version)
        set(contains_version TRUE)
    elseif(op STREQUAL ">" AND version VERSION_GREATER specifier_version)
        set(contains_version TRUE)
    elseif(op STREQUAL "===" AND version STREQUAL specifier_version)
        set(contains_version TRUE)
    elseif(op STREQUAL "~=")
        # compatible versions have an equivalent combination of >= and == 
        # e.g. ~=2.2 is equivalent to >=2.2,==2.*
        if(version VERSION_GREATER_EQUAL specifier_version)
            string(REPLACE "." ";" specifier_version_part_list ${specifier_version})
            list(LENGTH specifier_version_part_list list_length)
            if(list_length LESS 2)
                # truncating would leave nothing to compare 
                set(contains_version TRUE)
            else()
                # trim the last version part because CMake doesn't support '*'
                math(EXPR truncated_length "${list_length} - 1")
                list(SUBLIST specifier_version_part_list 0 ${truncated_length} specifier_version)
                string(REPLACE ";" "." specifier_version "${specifier_version}")
                string(REPLACE "." ";" version_part_list ${version})
                list(SUBLIST version_part_list 0 ${truncated_length} version)
                string(REPLACE ";" "." version "${version}")

                # compare the truncated versions
                if(version VERSION_EQUAL specifier_version)
                    set(contains_version TRUE)
                endif()
            endif()
        endif()
    endif()

    set(${is_compatible} ${contains_version} PARENT_SCOPE)
endfunction()

#! o3de_read_engine_default: Read a field from engine.json or use the default if not found
# \arg:output_value - name of output variable to set 
# \arg:key - name of field in engine.json 
# \arg:default_value - value to use if neither environment var or engine.json value found 
macro(o3de_read_engine_default output_value key default_value)
    get_property(engine_json_data GLOBAL PROPERTY O3DE_ENGINE_JSON_DATA)
    string(JSON tmp_value ERROR_VARIABLE manifest_json_error GET ${engine_json_data} ${key})

    # unset engine_json_data because we're in a macro
    unset(engine_json_data)

    if(manifest_json_error)
        message(WARNING "Failed to read ${key} from \"${LY_ROOT_FOLDER}/engine.json\" : ${manifest_json_error}")
        set(tmp_value ${default_value})
    endif()

    ly_set(${output_value} ${tmp_value})
    set_property(GLOBAL PROPERTY ${output_value} ${tmp_value})

    # unset tmp_value because we're in a macro
    unset(tmp_value)
endmacro()

#! o3de_set_major_minor_patch_with_prefix: Parses a SemVer string and sets
#  separate major, minor and patch global properties.  
#  e.g. given "1.2.3" and prefix VER the following properties are set:
#   VER_MAJOR 1
#   VER_MINOR 2
#   VER_PATCH 3
#
# \arg:prefix - prefix for cmake variable names 
# \arg:version_string - input string in SemVer format e.g. "1.2.3"
function(o3de_set_major_minor_patch_with_prefix prefix version_string)
    string(REPLACE "." ";" version_list ${version_string})
    list(GET version_list 0 major)
    list(GET version_list 1 minor)
    list(GET version_list 2 patch)
    set_property(GLOBAL PROPERTY ${prefix}_MAJOR ${major})
    set_property(GLOBAL PROPERTY ${prefix}_MINOR ${minor})
    set_property(GLOBAL PROPERTY ${prefix}_PATCH ${patch})
endfunction()

#! o3de_get_major_minor_patch_with_prefix: Get the major, minor and patch 
#  for global properties with the specified prefix and store the output
#  in the provided variables.
#
#  Example storing O3DE_VERSION_MAJOR in major, O3DE_VERSION_MINOR in minor
#  and O3DE_VERSION_PATCH in patch and print the SemVer:
#
#    o3de_get_major_minor_patch_with_prefix(O3DE_VERSION major minor patch) 
#    message(INFO "O3DE version is ${major}.${minor}.${patch}")
#
# \arg:prefix - global properties prefix
# \arg:output_major - output for <prefix>_MAJOR property
# \arg:output_minor - output for <prefix>_MINOR property 
# \arg:output_patch - output for <prefix>_PATCH property 
macro(o3de_get_major_minor_patch_with_prefix prefix output_major output_minor output_patch)
    get_property(${output_major} GLOBAL PROPERTY ${prefix}_MAJOR )
    get_property(${output_minor} GLOBAL PROPERTY ${prefix}_MINOR )
    get_property(${output_patch} GLOBAL PROPERTY ${prefix}_PATCH )
endmacro()

# set engine.json variables and global properties 
# these are not cached variables and cannot be configured by the user
o3de_read_engine_default(O3DE_VERSION_STRING "version" "0.0.0")
o3de_read_engine_default(O3DE_DISPLAY_VERSION_STRING "display_version" "00.00")
o3de_read_engine_default(O3DE_BUILD_VERSION "build" 0)
o3de_read_engine_default(O3DE_ENGINE_NAME "engine_name" "o3de")

# set O3DE_VERSION_MAJOR/MINOR/PATCH global properties
o3de_set_major_minor_patch_with_prefix(O3DE_VERSION ${O3DE_VERSION_STRING})

# set variables for INSTALL targets
# these ARE cached variables and can be configured by the user
o3de_set_from_env_with_default(O3DE_INSTALL_VERSION_STRING O3DE_INSTALL_VERSION "@O3DE_VERSION_STRING@" CACHE STRING "Open 3D Engine's version for the INSTALL target")
o3de_set_from_env_with_default(O3DE_INSTALL_DISPLAY_VERSION_STRING O3DE_INSTALL_DISPLAY_VERSION "@O3DE_DISPLAY_VERSION_STRING@" CACHE STRING "Open 3D Engine's display version for the INSTALL target")
o3de_set_from_env_with_default(O3DE_INSTALL_BUILD_VERSION O3DE_INSTALL_BUILD_VERSION "@O3DE_BUILD_VERSION@" CACHE STRING "Open 3D Engine's build number for the INSTALL target")
o3de_set_from_env_with_default(O3DE_INSTALL_ENGINE_NAME O3DE_INSTALL_ENGINE_NAME "@O3DE_ENGINE_NAME@" CACHE STRING "Open 3D Engine's engine name for the INSTALL target")
