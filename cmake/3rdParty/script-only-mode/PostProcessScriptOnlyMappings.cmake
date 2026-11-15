#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# this file is used during generation of the installer layout.
# note that ARGV0 is the cmake command itself, and ARGV1 is the -P command line option, so we skip them
# ARGV2 is the name of this script.
# ARGV3 is the output folder name
# ARGV4 and onwards are the names of input files, generated during the engine configure/generate
# containing information about the 3p libraries available.

#[[ Each lowercase input file contains calls for all referenced target spellings:
    o3de_process_script_only_target(target_name "source_paths" "destination_paths")

    Generate one mapping with the same filename, preserving each exact target name
    and its runtime copy dependencies for script-only builds.
]]#

if (${CMAKE_ARGC} LESS 5)
    message(FATAL_ERROR "This command was invoked incorrectly.\nUsage:\n"
                             "cmake -P (SCRIPT_NAME) (OUTPUT_DIR) (INPUT FILE) (INPUT FILE) ...")
    return()
endif()

set(output_directory ${CMAKE_ARGV3})

if (NOT output_directory)
    message(FATAL_ERROR "Expected the first argument to be an output directory")
endif()

foreach(index RANGE 4 ${CMAKE_ARGC})
    if (CMAKE_ARGV${index})
        list(APPEND file_list ${CMAKE_ARGV${index}})
    endif()
endforeach()


set(copyright_and_preamble [[#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Generated as part of the install command, this file contains calls to register 3p libraries
# for script-only mode, to ensure that their dependent files are copied to the final location
# during game deploy.  It should ONLY be used in script-only mode.
]])

function(o3de_process_script_only_target target_name copy_dependencies_source_file_path copy_dependencies_target_rel_path)
    unset(trimmed_source_paths)

    if (NOT target_name)
        message(FATAL_ERROR "Error parsing data file ${data_file} - consider cleaning it out and rebuilding.")
    endif()

    foreach(abs_filepath IN LISTS copy_dependencies_source_file_path)
        cmake_path(GET abs_filepath FILENAME filename_only)
        list(APPEND trimmed_source_paths ${filename_only})
    endforeach()

    # sanity check
    if (trimmed_source_paths OR copy_dependencies_target_rel_path)
        list(LENGTH trimmed_source_paths source_list_length)
        list(LENGTH copy_dependencies_target_rel_path relpath_list_lenth)
        if (NOT source_list_length EQUAL relpath_list_lenth)
            message(FATAL_ERROR "Programmer error - for target ${target_name}, lists are supposed to be same length, but\n\
                                ${source_list_length} source and ${relpath_list_lenth} relpath\n\
                                source:'${trimmed_source_paths}'\n\
                                relpath: '${copy_dependencies_target_rel_path}'")
        endif()
    endif()

    string(APPEND final_data "if (NOT TARGET ${target_name})\n")
    string(APPEND final_data "    add_library(${target_name} INTERFACE IMPORTED GLOBAL)\n")

    # binary_folder can be a genex but will always be the Default folder since monolithic
    # cannot be combined with script-only
    set(installer_binaries [[${LY_ROOT_FOLDER}/bin/${PAL_PLATFORM_NAME}/$<CONFIG>/Default]])
    if (trimmed_source_paths)
        foreach(file_name relative_path IN ZIP_LISTS trimmed_source_paths copy_dependencies_target_rel_path)
            if (relative_path)
                set(file_name "${installer_binaries}/${relative_path}/${file_name}")
            else()
                set(relative_path "")
                set(file_name "${installer_binaries}/${file_name}")
            endif()
            string(APPEND final_data 
                    "    ly_add_target_files(TARGETS ${target_name}\n\
                        FILES\n\
                            \"${file_name}\"\n\
                        OUTPUT_SUBDIRECTORY \"${relative_path}\")\n\n")
        endforeach()        
    endif()
    string(APPEND final_data "endif() # NOT TARGET ${target_name}\n")

    set(final_data "${final_data}" PARENT_SCOPE)
    set(processed_target TRUE PARENT_SCOPE)
endfunction()

foreach(data_file IN LISTS file_list)
    set(final_data "${copyright_and_preamble}")
    unset(processed_target)
    include("${data_file}")
    if(NOT processed_target)
        message(FATAL_ERROR "No target records in data file ${data_file}")
    endif()
    cmake_path(GET data_file FILENAME output_filename)
    file(WRITE "${output_directory}/${output_filename}" "${final_data}")
endforeach()
