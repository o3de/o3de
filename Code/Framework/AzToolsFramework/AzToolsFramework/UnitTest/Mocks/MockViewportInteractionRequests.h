/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Viewport/ViewportMessages.h>

#include <gmock/gmock.h>

namespace UnitTest
{
    class MockViewportInteractionRequests : public AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus::Handler
    {
    public:
        void Connect(AzFramework::ViewportId viewportId)
        {
            AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus::Handler::BusConnect(viewportId);
        }
        void Disconnect()
        {
            AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus::Handler::BusDisconnect();
        }

        // AzFramework::ViewportInteractionRequestBus overrides ...
        MOCK_METHOD0(GetCameraState, AzFramework::CameraState());
        MOCK_METHOD1(ViewportWorldToScreen, AzFramework::ScreenPoint(const AZ::Vector3&));
        MOCK_METHOD1(ViewportScreenToWorld, AZ::Vector3(const AzFramework::ScreenPoint&));
        MOCK_METHOD1(
            ViewportScreenToWorldRay, AzToolsFramework::ViewportInteraction::ProjectedViewportRay(const AzFramework::ScreenPoint&));
        MOCK_METHOD0(DeviceScalingFactor, float());
    };
} // namespace UnitTest
