/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Viewport/ViewportBus.h>

#include <gmock/gmock.h>

namespace UnitTest
{
    class MockViewportRequests : public AzFramework::ViewportRequestBus::Handler
    {
    public:
        void Connect(const AzFramework::ViewportId viewportId)
        {
            AzFramework::ViewportRequestBus::Handler::BusConnect(viewportId);
        }

        void Disconnect()
        {
            AzFramework::ViewportRequestBus::Handler::BusDisconnect();
        }

        // AzFramework::ViewportRequestBus overrides ...
        MOCK_CONST_METHOD0(GetCameraViewMatrix, const AZ::Matrix4x4&());
        MOCK_CONST_METHOD0(GetCameraViewMatrixAsMatrix3x4, AZ::Matrix3x4());
        MOCK_METHOD1(SetCameraViewMatrix, void(const AZ::Matrix4x4& matrix));
        MOCK_CONST_METHOD0(GetCameraProjectionMatrix, const AZ::Matrix4x4&());
        MOCK_METHOD1(SetCameraProjectionMatrix, void(const AZ::Matrix4x4& matrix));
        MOCK_CONST_METHOD0(GetCameraTransform, AZ::Transform());
        MOCK_METHOD1(SetCameraTransform, void(const AZ::Transform& transform));
    };
} // namespace UnitTest
