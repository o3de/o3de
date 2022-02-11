/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ColliderSphereMode.h"
#include <PhysX/EditorColliderComponentRequestBus.h>
#include <Source/Utils.h>

#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <AzToolsFramework/ComponentModes/BoxViewportEdit.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/NonUniformScaleBus.h>
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
        AZ::TransformBus::EventResult(colliderWorldTransform, idPair.GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(nonUniformScale, idPair.GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);

        const AZ::Transform colliderLocalTransform = Utils::GetColliderLocalTransform(idPair);

        float sphereRadius = 0.0f;
        PhysX::EditorColliderComponentRequestBus::EventResult(sphereRadius, idPair, &PhysX::EditorColliderComponentRequests::GetSphereRadius);

        m_radiusManipulator = AzToolsFramework::LinearManipulator::MakeShared(colliderWorldTransform);
        m_radiusManipulator->AddEntityComponentIdPair(idPair);
        m_radiusManipulator->SetAxis(ManipulatorAxis);
        m_radiusManipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);
        m_radiusManipulator->SetLocalTransform(colliderLocalTransform * AZ::Transform::CreateTranslation(ManipulatorAxis * sphereRadius));
        m_radiusManipulator->SetNonUniformScale(nonUniformScale);
        m_radiusManipulator->SetBoundsDirty();

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
        AZ::TransformBus::EventResult(colliderWorldTransform, idPair.GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
        m_radiusManipulator->SetSpace(colliderWorldTransform);

        AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(nonUniformScale, idPair.GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);

        const AZ::Transform colliderLocalTransform = Utils::GetColliderLocalTransform(idPair);

        float sphereRadius = 0.0f;
        PhysX::EditorColliderComponentRequestBus::EventResult(sphereRadius, idPair, &PhysX::EditorColliderComponentRequests::GetSphereRadius);
        m_radiusManipulator->SetLocalTransform(colliderLocalTransform * AZ::Transform::CreateTranslation(ManipulatorAxis * sphereRadius));
        m_radiusManipulator->SetBoundsDirty();
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

        if (!m_radiusManipulator->EntityComponentIdPairs().empty())
        {
            const AZ::EntityComponentIdPair idPair = *m_radiusManipulator->EntityComponentIdPairs().begin();
            float radius = 0.0f;
            PhysX::EditorColliderComponentRequestBus::EventResult(radius, idPair, &PhysX::EditorColliderComponentRequests::GetSphereRadius);
            const AZ::Transform colliderLocalTransform = Utils::GetColliderLocalTransform(idPair);
            m_radiusManipulator->SetAxis(cameraState.m_side);
            m_radiusManipulator->SetLocalTransform(colliderLocalTransform * AZ::Transform::CreateTranslation(cameraState.m_side * radius));
        }
    }

    void ColliderSphereMode::OnManipulatorMoved(const AzToolsFramework::LinearManipulator::Action& action, const AZ::EntityComponentIdPair& idPair)
    {
        // manipulator offsets do not take transform scale into account, need to handle it here
        const AZ::Transform colliderLocalTransform = Utils::GetColliderLocalTransform(idPair);
        const AZ::Vector3 manipulatorPosition = AzToolsFramework::GetPositionInManipulatorFrame(
            m_radiusManipulator->GetSpace().GetUniformScale(), colliderLocalTransform, action);

        // Get the distance the manipulator has moved along the axis.
        float extent = manipulatorPosition.Dot(action.m_fixed.m_axis);

        // Clamp the distance to a small value to prevent it going negative.
        extent = AZ::GetMax(extent, MinSphereRadius);

        // Update the manipulator and sphere radius
        m_radiusManipulator->SetLocalTransform(
            Utils::GetColliderLocalTransform(idPair) * AZ::Transform::CreateTranslation(extent * action.m_fixed.m_axis));
        PhysX::EditorColliderComponentRequestBus::Event(idPair, &PhysX::EditorColliderComponentRequests::SetSphereRadius, extent);
    }
}
