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

################################################################################
# Tests
################################################################################

ly_add_target(
    NAME TestImpact.TestTargetA.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
    NAMESPACE AZ
    FILES_CMAKE
        testimpactframework_testtargeta_tests_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Tests
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzTestShared
            AZ::AzTest
)