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

ly_add_target(
    NAME PhysX.NumericalMethods STATIC
    NAMESPACE Gem
    FILES_CMAKE
        numericalmethods_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
)

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME PhysX.NumericalMethods.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            numericalmethods_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Gem::PhysX.NumericalMethods
                AZ::AzFramework
    )
    ly_add_googletest(
        NAME Gem::PhysX.NumericalMethods.Tests
    )
endif()
