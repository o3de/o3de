#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(${CMAKE_ARGC} LESS 5)
    message(FATAL_ERROR "StripDebugSymbols script called with the wrong number of arguments: ${CMAKE_ARGC}")
endif()

# cmake command arguments (not including the first 2 arguments of '-P', 'StripDebugSymbols.cmake')

set(LLVM_STRIP_TOOL ${CMAKE_ARGV3})          # LLVM_STRIP_TOOL      : The location of the llvm strip tool (e.g. <ndk-root>/toolchains/llvm/prebuilt/windows-x86_64/bin/llvm-strip.exe)
set(STRIP_TARGET ${CMAKE_ARGV4})            # STRIP_TARGET        : The built binary to process
set(TARGET_TYPE ${CMAKE_ARGV5})             # TARGET_TYPE         : The target type (STATIC_LIBRARY, MODULE_LIBRARY, SHARED_LIBRARY, EXECUTABLE, or APPLICATION)

# This script only supports executables, applications, modules and shared libraries
if (NOT ${TARGET_TYPE} STREQUAL "MODULE_LIBRARY" AND
    NOT ${TARGET_TYPE} STREQUAL "SHARED_LIBRARY" AND
    NOT ${TARGET_TYPE} STREQUAL "EXECUTABLE" AND
    NOT ${TARGET_TYPE} STREQUAL "APPLICATION")
    return()
endif()

message(VERBOSE "Stripping debug symbols from ${STRIP_TARGET}")

execute_process(COMMAND ${LLVM_STRIP_TOOL} --strip-debug ${STRIP_TARGET})
