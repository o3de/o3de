/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Viewport/CameraState.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzManipulatorTestFramework/ViewportInteraction.h>
#include <AzToolsFramework/Manipulators/ManipulatorBus.h>

namespace AzManipulatorTestFramework
{
    // Null debug display for dummy draw calls
    class NullDebugDisplayRequests : public AzFramework::DebugDisplayRequests
    {
    public:
        virtual ~NullDebugDisplayRequests() = default;
    };

    ViewportInteraction::ViewportInteraction()
        : m_nullDebugDisplayRequests(AZStd::make_unique<NullDebugDisplayRequests>())
    {
        AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus::Handler::BusConnect(m_viewportId);
        AzToolsFramework::ViewportInteraction::ViewportSettingsRequestBus::Handler::BusConnect(m_viewportId);
        AzToolsFramework::ViewportInteraction::EditorEntityViewportInteractionRequestBus::Handler::BusConnect(m_viewportId);
    }

    ViewportInteraction::~ViewportInteraction()
    {
        AzToolsFramework::ViewportInteraction::EditorEntityViewportInteractionRequestBus::Handler::BusDisconnect();
        AzToolsFramework::ViewportInteraction::ViewportSettingsRequestBus::Handler::BusDisconnect();
        AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus::Handler::BusDisconnect();
    }

    AzFramework::CameraState ViewportInteraction::GetCameraState()
    {
        return m_cameraState;
    }

    bool ViewportInteraction::GridSnappingEnabled() const
    {
        return m_gridSnapping;
    }

    float ViewportInteraction::GridSize() const
    {
        return m_gridSize;
    }

    bool ViewportInteraction::ShowGrid() const
    {
        return false;
    }

    bool ViewportInteraction::AngleSnappingEnabled() const
    {
        return m_angularSnapping;
    }

    float ViewportInteraction::AngleStep() const
    {
        return m_angularStep;
    }

    float ViewportInteraction::ManipulatorLineBoundWidth() const
    {
        return 0.1f;
    }

    float ViewportInteraction::ManipulatorCircleBoundWidth() const
    {
        return 0.1f;
    }

    bool ViewportInteraction::StickySelectEnabled() const
    {
        return m_stickySelect;
    }

    void ViewportInteraction::FindVisibleEntities(AZStd::vector<AZ::EntityId>& visibleEntitiesOut)
    {
        visibleEntitiesOut.assign(m_entityVisibilityQuery.Begin(), m_entityVisibilityQuery.End());
    }

    void ViewportInteraction::UpdateVisibility()
    {
        m_entityVisibilityQuery.UpdateVisibility(m_cameraState);
    }

    AzFramework::ScreenPoint ViewportInteraction::ViewportWorldToScreen(const AZ::Vector3& worldPosition)
    {
        return AzFramework::WorldToScreen(worldPosition, m_cameraState);
    }

    void ViewportInteraction::SetCameraState(const AzFramework::CameraState& cameraState)
    {
        m_cameraState = cameraState;
    }

    AzFramework::DebugDisplayRequests& ViewportInteraction::GetDebugDisplay()
    {
        return *m_nullDebugDisplayRequests;
    }

    void ViewportInteraction::SetGridSnapping(const bool enabled)
    {
        m_gridSnapping = enabled;
    }

    void ViewportInteraction::SetAngularSnapping(const bool enabled)
    {
        m_angularSnapping = enabled;
    }

    void ViewportInteraction::SetStickySelect(const bool enabled)
    {
        m_stickySelect = enabled;
    }

    AZ::Vector3 ViewportInteraction::DefaultEditorCameraPosition() const
    {
        return {};
    }

    void ViewportInteraction::SetGridSize(float size)
    {
        m_gridSize = size;
    }

    void ViewportInteraction::SetAngularStep(float step)
    {
        m_angularStep = step;
    }

    AzFramework::ViewportId ViewportInteraction::GetViewportId() const
    {
        return m_viewportId;
    }

    AZ::Vector3 ViewportInteraction::ViewportScreenToWorld([[maybe_unused]] const AzFramework::ScreenPoint& screenPosition)
    {
        return AzFramework::ScreenToWorld(screenPosition, m_cameraState);
    }

    AzToolsFramework::ViewportInteraction::ProjectedViewportRay ViewportInteraction::ViewportScreenToWorldRay(
        [[maybe_unused]] const AzFramework::ScreenPoint& screenPosition)
    {
        return AzToolsFramework::ViewportInteraction::ViewportScreenToWorldRay(m_cameraState, screenPosition);
    }

    float ViewportInteraction::DeviceScalingFactor()
    {
        return 1.0f;
    }
} // namespace AzManipulatorTestFramework
