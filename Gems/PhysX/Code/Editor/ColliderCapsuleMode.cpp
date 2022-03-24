/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ColliderCapsuleMode.h"
#include <PhysX/EditorColliderComponentRequestBus.h>
#include <Source/Utils.h>

#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <AzToolsFramework/ComponentModes/BoxViewportEdit.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/NonUniformScaleBus.h>

namespace PhysX
{
    AZ_CLASS_ALLOCATOR_IMPL(ColliderCapsuleMode, AZ::SystemAllocator, 0);

    AZ::Transform ColliderCapsuleMode::GetCapsuleWorldTransform() const
    {
        AZ::Transform worldTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldTransform, m_entityComponentIdPair.GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
        return worldTransform;
    }

    AZ::Transform ColliderCapsuleMode::GetCapsuleLocalTransform() const
    {
        return Utils::GetColliderLocalTransform(m_entityComponentIdPair);
    }

    AZ::Vector3 ColliderCapsuleMode::GetCapsuleNonUniformScale() const
    {
        AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(
            nonUniformScale, m_entityComponentIdPair.GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);
        return nonUniformScale;
    }

    float ColliderCapsuleMode::GetCapsuleRadius() const
    {
        float capsuleRadius = 0.0f;
        PhysX::EditorColliderComponentRequestBus::EventResult(
            capsuleRadius, m_entityComponentIdPair, &PhysX::EditorColliderComponentRequests::GetCapsuleRadius);
        return capsuleRadius;
    }

    float ColliderCapsuleMode::GetCapsuleHeight() const
    {
        float capsuleHeight = 0.0f;
        PhysX::EditorColliderComponentRequestBus::EventResult(
            capsuleHeight, m_entityComponentIdPair, &PhysX::EditorColliderComponentRequests::GetCapsuleHeight);
        return capsuleHeight;
    }

    void ColliderCapsuleMode::SetCapsuleRadius(float radius)
    {
        PhysX::EditorColliderComponentRequestBus::Event(
            m_entityComponentIdPair, &PhysX::EditorColliderComponentRequests::SetCapsuleRadius, radius);
    }

    void ColliderCapsuleMode::SetCapsuleHeight(float height)
    {
        PhysX::EditorColliderComponentRequestBus::Event(
            m_entityComponentIdPair, &PhysX::EditorColliderComponentRequests::SetCapsuleHeight, height);
    }

    void ColliderCapsuleMode::Setup(const AZ::EntityComponentIdPair& idPair)
    {
        m_entityComponentIdPair = idPair;
        SetupCapsuleManipulators(AzToolsFramework::g_mainManipulatorManagerId);
        m_radiusManipulator->AddEntityComponentIdPair(idPair);
        m_heightManipulator->AddEntityComponentIdPair(idPair);
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(idPair.GetEntityId());
    }

    void ColliderCapsuleMode::Refresh([[maybe_unused]] const AZ::EntityComponentIdPair& idPair)
    {
        UpdateCapsuleManipulators();
    }

    void ColliderCapsuleMode::Teardown(const AZ::EntityComponentIdPair& idPair)
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        m_radiusManipulator->RemoveEntityComponentIdPair(idPair);
        m_heightManipulator->RemoveEntityComponentIdPair(idPair);
        TeardownCapsuleManipulators();
        m_entityComponentIdPair = AZ::EntityComponentIdPair();
    }

    void ColliderCapsuleMode::ResetValues([[maybe_unused]] const AZ::EntityComponentIdPair& idPair)
    {
        ResetCapsuleManipulators();
    }

    void ColliderCapsuleMode::DisplayEntityViewport(
        const AzFramework::ViewportInfo& viewportInfo,
        [[maybe_unused]] AzFramework::DebugDisplayRequests& debugDisplay)
    {
        const AzFramework::CameraState cameraState = AzToolsFramework::GetCameraState(viewportInfo.m_viewportId);
        OnCameraStateChanged(cameraState);
    }
}
