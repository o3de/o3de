/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzManipulatorTestFramework/AzManipulatorTestFramework.h>

namespace AzManipulatorTestFramework
{
    class NullDebugDisplayRequests;

    //! Implementation of the viewport interaction model to handle viewport interaction requests.
    class ViewportInteraction
        : public ViewportInteractionInterface
        , private AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus::Handler
    {
    public:
        ViewportInteraction();
        ~ViewportInteraction();

        // ViewportInteractionInterface ...
        AzFramework::CameraState GetCameraState() override;
        void SetCameraState(const AzFramework::CameraState& cameraState) override;
        AzFramework::DebugDisplayRequests& GetDebugDisplay() override;
        void EnableGridSnaping() override;
        void DisableGridSnaping() override;
        void EnableAngularSnaping() override;
        void DisableAngularSnaping() override;
        void SetGridSize(float size) override;
        void SetAngularStep(float step) override;
        int GetViewportId() const override;
        AZStd::optional<AZ::Vector3> ViewportScreenToWorld(const AzFramework::ScreenPoint& screenPosition, float depth) override;
        AZStd::optional<AzToolsFramework::ViewportInteraction::ProjectedViewportRay> ViewportScreenToWorldRay(
            const AzFramework::ScreenPoint& screenPosition) override;
        float DeviceScalingFactor() override;
    private:
        // ViewportInteractionRequestBus ...
        bool GridSnappingEnabled();
        float GridSize();
        bool ShowGrid();
        bool AngleSnappingEnabled();
        float AngleStep();
        AzFramework::ScreenPoint ViewportWorldToScreen(const AZ::Vector3& worldPosition);
    private:
        AZStd::unique_ptr<NullDebugDisplayRequests> m_nullDebugDisplayRequests;
        const int m_viewportId = 1234; // Arbitrary viewport id for manipulator tests
        AzFramework::CameraState m_cameraState;
        bool m_gridSnapping = false;
        bool m_angularSnapping = false;
        float m_gridSize = 1.0f;
        float m_angularStep = 0.0f;
    };
} // namespace AzManipulatorTestFramework
