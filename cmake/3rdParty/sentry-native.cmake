#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

include(FetchContent)

FetchContent_Declare(
    sentry_native
    GIT_REPOSITORY https://github.com/getsentry/sentry-native.git
    GIT_TAG        0.16.4
    GIT_SUBMODULES_RECURSE TRUE
)

set(SENTRY_BACKEND "crashpad" CACHE STRING "" FORCE)
set(SENTRY_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SENTRY_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SENTRY_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(sentry_native)

# sentry-native bundles its own crashpad/mini_chromium/zlib build as sub-targets (SENTRY_BACKEND=
# crashpad above), and warnings this engine promotes to errors engine-wide (Configurations_msvc.
# cmake's /we<code> flags) keep surfacing one file at a time across that whole vendored tree as
# different targets get built -- not ours to fix line-by-line. Rather than whack-a-mole one
# target/code at a time as each surfaces, walk every target actually defined anywhere under
# sentry_native's fetched source tree (recursing through nested add_subdirectory calls, which is
# how crashpad/mini_chromium/zlib get pulled in) and suppress the whole set up front. Scoped to
# this directory tree only -- our own targets keep the engine's normal strict warnings.
function(o3de_suppress_vendored_warnings_recursive dir)
    get_property(targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(target ${targets})
        get_target_property(target_type ${target} TYPE)
        if(NOT target_type STREQUAL "INTERFACE_LIBRARY")
            target_compile_options(${target} PRIVATE
                ${O3DE_COMPILE_OPTION_DISABLE_WARNINGS}
                /FI"${CMAKE_CURRENT_LIST_DIR}/SuppressSentryFormatStringWarning.h"
            )
        endif()
    endforeach()

    get_property(subdirs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)
    foreach(subdir ${subdirs})
        o3de_suppress_vendored_warnings_recursive(${subdir})
    endforeach()
endfunction()

if(DEFINED O3DE_COMPILE_OPTION_DISABLE_WARNINGS AND MSVC)
    o3de_suppress_vendored_warnings_recursive(${sentry_native_SOURCE_DIR})
endif()

# sentry-native shells out to crashpad_handler at crash time and looks for it next to the running
# executable, but sentry-native's own build drops it under _deps/ - so without staging, crash
# capture silently does nothing. Anchored to a caller-supplied target that actually lands in the
# binary output directory (a STATIC lib's TARGET_FILE_DIR is the lib folder, not bin).
function(o3de_stage_crashpad_handler target)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:crashpad_handler>
            $<TARGET_FILE_DIR:${target}>/$<TARGET_FILE_NAME:crashpad_handler>
        COMMENT "Staging crashpad_handler next to ${target}"
        VERBATIM
    )
    add_dependencies(${target} crashpad_handler)
endfunction()
