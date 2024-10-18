#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
include(cmake/CompilerSettings.cmake)

ly_set(PAL_EXECUTABLE_APPLICATION_FLAG WIN32)
ly_set(PAL_LINKOPTION_MODULE MODULE)

ly_set(PAL_TRAIT_BUILD_HOST_GUI_TOOLS TRUE)
ly_set(PAL_TRAIT_BUILD_HOST_TOOLS TRUE)
ly_set(PAL_TRAIT_BUILD_SERVER_SUPPORTED TRUE)
ly_set(PAL_TRAIT_BUILD_UNIFIED_SUPPORTED TRUE)
ly_set(PAL_TRAIT_BUILD_UNITY_SUPPORTED TRUE)
ly_set(PAL_TRAIT_BUILD_UNITY_EXCLUDE_EXTENSIONS)
ly_set(PAL_TRAIT_BUILD_EXCLUDE_ALL_TEST_RUNS_FROM_IDE FALSE)
ly_set(PAL_TRAIT_BUILD_CPACK_SUPPORTED TRUE)

ly_set(PAL_TRAIT_PROF_PIX_SUPPORTED TRUE)

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

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    ly_set(PAL_TRAIT_COMPILER_ID MSVC)
    ly_set(PAL_TRAIT_COMPILER_ID_LOWERCASE msvc)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    ly_set(PAL_TRAIT_COMPILER_ID Clang)
    ly_set(PAL_TRAIT_COMPILER_ID_LOWERCASE clang)
else()
    message(FATAL_ERROR "Compiler ${CMAKE_CXX_COMPILER_ID} not supported in ${PAL_PLATFORM_NAME}")
endif()

# Set the default asset type for deployment
set(LY_ASSET_DEPLOY_ASSET_TYPE "pc" CACHE STRING "Set the asset type for deployment.")

# Set the python cmd tool
ly_set(LY_PYTHON_CMD ${CMAKE_CURRENT_SOURCE_DIR}/python/python.cmd)

# Compiler flag to export all symbols from a library
# On Windows this is done directly with the WINDOWS_EXPORT_ALL_SYMBOLS cmake option instead
ly_set(PAL_TRAIT_EXPORT_ALL_SYMBOLS_COMPILE_OPTIONS)
