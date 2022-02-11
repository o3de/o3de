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
        AZ::TransformBus::EventResult(colliderWorldTransform, idPair.GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(nonUniformScale, idPair.GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);

        const AZ::Transform colliderLocalTransform = Utils::GetColliderLocalTransform(idPair);

        SetupRadiusManipulator(idPair, colliderWorldTransform, colliderLocalTransform, nonUniformScale);
        SetupHeightManipulator(idPair, colliderWorldTransform, colliderLocalTransform, nonUniformScale);

        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(idPair.GetEntityId());
    }

    void ColliderCapsuleMode::Refresh(const AZ::EntityComponentIdPair& idPair)
    {
        AZ::Transform colliderWorldTransform = AZ::Transform::Identity();
        AZ::TransformBus::EventResult(colliderWorldTransform, idPair.GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(nonUniformScale, idPair.GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);

        AZ::Transform colliderLocalTransform = Utils::GetColliderLocalTransform(idPair);

        // Read the state of the capsule into manipulators to support undo/redo
        float capsuleHeight = 0.0f;
        PhysX::EditorColliderComponentRequestBus::EventResult(capsuleHeight, idPair, &PhysX::EditorColliderComponentRequests::GetCapsuleHeight);

        float capsuleRadius = 0.0f;
        PhysX::EditorColliderComponentRequestBus::EventResult(capsuleRadius, idPair, &PhysX::EditorColliderComponentRequests::GetCapsuleRadius);

        m_radiusManipulator->SetSpace(colliderWorldTransform);
        m_radiusManipulator->SetLocalTransform(
            colliderLocalTransform * AZ::Transform::CreateTranslation(m_radiusManipulator->GetAxis() * capsuleRadius));
        m_radiusManipulator->SetNonUniformScale(nonUniformScale);
        m_heightManipulator->SetSpace(colliderWorldTransform);
        m_heightManipulator->SetLocalTransform(
            colliderLocalTransform * AZ::Transform::CreateTranslation(m_heightManipulator->GetAxis() * capsuleHeight * HalfHeight));
        m_heightManipulator->SetNonUniformScale(nonUniformScale);
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

        if (!m_radiusManipulator->EntityComponentIdPairs().empty())
        {
            const AZ::EntityComponentIdPair idPair = *m_radiusManipulator->EntityComponentIdPairs().begin();
            float radius = 0.0f;
            PhysX::EditorColliderComponentRequestBus::EventResult(radius, idPair, &PhysX::EditorColliderComponentRequests::GetCapsuleRadius);
            const AZ::Transform colliderLocalTransform = Utils::GetColliderLocalTransform(idPair);
            m_radiusManipulator->SetAxis(cameraState.m_side);
            m_radiusManipulator->SetLocalTransform(colliderLocalTransform * AZ::Transform::CreateTranslation(cameraState.m_side * radius));
        }
    }

    void ColliderCapsuleMode::SetupRadiusManipulator(
        const AZ::EntityComponentIdPair& idPair,
        const AZ::Transform& worldTransform,
        const AZ::Transform& localTransform,
        const AZ::Vector3& nonUniformScale)
    {
        // Radius manipulator
        float capsuleRadius = 0.0f;
        PhysX::EditorColliderComponentRequestBus::EventResult(capsuleRadius, idPair, &PhysX::EditorColliderComponentRequests::GetCapsuleRadius);

        m_radiusManipulator = AzToolsFramework::LinearManipulator::MakeShared(worldTransform);
        m_radiusManipulator->AddEntityComponentIdPair(idPair);
        m_radiusManipulator->SetAxis(RadiusManipulatorAxis);
        m_radiusManipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);
        m_radiusManipulator->SetLocalTransform(localTransform * AZ::Transform::CreateTranslation(RadiusManipulatorAxis * capsuleRadius));
        m_radiusManipulator->SetNonUniformScale(nonUniformScale);

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

    void ColliderCapsuleMode::SetupHeightManipulator(
        const AZ::EntityComponentIdPair& idPair,
        const AZ::Transform& worldTransform,
        const AZ::Transform& localTransform,
        const AZ::Vector3& nonUniformScale)
    {
        // Height manipulator
        float capsuleHeight = 0.0f;
        PhysX::EditorColliderComponentRequestBus::EventResult(capsuleHeight, idPair, &PhysX::EditorColliderComponentRequests::GetCapsuleHeight);

        m_heightManipulator = AzToolsFramework::LinearManipulator::MakeShared(worldTransform);
        m_heightManipulator->AddEntityComponentIdPair(idPair);
        m_heightManipulator->SetAxis(HeightManipulatorAxis);
        m_heightManipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);
        m_heightManipulator->SetLocalTransform(
            localTransform * AZ::Transform::CreateTranslation(HeightManipulatorAxis * capsuleHeight * HalfHeight));
        m_heightManipulator->SetNonUniformScale(nonUniformScale);

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
        // manipulator action offsets do not take entity transform scale into account, so need to apply it here
        const AZ::Transform colliderLocalTransform = Utils::GetColliderLocalTransform(idPair);
        const AZ::Vector3 manipulatorPosition = AzToolsFramework::GetPositionInManipulatorFrame(
            m_radiusManipulator->GetSpace().GetUniformScale(), colliderLocalTransform, action);

        // Get the distance the manipulator has moved along the axis.
        float extent = manipulatorPosition.Dot(action.m_fixed.m_axis);

        // Clamp radius to a small value.
        extent = AZ::GetMax(extent, MinCapsuleRadius);

        // Update the manipulator and capsule radius.
        m_radiusManipulator->SetLocalTransform(colliderLocalTransform * AZ::Transform::CreateTranslation(extent * action.m_fixed.m_axis));

        // Adjust the height manipulator so it is always clamped to twice the radius.
        AdjustHeightManipulator(idPair, static_cast<float>(extent));

        // The final radius of the capsule is the manipulator's extent.
        PhysX::EditorColliderComponentRequestBus::Event(idPair, &PhysX::EditorColliderComponentRequests::SetCapsuleRadius, extent);
    }

    void ColliderCapsuleMode::OnHeightManipulatorMoved(const AzToolsFramework::LinearManipulator::Action& action, const AZ::EntityComponentIdPair& idPair)
    {
        // manipulator action offsets do not take entity transform scale into account, so need to apply it here
        const AZ::Transform colliderLocalTransform = Utils::GetColliderLocalTransform(idPair);
        const AZ::Vector3 manipulatorPosition = AzToolsFramework::GetPositionInManipulatorFrame(
            m_heightManipulator->GetSpace().GetUniformScale(), colliderLocalTransform, action);

        // Get the distance the manipulator has moved along the axis.
        float extent = manipulatorPosition.Dot(action.m_fixed.m_axis);

        // Ensure capsule's half height is always greater than the radius.
        extent = AZ::GetMax(extent, MinCapsuleHeight);

        // Update the manipulator and capsule height.
        m_heightManipulator->SetLocalTransform(colliderLocalTransform * AZ::Transform::CreateTranslation(extent * action.m_fixed.m_axis));

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
        const AZ::Transform colliderLocalTransform = Utils::GetColliderLocalTransform(idPair);
        m_radiusManipulator->SetLocalTransform(
            colliderLocalTransform * AZ::Transform::CreateTranslation(capsuleRadius * m_radiusManipulator->GetAxis()));
        PhysX::EditorColliderComponentRequestBus::Event(idPair, &PhysX::EditorColliderComponentRequests::SetCapsuleRadius, capsuleRadius);
    }

    void ColliderCapsuleMode::AdjustHeightManipulator(const AZ::EntityComponentIdPair& idPair, const float capsuleRadius)
    {
        float capsuleHeight = 0.0f;
        PhysX::EditorColliderComponentRequestBus::EventResult(capsuleHeight, idPair, &PhysX::EditorColliderComponentRequests::GetCapsuleHeight);

        // Clamp the height to twice the radius.
        capsuleHeight = AZ::GetMax(capsuleHeight, capsuleRadius / HalfHeight);

        // Update the manipulator and capsule height.
        const AZ::Transform colliderLocalTransform = Utils::GetColliderLocalTransform(idPair);
        m_heightManipulator->SetLocalTransform(
            colliderLocalTransform * AZ::Transform::CreateTranslation(capsuleHeight * HalfHeight * m_heightManipulator->GetAxis()));
        PhysX::EditorColliderComponentRequestBus::Event(idPair, &PhysX::EditorColliderComponentRequests::SetCapsuleHeight, capsuleHeight);
    }
}
