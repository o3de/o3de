#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# CMake utility that provides functionality around command execution
# It provides:
# - ability to avoid the execution altogether by creating a .stamp file and comparing it to the "file reference".
#   Related to https://gitlab.kitware.com/cmake/cmake/issues/18530
# - ability to lock the execution by creating a .lock file of the "file reference"
#
# This script has to be invoked like:
# cmake -DLY_LOCK_FILE=<lock file> -P cmake/CommandExecution.cmake EXEC_COMMAND commandToExecute1 <EXEC_COMMAND commandToExecute2> ...
#   In the above case, all the commands will be locked on <lock file>. Meaning that there could be only one instance executing at the
#   the same time. Since we provide flexibility on what file gets locked, multiple invocations could be done with different commands.
#   The lock file acts as a mutex over a certain "resource" that the caller is trying to control access to.
# cmake -DLY_TIMESTAMP_REFERENCE=<referencefile> [-DLY_TIMESTAMP_FILE=<referencefile>] -P cmake/CommandExecution.cmake EXEC_COMMAND commandToExecute1 <EXEC_COMMAND commandToExecute2> ...
#   In this case, the file's timestamp of LY_TIMESTAMP_REFERENCE will be compared to LY_TIMESTAMP_FILE, if newer, it will execute the commands
#   If LY_TIMESTAMP_FILE is not passed, then ${LY_TIMESTAMP_REFERENCE}.stamp is used
#
# If LY_LOCK_FILE and LY_TIMESTAMP_REFERENCE are not passed, then the commands are executed.
#

# Find the first "EXEC_COMMAND"
set(argiP 0)
foreach(argi RANGE 3 ${CMAKE_ARGC}) # at least skip the command name and the "-P" argument
    if(CMAKE_ARGV${argi} STREQUAL EXEC_COMMAND)
        set(argiP ${argi})
        break()
    endif()
endforeach()
if(NOT argiP)
    message(FATAL_ERROR "Could not find argument \"EXEC_COMMAND\", please indicate at least one command to execute")
endif()

# Check for timestamp
if(LY_TIMESTAMP_REFERENCE)
    if(NOT EXISTS ${LY_TIMESTAMP_REFERENCE})
        message(FATAL_ERROR "File LY_TIMESTAMP_REFERENCE=${LY_TIMESTAMP_REFERENCE} does not exists")
    endif()
    if(NOT LY_TIMESTAMP_FILE)
        set(LY_TIMESTAMP_FILE ${LY_TIMESTAMP_REFERENCE}.stamp)
    endif()
    if(EXISTS ${LY_TIMESTAMP_FILE} AND NOT ${LY_TIMESTAMP_REFERENCE} IS_NEWER_THAN ${LY_TIMESTAMP_FILE})
        # Stamp newer, nothing to do
        return()
    endif()
endif()

if(LY_LOCK_FILE)
    # Lock the file
    file(LOCK ${LY_LOCK_FILE} TIMEOUT 1200 RESULT_VARIABLE lock_result)
    if(NOT ${lock_result} EQUAL 0)
        message(FATAL_ERROR "Lock failure ${lock_result}")
    endif()
    # There is no need to unlock the file since it will be contained to the execution of this process (cmake -P)
endif()

set(command_arguments)
foreach(argi RANGE ${argiP}+1 ${CMAKE_ARGC})
    # Check if we have a new EXEC_COMMAND
    if(CMAKE_ARGV${argi} STREQUAL EXEC_COMMAND)
        list(LENGTH command_arguments command_arguments_len)
        if(command_arguments_len) # Check if this is the first command (or if the user passed two consecutive EXEC_COMMAND keywords)
            execute_process(COMMAND ${command_arguments} RESULT_VARIABLE command_result)
            if (NOT ${command_result} EQUAL 0)
                message(FATAL_ERROR "[CommandExecution] \"${command_arguments}\" returned ${command_result}")
            endif()
        endif()
        set(command_arguments)
    else()
        list(APPEND command_arguments ${CMAKE_ARGV${argi}})
    endif()
endforeach()

list(LENGTH command_arguments command_arguments_len)
if(command_arguments_len)
    execute_process(COMMAND ${command_arguments} RESULT_VARIABLE command_result)
endif()

if(LY_TIMESTAMP_REFERENCE)
    # Touch the timestamp file
    file(TOUCH ${LY_TIMESTAMP_FILE})
endif()
