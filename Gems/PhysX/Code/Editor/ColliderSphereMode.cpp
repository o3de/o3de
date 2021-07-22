/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ColliderSphereMode.h"
#include <PhysX/EditorColliderComponentRequestBus.h>

#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/ToString.h>
#include <AzFramework/Viewport/ViewportColors.h>
#include <AzFramework/Viewport/ViewportConstants.h>

namespace PhysX
{
    AZ_CLASS_ALLOCATOR_IMPL(ColliderSphereMode, AZ::SystemAllocator, 0);

    static const float MinSphereRadius = 0.01f;
    static const AZ::Vector3 ManipulatorAxis = AZ::Vector3::CreateAxisX();

    void ColliderSphereMode::Setup(const AZ::EntityComponentIdPair& idPair)
    {
        AZ::Transform colliderWorldTransform = AZ::Transform::Identity();
        PhysX::EditorColliderComponentRequestBus::EventResult(colliderWorldTransform, idPair, &PhysX::EditorColliderComponentRequests::GetColliderWorldTransform);

        float sphereRadius = 0.0f;
        PhysX::EditorColliderComponentRequestBus::EventResult(sphereRadius, idPair, &PhysX::EditorColliderComponentRequests::GetSphereRadius);

        m_radiusManipulator = AzToolsFramework::LinearManipulator::MakeShared(colliderWorldTransform);
        m_radiusManipulator->AddEntityComponentIdPair(idPair);
        m_radiusManipulator->SetAxis(ManipulatorAxis);
        m_radiusManipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);
        m_radiusManipulator->SetLocalPosition(ManipulatorAxis * sphereRadius);

        AzToolsFramework::ManipulatorViews views;
        views.emplace_back(AzToolsFramework::CreateManipulatorViewQuadBillboard(AzFramework::ViewportColors::DefaultManipulatorHandleColor, AzFramework::ViewportConstants::DefaultManipulatorHandleSize));
        m_radiusManipulator->SetViews(AZStd::move(views));

        m_radiusManipulator->InstallMouseMoveCallback([this, idPair]
        (const AzToolsFramework::LinearManipulator::Action& action)
        {
            OnManipulatorMoved(action, idPair);
        });
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(idPair.GetEntityId());
    }

    void ColliderSphereMode::Refresh(const AZ::EntityComponentIdPair& idPair)
    {
        AZ::Transform colliderWorldTransform = AZ::Transform::Identity();
        PhysX::EditorColliderComponentRequestBus::EventResult(colliderWorldTransform, idPair, &PhysX::EditorColliderComponentRequests::GetColliderWorldTransform);
        m_radiusManipulator->SetSpace(colliderWorldTransform);

        float sphereRadius = 0.0f;
        PhysX::EditorColliderComponentRequestBus::EventResult(sphereRadius, idPair, &PhysX::EditorColliderComponentRequests::GetSphereRadius);
        m_radiusManipulator->SetLocalPosition(ManipulatorAxis * sphereRadius);
    }

    void ColliderSphereMode::Teardown(const AZ::EntityComponentIdPair& idPair)
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        m_radiusManipulator->RemoveEntityComponentIdPair(idPair);
        m_radiusManipulator->Unregister();
    }

    void ColliderSphereMode::ResetValues(const AZ::EntityComponentIdPair& idPair)
    {
        PhysX::EditorColliderComponentRequestBus::Event(idPair, &PhysX::EditorColliderComponentRequests::SetSphereRadius, 0.5f);
    }

    void ColliderSphereMode::DisplayEntityViewport(
        const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ_UNUSED(debugDisplay);

        const AzFramework::CameraState cameraState = AzToolsFramework::GetCameraState(viewportInfo.m_viewportId);
        float radius = m_radiusManipulator->GetLocalPosition().GetLength();

        m_radiusManipulator->SetAxis(cameraState.m_side);
        m_radiusManipulator->SetLocalPosition(cameraState.m_side * radius);
    }

    void ColliderSphereMode::OnManipulatorMoved(const AzToolsFramework::LinearManipulator::Action& action, const AZ::EntityComponentIdPair& idPair)
    {
        // Get the distance the manipulator has moved along the axis.
        float extent = action.LocalPosition().Dot(action.m_fixed.m_axis);

        // Clamp the distance to a small value to prevent it going negative.
        extent = AZ::GetMax(extent, MinSphereRadius);

        // Update the manipulator and sphere radius
        m_radiusManipulator->SetLocalPosition(extent * action.m_fixed.m_axis);
        PhysX::EditorColliderComponentRequestBus::Event(idPair, &PhysX::EditorColliderComponentRequests::SetSphereRadius, extent);
    }
}
