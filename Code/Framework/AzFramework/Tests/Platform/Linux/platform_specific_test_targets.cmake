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
                3rdParty::X11::xcb
                3rdParty::X11::xcb_xkb
                3rdParty::X11::xcb_xfixes
                xcb-xinput
                xcb-keysyms
                AZ::AzFramework
                AZ::AzFramework.NativeUI
                AZ::AzTest
                AZ::AzTestShared
                AZ::AzFrameworkTestShared
    )

    # Disable all the XCB tests as a result of converting AzFramework.NativeUI to a shared library.
    # We cannot mock the xcb_ system calls inside the provide shared library. We will need to split
    # out the xcp related code to its own static library and create a test module for that in order
    # to be able to mock the xcb function. Once we can mock the xcb calls that are made in the
    # Linux version of AzFramework.NativeUI, then re-enable the following google test.
    #ly_add_googletest(
    #    NAME AZ::AzFramework.Xcb.Tests
    #)
endif()
