/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Viewport/ViewportColors.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <EMotionFX/CommandSystem/Source/JointLimitCommands.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <Editor/Plugins/Ragdoll/JointLimitRotationManipulators.h>
#include <Editor/Plugins/Ragdoll/RagdollJointLimitWidget.h>

namespace EMotionFX
{
    JointLimitRotationManipulators::JointLimitRotationManipulators(JointLimitFrame jointLimitFrame)
        : m_jointLimitFrame(jointLimitFrame)
    {
        m_adjustJointLimitCallback = AZStd::make_unique<PhysicsSetupManipulatorCommandCallback>(this, false);
        EMStudio::GetCommandManager()->RegisterCommandCallback("AdjustJointLimit", m_adjustJointLimitCallback.get());
        m_rotationManipulators.SetCircleBoundWidth(AzToolsFramework::ManipulatorCicleBoundWidth());
    }

    JointLimitRotationManipulators::~JointLimitRotationManipulators()
    {
        EMStudio::GetCommandManager()->RemoveCommandCallback(m_adjustJointLimitCallback.get(), false);
    }

    void JointLimitRotationManipulators::Setup(const PhysicsSetupManipulatorData& physicsSetupManipulatorData)
    {
        m_physicsSetupManipulatorData = physicsSetupManipulatorData;

        if (!m_physicsSetupManipulatorData.HasJointLimit())
        {
            return;
        }

        m_rotationManipulators.SetSpace(physicsSetupManipulatorData.m_nodeWorldTransform);
        Refresh();
        m_rotationManipulators.Register(EMStudio::g_animManipulatorManagerId);
        m_rotationManipulators.SetLocalAxes(AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY(), AZ::Vector3::CreateAxisZ());
        m_rotationManipulators.ConfigureView(
            2.0f, AzFramework::ViewportColors::XAxisColor, AzFramework::ViewportColors::YAxisColor,
            AzFramework::ViewportColors::ZAxisColor);

        m_rotationManipulators.InstallLeftMouseDownCallback(
            [this]([[maybe_unused]] const AzToolsFramework::AngularManipulator::Action& action)
            {
                BeginEditing();
            });

        m_rotationManipulators.InstallMouseMoveCallback(
            [this](const AzToolsFramework::AngularManipulator::Action& action)
            {
                OnManipulatorMoved(action.LocalOrientation());
            });

        m_rotationManipulators.InstallLeftMouseUpCallback(
            [this]([[maybe_unused]] const AzToolsFramework::AngularManipulator::Action& action)
            {
                FinishEditing();
            });

        AZ::TickBus::Handler::BusConnect();
        PhysicsSetupManipulatorRequestBus::Handler::BusConnect();
    }

    void JointLimitRotationManipulators::Refresh()
    {
        if (m_physicsSetupManipulatorData.HasJointLimit())
        {
            m_rotationManipulators.SetLocalPosition(AZ::Vector3::CreateZero());
            m_rotationManipulators.SetLocalOrientation(GetLocalOrientation());
        }
    }

    void JointLimitRotationManipulators::Teardown()
    {
        PhysicsSetupManipulatorRequestBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
        m_rotationManipulators.Unregister();
    }

    void JointLimitRotationManipulators::ResetValues()
    {
        if (m_physicsSetupManipulatorData.HasJointLimit())
        {
            BeginEditing();
            GetLocalOrientation() = AZ::Quaternion::CreateIdentity();
            FinishEditing();
            Refresh();
        }
    }

    void JointLimitRotationManipulators::OnManipulatorMoved(const AZ::Quaternion& rotation)
    {
        m_rotationManipulators.SetLocalOrientation(rotation);
        if (m_physicsSetupManipulatorData.HasJointLimit())
        {
            GetLocalOrientation() = rotation;
        }
        InvalidateEditorValues();
    }

    void JointLimitRotationManipulators::BeginEditing()
    {
        CreateCommandAdjustJointLimit(m_commandGroup, m_physicsSetupManipulatorData);
    }

    void JointLimitRotationManipulators::FinishEditing()
    {
        ExecuteCommandAdjustJointLimit(m_commandGroup, m_physicsSetupManipulatorData);
    }

    void JointLimitRotationManipulators::OnTick([[maybe_unused]] float delta, [[maybe_unused]] AZ::ScriptTimePoint timePoint)
    {
        const AzFramework::CameraState cameraState = AzToolsFramework::GetCameraState(m_viewportId);
        m_rotationManipulators.RefreshView(cameraState.m_position);
    }

    void JointLimitRotationManipulators::OnUnderlyingPropertiesChanged()
    {
        Refresh();
    }

    AZ::Quaternion& JointLimitRotationManipulators::GetLocalOrientation()
    {
        return const_cast<AZ::Quaternion&>(static_cast<const JointLimitRotationManipulators&>(*this).GetLocalOrientation());
    }

    const AZ::Quaternion& JointLimitRotationManipulators::GetLocalOrientation() const
    {
        return m_jointLimitFrame == JointLimitFrame::Parent
            ? m_physicsSetupManipulatorData.m_jointConfiguration->m_parentLocalRotation
            : m_physicsSetupManipulatorData.m_jointConfiguration->m_childLocalRotation;
    }

    void JointLimitRotationManipulators::InvalidateEditorValues()
    {
        if (m_physicsSetupManipulatorData.m_jointLimitWidget)
        {
            m_physicsSetupManipulatorData.m_jointLimitWidget->InvalidateValues();
        }
    }

    void CreateCommandAdjustJointLimit(MCore::CommandGroup& commandGroup, const PhysicsSetupManipulatorData& physicsSetupManipulatorData)
    {
        if (!commandGroup.IsEmpty())
        {
            return;
        }
        commandGroup.SetGroupName("Adjust joint limit");
        const AZ::u32 actorId = physicsSetupManipulatorData.m_actor->GetID();
        const AZStd::string& nodeName = physicsSetupManipulatorData.m_node->GetNameString();
        CommandAdjustJointLimit* command = aznew CommandAdjustJointLimit(actorId, nodeName);
        commandGroup.AddCommand(command);
        command->SetOldJointConfiguration(physicsSetupManipulatorData.m_jointConfiguration);
    }

    void ExecuteCommandAdjustJointLimit(MCore::CommandGroup& commandGroup, const PhysicsSetupManipulatorData& physicsSetupManipulatorData)
    {
        if (commandGroup.IsEmpty())
        {
            return;
        }

        if (auto* command = azdynamic_cast<CommandAdjustJointLimit*>(commandGroup.GetCommand(0)))
        {
            command->SetJointConfiguration(physicsSetupManipulatorData.m_jointConfiguration);
        }
        AZStd::string result;
        CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result);
        commandGroup.Clear();
    }
} // namespace EMotionFX
