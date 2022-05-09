/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Visibility/EntityVisibilityQuery.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFramework.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

namespace AzFramework
{
    class DebugDisplayRequests;
}

namespace AzManipulatorTestFramework
{
    //! Implementation of the viewport interaction model to handle viewport interaction requests.
    class ViewportInteraction
        : public ViewportInteractionInterface
        , public AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus::Handler
        , public UnitTest::ViewportSettingsTestImpl
        , private AzToolsFramework::ViewportInteraction::EditorEntityViewportInteractionRequestBus::Handler
    {
    public:
        explicit ViewportInteraction(AZStd::shared_ptr<AzFramework::DebugDisplayRequests> debugDisplayRequests);
        ~ViewportInteraction();

        // ViewportInteractionInterface overrides ...
        void SetCameraState(const AzFramework::CameraState& cameraState) override;
        AzFramework::DebugDisplayRequests& GetDebugDisplay() override;
        void SetGridSnapping(bool enabled) override;
        void SetAngularSnapping(bool enabled) override;
        void SetGridSize(float size) override;
        void SetAngularStep(float step) override;
        AzFramework::ViewportId GetViewportId() const override;
        void UpdateVisibility() override;
        void SetStickySelect(bool enabled) override;
        void SetIconsVisible(bool visible) override;
        void SetHelpersVisible(bool visible) override;

        // ViewportInteractionRequestBus overrides ...
        AzFramework::CameraState GetCameraState() override;
        AzFramework::ScreenPoint ViewportWorldToScreen(const AZ::Vector3& worldPosition) override;
        AZ::Vector3 ViewportScreenToWorld(const AzFramework::ScreenPoint& screenPosition) override;
        AzToolsFramework::ViewportInteraction::ProjectedViewportRay ViewportScreenToWorldRay(
            const AzFramework::ScreenPoint& screenPosition) override;
        float DeviceScalingFactor() override;

        // EditorEntityViewportInteractionRequestBus overrides ...
        void FindVisibleEntities(AZStd::vector<AZ::EntityId>& visibleEntities) override;

    private:
        static constexpr AzFramework::ViewportId m_viewportId = 1234; //!< Arbitrary viewport id for manipulator tests.

        AzFramework::EntityVisibilityQuery m_entityVisibilityQuery;
        AZStd::shared_ptr<AzFramework::DebugDisplayRequests> m_debugDisplayRequests;
        AzFramework::CameraState m_cameraState;
    };
} // namespace AzManipulatorTestFramework
