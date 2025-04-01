/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Viewport/ViewportColors.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <EMotionFX/CommandSystem/Source/ColliderCommands.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/RenderPlugin/ViewportPluginBus.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <Editor/ColliderContainerWidget.h>
#include <Editor/Plugins/Ragdoll/ColliderRotationManipulators.h>

namespace EMotionFX
{
    ColliderRotationManipulators::ColliderRotationManipulators()
        : m_rotationManipulators(AZ::Transform::Identity())
    {
        m_rotationManipulators.SetCircleBoundWidth(AzToolsFramework::ManipulatorCicleBoundWidth());
    }

    void ColliderRotationManipulators::Setup(const PhysicsSetupManipulatorData& physicsSetupManipulatorData)
    {
        m_physicsSetupManipulatorData = physicsSetupManipulatorData;

        if (!m_physicsSetupManipulatorData.HasColliders())
        {
            return;
        }

        const Physics::ColliderConfiguration* colliderConfiguration =
            m_physicsSetupManipulatorData.m_colliderNodeConfiguration->m_shapes[0].first.get();

        m_rotationManipulators.SetSpace(physicsSetupManipulatorData.m_nodeWorldTransform);
        m_rotationManipulators.SetLocalPosition(colliderConfiguration->m_position);
        m_rotationManipulators.SetLocalOrientation(colliderConfiguration->m_rotation);
        m_rotationManipulators.Register(EMStudio::g_animManipulatorManagerId);
        m_rotationManipulators.SetLocalAxes(AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY(), AZ::Vector3::CreateAxisZ());
        m_rotationManipulators.ConfigureView(
            2.0f, AzFramework::ViewportColors::XAxisColor, AzFramework::ViewportColors::YAxisColor,
            AzFramework::ViewportColors::ZAxisColor);

        m_rotationManipulators.InstallLeftMouseDownCallback(
            [this](const AzToolsFramework::AngularManipulator::Action& action)
            {
                BeginEditing(action.m_start.m_rotation);
            });

        m_rotationManipulators.InstallMouseMoveCallback(
            [this](const AzToolsFramework::AngularManipulator::Action& action)
            {
                OnManipulatorMoved(action.LocalOrientation());
            });

        m_rotationManipulators.InstallLeftMouseUpCallback(
            [this](const AzToolsFramework::AngularManipulator::Action& action)
            {
                EndEditing(action.LocalOrientation());
            });

        AZ::TickBus::Handler::BusConnect();
        PhysicsSetupManipulatorRequestBus::Handler::BusConnect();
        m_adjustColliderCallback = AZStd::make_unique<PhysicsSetupManipulatorCommandCallback>(this, false);
        EMStudio::GetCommandManager()->RegisterCommandCallback("AdjustCollider", m_adjustColliderCallback.get());
    }

    void ColliderRotationManipulators::Refresh()
    {
        if (m_physicsSetupManipulatorData.HasColliders())
        {
            m_rotationManipulators.SetLocalPosition(
                m_physicsSetupManipulatorData.m_colliderNodeConfiguration->m_shapes[0].first->m_position);
            m_rotationManipulators.SetLocalOrientation(
                m_physicsSetupManipulatorData.m_colliderNodeConfiguration->m_shapes[0].first->m_rotation);
        }
    }

    void ColliderRotationManipulators::Teardown()
    {
        if (!m_physicsSetupManipulatorData.HasColliders())
        {
            return;
        }

        EMStudio::GetCommandManager()->RemoveCommandCallback(m_adjustColliderCallback.get(), false);
        m_adjustColliderCallback.reset();
        PhysicsSetupManipulatorRequestBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
        m_rotationManipulators.Unregister();
    }

    void ColliderRotationManipulators::ResetValues()
    {
        if (m_physicsSetupManipulatorData.HasColliders())
        {
            BeginEditing(m_physicsSetupManipulatorData.m_colliderNodeConfiguration->m_shapes[0].first->m_rotation);
            EndEditing(AZ::Quaternion::CreateIdentity());
            Refresh();
        }
    }

    void ColliderRotationManipulators::OnManipulatorMoved(const AZ::Quaternion& rotation)
    {
        m_rotationManipulators.SetLocalOrientation(rotation);
        if (m_physicsSetupManipulatorData.HasColliders())
        {
            m_physicsSetupManipulatorData.m_colliderNodeConfiguration->m_shapes[0].first->m_rotation = rotation;
            m_physicsSetupManipulatorData.m_collidersWidget->Update();
        }
    }

    void ColliderRotationManipulators::BeginEditing(const AZ::Quaternion& rotation)
    {
        if (!m_commandGroup.IsEmpty())
        {
            return;
        }
        m_commandGroup.SetGroupName("Adjust collider");

        const AZ::u32 actorId = m_physicsSetupManipulatorData.m_actor->GetID();
        const AZStd::string& nodeName = m_physicsSetupManipulatorData.m_node->GetNameString();
        auto colliderType = PhysicsSetup::ColliderConfigType::Ragdoll;
        const size_t colliderIndex = 0;
        CommandAdjustCollider* command = aznew CommandAdjustCollider(actorId, nodeName, colliderType, colliderIndex);
        m_commandGroup.AddCommand(command);
        command->SetOldRotation(rotation);
    }

    void ColliderRotationManipulators::EndEditing(const AZ::Quaternion& rotation)
    {
        if (m_commandGroup.IsEmpty())
        {
            return;
        }

        if (CommandAdjustCollider* command = azdynamic_cast<CommandAdjustCollider*>(m_commandGroup.GetCommand(0)))
        {
            command->SetRotation(rotation);
        }
        AZStd::string result;
        CommandSystem::GetCommandManager()->ExecuteCommandGroup(m_commandGroup, result);
        m_commandGroup.Clear();
    }

    void ColliderRotationManipulators::OnTick([[maybe_unused]] float delta, [[maybe_unused]] AZ::ScriptTimePoint timePoint)
    {
        const AzFramework::CameraState cameraState = AzToolsFramework::GetCameraState(m_viewportId);
        m_rotationManipulators.RefreshView(cameraState.m_position);
    }

    void ColliderRotationManipulators::OnUnderlyingPropertiesChanged()
    {
        Refresh();
    }
} // namespace EMotionFX
