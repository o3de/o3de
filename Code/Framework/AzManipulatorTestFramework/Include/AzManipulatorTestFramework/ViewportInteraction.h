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
        , public AzToolsFramework::ViewportInteraction::ViewportSettingsRequestBus::Handler
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
        AZ::Vector3 DefaultEditorCameraPosition() const override;

        // ViewportInteractionRequestBus overrides ...
        AzFramework::CameraState GetCameraState() override;
        AzFramework::ScreenPoint ViewportWorldToScreen(const AZ::Vector3& worldPosition) override;
        AZ::Vector3 ViewportScreenToWorld(const AzFramework::ScreenPoint& screenPosition) override;
        AzToolsFramework::ViewportInteraction::ProjectedViewportRay ViewportScreenToWorldRay(
            const AzFramework::ScreenPoint& screenPosition) override;
        float DeviceScalingFactor() override;

        // ViewportSettingsRequestBus overrides ...
        bool GridSnappingEnabled() const override;
        float GridSize() const override;
        bool ShowGrid() const override;
        bool AngleSnappingEnabled() const override;
        float AngleStep() const override;
        float ManipulatorLineBoundWidth() const override;
        float ManipulatorCircleBoundWidth() const override;
        bool StickySelectEnabled() const override;

        // EditorEntityViewportInteractionRequestBus overrides ...
        void FindVisibleEntities(AZStd::vector<AZ::EntityId>& visibleEntities) override;

    private:
        static constexpr AzFramework::ViewportId m_viewportId = 1234; //!< Arbitrary viewport id for manipulator tests.

        AzFramework::EntityVisibilityQuery m_entityVisibilityQuery;
        AZStd::shared_ptr<AzFramework::DebugDisplayRequests> m_debugDisplayRequests;
        AzFramework::CameraState m_cameraState;
        bool m_gridSnapping = false;
        bool m_angularSnapping = false;
        bool m_stickySelect = true;
        float m_gridSize = 1.0f;
        float m_angularStep = 0.0f;
    };
} // namespace AzManipulatorTestFramework
