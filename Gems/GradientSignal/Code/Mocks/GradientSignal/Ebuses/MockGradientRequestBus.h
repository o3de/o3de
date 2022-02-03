/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <gmock/gmock.h>
#include <AzCore/Component/ComponentApplication.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>

namespace UnitTest
{
    class MockGradientRequests
         : private GradientSignal::GradientRequestBus::Handler
    {
    public:
        MockGradientRequests(AZ::EntityId entityId)
        {
            GradientSignal::GradientRequestBus::Handler::BusConnect(entityId);
        }

        ~MockGradientRequests()
        {
            GradientSignal::GradientRequestBus::Handler::BusDisconnect();
        }

        MOCK_CONST_METHOD1(GetValue, float(const GradientSignal::GradientSampleParams&));
        MOCK_CONST_METHOD1(IsEntityInHierarchy, bool(const AZ::EntityId&));
    };
} // namespace UnitTest

