/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/Transform.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/RenderPlugin/ViewportPluginBus.h>
#include <Editor/Plugins/Ragdoll/ColliderCapsuleManipulators.h>
#include <Editor/Plugins/Ragdoll/ColliderRotationManipulators.h>
#include <Editor/Plugins/Ragdoll/ColliderTranslationManipulators.h>
#include <Editor/Plugins/Ragdoll/JointLimitOptimizer.h>
#include <Editor/Plugins/Ragdoll/JointLimitRotationManipulators.h>
#include <Editor/Plugins/Ragdoll/JointSwingLimitManipulators.h>
#include <Editor/Plugins/Ragdoll/JointTwistLimitManipulators.h>
#include <Editor/Plugins/Ragdoll/PhysicsSetupViewportUiCluster.h>

namespace EMotionFX
{
    PhysicsSetupViewportUiCluster::PhysicsSetupViewportUiCluster()
    {
        m_subModes[SubMode::ColliderTranslation] = AZStd::make_unique<ColliderTranslationManipulators>();
        m_subModes[SubMode::ColliderRotation] = AZStd::make_unique<ColliderRotationManipulators>();
        m_subModes[SubMode::ColliderDimensions] = AZStd::make_unique<ColliderCapsuleManipulators>();
        m_subModes[SubMode::JointLimitParentRotation] = AZStd::make_unique<JointLimitRotationManipulators>(JointLimitFrame::Parent);
        m_subModes[SubMode::JointLimitChildRotation] = AZStd::make_unique<JointLimitRotationManipulators>(JointLimitFrame::Child);
        m_subModes[SubMode::JointSwingLimits] = AZStd::make_unique<JointSwingLimitManipulators>();
        m_subModes[SubMode::JointTwistLimits] = AZStd::make_unique<JointTwistLimitManipulators>();
    }

    AZ::s32 PhysicsSetupViewportUiCluster::GetViewportId() const
    {
        if (!m_viewportId.has_value())
        {
            AZ::s32 viewportId = -1;
            EMStudio::ViewportPluginRequestBus::BroadcastResult(viewportId, &EMStudio::ViewportPluginRequestBus::Events::GetViewportId);
            m_viewportId = viewportId;
        }
        return m_viewportId.value();
    }

    void PhysicsSetupViewportUiCluster::SetCurrentMode(SubMode mode)
    {
        AZ_Assert(m_subModes.find(mode) != m_subModes.end(), "Submode not found:%d", static_cast<AZ::u32>(mode));
        m_subModes[m_subMode]->Teardown();
        m_subMode = mode;
        m_subModes[m_subMode]->SetViewportId(GetViewportId());
        m_subModes[m_subMode]->Setup(m_physicsSetupManipulatorData);

        const auto modeIndex = static_cast<size_t>(mode);
        AZ_Assert(modeIndex < m_buttonIds.size(), "Invalid mode index %i.", modeIndex);
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            GetViewportId(), &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::SetClusterActiveButton, m_clusterId,
            m_buttonIds[modeIndex]);
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
        const bool hasCapsuleCollider = m_physicsSetupManipulatorData.HasCapsuleCollider();
        const bool hasJointLimit = m_physicsSetupManipulatorData.HasJointLimit();
        const bool hasChanged = (hasCapsuleCollider != m_hasCapsuleCollider) || (hasJointLimit != m_hasJointLimit);

        if (hasChanged)
        {
            DestroyClusterIfExists();
        }

        if (m_clusterId == AzToolsFramework::ViewportUi::InvalidClusterId || hasChanged)
        {
            m_hasCapsuleCollider = hasCapsuleCollider;
            m_hasJointLimit = hasJointLimit;
            const AZ::s32 viewportId = GetViewportId();
            AzToolsFramework::ViewportUi::ViewportUiRequestBus::EventResult(
                m_clusterId, viewportId, &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::CreateCluster,
                AzToolsFramework::ViewportUi::Alignment::TopLeft);

            m_buttonIds.resize(static_cast<size_t>(SubMode::NumModes));
            m_buttonIds[static_cast<size_t>(SubMode::ColliderTranslation)] = RegisterClusterButton(viewportId, m_clusterId, "Move");
            m_buttonIds[static_cast<size_t>(SubMode::ColliderRotation)] = RegisterClusterButton(viewportId, m_clusterId, "Rotate");
            if (m_hasCapsuleCollider)
            {
                m_buttonIds[static_cast<size_t>(SubMode::ColliderDimensions)] = RegisterClusterButton(viewportId, m_clusterId, "Scale");
            }
            if (m_hasJointLimit)
            {
                m_buttonIds[static_cast<size_t>(SubMode::JointLimitParentRotation)] =
                    RegisterClusterButton(viewportId, m_clusterId, "Rotate");
                m_buttonIds[static_cast<size_t>(SubMode::JointLimitChildRotation)] =
                    RegisterClusterButton(viewportId, m_clusterId, "Rotate");
                m_buttonIds[static_cast<size_t>(SubMode::JointSwingLimits)] =
                    RegisterClusterButton(viewportId, m_clusterId, "joints/SwingLimits");
                m_buttonIds[static_cast<size_t>(SubMode::JointTwistLimits)] =
                    RegisterClusterButton(viewportId, m_clusterId, "joints/TwistLimits");
                m_buttonIds[static_cast<size_t>(SubMode::JointLimitOptimization)] =
                    RegisterClusterButton(viewportId, m_clusterId, "Question");
            }

            const auto onButtonClicked = [this](AzToolsFramework::ViewportUi::ButtonId buttonId)
            {
                if (buttonId == m_buttonIds[static_cast<size_t>(SubMode::ColliderTranslation)])
                {
                    SetCurrentMode(SubMode::ColliderTranslation);
                }
                else if (buttonId == m_buttonIds[static_cast<size_t>(SubMode::ColliderRotation)])
                {
                    SetCurrentMode(SubMode::ColliderRotation);
                }
                else if (buttonId == m_buttonIds[static_cast<size_t>(SubMode::ColliderDimensions)])
                {
                    SetCurrentMode(SubMode::ColliderDimensions);
                }
                else if (buttonId == m_buttonIds[static_cast<size_t>(SubMode::JointLimitParentRotation)])
                {
                    SetCurrentMode(SubMode::JointLimitParentRotation);
                }
                else if (buttonId == m_buttonIds[static_cast<size_t>(SubMode::JointLimitChildRotation)])
                {
                    SetCurrentMode(SubMode::JointLimitChildRotation);
                }
                else if (buttonId == m_buttonIds[static_cast<size_t>(SubMode::JointSwingLimits)])
                {
                    SetCurrentMode(SubMode::JointSwingLimits);
                }
                else if (buttonId == m_buttonIds[static_cast<size_t>(SubMode::JointTwistLimits)])
                {
                    SetCurrentMode(SubMode::JointTwistLimits);
                }
                else if (buttonId == m_buttonIds[static_cast<size_t>(SubMode::JointLimitOptimization)])
                {
                    OptimizeJointLimits(m_physicsSetupManipulatorData);
                }
            };

            m_modeSelectionHandler = AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler(onButtonClicked);
            AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
                viewportId, &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::RegisterClusterEventHandler, m_clusterId,
                m_modeSelectionHandler);
        }
        SetCurrentMode(m_subMode);
    }

    void PhysicsSetupViewportUiCluster::DestroyClusterIfExists()
    {
        if (m_clusterId != AzToolsFramework::ViewportUi::InvalidClusterId)
        {
            AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
                GetViewportId(), &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::RemoveCluster, m_clusterId);
            m_clusterId = AzToolsFramework::ViewportUi::InvalidClusterId;
            m_subModes[m_subMode]->Teardown();
        }
    }
} // namespace EMotionFX
