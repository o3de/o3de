#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

set(PAL_EXECUTABLE_APPLICATION_FLAG MACOSX_BUNDLE)
set(PAL_LINKOPTION_MODULE MODULE)

set(PAL_TRAIT_BUILD_HOST_GUI_TOOLS TRUE)
set(PAL_TRAIT_BUILD_HOST_TOOLS TRUE)
set(PAL_TRAIT_BUILD_SERVER_SUPPORTED FALSE)
set(PAL_TRAIT_BUILD_TESTS_SUPPORTED TRUE)
set(PAL_TRAIT_BUILD_UNITY_SUPPORTED TRUE)
set(PAL_TRAIT_BUILD_UNITY_EXCLUDE_EXTENSIONS ".mm")
set(PAL_TRAIT_BUILD_EXCLUDE_ALL_TEST_RUNS_FROM_IDE FALSE)

# Test library support
set(PAL_TRAIT_TEST_GOOGLE_TEST_SUPPORTED TRUE)
set(PAL_TRAIT_TEST_GOOGLE_BENCHMARK_SUPPORTED TRUE)
set(PAL_TRAIT_TEST_PYTEST_SUPPORTED FALSE)
set(PAL_TRAIT_TEST_TARGET_TYPE MODULE)

if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    set(PAL_TRAIT_COMPILER_ID Clang)
    set(PAL_TRAIT_COMPILER_ID_LOWERCASE clang)
else()
    message(FATAL_ERROR "Compiler ${CMAKE_CXX_COMPILER_ID} not supported in ${PAL_PLATFORM_NAME}")
endif()

# Set the default asset type for deployment
set(LY_ASSET_DEPLOY_ASSET_TYPE "osx_gl" CACHE STRING "Set the asset type for deployment.")

# Set the python cmd tool
set(LY_PYTHON_CMD ${CMAKE_CURRENT_SOURCE_DIR}/Tools/Python/python3.sh)
