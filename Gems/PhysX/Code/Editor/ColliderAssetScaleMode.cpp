/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ColliderAssetScaleMode.h"
#include <PhysX/EditorColliderComponentRequestBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Viewport/ViewportColors.h>
#include <AzFramework/Viewport/ViewportConstants.h>
#include <AzToolsFramework/Manipulators/AngularManipulator.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

namespace PhysX
{
    AZ_CLASS_ALLOCATOR_IMPL(ColliderAssetScaleMode, AZ::SystemAllocator);

    namespace 
    {
        const float MinAssetScale = 0.001f;
        const AZ::Vector3 ResetScale = AZ::Vector3::CreateOne();
    }

    ColliderAssetScaleMode::ColliderAssetScaleMode()
        : m_dimensionsManipulators(AZ::Transform::Identity())
    {

    }

    void ColliderAssetScaleMode::Setup(const AZ::EntityComponentIdPair& idPair)
    {
        AZ::Transform colliderWorldTransform = AZ::Transform::Identity();
        PhysX::EditorColliderComponentRequestBus::EventResult(colliderWorldTransform, idPair, &PhysX::EditorColliderComponentRequests::GetColliderWorldTransform);

        m_dimensionsManipulators.SetSpace(colliderWorldTransform);
        m_dimensionsManipulators.AddEntityComponentIdPair(idPair);
        m_dimensionsManipulators.Register(AzToolsFramework::g_mainManipulatorManagerId);
        m_dimensionsManipulators.SetAxes(
            AZ::Vector3::CreateAxisX(),
            AZ::Vector3::CreateAxisY(),
            AZ::Vector3::CreateAxisZ());
        m_dimensionsManipulators.ConfigureView(
            AzFramework::ViewportConstants::DefaultLinearManipulatorAxisLength,
            AzFramework::ViewportColors::XAxisColor,
            AzFramework::ViewportColors::YAxisColor,
            AzFramework::ViewportColors::ZAxisColor);
        
        m_dimensionsManipulators.InstallAxisLeftMouseDownCallback(
            [this, idPair] (const AzToolsFramework::LinearManipulator::Action& action)
        {
            AZ_UNUSED(action);
            OnManipulatorDown(idPair);
        });

        m_dimensionsManipulators.InstallAxisMouseMoveCallback(
            [this, idPair] (const AzToolsFramework::LinearManipulator::Action& action)
        {
            OnManipulatorMoved(action.m_start.m_sign * action.LocalScaleOffset() + m_initialScale, idPair);
        });

        m_dimensionsManipulators.InstallUniformLeftMouseDownCallback(
            [this, idPair](const AzToolsFramework::LinearManipulator::Action& action)
        {
            AZ_UNUSED(action);
            OnManipulatorDown(idPair);
        });

        m_dimensionsManipulators.InstallUniformMouseMoveCallback(
            [this, idPair] (const AzToolsFramework::LinearManipulator::Action& action)
        {
            // Uniform scale manipulator will only set the Z value for the offset, use this to create uniform scale.
            const AZ::Vector3 uniformScale(action.LocalScaleOffset().GetZ());
            OnManipulatorMoved(uniformScale + m_initialScale, idPair);
        });
    }

    void ColliderAssetScaleMode::Refresh(const AZ::EntityComponentIdPair& idPair)
    {
        AZ::Transform colliderWorldTransform = AZ::Transform::Identity();
        PhysX::EditorColliderComponentRequestBus::EventResult(colliderWorldTransform, idPair, &PhysX::EditorColliderComponentRequests::GetColliderWorldTransform);
        m_dimensionsManipulators.SetSpace(colliderWorldTransform);
    }

    void ColliderAssetScaleMode::Teardown(const AZ::EntityComponentIdPair& idPair)
    {
        m_dimensionsManipulators.RemoveEntityComponentIdPair(idPair);
        m_dimensionsManipulators.Unregister();
    }

    void ColliderAssetScaleMode::OnManipulatorDown(const AZ::EntityComponentIdPair& idPair)
    {
        // Remember the initial asset scale here.
        PhysX::EditorMeshColliderComponentRequestBus::EventResult(m_initialScale, idPair, &PhysX::EditorMeshColliderComponentRequests::GetAssetScale);
    }

    void ColliderAssetScaleMode::OnManipulatorMoved(const AZ::Vector3& scale, const AZ::EntityComponentIdPair& idPair)
    {
        AZ::Vector3 clampedScale = scale.GetMax(AZ::Vector3(MinAssetScale));
        PhysX::EditorMeshColliderComponentRequestBus::Event(idPair, &PhysX::EditorMeshColliderComponentRequests::SetAssetScale, clampedScale);
    }

    void ColliderAssetScaleMode::ResetValues(const AZ::EntityComponentIdPair& idPair)
    {
        PhysX::EditorMeshColliderComponentRequestBus::Event(idPair, &PhysX::EditorMeshColliderComponentRequests::SetAssetScale, ResetScale);
    }
} // namespace PhysX
