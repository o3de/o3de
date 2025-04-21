#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

#
# o3de_compile_cache_activation activates compiler caching support for O3DE builds in using MSVC 
# This currently supports ccache or sccache and can significantly speed up build times 
# by caching compilation results and reusing them when possible, but only under certain conditions:
# 1. /Z7 or embedded debug must be set
# 2. If on CMake versions > 3.30, set cmake policy CMP0141 to NEW to use CMAKE_MSVC_DEBUG_INFORMATION_FORMAT instead of /Z7
# 2. TrackFileAccess should be disabled or the cache folder has to be placed in %TMP% or %APPDATA%
# Compiler flag examples for CMake can be found here: 
# https://github.com/ccache/ccache/wiki/MS-Visual-Studio
# https://github.com/mozilla/sccache?tab=readme-ov-file#usage 
# 

# - To enable compiler caching, you need to:
#   1. Have ccache or sccache installed
#   2. Set O3DE_ENABLE_COMPILER_CACHE to "true"
#   2. Set O3DE_COMPILER_CACHE_PATH to either:
#      - Direct path to the ccache/sccache executable
#      - A partial directory containing ccache.exe or sccache.exe
#
# - The cache path and enable flag can be set either through:
#   - CMake variable: -DO3DE_COMPILER_CACHE_PATH=<path> -DO3DE_ENABLE_COMPILER_CACHE=true
#   - Environment variable: O3DE_COMPILER_CACHE_PATH=<path> O3DE_ENABLE_COMPILER_CACHE=true
#
# - CMake variables take precedence over environment variables
# - This is primarily used for AR/CI processes but can also be used for local builds
#

function(o3de_compiler_cache_activation)
    message(STATUS "[COMPILER CACHE] Cache is enabled")

    # Check for custom compiler cache path, CMake variable takes precedence over environment
    if(DEFINED O3DE_COMPILER_CACHE_PATH)
        set(cache_path ${O3DE_COMPILER_CACHE_PATH})
    elseif(DEFINED ENV{O3DE_COMPILER_CACHE_PATH})
        set(cache_path $ENV{O3DE_COMPILER_CACHE_PATH})
    else()
        message(FATAL_ERROR "[COMPILER CACHE] O3DE_COMPILER_CACHE_PATH not provided. This required if compiler cache is enabled.")
    endif()

    # Convert to absolute path and normalize slashes
    cmake_path(ABSOLUTE_PATH cache_path OUTPUT_VARIABLE cache_path)
    string(REPLACE "\\" "/" cache_path "${cache_path}")
    string(TOLOWER "${cache_path}" cache_path)
    
    if(NOT EXISTS "${cache_path}")
        message(FATAL_ERROR "[COMPILER CACHE] Path does not exist: ${cache_path}")
    endif()

    # If it's a Chocolatey shim or directory, search for the actual executable
    if(cache_path MATCHES ".*/chocolatey/bin/.*" OR IS_DIRECTORY "${cache_path}")
        set(search_path "${cache_path}")
        
        # If it's a Chocolatey shim, convert bin path to lib path
        if(cache_path MATCHES ".*/chocolatey/bin/.*")
            string(REPLACE "/bin/" "/lib/" search_path "${cache_path}")
            get_filename_component(search_path "${search_path}" DIRECTORY)
            message(STATUS "[COMPILER CACHE] Detected Chocolatey shim path, searching in lib directory")
        endif()

        file(GLOB_RECURSE potential_exes 
            "${search_path}/**/ccache.exe" 
            "${search_path}/**/sccache.exe")
        
        if(potential_exes)
            list(GET potential_exes 0 cache_exe)
            string(REPLACE "\\" "/" cache_exe "${cache_exe}")
        else()
            message(FATAL_ERROR "[COMPILER CACHE] Could not find ccache.exe or sccache.exe in directory: ${search_path}")
        endif()
    else()
        set(cache_exe "${cache_path}")
    endif()

    message(STATUS "[COMPILER CACHE] Found at ${cache_exe}, using it for this build")

    # Copy cache executable as an alternative cl.exe. This will act as a wrapper for the real cl.exe
    file(COPY_FILE ${cache_exe} ${CMAKE_BINARY_DIR}/cl.exe ONLY_IF_DIFFERENT)
endfunction()
