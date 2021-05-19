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

add_subdirectory(asset_processor_tests)

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS)
## AP Python Tests ##
    ly_add_pytest(
        NAME AssetPipelineTests.AuxiliaryContent
        PATH ${CMAKE_CURRENT_LIST_DIR}/auxiliary_content_tests/auxiliary_content_tests.py
        EXCLUDE_TEST_RUN_TARGET_FROM_IDE
        TEST_SUITE periodic
    )

    ly_add_pytest(
        NAME AssetPipelineTests.BankInfoParser
        PATH ${CMAKE_CURRENT_LIST_DIR}/wwise_bank_dependency_tests/bank_info_parser_tests.py
        EXCLUDE_TEST_RUN_TARGET_FROM_IDE
        TEST_SUITE periodic
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessorBatch   
    )
        
endif()
