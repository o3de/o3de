#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if (${PAL_TRAIT_LINUX_WINDOW_MANAGER} STREQUAL "xcb")
    # This library defines local implementations of all the used xcb functions,
    # in order to allow tests to run in the absence of a running X server.
    # These have to be in a separate library, so that the normal AzFramework
    # tests do not need to set up a mock Xcb interface.
    ly_add_target(
        NAME AzFramework.Xcb.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE AZ
        FILES_CMAKE
            Tests/Platform/Common/Xcb/azframework_xcb_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                ${pal_dir}
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzFramework
                AZ::AzTest
                AZ::AzTestShared
                AZ::AzFrameworkTestShared
    )
    ly_add_googletest(
        NAME AZ::AzFramework.Xcb.Tests
    )
endif()
