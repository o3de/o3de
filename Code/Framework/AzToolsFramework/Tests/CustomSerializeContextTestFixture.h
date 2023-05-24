/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/UnitTest/MockComponentApplication.h>
#include <AzCore/UnitTest/TestTypes.h>


namespace UnitTest
{
    class CustomSerializeContextTestFixture
        : public LeakDetectionFixture
    {
    public:
        void SetUp() override;
        void TearDown() override;

    protected:
        AZStd::unique_ptr<testing::NiceMock<MockComponentApplication>> m_componentApplicationMock;
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
    };
} // namespace UnitTest
