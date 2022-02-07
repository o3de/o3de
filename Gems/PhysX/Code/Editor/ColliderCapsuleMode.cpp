/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ColliderCapsuleMode.h"
#include <PhysX/EditorColliderComponentRequestBus.h>

#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Viewport/ViewportColors.h>
#include <AzFramework/Viewport/ViewportConstants.h>

namespace PhysX
{
    AZ_CLASS_ALLOCATOR_IMPL(ColliderCapsuleMode, AZ::SystemAllocator, 0);

    namespace
    {
        const AZ::Vector3 RadiusManipulatorAxis = AZ::Vector3::CreateAxisX();
        const AZ::Vector3 HeightManipulatorAxis = AZ::Vector3::CreateAxisZ();
        const float MinCapsuleRadius = 0.001f;
        const float MinCapsuleHeight = 0.002f;
        const float HalfHeight = 0.5f;
        const float ResetCapsuleHeight = 1.0f;
        const float ResetCapsuleRadius = 0.25f;
    }

    void ColliderCapsuleMode::Setup(const AZ::EntityComponentIdPair& idPair)
    {
        AZ::Transform colliderWorldTransform = AZ::Transform::Identity();
        PhysX::EditorColliderComponentRequestBus::EventResult(colliderWorldTransform, idPair, &PhysX::EditorColliderComponentRequests::GetColliderWorldTransform);

        SetupRadiusManipulator(idPair, colliderWorldTransform);
        SetupHeightManipulator(idPair, colliderWorldTransform);

        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(idPair.GetEntityId());
    }

    void ColliderCapsuleMode::Refresh(const AZ::EntityComponentIdPair& idPair)
    {
        AZ::Transform colliderWorldTransform = AZ::Transform::Identity();
        PhysX::EditorColliderComponentRequestBus::EventResult(colliderWorldTransform, idPair, &PhysX::EditorColliderComponentRequests::GetColliderWorldTransform);

        // Read the state of the capsule into manipulators to support undo/redo
        float capsuleHeight = 0.0f;
        PhysX::EditorColliderComponentRequestBus::EventResult(capsuleHeight, idPair, &PhysX::EditorColliderComponentRequests::GetCapsuleHeight);

        float capsuleRadius = 0.0f;
        PhysX::EditorColliderComponentRequestBus::EventResult(capsuleRadius, idPair, &PhysX::EditorColliderComponentRequests::GetCapsuleRadius);

        m_radiusManipulator->SetSpace(colliderWorldTransform);
        m_radiusManipulator->SetLocalPosition(m_radiusManipulator->GetAxis() * capsuleRadius);
        m_heightManipulator->SetSpace(colliderWorldTransform);
        m_heightManipulator->SetLocalPosition(m_heightManipulator->GetAxis() * capsuleHeight * HalfHeight);
    }

    void ColliderCapsuleMode::Teardown(const AZ::EntityComponentIdPair& idPair)
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        m_radiusManipulator->RemoveEntityComponentIdPair(idPair);
        m_radiusManipulator->Unregister();

        m_heightManipulator->RemoveEntityComponentIdPair(idPair);
        m_heightManipulator->Unregister();
    }

    void ColliderCapsuleMode::ResetValues(const AZ::EntityComponentIdPair& idPair)
    {
        PhysX::EditorColliderComponentRequestBus::Event(idPair, &PhysX::EditorColliderComponentRequests::SetCapsuleHeight, ResetCapsuleHeight);
        PhysX::EditorColliderComponentRequestBus::Event(idPair, &PhysX::EditorColliderComponentRequests::SetCapsuleRadius, ResetCapsuleRadius);
    }

    void ColliderCapsuleMode::DisplayEntityViewport(
        const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ_UNUSED(debugDisplay);

        const AzFramework::CameraState cameraState = AzToolsFramework::GetCameraState(viewportInfo.m_viewportId);
        float radius = m_radiusManipulator->GetLocalPosition().GetLength();

        m_radiusManipulator->SetAxis(cameraState.m_side);
        m_radiusManipulator->SetLocalPosition(cameraState.m_side * radius);
    }

    void ColliderCapsuleMode::SetupRadiusManipulator(const AZ::EntityComponentIdPair& idPair, const AZ::Transform& worldTransform)
    {
        // Radius manipulator
        float capsuleRadius = 0.0f;
        PhysX::EditorColliderComponentRequestBus::EventResult(capsuleRadius, idPair, &PhysX::EditorColliderComponentRequests::GetCapsuleRadius);

        m_radiusManipulator = AzToolsFramework::LinearManipulator::MakeShared(worldTransform);
        m_radiusManipulator->AddEntityComponentIdPair(idPair);
        m_radiusManipulator->SetAxis(RadiusManipulatorAxis);
        m_radiusManipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);
        m_radiusManipulator->SetLocalPosition(RadiusManipulatorAxis * capsuleRadius);

        {
            AzToolsFramework::ManipulatorViews views;
            views.emplace_back(AzToolsFramework::CreateManipulatorViewQuadBillboard(AzFramework::ViewportColors::DefaultManipulatorHandleColor, AzFramework::ViewportConstants::DefaultManipulatorHandleSize));
            m_radiusManipulator->SetViews(AZStd::move(views));
        }

        m_radiusManipulator->InstallMouseMoveCallback([this, idPair]
        (const AzToolsFramework::LinearManipulator::Action& action)
        {
            OnRadiusManipulatorMoved(action, idPair);
        });
    }

    void ColliderCapsuleMode::SetupHeightManipulator(const AZ::EntityComponentIdPair& idPair, const AZ::Transform& worldTransform)
    {
        // Height manipulator
        float capsuleHeight = 0.0f;
        PhysX::EditorColliderComponentRequestBus::EventResult(capsuleHeight, idPair, &PhysX::EditorColliderComponentRequests::GetCapsuleHeight);

        m_heightManipulator = AzToolsFramework::LinearManipulator::MakeShared(worldTransform);
        m_heightManipulator->AddEntityComponentIdPair(idPair);
        m_heightManipulator->SetAxis(HeightManipulatorAxis);
        m_heightManipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);
        m_heightManipulator->SetLocalPosition(HeightManipulatorAxis * capsuleHeight * HalfHeight); // Manipulator positioned at half the capsules height.

        {
            AzToolsFramework::ManipulatorViews views;
            views.emplace_back(AzToolsFramework::CreateManipulatorViewQuadBillboard(AzFramework::ViewportColors::DefaultManipulatorHandleColor, AzFramework::ViewportConstants::DefaultManipulatorHandleSize));
            m_heightManipulator->SetViews(AZStd::move(views));
        }

        m_heightManipulator->InstallMouseMoveCallback([this, idPair]
        (const AzToolsFramework::LinearManipulator::Action& action)
        {
            OnHeightManipulatorMoved(action, idPair);
        });

    }

    void ColliderCapsuleMode::OnRadiusManipulatorMoved(const AzToolsFramework::LinearManipulator::Action& action, const AZ::EntityComponentIdPair& idPair)
    {
        // Get the distance the manipulator has moved along the axis.
        float extent = action.LocalPosition().Dot(action.m_fixed.m_axis);

        // Clamp radius to a small value.
        extent = AZ::GetMax(extent, MinCapsuleRadius);

        // Update the manipulator and capsule radius.
        m_radiusManipulator->SetLocalPosition(extent * action.m_fixed.m_axis);

        // Adjust the height manipulator so it is always clamped to twice the radius.
        AdjustHeightManipulator(idPair, static_cast<float>(extent));

        // The final radius of the capsule is the manipulator's extent.
        PhysX::EditorColliderComponentRequestBus::Event(idPair, &PhysX::EditorColliderComponentRequests::SetCapsuleRadius, extent);
    }

    void ColliderCapsuleMode::OnHeightManipulatorMoved(const AzToolsFramework::LinearManipulator::Action& action, const AZ::EntityComponentIdPair& idPair)
    {
        // Get the distance the manipulator has moved along the axis.
        float extent = action.LocalPosition().Dot(action.m_fixed.m_axis);

        // Ensure capsule's half height is always greater than the radius.
        extent = AZ::GetMax(extent, MinCapsuleHeight);

        // Update the manipulator and capsule height.
        m_heightManipulator->SetLocalPosition(extent * action.m_fixed.m_axis);

        // The final height of the capsule is twice the manipulator's extent.
        float capsuleHeight = extent / HalfHeight;

        // Adjust the radius manipulator so it is always clamped to half the capsule height.
        AdjustRadiusManipulator(idPair, capsuleHeight);

        // Finally adjust the capsule height
        PhysX::EditorColliderComponentRequestBus::Event(idPair, &PhysX::EditorColliderComponentRequests::SetCapsuleHeight, capsuleHeight);
    }

    void ColliderCapsuleMode::AdjustRadiusManipulator(const AZ::EntityComponentIdPair& idPair, const float capsuleHeight)
    {
        float capsuleRadius = 0.0f;
        PhysX::EditorColliderComponentRequestBus::EventResult(capsuleRadius, idPair, &PhysX::EditorColliderComponentRequests::GetCapsuleRadius);

        // Clamp the radius to half height.
        capsuleRadius = AZ::GetMin(capsuleRadius, capsuleHeight * HalfHeight);

        // Update manipulator and the capsule radius.
        m_radiusManipulator->SetLocalPosition(capsuleRadius * m_radiusManipulator->GetAxis());
        PhysX::EditorColliderComponentRequestBus::Event(idPair, &PhysX::EditorColliderComponentRequests::SetCapsuleRadius, capsuleRadius);
    }

    void ColliderCapsuleMode::AdjustHeightManipulator(const AZ::EntityComponentIdPair& idPair, const float capsuleRadius)
    {
        float capsuleHeight = 0.0f;
        PhysX::EditorColliderComponentRequestBus::EventResult(capsuleHeight, idPair, &PhysX::EditorColliderComponentRequests::GetCapsuleHeight);

        // Clamp the height to twice the radius.
        capsuleHeight = AZ::GetMax(capsuleHeight, capsuleRadius / HalfHeight);

        // Update the manipulator and capsule height.
        m_heightManipulator->SetLocalPosition(capsuleHeight * HalfHeight * m_heightManipulator->GetAxis());
        PhysX::EditorColliderComponentRequestBus::Event(idPair, &PhysX::EditorColliderComponentRequests::SetCapsuleHeight, capsuleHeight);
    }
}
