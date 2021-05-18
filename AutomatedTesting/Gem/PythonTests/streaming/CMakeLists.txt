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

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS)
    # Unstable, SPEC-3838 will restore
    #ly_add_pytest(
    #    NAME AutomatedTesting::asset_load_benchmark_test
    #    TEST_SERIAL
    #    TEST_SUITE benchmark
    #    PATH ${CMAKE_CURRENT_LIST_DIR}/benchmark/asset_load_benchmark_test.py
    #    RUNTIME_DEPENDENCIES
    #        AZ::AssetProcessor
    #        AZ::AssetProcessorBatch
    #        AutomatedTesting.GameLauncher
    #)
endif()
