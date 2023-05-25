/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/Configuration/JointConfiguration.h>
#include <AzFramework/Viewport/ViewportColors.h>
#include <AzFramework/Viewport/ViewportConstants.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <EMotionFX/CommandSystem/Source/JointLimitCommands.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <Editor/Plugins/Ragdoll/JointLimitRotationManipulators.h>
#include <Editor/Plugins/Ragdoll/JointSwingLimitManipulators.h>
#include <Editor/Plugins/Ragdoll/RagdollJointLimitWidget.h>

namespace EMotionFX
{
    static constexpr float ManipulatorOffsetX = 0.2f; // manipulator position offset along the joint parent frame's X axis
    static constexpr float ManipulatorScale = 400.0f; // scaling factor between linear position of manipulator and swing limit in degrees
    static constexpr float ManipulatorInverseScale = 1.0f / ManipulatorScale;

    void JointSwingLimitManipulators::Setup(const PhysicsSetupManipulatorData& physicsSetupManipulatorData)
    {
        m_physicsSetupManipulatorData = physicsSetupManipulatorData;

        if (!m_physicsSetupManipulatorData.HasJointLimit())
        {
            return;
        }

        AzFramework::DebugDisplayRequestBus::BusPtr debugDisplayBus;
        AzFramework::DebugDisplayRequestBus::Bind(debugDisplayBus, m_viewportId);
        m_debugDisplay = AzFramework::DebugDisplayRequestBus::FindFirstHandler(debugDisplayBus);

        // swing limit Y manipulator
        m_swingYManipulator = AzToolsFramework::LinearManipulator::MakeShared(AZ::Transform::CreateIdentity());
        m_swingYManipulator->SetAxis(AZ::Vector3::CreateAxisZ());
        m_swingYManipulator->Register(EMStudio::g_animManipulatorManagerId);
        {
            AzToolsFramework::ManipulatorViews views;
            views.emplace_back(AzToolsFramework::CreateManipulatorViewQuadBillboard(
                AzFramework::ViewportColors::DefaultManipulatorHandleColor, AzFramework::ViewportConstants::DefaultManipulatorHandleSize));
            m_swingYManipulator->SetViews(AZStd::move(views));
        }

        m_swingYManipulator->InstallLeftMouseDownCallback(
            [this]([[maybe_unused]] const AzToolsFramework::LinearManipulator::Action& action)
            {
                BeginEditing();
            });
        m_swingYManipulator->InstallMouseMoveCallback(
            [this](const AzToolsFramework::LinearManipulator::Action& action)
            {
                if (m_jointSwingLimitState.m_swingLimitY.has_value())
                {
                    const float newSwingLimitY = ManipulatorScale * action.LocalPosition().GetZ();
                    m_physicsSetupManipulatorData.m_jointConfiguration->SetPropertyValue(AZ::Name("SwingLimitY"), newSwingLimitY);
                    // get the value again, in case it is different from the value set due to validation
                    AZStd::optional<float> validatedSwingLimitY =
                        m_physicsSetupManipulatorData.m_jointConfiguration->GetPropertyValue(AZ::Name("SwingLimitY"));
                    if (validatedSwingLimitY.has_value())
                    {
                        m_swingYManipulator->SetLocalPosition(
                            AZ::Vector3(ManipulatorOffsetX, 0.0f, ManipulatorInverseScale * validatedSwingLimitY.value()));
                    }
                    InvalidateEditorValues();
                }
            });
        m_swingYManipulator->InstallLeftMouseUpCallback(
            [this]([[maybe_unused]] const AzToolsFramework::LinearManipulator::Action& action)
            {
                EndEditing();
            });

        // swing limit Z manipulator
        m_swingZManipulator = AzToolsFramework::LinearManipulator::MakeShared(AZ::Transform::CreateIdentity());
        m_swingZManipulator->SetAxis(AZ::Vector3::CreateAxisY());
        m_swingZManipulator->Register(EMStudio::g_animManipulatorManagerId);
        {
            AzToolsFramework::ManipulatorViews views;
            views.emplace_back(AzToolsFramework::CreateManipulatorViewQuadBillboard(
                AzFramework::ViewportColors::DefaultManipulatorHandleColor, AzFramework::ViewportConstants::DefaultManipulatorHandleSize));
            m_swingZManipulator->SetViews(AZStd::move(views));
        }

        m_swingZManipulator->InstallLeftMouseDownCallback(
            [this]([[maybe_unused]] const AzToolsFramework::LinearManipulator::Action& action)
            {
                BeginEditing();
            });
        m_swingZManipulator->InstallMouseMoveCallback(
            [this](const AzToolsFramework::LinearManipulator::Action& action)
            {
                if (m_jointSwingLimitState.m_swingLimitZ.has_value())
                {
                    const float newSwingLimitZ = ManipulatorScale * action.LocalPosition().GetY();
                    m_physicsSetupManipulatorData.m_jointConfiguration->SetPropertyValue(AZ::Name("SwingLimitZ"), newSwingLimitZ);
                    // get the value again, in case it is different from the value set due to validation
                    AZStd::optional<float> validatedSwingLimitZ =
                        m_physicsSetupManipulatorData.m_jointConfiguration->GetPropertyValue(AZ::Name("SwingLimitZ"));
                    if (validatedSwingLimitZ.has_value())
                    {
                        m_swingZManipulator->SetLocalPosition(
                            AZ::Vector3(ManipulatorOffsetX, ManipulatorInverseScale * validatedSwingLimitZ.value(), 0.0f));
                    }
                    InvalidateEditorValues();
                }
            });
        m_swingZManipulator->InstallLeftMouseUpCallback(
            [this]([[maybe_unused]] const AzToolsFramework::LinearManipulator::Action& action)
            {
                EndEditing();
            });

        Refresh();

        AZ::TickBus::Handler::BusConnect();
        PhysicsSetupManipulatorRequestBus::Handler::BusConnect();
        m_adjustJointLimitCallback = AZStd::make_unique<PhysicsSetupManipulatorCommandCallback>(this, false);
        EMStudio::GetCommandManager()->RegisterCommandCallback("AdjustJointLimit", m_adjustJointLimitCallback.get());
    }

    void JointSwingLimitManipulators::Refresh()
    {
        if (m_physicsSetupManipulatorData.HasJointLimit())
        {
            const AZ::Transform parentWorldTransform = m_physicsSetupManipulatorData.GetJointParentFrameWorld();

            AZStd::optional<float> swingLimitY =
                m_physicsSetupManipulatorData.m_jointConfiguration->GetPropertyValue(AZ::Name("SwingLimitY"));
            if (swingLimitY.has_value())
            {
                m_swingYManipulator->SetSpace(parentWorldTransform);
                m_swingYManipulator->SetLocalPosition(AZ::Vector3(ManipulatorOffsetX, 0.0f, ManipulatorInverseScale * swingLimitY.value()));
            }
            AZStd::optional<float> swingLimitZ =
                m_physicsSetupManipulatorData.m_jointConfiguration->GetPropertyValue(AZ::Name("SwingLimitZ"));
            if (swingLimitZ.has_value())
            {
                m_swingZManipulator->SetSpace(parentWorldTransform);
                m_swingZManipulator->SetLocalPosition(AZ::Vector3(ManipulatorOffsetX, ManipulatorInverseScale * swingLimitZ.value(), 0.0f));
            }
        }
    }

    void JointSwingLimitManipulators::Teardown()
    {
        if (!m_physicsSetupManipulatorData.HasJointLimit())
        {
            return;
        }

        EMStudio::GetCommandManager()->RemoveCommandCallback(m_adjustJointLimitCallback.get(), false);
        m_adjustJointLimitCallback.reset();
        PhysicsSetupManipulatorRequestBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
        if (m_swingYManipulator)
        {
            m_swingYManipulator->Unregister();
        }
        if (m_swingZManipulator)
        {
            m_swingZManipulator->Unregister();
        }
        m_swingYManipulator.reset();
        m_swingZManipulator.reset();
        m_debugDisplay = nullptr;
    }

    void JointSwingLimitManipulators::ResetValues()
    {
        if (m_physicsSetupManipulatorData.HasJointLimit())
        {
            BeginEditing();
            m_physicsSetupManipulatorData.m_jointConfiguration->SetPropertyValue(AZ::Name("SwingLimitY"), 45.0f);
            m_physicsSetupManipulatorData.m_jointConfiguration->SetPropertyValue(AZ::Name("SwingLimitZ"), 45.0f);
            EndEditing();
            Refresh();
        }
    }

    void JointSwingLimitManipulators::InvalidateEditorValues()
    {
        if (m_physicsSetupManipulatorData.m_jointLimitWidget)
        {
            m_physicsSetupManipulatorData.m_jointLimitWidget->InvalidateValues();
        }
    }

    void JointSwingLimitManipulators::OnUnderlyingPropertiesChanged()
    {
        Refresh();
    }

    void JointSwingLimitManipulators::BeginEditing()
    {
        m_jointSwingLimitState.m_swingLimitY =
            m_physicsSetupManipulatorData.m_jointConfiguration->GetPropertyValue(AZ::Name("SwingLimitY"));
        m_jointSwingLimitState.m_swingLimitZ =
            m_physicsSetupManipulatorData.m_jointConfiguration->GetPropertyValue(AZ::Name("SwingLimitZ"));

        CreateCommandAdjustJointLimit(m_commandGroup, m_physicsSetupManipulatorData);
    }

    void JointSwingLimitManipulators::EndEditing()
    {
        ExecuteCommandAdjustJointLimit(m_commandGroup, m_physicsSetupManipulatorData);
    }

    void JointSwingLimitManipulators::OnTick([[maybe_unused]] float delta, [[maybe_unused]] AZ::ScriptTimePoint timePoint)
    {
        if (!m_debugDisplay || !m_physicsSetupManipulatorData.HasJointLimit())
        {
            return;
        }

        AZStd::optional<float> swingLimitY = m_physicsSetupManipulatorData.m_jointConfiguration->GetPropertyValue(AZ::Name("SwingLimitY"));
        AZStd::optional<float> swingLimitZ = m_physicsSetupManipulatorData.m_jointConfiguration->GetPropertyValue(AZ::Name("SwingLimitZ"));

        if (!swingLimitY.has_value() && !swingLimitZ.has_value())
        {
            return;
        }

        AZ::u32 previousState = m_debugDisplay->GetState();
        m_debugDisplay->CullOff();
        m_debugDisplay->SetColor(AZ::Colors::White);
        m_debugDisplay->PushMatrix(m_physicsSetupManipulatorData.GetJointParentFrameWorld());
        if (swingLimitY.has_value())
        {
            m_debugDisplay->DrawLine(
                AZ::Vector3(ManipulatorOffsetX, 0.0f, 0.0f),
                AZ::Vector3(ManipulatorOffsetX, 0.0f, ManipulatorInverseScale * swingLimitY.value()));
        }
        if (swingLimitZ.has_value())
        {
            m_debugDisplay->DrawLine(
                AZ::Vector3(ManipulatorOffsetX, 0.0f, 0.0f),
                AZ::Vector3(ManipulatorOffsetX, ManipulatorInverseScale * swingLimitZ.value(), 0.0f));
        }
        m_debugDisplay->PopMatrix();
        m_debugDisplay->SetState(previousState);
    }
} // namespace EMotionFX
