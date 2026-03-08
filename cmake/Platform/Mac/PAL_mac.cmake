#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_set(PAL_EXECUTABLE_APPLICATION_FLAG MACOSX_BUNDLE)
ly_set(PAL_LINKOPTION_MODULE MODULE)

ly_set(PAL_TRAIT_BUILD_HOST_GUI_TOOLS TRUE)
ly_set(PAL_TRAIT_BUILD_HOST_TOOLS TRUE)
ly_set(PAL_TRAIT_BUILD_SERVER_SUPPORTED FALSE)
ly_set(PAL_TRAIT_BUILD_UNIFIED_SUPPORTED FALSE)
ly_set(PAL_TRAIT_BUILD_UNITY_SUPPORTED TRUE)
ly_set(PAL_TRAIT_BUILD_UNITY_EXCLUDE_EXTENSIONS ".mm")
ly_set(PAL_TRAIT_BUILD_EXCLUDE_ALL_TEST_RUNS_FROM_IDE FALSE)
ly_set(PAL_TRAIT_BUILD_CPACK_SUPPORTED FALSE)

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
ly_set(PAL_TRAIT_TEST_PYTEST_SUPPORTED FALSE)
ly_set(PAL_TRAIT_TEST_TARGET_TYPE MODULE)

if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    ly_set(PAL_TRAIT_COMPILER_ID Clang)
    ly_set(PAL_TRAIT_COMPILER_ID_LOWERCASE clang)
else()
    message(FATAL_ERROR "Compiler ${CMAKE_CXX_COMPILER_ID} not supported in ${PAL_PLATFORM_NAME}")
endif()

# Non-AppleClang compilers do not automatically discover the macOS SDK sysroot.
# Detect it via xcrun so that system headers are found.
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" AND NOT CMAKE_OSX_SYSROOT)
    execute_process(
        COMMAND xcrun --sdk macosx --show-sdk-path
        OUTPUT_VARIABLE xcrun_sdk_path
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(xcrun_sdk_path)
        set(CMAKE_OSX_SYSROOT "${xcrun_sdk_path}" CACHE PATH "" FORCE)
    else()
        message(WARNING "Could not detect macOS SDK sysroot via xcrun. System headers may not be found.")
    endif()
endif()

# Set the default asset type for deployment
set(LY_ASSET_DEPLOY_ASSET_TYPE "mac" CACHE STRING "Set the asset type for deployment.")

# Set the deployment target for MacOS
set(LY_MAC_DEPLOYMENT_TARGET "11.0" CACHE STRING "Mac Deployment Target")
set(CMAKE_OSX_DEPLOYMENT_TARGET ${LY_MAC_DEPLOYMENT_TARGET})

# Set the python cmd tool
ly_set(LY_PYTHON_CMD ${CMAKE_CURRENT_SOURCE_DIR}/python/python.sh)

# Compiler flag to export all symbols from a library
ly_set(PAL_TRAIT_EXPORT_ALL_SYMBOLS_COMPILE_OPTIONS -fvisibility=default)

# Only x86_64 is currently supported on Mac
ly_set(CMAKE_OSX_ARCHITECTURES "x86_64")
