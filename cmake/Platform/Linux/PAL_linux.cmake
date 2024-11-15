#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_set(PAL_EXECUTABLE_APPLICATION_FLAG)
ly_set(PAL_LINKOPTION_MODULE MODULE)

ly_set(PAL_TRAIT_BUILD_HOST_GUI_TOOLS FALSE)
ly_set(PAL_TRAIT_BUILD_HOST_TOOLS TRUE)
ly_set(PAL_TRAIT_BUILD_SERVER_SUPPORTED TRUE)
ly_set(PAL_TRAIT_BUILD_UNIFIED_SUPPORTED TRUE)
ly_set(PAL_TRAIT_BUILD_UNITY_SUPPORTED TRUE)
ly_set(PAL_TRAIT_BUILD_UNITY_EXCLUDE_EXTENSIONS)
ly_set(PAL_TRAIT_BUILD_EXCLUDE_ALL_TEST_RUNS_FROM_IDE FALSE)
ly_set(PAL_TRAIT_BUILD_CPACK_SUPPORTED TRUE)

ly_set(PAL_TRAIT_PROF_PIX_SUPPORTED FALSE)

# Determine if tests are supported based on the PAL_TRAIT_BUILD_TESTS_SUPPORTED_DEFAULT global property
get_property(is_test_supported_default_set GLOBAL PROPERTY PAL_TRAIT_BUILD_TESTS_SUPPORTED_DEFAULT SET)
if (is_test_supported_default_set)
    get_property(test_supported_default GLOBAL PROPERTY PAL_TRAIT_BUILD_TESTS_SUPPORTED_DEFAULT)
    ly_set(PAL_TRAIT_BUILD_TESTS_SUPPORTED ${test_supported_default})
else()
    ly_set(PAL_TRAIT_BUILD_TESTS_SUPPORTED TRUE)
endif()

# Test library support
ly_set(PAL_TRAIT_TEST_GOOGLE_TEST_SUPPORTED TRUE)
ly_set(PAL_TRAIT_TEST_GOOGLE_BENCHMARK_SUPPORTED TRUE)
ly_set(PAL_TRAIT_TEST_LYTESTTOOLS_SUPPORTED TRUE)
ly_set(PAL_TRAIT_TEST_PYTEST_SUPPORTED TRUE)
ly_set(PAL_TRAIT_TEST_TARGET_TYPE MODULE)

if ($ENV{O3DE_SNAP})
    list(APPEND CMAKE_PREFIX_PATH "$ENV{SNAP}/usr/lib/x86_64-linux-gnu")
endif()

get_property(O3DE_SCRIPT_ONLY GLOBAL PROPERTY "O3DE_SCRIPT_ONLY")
if (O3DE_SCRIPT_ONLY)
    if (NOT CMAKE_CXX_COMPILER_ID)
        set(CMAKE_CXX_COMPILER_ID "Clang")
        set(CMAKE_C_COMPILER_ID "Clang")
    endif()
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    ly_set(PAL_TRAIT_COMPILER_ID Clang)
    ly_set(PAL_TRAIT_COMPILER_ID_LOWERCASE clang)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    ly_set(PAL_TRAIT_COMPILER_ID GCC)
    ly_set(PAL_TRAIT_COMPILER_ID_LOWERCASE gcc)
else()
    message(FATAL_ERROR "Compiler ${CMAKE_CXX_COMPILER_ID} not supported in ${PAL_PLATFORM_NAME}")
endif()

# Set the default asset type for deployment
set(LY_ASSET_DEPLOY_ASSET_TYPE "linux" CACHE STRING "Set the asset type for deployment.")

# Set the python cmd tool
ly_set(LY_PYTHON_CMD ${CMAKE_CURRENT_SOURCE_DIR}/python/python.sh)

# Compiler flag to export all symbols from a library
ly_set(PAL_TRAIT_EXPORT_ALL_SYMBOLS_COMPILE_OPTIONS -fvisibility=default)

# Set the default window manager that applications should be using on Linux 
# Note: Only ("xcb" or "wayland" should be considered)
set(PAL_TRAIT_LINUX_WINDOW_MANAGER "xcb" CACHE STRING "Sets the Window Manager type to use when configuring Linux")  
set_property(CACHE PAL_TRAIT_LINUX_WINDOW_MANAGER PROPERTY STRINGS xcb wayland)

# Use system default libunwind instead of maintaining an O3DE version for Linux
include(${CMAKE_CURRENT_LIST_DIR}/libunwind_linux.cmake)

# Use system default libzstd instead of maintaining an O3DE version for Linux
include(${CMAKE_CURRENT_LIST_DIR}/libzstd_linux.cmake)
