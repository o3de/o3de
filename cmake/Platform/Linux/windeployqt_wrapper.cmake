#!/usr/bin/cmake -P

#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

# We use windeployqt on Linux to copy the necessary Qt libraries and files to
# the build directory. After these files are copied, we need to adjust their
# rpath to point to Qt from the build tree.

# This script is invoked through the CommandExecution.cmake script. The invoked
# commandline is something like:
# cmake -P CommandExecution.cmake EXEC_COMMAND cmake -E env ... cmake -P windeployqt_wrapper.cmake args
#   ^ 1                                          ^ 2              ^ 3
# cmake #1 invokes cmake #2 which invokes cmake #3. But cmake #1 also sees 2 -P
# arguments, so cmake #1 also invokes windeployqt_wrapper.cmake after
# CommandExecution.cmake finishes. We don't want to run this script 2x, so
# detect this case and exit early

# Find the first -P argument
foreach(argi RANGE 1 ${CMAKE_ARGC})
    if("${CMAKE_ARGV${argi}}" STREQUAL "-P")
        math(EXPR script_argi "${argi} + 1")
        set(script_file "${CMAKE_ARGV${script_argi}}")
        break()
    endif()
endforeach()

if(NOT script_file STREQUAL "${CMAKE_CURRENT_LIST_FILE}")
    return()
endif()

set(runtime_directory ${CMAKE_ARGV3})

foreach(argi RANGE 4 ${CMAKE_ARGC})
    list(APPEND command_arguments "${CMAKE_ARGV${argi}}")
endforeach()
execute_process(COMMAND ${command_arguments} RESULT_VARIABLE command_result OUTPUT_VARIABLE command_output ERROR_VARIABLE command_err)

if(command_result)
    message(FATAL_ERROR "windeployqt returned a non-zero exit status. stdout: ${command_output} stderr: ${command_err}")
endif()

# Process the output to find the list of files that were updated

# Transform the output to a list
string(REGEX REPLACE ";" "\\\\;" command_output "${command_output}")
string(REGEX REPLACE "\n" ";" command_output "${command_output}")

foreach(line IN LISTS command_output)
    # windeployqt has output that looks like this if it updated a file:
    # > Checking /path/to/src/file.so, /path/to/dst/file.so
    # > Updating file.so
    # If the file was not modified, it will look like this:
    # > Checking /path/to/src/file.so, /path/to/dst/file.so
    # > file.so is up to date
    if(line MATCHES "^Checking .*")
        set(curfile "${line}")
        continue()
    endif()

    if(line MATCHES "^Updating .*")
        # curline has 3 parts, 1) "Checking ", 2) source_file, 3) updated_target_file. We
        # just need part 3. But we also want to handle the unfortunate
        # possibility of spaces in the filename.

        string(REGEX REPLACE "^Checking " "" curfile "${curfile}")
        string(REPLACE ", " ";" curfile "${curfile}")
        list(LENGTH curfile curfile_parts_count)
        if(NOT curfile_parts_count EQUAL 2)
            message(SEND_ERROR "Unable to parse output of windeployqt output line ${curfile}")
            continue()
        endif()
        list(GET curfile 1 updated_file)

        file(RELATIVE_PATH relative_file "${runtime_directory}" "${updated_file}")

        get_filename_component(basename "${relative_file}" NAME)
        if(basename MATCHES "^libQt5Core\\.so.*")
            # We don't need to patch QtCore
            continue()
        endif()

        # READ_ELF has a CAPTURE_ERROR argument, but that is only set on
        # platforms that don't support cmake's elf parser. On linux, no error
        # will be set, even when the input is not an ELF formatted file. We
        # want to skip any non-executable files, so check for the ELF tag at
        # the head of the file.
        file(READ "${updated_file}" elf_tag LIMIT 4 HEX)
        if(NOT elf_tag STREQUAL 7f454c46) # Binary \0x7f followed by ELF
            continue()
        endif()

        # READ_ELF is an undocumented command that allows us to introspect the
        # current rpath set in the file
        file(READ_ELF "${updated_file}" RUNPATH plugin_runpath)

        get_filename_component(dirname "${relative_file}" DIRECTORY)
        if(dirname)
            file(RELATIVE_PATH parent_dirs "${updated_file}" "${runtime_directory}")
            string(REGEX REPLACE "/../$" "" parent_dirs "${parent_dirs}")
            set(new_runpath "\$ORIGIN/${parent_dirs}")
        else()
            set(new_runpath "\$ORIGIN")
        endif()

        # RPATH_CHANGE is an undocumented command that allows for replacing an
        # existing rpath entry with a new value, as long as the new value's
        # strlen is <= the current rpath
        file(RPATH_CHANGE FILE "${updated_file}" OLD_RPATH "${plugin_runpath}" NEW_RPATH "${new_runpath}")

        unset(curfile)
    endif()
endforeach()
