/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "XcbBaseTestFixture.h"

namespace AzFramework
{
    XcbBaseTestFixture::XcbBaseTestFixture() :
        ::UnitTest::LeakDetectionFixture()
    {
        m_nativeUiComponent = AZStd::make_unique<AzFramework::NativeUISystemComponent>();
    }
    void XcbBaseTestFixture::SetUp()
    {
        using testing::Return;
        using testing::_;

        testing::Test::SetUp();

        EXPECT_CALL(m_interface, xcb_connect(_, _))
            .WillOnce(Return(&m_connection));
        EXPECT_CALL(m_interface, xcb_disconnect(&m_connection))
            .Times(1);
    }
} // namespace AzFramework
