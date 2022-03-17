/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <gmock/gmock.h>

#include <AzFramework/Physics/HeightfieldProviderBus.h>

namespace UnitTest
{
    class MockHeightfieldProviderNotificationBusListener
        : private Physics::HeightfieldProviderNotificationBus::Handler
    {
    public:
        MockHeightfieldProviderNotificationBusListener(AZ::EntityId entityid)
        {
            Physics::HeightfieldProviderNotificationBus::Handler::BusConnect(entityid);
        }

        ~MockHeightfieldProviderNotificationBusListener()
        {
            Physics::HeightfieldProviderNotificationBus::Handler::BusDisconnect();
        }

        MOCK_METHOD2(OnHeightfieldDataChanged, void(const AZ::Aabb&, Physics::HeightfieldProviderNotifications::HeightfieldChangeMask));
    };
} // namespace UnitTest
