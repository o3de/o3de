/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CustomSerializeContextTestFixture.h>

namespace UnitTest
{
    void CustomSerializeContextTestFixture::SetUp()
    {
        LeakDetectionFixture::SetUp();
        m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
        m_componentApplicationMock = AZStd::make_unique<testing::NiceMock<MockComponentApplication>>();
        ON_CALL(*m_componentApplicationMock.get(), GetSerializeContext()).WillByDefault(::testing::Return(m_serializeContext.get()));
        ON_CALL(*m_componentApplicationMock.get(), AddEntity(::testing::_)).WillByDefault(::testing::Return(true));
    }

    void CustomSerializeContextTestFixture::TearDown()
    {
        m_componentApplicationMock.reset();
        m_serializeContext.reset();
        LeakDetectionFixture::TearDown();
    }
} // namespace UnitTest
