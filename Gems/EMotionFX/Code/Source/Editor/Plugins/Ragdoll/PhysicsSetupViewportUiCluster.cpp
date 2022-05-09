/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/Transform.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/RenderPlugin/ViewportPluginBus.h>
#include <Editor/Plugins/Ragdoll/PhysicsSetupColliderTranslationManipulators.h>
#include <Editor/Plugins/Ragdoll/PhysicsSetupViewportUiCluster.h>

namespace EMotionFX
{
    PhysicsSetupViewportUiCluster::PhysicsSetupViewportUiCluster()
    {
        m_subModes[SubMode::ColliderTranslation] = AZStd::make_unique<PhysicsSetupColliderTranslationManipulators>();
    }

    void PhysicsSetupViewportUiCluster::SetCurrentMode(SubMode mode)
    {
        AZ_Assert(m_subModes.find(mode) != m_subModes.end(), "Submode not found:%d", mode);
        m_subModes[m_subMode]->Teardown();
        m_subMode = mode;
        m_subModes[m_subMode]->Setup(m_physicsSetupManipulatorData);

        const auto modeIndex = static_cast<size_t>(mode);
        AZ_Assert(modeIndex < m_buttonIds.size(), "Invalid mode index %i.", modeIndex);
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::SetClusterActiveButton, m_clusterId, m_buttonIds[modeIndex]);
    }

    static AzToolsFramework::ViewportUi::ButtonId RegisterClusterButton(
        AZ::s32 viewportId, AzToolsFramework::ViewportUi::ClusterId clusterId, const char* iconName)
    {
        AzToolsFramework::ViewportUi::ButtonId buttonId;
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::EventResult(
            buttonId, viewportId, &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::CreateClusterButton, clusterId,
            AZStd::string::format(":/stylesheet/img/UI20/toolbar/%s.svg", iconName));

        return buttonId;
    }

    void PhysicsSetupViewportUiCluster::CreateClusterIfNoneExists(PhysicsSetupManipulatorData physicsSetupManipulatorData)
    {
        m_physicsSetupManipulatorData = physicsSetupManipulatorData;

        if (m_clusterId == AzToolsFramework::ViewportUi::InvalidClusterId)
        {
            AZ::s32 viewportId = -1;
            EMStudio::ViewportPluginRequestBus::BroadcastResult(viewportId, &EMStudio::ViewportPluginRequestBus::Events::GetViewportId);
            AzToolsFramework::ViewportUi::ViewportUiRequestBus::EventResult(
                m_clusterId, viewportId, &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::CreateCluster,
                AzToolsFramework::ViewportUi::Alignment::TopLeft);

            m_buttonIds.resize(static_cast<size_t>(SubMode::NumModes));
            m_buttonIds[static_cast<size_t>(SubMode::ColliderTranslation)] = RegisterClusterButton(viewportId, m_clusterId, "Move");

            const auto onButtonClicked = [this](AzToolsFramework::ViewportUi::ButtonId buttonId)
            {
                if (buttonId == m_buttonIds[static_cast<size_t>(SubMode::ColliderTranslation)])
                {
                    SetCurrentMode(SubMode::ColliderTranslation);
                }
            };

            m_modeSelectionHandler = AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler(onButtonClicked);
            AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
                AzToolsFramework::ViewportUi::DefaultViewportId,
                &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::RegisterClusterEventHandler, m_clusterId,
                m_modeSelectionHandler);
        }
        SetCurrentMode(m_subMode);
    }

    void PhysicsSetupViewportUiCluster::DestroyClusterIfExists()
    {
        if (m_clusterId != AzToolsFramework::ViewportUi::InvalidClusterId)
        {
            AZ::s32 viewportId = -1;
            EMStudio::ViewportPluginRequestBus::BroadcastResult(viewportId, &EMStudio::ViewportPluginRequestBus::Events::GetViewportId);
            AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
                viewportId, &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::RemoveCluster, m_clusterId);
            m_clusterId = AzToolsFramework::ViewportUi::InvalidClusterId;
            m_subModes[m_subMode]->Teardown();
        }
    }
} // namespace EMotionFX
