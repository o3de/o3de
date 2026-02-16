#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Unified compiler launcher support.
#
# Adapts CMAKE_<LANG>_COMPILER_LAUNCHER for generators that don't natively support it.
# Users simply set CMAKE_C_COMPILER_LAUNCHER and/or CMAKE_CXX_COMPILER_LAUNCHER to a compiler launcher executable
# (e.g. ccache, sccache) and this module handles the rest.
#
# Supported generators:
#   - Ninja / Makefile: Native support via CMAKE_<LANG>_COMPILER_LAUNCHER
#   - Visual Studio: cl.exe wrapper via CMAKE_VS_GLOBALS
#   - Xcode: Wrapper scripts via CMAKE_XCODE_ATTRIBUTE_CC/CXX
#
# Usage:
#   - CMake flag: -DCMAKE_C_COMPILER_LAUNCHER=ccache
#   - Environment variable: CMAKE_C_COMPILER_LAUNCHER=ccache
#   - CMake preset: "cacheVariables": { "CMAKE_C_COMPILER_LAUNCHER": "ccache" }
#
# This file must be included AFTER project() so that CMAKE_C_COMPILER
# and CMAKE_CXX_COMPILER are resolved (needed for Xcode wrapper generation).

include_guard(GLOBAL)

function(_o3de_compiler_launcher)
    set(supported_languages C CXX)
    set(caching_launchers ccache sccache)

    # Resolves Chocolatey shim executables to their real tool paths.
    # Chocolatey commonly places shims in <ChocolateyInstall>/bin. The real tool
    # is usually located somewhere under <ChocolateyInstall>/lib/<tool>/tools/**.
    # Copying the shim elsewhere breaks its relative lookup.
    function(resolve_chocolatey_shim in_path out_path)
        if (NOT in_path)
            set(${out_path} "" PARENT_SCOPE)
            return()
        endif ()

        set(resolved "${in_path}")
        set(normalized "${resolved}")
        string(REPLACE "\\" "/" normalized "${normalized}")
        string(TOLOWER "${normalized}" normalized)

        # Attempt to resolve any Chocolatey shim under chocolatey/bin/<tool>.exe
        if (normalized MATCHES ".*/chocolatey/bin/([^/]+)\\.exe$")
            set(tool "${CMAKE_MATCH_1}")
            set(search_root "${normalized}")
            string(REPLACE "/bin/${tool}.exe" "/lib/${tool}" search_root "${search_root}")

            file(GLOB_RECURSE candidates "${search_root}/tools/**/${tool}.exe")
            if (candidates)
                list(GET candidates 0 candidate)
                string(REPLACE "\\" "/" candidate "${candidate}")
                set(resolved "${candidate}")
            else ()
                # Some Chocolatey packages ship a differently named executable in tools/.
                # If there is exactly one .exe under tools, prefer that rather than the shim.
                file(GLOB_RECURSE candidates "${search_root}/tools/**/*.exe")
                list(LENGTH candidates candidate_count)
                if (candidate_count EQUAL 1)
                    list(GET candidates 0 candidate)
                    string(REPLACE "\\" "/" candidate "${candidate}")
                    set(resolved "${candidate}")
                endif ()
            endif ()
        endif ()

        set(${out_path} "${resolved}" PARENT_SCOPE)
    endfunction()

    # CMake does not natively initialize CMAKE_<LANG>_COMPILER_LAUNCHER from
    # environment variables, so we do it here as a convenience.
    foreach (cl_lang IN ITEMS ${supported_languages})
        if (NOT DEFINED CMAKE_${cl_lang}_COMPILER_LAUNCHER
            AND DEFINED ENV{CMAKE_${cl_lang}_COMPILER_LAUNCHER}
            AND NOT "$ENV{CMAKE_${cl_lang}_COMPILER_LAUNCHER}" STREQUAL "")
            set(CMAKE_${cl_lang}_COMPILER_LAUNCHER "$ENV{CMAKE_${cl_lang}_COMPILER_LAUNCHER}" CACHE STRING "")
        endif ()
    endforeach ()

    # Early return if no launcher is configured
    if (NOT CMAKE_C_COMPILER_LAUNCHER AND NOT CMAKE_CXX_COMPILER_LAUNCHER)
        message(VERBOSE "CompilerLauncher: Not configured")
        return()
    endif ()

    # Resolve launcher paths
    # If the launcher value is not an absolute path to an existing file, resolve it via find_program.
    # This handles bare names like "ccache" or "sccache".
    foreach (cl_lang IN ITEMS ${supported_languages})
        if (CMAKE_${cl_lang}_COMPILER_LAUNCHER)
            set(cl_value "${CMAKE_${cl_lang}_COMPILER_LAUNCHER}")

            if (IS_ABSOLUTE "${cl_value}" AND EXISTS "${cl_value}")
                # Already a valid absolute path
                set(cl_${cl_lang}_resolved "${cl_value}")
            else ()
                find_program(cl_found_${cl_lang} "${cl_value}" NO_CACHE)
                if (cl_found_${cl_lang})
                    set(cl_${cl_lang}_resolved "${cl_found_${cl_lang}}")
                elseif (IS_ABSOLUTE "${cl_value}")
                    message(WARNING "CompilerLauncher: ${cl_lang} launcher path does not exist: ${cl_value}")
                else ()
                    # Keep the original value; the generator may still find it
                    set(cl_${cl_lang}_resolved "${cl_value}")
                    message(WARNING "CompilerLauncher: Could not resolve '${cl_value}' to an absolute path")
                endif ()
            endif ()

            if (cl_${cl_lang}_resolved)
                message(STATUS "CompilerLauncher: ${cl_lang} launcher: ${cl_${cl_lang}_resolved}")
            endif ()
        endif ()
    endforeach ()

    # Bail out if no valid launcher was resolved
    foreach (cl_lang IN ITEMS ${supported_languages})
        if (cl_${cl_lang}_resolved)
            set(cl_valid_launcher_found TRUE)
            break()
        endif ()
    endforeach ()

    if (NOT cl_valid_launcher_found)
        message(WARNING "CompilerLauncher: No valid compiler launcher found")
        return()
    endif ()

    # Signal to downstream platform configuration files that a compiler launcher is active.
    set_property(GLOBAL PROPERTY O3DE_COMPILER_LAUNCHER_ENABLED TRUE)

    # Detect whether the launcher is a known caching compiler (ccache, sccache).
    # Caching compilers require embedded debug info (/Z7) instead of separate PDBs (/Zi)
    # because they hash the .obj content and PDB writes are non-deterministic.
    foreach (cl_lang IN ITEMS ${supported_languages})
        if (cl_${cl_lang}_resolved)
            get_filename_component(cl_launcher_name "${cl_${cl_lang}_resolved}" NAME_WE)
            string(TOLOWER "${cl_launcher_name}" cl_launcher_name)
            if (cl_launcher_name IN_LIST caching_launchers)
                set_property(GLOBAL PROPERTY O3DE_CACHING_COMPILER_LAUNCHER_ENABLED TRUE)
                break()
            endif ()
        endif ()
    endforeach ()

    # == MAKEFILE / NINJA == #
    if (CMAKE_GENERATOR MATCHES "Makefiles|Ninja|Ninja Multi-Config")
        foreach (cl_lang IN ITEMS ${supported_languages})
            if (cl_${cl_lang}_resolved)
                set(CMAKE_${cl_lang}_COMPILER_LAUNCHER "${cl_${cl_lang}_resolved}" CACHE STRING "" FORCE)
            endif ()
        endforeach ()

        message(STATUS "CompilerLauncher: Configured for ${CMAKE_GENERATOR} (native support)")
        return()
    endif ()

    # == VISUAL STUDIO == #
    # Visual Studio does not support CMAKE_<LANG>_COMPILER_LAUNCHER.
    # Instead, the launcher executable (ccache / sccache) is copied as cl.exe into the build directory,
    # and CMAKE_VS_GLOBALS redirects MSBuild to use it.
    # When invoked as cl.exe, ccache/sccache detect they are wrapping MSVC and proxy the call to the real cl.exe found in PATH.
    # Compiler launchers also require embedded debug information (/Z7 instead of /Zi) to work correctly with MSVC.
    # CMAKE_MSVC_DEBUG_INFORMATION_FORMAT is set here; platform config files should also add /Z7 flags as a fallback.
    if (CMAKE_GENERATOR MATCHES "Visual Studio")
        # Prefer the C++ launcher; fall back to C launcher
        set(cl_vs_launcher "${cl_CXX_resolved}")
        if (NOT cl_vs_launcher)
            set(cl_vs_launcher "${cl_C_resolved}")
        endif ()

        if (NOT cl_vs_launcher)
            message(WARNING "CompilerLauncher: No valid launcher for Visual Studio generator")
            return()
        endif ()

        # Visual Studio requires a concrete executable path; re-resolve here if needed.
        if (NOT (IS_ABSOLUTE "${cl_vs_launcher}" AND EXISTS "${cl_vs_launcher}"))
            find_program(cl_vs_launcher_found "${cl_vs_launcher}" NO_CACHE)
            if (cl_vs_launcher_found)
                set(cl_vs_launcher "${cl_vs_launcher_found}")
            else ()
                message(WARNING "CompilerLauncher: Could not resolve Visual Studio launcher '${cl_vs_launcher}' to an absolute path")
                return()
            endif ()
        endif ()

        # Resolve Chocolatey shims (in .../Chocolatey/bin) to the real executable under .../Chocolatey/lib/.../tools/.
        # This is required even when using a symlink, because the shim uses install-relative paths based on
        # its own module location; executing it via a build-directory symlink makes it look for ..\lib\... in the build tree.
        resolve_chocolatey_shim("${cl_vs_launcher}" cl_vs_launcher_real)
        if (cl_vs_launcher_real)
            set(cl_vs_launcher "${cl_vs_launcher_real}")
        endif ()

        # Prefer a symlink to avoid copying the launcher.
        # If symlinks are not permitted, fall back to copying.
        set(cl_vs_wrapper "${CMAKE_BINARY_DIR}/cl.exe")
        if (EXISTS "${cl_vs_wrapper}")
            file(REMOVE "${cl_vs_wrapper}")
        endif ()

        execute_process(
            COMMAND "${CMAKE_COMMAND}" -E create_symlink "${cl_vs_launcher}" "${cl_vs_wrapper}"
            RESULT_VARIABLE cl_vs_link_result
            OUTPUT_QUIET
            ERROR_VARIABLE cl_vs_link_error
        )

        if (NOT cl_vs_link_result EQUAL 0)
            # Copy launcher as cl.exe in the build directory.
            file(COPY_FILE "${cl_vs_launcher}" "${cl_vs_wrapper}" ONLY_IF_DIFFERENT)
            message(STATUS "CompilerLauncher: Visual Studio cl.exe wrapper copied (symlink unavailable)")
        else ()
            message(STATUS "CompilerLauncher: Visual Studio cl.exe wrapper symlinked")
        endif ()

        list(APPEND CMAKE_VS_GLOBALS
            "CLToolExe=cl.exe"
            "CLToolPath=${CMAKE_BINARY_DIR}"
            "TrackFileAccess=false"
            "UseMultiToolTask=true"
        )
        set(CMAKE_VS_GLOBALS "${CMAKE_VS_GLOBALS}" PARENT_SCOPE)

        message(STATUS "CompilerLauncher: Configured for Visual Studio via cl.exe wrapper")
        return()
    endif ()

    # == XCODE == #
    # Xcode does not support CMAKE_<LANG>_COMPILER_LAUNCHER.
    # Instead, we create thin wrapper scripts that invoke the launcher with the CMake-resolved compiler.
    # This uses CMAKE_C_COMPILER / CMAKE_CXX_COMPILER, so it respects any user overrides.
    # See: https://crascit.com/2016/04/09/using-ccache-with-cmake/
    if (CMAKE_GENERATOR MATCHES "Xcode")
        if (cl_C_resolved AND CMAKE_C_COMPILER)
            set(cl_launch_c "${CMAKE_BINARY_DIR}/launch-c")
            file(WRITE "${cl_launch_c}" "#!/usr/bin/env sh\nexec \"${cl_C_resolved}\" \"${CMAKE_C_COMPILER}\" \"$@\"\n")
            file(CHMOD "${cl_launch_c}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
            set(CMAKE_XCODE_ATTRIBUTE_CC "${cl_launch_c}" CACHE STRING "" FORCE)
            set(CMAKE_XCODE_ATTRIBUTE_LD "${cl_launch_c}" CACHE STRING "" FORCE)
        endif ()

        if (cl_CXX_resolved AND CMAKE_CXX_COMPILER)
            set(cl_launch_cxx "${CMAKE_BINARY_DIR}/launch-cxx")
            file(WRITE "${cl_launch_cxx}" "#!/usr/bin/env sh\nexec \"${cl_CXX_resolved}\" \"${CMAKE_CXX_COMPILER}\" \"$@\"\n")
            file(CHMOD "${cl_launch_cxx}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
            set(CMAKE_XCODE_ATTRIBUTE_CXX "${cl_launch_cxx}" CACHE STRING "" FORCE)
            set(CMAKE_XCODE_ATTRIBUTE_LDPLUSPLUS "${cl_launch_cxx}" CACHE STRING "" FORCE)
        endif ()

        message(STATUS "CompilerLauncher: Configured for Xcode via wrapper scripts")
        return()
    endif ()

    # == UNKNOWN == #
    # Leave CMAKE_<LANG>_COMPILER_LAUNCHER as-is
    message(STATUS "CompilerLauncher: Unsupported generator: ${CMAKE_GENERATOR}")
endfunction()

# TODO: Replace with block() when we use a version of cmake that supports it (3.26+)
_o3de_compiler_launcher()
unset(_o3de_compiler_launcher)
