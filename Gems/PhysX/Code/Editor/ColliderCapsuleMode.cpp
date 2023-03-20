/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/ColliderCapsuleMode.h>

#include <AzToolsFramework/ComponentModes/BaseShapeComponentMode.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <PhysX/EditorColliderComponentRequestBus.h>

namespace PhysX
{
    AZ_CLASS_ALLOCATOR_IMPL(ColliderCapsuleMode, AZ::SystemAllocator);

    void ColliderCapsuleMode::Setup(const AZ::EntityComponentIdPair& idPair)
    {
        m_entityComponentIdPair = idPair;
        const bool allowAsymmetricalEditing = true;
        m_capsuleViewportEdit = AZStd::make_unique<AzToolsFramework::CapsuleViewportEdit>(allowAsymmetricalEditing);
        AzToolsFramework::InstallBaseShapeViewportEditFunctions(m_capsuleViewportEdit.get(), idPair);
        m_capsuleViewportEdit->InstallGetCapsuleRadius(
            [this]()
            {
                float capsuleRadius = 0.0f;
                EditorPrimitiveColliderComponentRequestBus::EventResult(
                    capsuleRadius, m_entityComponentIdPair, &EditorPrimitiveColliderComponentRequests::GetCapsuleRadius);
                return capsuleRadius;
            });
        m_capsuleViewportEdit->InstallGetCapsuleHeight(
            [this]()
            {
                float capsuleHeight = 0.0f;
                EditorPrimitiveColliderComponentRequestBus::EventResult(
                    capsuleHeight, m_entityComponentIdPair, &EditorPrimitiveColliderComponentRequests::GetCapsuleHeight);
                return capsuleHeight;
            });
        m_capsuleViewportEdit->InstallSetCapsuleRadius(
            [this](float radius)
            {
                EditorPrimitiveColliderComponentRequestBus::Event(
                    m_entityComponentIdPair, &EditorPrimitiveColliderComponentRequests::SetCapsuleRadius, radius);
            });
        m_capsuleViewportEdit->InstallSetCapsuleHeight(
            [this](float height)
            {
                EditorPrimitiveColliderComponentRequestBus::Event(
                    m_entityComponentIdPair, &EditorPrimitiveColliderComponentRequests::SetCapsuleHeight, height);
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
