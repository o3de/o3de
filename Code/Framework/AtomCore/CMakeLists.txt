#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME AtomCore STATIC
    NAMESPACE AZ
    FILES_CMAKE
        AtomCore/atomcore_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
        PUBLIC
            3rdParty::RapidJSON
)


################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME AtomCore.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE AZ
        FILES_CMAKE
            Tests/atomcore_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::AtomCore
    )
    ly_add_googletest(
        NAME AZ::AtomCore.Tests
        LABELS REQUIRES_tiaf
    )
endif()
