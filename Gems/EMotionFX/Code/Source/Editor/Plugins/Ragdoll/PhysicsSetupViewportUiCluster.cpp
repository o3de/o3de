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
        m_subModes[SubMode::Null] = AZStd::make_unique<PhysicsSetupManipulatorsNull>();
        m_subModes[SubMode::ColliderTranslation] = AZStd::make_unique<ColliderTranslationManipulators>();
        m_subModes[SubMode::ColliderRotation] = AZStd::make_unique<ColliderRotationManipulators>();
        m_subModes[SubMode::ColliderDimensions] = AZStd::make_unique<ColliderCapsuleManipulators>();
        m_subModes[SubMode::JointLimitParentRotation] = AZStd::make_unique<JointLimitRotationManipulators>(JointLimitFrame::Parent);
        m_subModes[SubMode::JointLimitChildRotation] = AZStd::make_unique<JointLimitRotationManipulators>(JointLimitFrame::Child);
        m_subModes[SubMode::JointSwingLimits] = AZStd::make_unique<JointSwingLimitManipulators>();
        m_subModes[SubMode::JointTwistLimits] = AZStd::make_unique<JointTwistLimitManipulators>();
        m_buttonData.resize(static_cast<size_t>(SubMode::NumModes));
        m_buttonData[static_cast<size_t>(SubMode::Null)] = ButtonData();
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
        AZ_Assert(modeIndex < m_buttonData.size(), "Invalid mode index %i.", modeIndex);

        const AZ::s32 viewportId = GetViewportId();
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            viewportId, &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::ClearClusterActiveButton, m_colliderClusterId);
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            viewportId, &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::ClearClusterActiveButton, m_jointLimitClusterId);

        if (m_buttonData[modeIndex].m_clusterId != AzToolsFramework::ViewportUi::InvalidClusterId)
        {
            AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
                viewportId,
                &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::SetClusterActiveButton,
                m_buttonData[modeIndex].m_clusterId,
                m_buttonData[modeIndex].m_buttonId);
        }
    }

    static PhysicsSetupViewportUiCluster::ButtonData RegisterClusterButton(
        AZ::s32 viewportId, AzToolsFramework::ViewportUi::ClusterId clusterId, const char* iconName, const char* tooltip)
    {
        AzToolsFramework::ViewportUi::ButtonId buttonId;
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::EventResult(
            buttonId, viewportId, &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::CreateClusterButton, clusterId,
            AZStd::string::format(":/stylesheet/img/UI20/toolbar/%s.svg", iconName));

        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            viewportId, &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::SetClusterButtonTooltip, clusterId, buttonId, tooltip);

        return {clusterId, buttonId};
    }

    void PhysicsSetupViewportUiCluster::UpdateClusters(PhysicsSetupManipulatorData physicsSetupManipulatorData)
    {
        m_physicsSetupManipulatorData = physicsSetupManipulatorData;
        const bool hasColliders = m_physicsSetupManipulatorData.HasColliders();
        const bool hasCapsuleCollider = m_physicsSetupManipulatorData.HasCapsuleCollider();
        const bool hasJointLimit = m_physicsSetupManipulatorData.HasJointLimit();
        const bool hasChanged =
            (hasColliders != m_hasColliders) || (hasCapsuleCollider != m_hasCapsuleCollider) || (hasJointLimit != m_hasJointLimit);

        if (hasChanged)
        {
            AZStd::fill(m_buttonData.begin(), m_buttonData.end(), ButtonData());
            DestroyClusterIfExists();
            m_hasColliders = hasColliders;
            m_hasCapsuleCollider = hasCapsuleCollider;
            m_hasJointLimit = hasJointLimit;
            const AZ::s32 viewportId = GetViewportId();

            if (m_hasColliders)
            {
                AzToolsFramework::ViewportUi::ViewportUiRequestBus::EventResult(
                    m_colliderClusterId, viewportId, &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::CreateCluster,
                    AzToolsFramework::ViewportUi::Alignment::TopLeft);
                m_buttonData[static_cast<size_t>(SubMode::ColliderTranslation)] =
                    RegisterClusterButton(viewportId, m_colliderClusterId, "Move", ColliderTranslationTooltip);
                m_buttonData[static_cast<size_t>(SubMode::ColliderRotation)] =
                    RegisterClusterButton(viewportId, m_colliderClusterId, "Rotate", ColliderRotationTooltip);
            }

            if (m_hasCapsuleCollider)
            {
                m_buttonData[static_cast<size_t>(SubMode::ColliderDimensions)] =
                    RegisterClusterButton(viewportId, m_colliderClusterId, "Scale", ColliderDimensionsTooltip);
            }

            if (m_hasColliders)
            {
                const auto onColliderButtonClicked = [this](AzToolsFramework::ViewportUi::ButtonId buttonId)
                {
                    if (buttonId == m_buttonData[static_cast<size_t>(SubMode::ColliderTranslation)].m_buttonId)
                    {
                        SetCurrentMode(SubMode::ColliderTranslation);
                    }
                    else if (buttonId == m_buttonData[static_cast<size_t>(SubMode::ColliderRotation)].m_buttonId)
                    {
                        SetCurrentMode(SubMode::ColliderRotation);
                    }
                    else if (buttonId == m_buttonData[static_cast<size_t>(SubMode::ColliderDimensions)].m_buttonId)
                    {
                        SetCurrentMode(SubMode::ColliderDimensions);
                    }
                };

                m_colliderModeSelectionHandler = AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler(onColliderButtonClicked);
                AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
                    viewportId, &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::RegisterClusterEventHandler, m_colliderClusterId,
                    m_colliderModeSelectionHandler);
            }

            if (m_hasJointLimit)
            {
                AzToolsFramework::ViewportUi::ViewportUiRequestBus::EventResult(
                    m_jointLimitClusterId, viewportId, &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::CreateCluster,
                    AzToolsFramework::ViewportUi::Alignment::TopLeft);

                m_buttonData[static_cast<size_t>(SubMode::JointLimitParentRotation)] =
                    RegisterClusterButton(viewportId, m_jointLimitClusterId, "joints/ParentFrame", JointLimitParentRotationTooltip);
                m_buttonData[static_cast<size_t>(SubMode::JointLimitChildRotation)] =
                    RegisterClusterButton(viewportId, m_jointLimitClusterId, "joints/ChildFrame", JointLimitChildRotationTooltip);
                m_buttonData[static_cast<size_t>(SubMode::JointSwingLimits)] =
                    RegisterClusterButton(viewportId, m_jointLimitClusterId, "joints/SwingLimits", JointLimitSwingTooltip);
                m_buttonData[static_cast<size_t>(SubMode::JointTwistLimits)] =
                    RegisterClusterButton(viewportId, m_jointLimitClusterId, "joints/TwistLimits", JointLimitTwistTooltip);
                m_buttonData[static_cast<size_t>(SubMode::JointLimitOptimization)] =
                    RegisterClusterButton(viewportId, m_jointLimitClusterId, "AutoFit", JointLimitAutofitTooltip);

                const auto onJointLimitButtonClicked = [this](AzToolsFramework::ViewportUi::ButtonId buttonId)
                {
                    if (buttonId == m_buttonData[static_cast<size_t>(SubMode::JointLimitParentRotation)].m_buttonId)
                    {
                        SetCurrentMode(SubMode::JointLimitParentRotation);
                    }
                    else if (buttonId == m_buttonData[static_cast<size_t>(SubMode::JointLimitChildRotation)].m_buttonId)
                    {
                        SetCurrentMode(SubMode::JointLimitChildRotation);
                    }
                    else if (buttonId == m_buttonData[static_cast<size_t>(SubMode::JointSwingLimits)].m_buttonId)
                    {
                        SetCurrentMode(SubMode::JointSwingLimits);
                    }
                    else if (buttonId == m_buttonData[static_cast<size_t>(SubMode::JointTwistLimits)].m_buttonId)
                    {
                        SetCurrentMode(SubMode::JointTwistLimits);
                    }
                    else if (buttonId == m_buttonData[static_cast<size_t>(SubMode::JointLimitOptimization)].m_buttonId)
                    {
                        OptimizeJointLimits(m_physicsSetupManipulatorData);
                    }
                };

                m_jointLimitModeSelectionHandler = AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler(onJointLimitButtonClicked);
                AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
                    viewportId,
                    &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::RegisterClusterEventHandler,
                    m_jointLimitClusterId,
                    m_jointLimitModeSelectionHandler);
            }
        }

        const bool isJointLimitSubMode = (m_subMode == SubMode::JointLimitParentRotation ||
            m_subMode == SubMode::JointLimitChildRotation || m_subMode == SubMode::JointSwingLimits ||
            m_subMode == SubMode::JointTwistLimits);
        const bool modeValid =
            ((m_subMode == SubMode::ColliderTranslation || m_subMode == SubMode::ColliderRotation) && hasColliders) ||
            (m_subMode == SubMode::ColliderDimensions && hasCapsuleCollider) ||
            (isJointLimitSubMode && hasJointLimit);

        if (!modeValid)
        {
            SetCurrentMode(SubMode::Null);
        }
        else
        {
            SetCurrentMode(m_subMode);
        }
    }

    void PhysicsSetupViewportUiCluster::DestroyClusterIfExists()
    {
        if (m_jointLimitClusterId != AzToolsFramework::ViewportUi::InvalidClusterId)
        {
            AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
                GetViewportId(), &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::RemoveCluster, m_jointLimitClusterId);
            m_jointLimitClusterId = AzToolsFramework::ViewportUi::InvalidClusterId;
        }
        if (m_colliderClusterId != AzToolsFramework::ViewportUi::InvalidClusterId)
        {
            AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
                GetViewportId(), &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::RemoveCluster, m_colliderClusterId);
            m_colliderClusterId = AzToolsFramework::ViewportUi::InvalidClusterId;
        }
    }
} // namespace EMotionFX
