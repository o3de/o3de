/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ColliderOffsetMode.h"
#include <PhysX/EditorColliderComponentRequestBus.h>

#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/ComponentBus.h>

namespace PhysX
{
    AZ_CLASS_ALLOCATOR_IMPL(ColliderOffsetMode, AZ::SystemAllocator);

    ColliderOffsetMode::ColliderOffsetMode()
        : m_translationManipulators(AzToolsFramework::TranslationManipulators::Dimensions::Three,
            AZ::Transform::Identity(), AZ::Vector3::CreateOne())
    {
    }

    void ColliderOffsetMode::Setup(const AZ::EntityComponentIdPair& idPair)
    {
        AZ::Transform worldTransform;
        AZ::TransformBus::EventResult(worldTransform, idPair.GetEntityId(), &AZ::TransformInterface::GetWorldTM);

        AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(nonUniformScale, idPair.GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);

        AZ::Vector3 colliderOffset;
        PhysX::EditorColliderComponentRequestBus::EventResult(colliderOffset, idPair, &PhysX::EditorColliderComponentRequests::GetColliderOffset);

        m_translationManipulators.SetSpace(worldTransform);
        m_translationManipulators.SetNonUniformScale(nonUniformScale);
        m_translationManipulators.SetLocalPosition(colliderOffset);
        m_translationManipulators.AddEntityComponentIdPair(idPair);
        m_translationManipulators.Register(AzToolsFramework::g_mainManipulatorManagerId);
        AzToolsFramework::ConfigureTranslationManipulatorAppearance3d(&m_translationManipulators);

        m_translationManipulators.InstallLinearManipulatorMouseMoveCallback([this, idPair](
            const AzToolsFramework::LinearManipulator::Action& action)
        {
            OnManipulatorMoved(action.m_start.m_localPosition, action.m_current.m_localPositionOffset, idPair);
        });

        m_translationManipulators.InstallPlanarManipulatorMouseMoveCallback([this, idPair](
            const AzToolsFramework::PlanarManipulator::Action& action)
        {
            OnManipulatorMoved(action.m_start.m_localPosition, action.m_current.m_localOffset, idPair);
        });

        m_translationManipulators.InstallSurfaceManipulatorMouseMoveCallback([this, idPair](
            const AzToolsFramework::SurfaceManipulator::Action& action)
        {
            OnManipulatorMoved(action.m_start.m_localPosition, action.m_current.m_localOffset, idPair);
        });
    }

    void ColliderOffsetMode::Refresh(const AZ::EntityComponentIdPair& idPair)
    {
        AZ::Vector3 colliderOffset = AZ::Vector3::CreateZero();
        PhysX::EditorColliderComponentRequestBus::EventResult(colliderOffset, idPair, &PhysX::EditorColliderComponentRequests::GetColliderOffset);
        m_translationManipulators.SetLocalPosition(colliderOffset);
    }

    void ColliderOffsetMode::Teardown(const AZ::EntityComponentIdPair& idPair)
    {
        m_translationManipulators.RemoveEntityComponentIdPair(idPair);
        m_translationManipulators.Unregister();
    }

    void ColliderOffsetMode::OnManipulatorMoved(const AZ::Vector3& startPosition, const AZ::Vector3& offset, const AZ::EntityComponentIdPair& idPair)
    {
        AZ::Transform worldTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldTransform, idPair.GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
        const float scale = AZ::GetMax(AZ::MinTransformScale, worldTransform.GetUniformScale());
        const AZ::Vector3 newPosition = startPosition + offset / scale;
        m_translationManipulators.SetLocalPosition(newPosition);
        PhysX::EditorColliderComponentRequestBus::Event(idPair, &PhysX::EditorColliderComponentRequests::SetColliderOffset, newPosition);
    }

    void ColliderOffsetMode::ResetValues(const AZ::EntityComponentIdPair& idPair)
    {
        PhysX::EditorColliderComponentRequestBus::Event(idPair, &PhysX::EditorColliderComponentRequests::SetColliderOffset, AZ::Vector3::CreateZero());
    }
}
