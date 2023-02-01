# {BEGIN_LICENSE}
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# {END_LICENSE}
# This file is copied during engine registration. Edits to this file will be lost next
# time a registration happens.

include_guard()

function(o3de_version_specifier_contains specifier_comparison specifier_version version result)
    set(${result} FALSE PARENT_SCOPE)
    if(specifier_comparison STREQUAL "~=")
        # Compatible versions have an equivalent combination of >= and ==. That
        # is that ~=2.2 is equivalent to >=2.2,==2.*
        if(version VERSION_GREATER_EQUAL specifier_version)
            string(REPLACE "." ";" specifer_version_part_list ${specifier_version})
            list(LENGTH specifer_version_part_list list_length)
            math(EXPR truncated_length "${list_length} - 1")

            if(truncated_length LESS 1)
                set(${result} TRUE PARENT_SCOPE)
            else()
                # trim the last part of the version because CMake doesn't support * in versions
                list(SUBLIST specifer_version_part_list 0 ${truncated_length} specifier_version)
                string(REPLACE ";" "." specifier_version "${specifier_version}")
                string(REPLACE "." ";" version_part_list ${version})
                list(SUBLIST version_part_list 0 ${truncated_length} version)
                string(REPLACE ";" "." version "${version}")
                # compare the truncated versions
                if(version VERSION_EQUAL specifier_version)
                    set(${result} TRUE PARENT_SCOPE)
                endif()
            endif()
        endif()
    elseif(specifier_comparison STREQUAL "==" AND version VERSION_EQUAL specifier_version)
        set(${result} TRUE PARENT_SCOPE)
    elseif(specifier_comparison STREQUAL "!=" AND NOT version VERSION_EQUAL specifier_version)
        set(${result} TRUE PARENT_SCOPE)
    elseif(specifier_comparison STREQUAL "<=" AND version VERSION_LESS_EQUAL specifier_version)
        set(${result} TRUE PARENT_SCOPE)
    elseif(specifier_comparison STREQUAL ">=" AND version VERSION_GREATER_EQUAL specifier_version)
        set(${result} TRUE PARENT_SCOPE)
    elseif(specifier_comparison STREQUAL "<" AND version VERSION_LESS specifier_version)
        set(${result} TRUE PARENT_SCOPE)
    elseif(specifier_comparison STREQUAL ">" AND version VERSION_GREATER specifier_version)
        set(${result} TRUE PARENT_SCOPE)
    elseif(specifier_comparison STREQUAL "===" AND version VERSION_EQUAL specifier_version)
        set(${result} TRUE PARENT_SCOPE)
    endif()
endfunction()

function(o3de_get_version_specifier_parts input_string object_name comparitor version_specifier)
    # This REGEX match currently only supports a single version specifier 
    if("${input_string}" MATCHES "^(.*)(~=|==|!=|<=|>=|<|>|===)(.*)$")
        if(${CMAKE_MATCH_COUNT} GREATER_EQUAL 1)
            set(${object_name} ${CMAKE_MATCH_1} PARENT_SCOPE)
        endif()
        if(${CMAKE_MATCH_COUNT} GREATER_EQUAL 2)
            set(${comparitor} ${CMAKE_MATCH_2} PARENT_SCOPE)
        endif()
        if(${CMAKE_MATCH_COUNT} GREATER_EQUAL 3)
            set(${version_specifier} ${CMAKE_MATCH_3} PARENT_SCOPE)
        endif()
    else()
        set(${object_name} ${input_string} PARENT_SCOPE)
    endif()
endfunction()

function(o3de_engine_compatible engine_path expected_engine_name version_comparitor version_specifier is_compatible out_engine_version verbose)
    # assume incompatible till proven compatible
    set(${is_compatible} FALSE PARENT_SCOPE)

    file(READ ${engine_path}/engine.json engine_json)
    string(JSON engine_name ERROR_VARIABLE json_error GET ${engine_json} engine_name)
    if(json_error)
        set(warning_msg "Unable to read key 'engine_name' from '${engine_path}/engine.json'\nError: ${json_error}")
    elseif(NOT engine_name STREQUAL expected_engine_name)
        set(warning_msg " Engine name '${engine_name}' found in '${engine_path}/engine.json' does not match expected engine name '${expected_engine_name}'")
    elseif(version_comparitor AND version_specifier)
        string(JSON engine_version ERROR_VARIABLE json_error GET ${engine_json} version)
        if(json_error)
            set(warning_msg "Unable to verify engine compatibility because we could not read key 'version' from '${engine_path}/engine.json'\nError: ${json_error}")
        else()
            set(${out_engine_version} ${engine_version} PARENT_SCOPE)
            o3de_version_specifier_contains(${version_comparitor} ${version_specifier} ${engine_version} version_is_compatible)
            set(${is_compatible} ${version_is_compatible} PARENT_SCOPE)
            if(NOT version_is_compatible)
                set(warning_msg "The engine ${engine_name} version ${engine_version} at ${engine_path} is not compatible with the project's version specifier '${version_comparitor}${version_specifier}'\n Please use the o3de CLI to register the project to a compatible engine or update the project or engine.")
            endif()
        endif()
    else()
        # if the engine name matches and there are no version specifiers, assume compatible
        set(${is_compatible} TRUE PARENT_SCOPE)
    endif()

    if(verbose AND warning_msg)
        message(WARNING ${warning_msg})
    endif()
endfunction()