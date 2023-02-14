#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

include_guard()

function(o3de_version_specifier_contains op specifier_version version result)
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
            string(REPLACE "." ";" specifer_version_part_list ${specifier_version})
            list(LENGTH specifer_version_part_list list_length)
            if(list_length LESS 2)
                # truncating would leave nothing to compare 
                set(contains_version TRUE)
            else()
                # trim the last version part because CMake doesn't support '*'
                math(EXPR truncated_length "${list_length} - 1")
                list(SUBLIST specifer_version_part_list 0 ${truncated_length} specifier_version)
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

    set(${result} ${contains_version} PARENT_SCOPE)
endfunction()

function(o3de_get_version_specifier_parts input_string dependency_name op version)
    # this REGEX match currently only supports a single version specifier 
    if("${input_string}" MATCHES "^(.*)(~=|==|!=|<=|>=|<|>|===)(.*)$")
        if(${CMAKE_MATCH_COUNT} GREATER_EQUAL 1)
            set(${dependency_name} ${CMAKE_MATCH_1} PARENT_SCOPE)
        endif()
        if(${CMAKE_MATCH_COUNT} GREATER_EQUAL 2)
            set(${op} ${CMAKE_MATCH_2} PARENT_SCOPE)
        endif()
        if(${CMAKE_MATCH_COUNT} GREATER_EQUAL 3)
            set(${version} ${CMAKE_MATCH_3} PARENT_SCOPE)
        endif()
    else()
        # format unknown, assume it's just the dependency name
        set(${dependency_name} ${input_string} PARENT_SCOPE)
        set(${op} FALSE PARENT_SCOPE)
        set(${version} FALSE PARENT_SCOPE)
    endif()
endfunction()

macro(o3de_conditional_warning warn message)
    if(warn)
        message(WARNING " ${message}")
    else()
        message(VERBOSE " ${message}")
    endif()
endmacro()

function(o3de_engine_compatible engine_path expected_engine_name version_op version_specifier out_result out_engine_version warn)
    set(is_compatible FALSE)

    file(READ ${engine_path}/engine.json engine_json)
    string(JSON engine_name ERROR_VARIABLE json_error GET ${engine_json} engine_name)
    if(json_error)
        o3de_conditional_warning(warn "Unable to read key 'engine_name' from '${engine_path}/engine.json'\nError: ${json_error}")
    elseif(NOT engine_name STREQUAL expected_engine_name)
        o3de_conditional_warning(warn " Engine name '${engine_name}' found in '${engine_path}/engine.json' does not match expected engine name '${expected_engine_name}'")
    else()
        string(JSON engine_version ERROR_VARIABLE json_error GET ${engine_json} version)
        if(json_error)
            o3de_conditional_warning(warn "Unable to verify engine compatibility because we could not read key 'version' from '${engine_path}/engine.json'\nError: ${json_error}")
        else()
            set(${out_engine_version} ${engine_version} PARENT_SCOPE)

            if(version_op AND version_specifier)
                o3de_version_specifier_contains(${version_op} ${version_specifier} ${engine_version} is_compatible)
                if(NOT is_compatible)
                    o3de_conditional_warning(warn "The engine ${engine_name} version ${engine_version} at ${engine_path} is not compatible with the project's version specifier '${version_op}${version_specifier}'\n Please use the o3de CLI to register the project to a compatible engine or update the project or engine.")
                endif()
            else()
                # assume compatible if the engine name matches and there are no version specifiers
                set(is_compatible TRUE)
            endif()
        endif()
    endif()

    set(${out_result} ${is_compatible} PARENT_SCOPE)
endfunction()
