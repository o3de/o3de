#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# This file is used to patch a file only if it has not already been patched.
# This is useful for patching a file that is fetched by FetchContent, which
# does not have a built-in way to only apply a patch if it has not already been
# applied.

# Usage:
# cmake -P PatchIfNotAlreadyPatched.cmake <patchfile>
# it expects the current working directory to be in the root of the repository
# for which the patch applies and uses `git apply` to patch it.

# note that CMAKE_ARGV0 will be the cmake executable
#  and that CMAKE_ARGV1 will be -P to enable the script mode
#  and that CMAKE_ARGV2 will be the name of this script file itself,
# so we start at CMAKE_ARGV3 to get the first argument passed to this script.

set(PATCH_FILE_NAME "${CMAKE_ARGV3}")
set(PATCH_ALREADY_APPLIED "")

# Check if the patch has already been applied.
# if this command returns 0 it means that reversing the patch was successful
# which means the patch was already applied.
execute_process(
    COMMAND git apply --check --reverse "${PATCH_FILE_NAME}"
    RESULT_VARIABLE PATCH_ALREADY_APPLIED
    OUTPUT_QUIET
    ERROR_QUIET
)

if (PATCH_ALREADY_APPLIED STREQUAL "0")
    message(STATUS "Skipping already applied patch ${PATCH_FILE_NAME} in dir ${CMAKE_BINARY_DIR}")
else()
    message(STATUS "Cleaning directory before patch...")
    execute_process(COMMAND git restore .)
    execute_process(COMMAND git clean -fdx)
    message(STATUS "Applying patch ${PATCH_FILE_NAME} at ${CMAKE_BINARY_DIR}...")
    execute_process(
        COMMAND git apply --ignore-whitespace "${PATCH_FILE_NAME}"
        RESULT_VARIABLE PATCH_RESULT
    )
    if (NOT PATCH_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to apply patch")
    else()
        message(STATUS "Patch applied successfully")
    endif()
endif()
