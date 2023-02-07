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

    void ColliderCapsuleMode::Setup(const AZ::EntityComponentIdPair& idPair)
    {
        m_entityComponentIdPair = idPair;
        const bool allowAsymmetricalEditing = true;
        m_capsuleViewportEdit = AZStd::make_unique<AzToolsFramework::CapsuleViewportEdit>(allowAsymmetricalEditing);
        m_capsuleViewportEdit->InstallGetManipulatorSpace(
            [this]()
            {
                AZ::Transform worldTransform = AZ::Transform::CreateIdentity();
                AZ::TransformBus::EventResult(worldTransform, m_entityComponentIdPair.GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
                return worldTransform;
            });
        m_capsuleViewportEdit->InstallGetNonUniformScale(
            [this]()
            {
                AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
                AZ::NonUniformScaleRequestBus::EventResult(
                    nonUniformScale, m_entityComponentIdPair.GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);
                return nonUniformScale;
            });
        m_capsuleViewportEdit->InstallGetTranslationOffset(
            [this]()
            {
                AZ::Vector3 colliderTranslation = AZ::Vector3::CreateZero();
                EditorColliderComponentRequestBus::EventResult(
                    colliderTranslation, m_entityComponentIdPair, &EditorColliderComponentRequests::GetColliderOffset);
                return colliderTranslation;
            });
        m_capsuleViewportEdit->InstallGetRotationOffset(
            [this]()
            {
                AZ::Quaternion colliderRotation = AZ::Quaternion::CreateIdentity();
                EditorColliderComponentRequestBus::EventResult(
                    colliderRotation, m_entityComponentIdPair, &EditorColliderComponentRequests::GetColliderRotation);
                return colliderRotation;
            });
        m_capsuleViewportEdit->InstallGetCapsuleRadius(
            [this]()
            {
                float capsuleRadius = 0.0f;
                EditorColliderComponentRequestBus::EventResult(
                    capsuleRadius, m_entityComponentIdPair, &EditorColliderComponentRequests::GetCapsuleRadius);
                return capsuleRadius;
            });
        m_capsuleViewportEdit->InstallGetCapsuleHeight(
            [this]()
            {
                float capsuleHeight = 0.0f;
                EditorColliderComponentRequestBus::EventResult(
                    capsuleHeight, m_entityComponentIdPair, &EditorColliderComponentRequests::GetCapsuleHeight);
                return capsuleHeight;
            });
        m_capsuleViewportEdit->InstallSetCapsuleRadius(
            [this](float radius)
            {
                EditorColliderComponentRequestBus::Event(
                    m_entityComponentIdPair, &EditorColliderComponentRequests::SetCapsuleRadius, radius);
            });
        m_capsuleViewportEdit->InstallSetCapsuleHeight(
            [this](float height)
            {
                EditorColliderComponentRequestBus::Event(
                    m_entityComponentIdPair, &EditorColliderComponentRequests::SetCapsuleHeight, height);
            });
        m_capsuleViewportEdit->InstallSetTranslationOffset(
            [this](const AZ::Vector3& translationOffset)
            {
                EditorColliderComponentRequestBus::Event(
                    m_entityComponentIdPair, &EditorColliderComponentRequests::SetColliderOffset, translationOffset);
            });
        m_capsuleViewportEdit->Setup(AzToolsFramework::g_mainManipulatorManagerId);
        m_capsuleViewportEdit->AddEntityComponentIdPair(idPair);
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(idPair.GetEntityId());
    }

    void ColliderCapsuleMode::Refresh([[maybe_unused]] const AZ::EntityComponentIdPair& idPair)
    {
        if (m_capsuleViewportEdit)
        {
            m_capsuleViewportEdit->UpdateManipulators();
        }
    }

    void ColliderCapsuleMode::Teardown([[maybe_unused]] const AZ::EntityComponentIdPair& idPair)
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        if (m_capsuleViewportEdit)
        {
            m_capsuleViewportEdit->Teardown();
            m_capsuleViewportEdit.reset();
        }
        m_entityComponentIdPair = AZ::EntityComponentIdPair();
    }

    void ColliderCapsuleMode::ResetValues([[maybe_unused]] const AZ::EntityComponentIdPair& idPair)
    {
        if (m_capsuleViewportEdit)
        {
            m_capsuleViewportEdit->ResetValues();
        }
    }

    void ColliderCapsuleMode::DisplayEntityViewport(
        const AzFramework::ViewportInfo& viewportInfo,
        [[maybe_unused]] AzFramework::DebugDisplayRequests& debugDisplay)
    {
        if (m_capsuleViewportEdit)
        {
            const AzFramework::CameraState cameraState = AzToolsFramework::GetCameraState(viewportInfo.m_viewportId);
            m_capsuleViewportEdit->OnCameraStateChanged(cameraState);
        }
    }
} // namespace PhysX
