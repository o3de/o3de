/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/MockComponentApplication.h>

namespace UnitTest
{
    MockComponentApplication::MockComponentApplication()
    {
        AZ::ComponentApplicationBus::Handler::BusConnect();
        AZ::Interface<AZ::ComponentApplicationRequests>::Register(this);
    }

    MockComponentApplication::~MockComponentApplication()
    {
        AZ::Interface<AZ::ComponentApplicationRequests>::Unregister(this);
        AZ::ComponentApplicationBus::Handler::BusDisconnect();
    }
} // namespace UnitTest
