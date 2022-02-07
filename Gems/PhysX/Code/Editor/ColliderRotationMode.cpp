/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ColliderRotationMode.h"
#include <PhysX/EditorColliderComponentRequestBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Viewport/ViewportColors.h>
#include <AzToolsFramework/Manipulators/AngularManipulator.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

namespace PhysX
{
    AZ_CLASS_ALLOCATOR_IMPL(ColliderRotationMode, AZ::SystemAllocator, 0);

    ColliderRotationMode::ColliderRotationMode()
        : m_rotationManipulators(AZ::Transform::Identity())
    {
        m_rotationManipulators.SetCircleBoundWidth(AzToolsFramework::ManipulatorCicleBoundWidth());
    }

    void ColliderRotationMode::Setup(const AZ::EntityComponentIdPair& idPair)
    {
        AZ::Transform worldTransform;
        AZ::TransformBus::EventResult(worldTransform, idPair.GetEntityId(), &AZ::TransformInterface::GetWorldTM);

        AZ::Quaternion colliderRotation;
        PhysX::EditorColliderComponentRequestBus::EventResult(colliderRotation, idPair, &PhysX::EditorColliderComponentRequests::GetColliderRotation);

        AZ::Vector3 colliderOffset;
        PhysX::EditorColliderComponentRequestBus::EventResult(colliderOffset, idPair, &PhysX::EditorColliderComponentRequests::GetColliderOffset);

        m_rotationManipulators.SetSpace(worldTransform);
        m_rotationManipulators.SetLocalPosition(colliderOffset);
        m_rotationManipulators.SetLocalOrientation(colliderRotation);
        m_rotationManipulators.AddEntityComponentIdPair(idPair);
        m_rotationManipulators.Register(AzToolsFramework::g_mainManipulatorManagerId);       
        m_rotationManipulators.SetLocalAxes(
            AZ::Vector3::CreateAxisX(),
            AZ::Vector3::CreateAxisY(),
            AZ::Vector3::CreateAxisZ());
        m_rotationManipulators.ConfigureView(
            2.0f, 
            AzFramework::ViewportColors::XAxisColor, 
            AzFramework::ViewportColors::YAxisColor, 
            AzFramework::ViewportColors::ZAxisColor);

        m_rotationManipulators.InstallMouseMoveCallback(
            [this, idPair]
            (const AzToolsFramework::AngularManipulator::Action& action)
        {
            // Update the manipulator's rotation and the rotation of the collider itself when the manipulator is dragged.
            m_rotationManipulators.SetLocalOrientation(action.LocalOrientation());
            PhysX::EditorColliderComponentRequestBus::Event(idPair, &PhysX::EditorColliderComponentRequests::SetColliderRotation, action.LocalOrientation());
        });

        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(idPair.GetEntityId());
    }

    void ColliderRotationMode::Refresh(const AZ::EntityComponentIdPair& idPair)
    {
        AZ::Quaternion colliderRotation = AZ::Quaternion::CreateIdentity();
        PhysX::EditorColliderComponentRequestBus::EventResult(colliderRotation, idPair, &PhysX::EditorColliderComponentRequests::GetColliderRotation);
        m_rotationManipulators.SetLocalOrientation(colliderRotation);

        AZ::Vector3 colliderOffset = AZ::Vector3::CreateZero();
        PhysX::EditorColliderComponentRequestBus::EventResult(colliderOffset, idPair, &PhysX::EditorColliderComponentRequests::GetColliderOffset);
        m_rotationManipulators.SetLocalPosition(colliderOffset);
    }

    void ColliderRotationMode::Teardown(const AZ::EntityComponentIdPair& idPair)
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        m_rotationManipulators.RemoveEntityComponentIdPair(idPair);
        m_rotationManipulators.Unregister();
    }

    void ColliderRotationMode::ResetValues(const AZ::EntityComponentIdPair& idPair)
    {
        PhysX::EditorColliderComponentRequestBus::Event(idPair, &PhysX::EditorColliderComponentRequests::SetColliderRotation, AZ::Quaternion::CreateIdentity());
    }

    void ColliderRotationMode::DisplayEntityViewport(
        const AzFramework::ViewportInfo& viewportInfo,
        [[maybe_unused]] AzFramework::DebugDisplayRequests& debugDisplay)
    {
        const AzFramework::CameraState cameraState = AzToolsFramework::GetCameraState(viewportInfo.m_viewportId);
        m_rotationManipulators.RefreshView(cameraState.m_position);
    }
} // namespace PhysX
