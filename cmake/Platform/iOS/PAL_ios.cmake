#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_set(PAL_EXECUTABLE_APPLICATION_FLAG MACOSX_BUNDLE)
ly_set(PAL_LINKOPTION_MODULE SHARED)  # For iOS, 'MODULE' creates a tool/bundle, but we treat it as a shared library

ly_set(PAL_TRAIT_BUILD_HOST_GUI_TOOLS FALSE)
ly_set(PAL_TRAIT_BUILD_HOST_TOOLS FALSE)
ly_set(PAL_TRAIT_BUILD_SERVER_SUPPORTED FALSE)
ly_set(PAL_TRAIT_BUILD_TESTS_SUPPORTED TRUE)
ly_set(PAL_TRAIT_BUILD_UNITY_SUPPORTED TRUE)
ly_set(PAL_TRAIT_BUILD_UNITY_EXCLUDE_EXTENSIONS ".mm")
ly_set(PAL_TRAIT_BUILD_EXCLUDE_ALL_TEST_RUNS_FROM_IDE TRUE)
ly_set(PAL_TRAIT_BUILD_CPACK_SUPPORTED FALSE)

ly_set(PAL_TRAIT_PROF_PIX_SUPPORTED FALSE)

# Test library support
ly_set(PAL_TRAIT_TEST_GOOGLE_TEST_SUPPORTED FALSE)
ly_set(PAL_TRAIT_TEST_GOOGLE_BENCHMARK_SUPPORTED FALSE)
ly_set(PAL_TRAIT_TEST_LYTESTTOOLS_SUPPORTED FALSE)
ly_set(PAL_TRAIT_TEST_PYTEST_SUPPORTED FALSE)
ly_set(PAL_TRAIT_TEST_TARGET_TYPE MODULE)

if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    ly_set(PAL_TRAIT_COMPILER_ID Clang)
    ly_set(PAL_TRAIT_COMPILER_ID_LOWERCASE clang)
else()
    message(FATAL_ERROR "Compiler ${CMAKE_CXX_COMPILER_ID} not supported in ${PAL_PLATFORM_NAME}")
endif()

# Set the default asset type for deployment
set(LY_ASSET_DEPLOY_ASSET_TYPE "ios" CACHE STRING "Set the asset type for deployment.")

# Set the python cmd tool
ly_set(LY_PYTHON_CMD ${CMAKE_CURRENT_SOURCE_DIR}/python/python.sh)

