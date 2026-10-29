#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

if(NOT IS_DIRECTORY "${O3DE_ROOT}" OR NOT TEST_BINARY_ROOT)
    message(FATAL_ERROR "O3DE_ROOT and TEST_BINARY_ROOT are required")
endif()

set(test_source_dir "${TEST_BINARY_ROOT}/source")
file(MAKE_DIRECTORY
    "${test_source_dir}/cmake"
    "${test_source_dir}/Code/3rdParty/lazy"
    "${test_source_dir}/Code/3rdParty/missing"
    "${test_source_dir}/Code/3rdParty/recursive"
)

file(WRITE "${test_source_dir}/cmake/LyAutoGen.cmake" "")
file(WRITE "${test_source_dir}/cmake/LYWrappers_test.cmake" "")

file(WRITE "${test_source_dir}/Code/3rdParty/lazy/CMakeLists.txt" [=[
get_property(load_count GLOBAL PROPERTY TEST_LAZY_PROVIDER_LOAD_COUNT)
if(NOT load_count)
    set(load_count 0)
endif()
math(EXPR load_count "${load_count} + 1")
set_property(GLOBAL PROPERTY TEST_LAZY_PROVIDER_LOAD_COUNT "${load_count}")
add_library(lazy INTERFACE)
add_library(3rdParty::lazy ALIAS lazy)
]=])

file(WRITE "${test_source_dir}/Code/3rdParty/missing/CMakeLists.txt" [=[
# This provider intentionally does not create 3rdParty::missing.
]=])

file(WRITE "${test_source_dir}/Code/3rdParty/recursive/CMakeLists.txt" [=[
o3de_resolve_3rdparty_target(3rdParty::recursive resolved)
]=])

file(WRITE "${test_source_dir}/CMakeLists.txt" [=[
cmake_minimum_required(VERSION 3.25)
project(ThirdPartyResolverFixture NONE)

if(NOT O3DE_ROOT OR NOT TEST_MODE)
    message(FATAL_ERROR "O3DE_ROOT and TEST_MODE are required")
endif()

set(LY_ROOT_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}")
set(PAL_PLATFORM_NAME Test)
set(PAL_PLATFORM_NAME_LOWERCASE test)
set(PAL_TRAIT_BUILD_UNITY_SUPPORTED TRUE)
set(INSTALLED_ENGINE TRUE)
set(O3DE_ENGINE_NAME O3DE)
set(LY_3RDPARTY_PATH "${CMAKE_CURRENT_BINARY_DIR}/packages" CACHE PATH "")

function(o3de_pal_dir output)
    set(${output} "${CMAKE_CURRENT_SOURCE_DIR}/cmake" PARENT_SCOPE)
endfunction()

function(ly_get_vs_folder_directory directory output)
    set(${output} ThirdParty PARENT_SCOPE)
endfunction()

include("${O3DE_ROOT}/cmake/3rdParty.cmake")
include("${O3DE_ROOT}/cmake/LYWrappers.cmake")

if(TARGET 3rdParty::lazy)
    message(FATAL_ERROR "The lazy provider loaded before it was requested")
endif()

add_library(consumer INTERFACE)

if(TEST_MODE STREQUAL success)
    ly_target_link_libraries(consumer INTERFACE 3rdParty::lazy)
    if(NOT TARGET 3rdParty::lazy)
        message(FATAL_ERROR "The requested provider was not loaded")
    endif()

    o3de_resolve_3rdparty_target(3rdParty::lazy resolved)
    get_property(load_count GLOBAL PROPERTY TEST_LAZY_PROVIDER_LOAD_COUNT)
    if(NOT resolved OR NOT load_count EQUAL 1)
        message(FATAL_ERROR "The provider was loaded ${load_count} times")
    endif()

    ly_delayed_target_link_libraries()
    get_target_property(consumer_links consumer INTERFACE_LINK_LIBRARIES)
    if(NOT "3rdParty::lazy" IN_LIST consumer_links)
        message(FATAL_ERROR "The resolved target was not linked: ${consumer_links}")
    endif()
elseif(TEST_MODE STREQUAL missing)
    ly_target_link_libraries(consumer INTERFACE 3rdParty::missing)
elseif(TEST_MODE STREQUAL recursive)
    ly_target_link_libraries(consumer INTERFACE 3rdParty::recursive)
else()
    message(FATAL_ERROR "Unknown TEST_MODE: ${TEST_MODE}")
endif()
]=])

function(run_configure mode should_succeed expected_message)
    set(test_binary_dir "${TEST_BINARY_ROOT}/${mode}")
    set(command
        "${CMAKE_COMMAND}"
        -S "${test_source_dir}"
        -B "${test_binary_dir}"
        "-DO3DE_ROOT=${O3DE_ROOT}"
        "-DTEST_MODE=${mode}"
    )
    if(TEST_GENERATOR)
        list(APPEND command -G "${TEST_GENERATOR}")
    endif()

    execute_process(
        COMMAND ${command}
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error
        TIMEOUT 60
    )
    string(CONCAT log "${output}" "${error}")
    string(REGEX REPLACE "[ \t\r\n]+" " " normalized_log "${log}")

    if(should_succeed AND NOT result EQUAL 0)
        message(FATAL_ERROR "${mode} configure failed unexpectedly:\n${log}")
    elseif(NOT should_succeed AND result EQUAL 0)
        message(FATAL_ERROR "${mode} configure succeeded unexpectedly:\n${log}")
    endif()
    if(expected_message AND NOT normalized_log MATCHES "${expected_message}")
        message(FATAL_ERROR "${mode} configure did not report '${expected_message}':\n${log}")
    endif()
endfunction()

run_configure(success TRUE "")
run_configure(missing FALSE "did not create 3rdParty::missing")
run_configure(recursive FALSE "Recursive third-party activation")
