/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/ColliderSphereMode.h>

#include <AzToolsFramework/ComponentModes/BaseShapeComponentMode.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <PhysX/EditorColliderComponentRequestBus.h>

namespace PhysX
{
    AZ_CLASS_ALLOCATOR_IMPL(ColliderSphereMode, AZ::SystemAllocator);

    void ColliderSphereMode::Setup(const AZ::EntityComponentIdPair& idPair)
    {
        m_entityComponentIdPair = idPair;
        m_sphereViewportEdit = AZStd::make_unique<AzToolsFramework::SphereViewportEdit>();
        AzToolsFramework::InstallBaseShapeViewportEditFunctions(m_sphereViewportEdit.get(), idPair);
        m_sphereViewportEdit->InstallGetSphereRadius(
            [this]()
            {
                float capsuleRadius = 0.0f;
                EditorPrimitiveColliderComponentRequestBus::EventResult(
                    capsuleRadius, m_entityComponentIdPair, &EditorPrimitiveColliderComponentRequests::GetSphereRadius);
                return capsuleRadius;
            });
        m_sphereViewportEdit->InstallSetSphereRadius(
            [this](float radius)
            {
                EditorPrimitiveColliderComponentRequestBus::Event(
                    m_entityComponentIdPair, &EditorPrimitiveColliderComponentRequests::SetSphereRadius, radius);
            });
        m_sphereViewportEdit->Setup(AzToolsFramework::g_mainManipulatorManagerId);
        m_sphereViewportEdit->AddEntityComponentIdPair(idPair);
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(idPair.GetEntityId());
    }

    void ColliderSphereMode::Refresh([[maybe_unused]] const AZ::EntityComponentIdPair& idPair)
    {
        if (m_sphereViewportEdit)
        {
            m_sphereViewportEdit->UpdateManipulators();
        }
    }

    void ColliderSphereMode::Teardown([[maybe_unused]] const AZ::EntityComponentIdPair& idPair)
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        if (m_sphereViewportEdit)
        {
            m_sphereViewportEdit->Teardown();
            m_sphereViewportEdit.reset();
        }
        m_entityComponentIdPair = AZ::EntityComponentIdPair();
    }

    void ColliderSphereMode::ResetValues([[maybe_unused]] const AZ::EntityComponentIdPair& idPair)
    {
        if (m_sphereViewportEdit)
        {
            m_sphereViewportEdit->ResetValues();
        }
    }

    void ColliderSphereMode::DisplayEntityViewport(
        const AzFramework::ViewportInfo& viewportInfo,
        [[maybe_unused]] AzFramework::DebugDisplayRequests& debugDisplay)
    {
        if (m_sphereViewportEdit)
        {
            const AzFramework::CameraState cameraState = AzToolsFramework::GetCameraState(viewportInfo.m_viewportId);
            m_sphereViewportEdit->OnCameraStateChanged(cameraState);
        }
    }
}
