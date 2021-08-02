#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME ExpressionEvaluation.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        expressionevaluation_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
)

ly_add_target(
    NAME ExpressionEvaluation ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        expressionevaluation_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            Gem::ExpressionEvaluation.Static
)

# all types of applications use the above module
ly_create_alias(NAME ExpressionEvaluation.Clients  NAMESPACE Gem TARGETS ExpressionEvaluation)
ly_create_alias(NAME ExpressionEvaluation.Servers  NAMESPACE Gem TARGETS ExpressionEvaluation)
ly_create_alias(NAME ExpressionEvaluation.Builders NAMESPACE Gem TARGETS ExpressionEvaluation)
ly_create_alias(NAME ExpressionEvaluation.Tools    NAMESPACE Gem TARGETS ExpressionEvaluation)

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME ExpressionEvaluation.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            expressionevaluation_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Tests
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Gem::ExpressionEvaluation.Static
    )
    ly_add_googletest(
        NAME Gem::ExpressionEvaluation.Tests
    )
endif()
