/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzFramework/Physics/Character.h>
#include <EMotionFX/CommandSystem/Source/ColliderCommands.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <Editor/ColliderContainerWidget.h>
#include <Editor/ObjectEditor.h>
#include <Editor/Plugins/Ragdoll/PhysicsSetupColliderTranslationManipulators.h>

namespace EMotionFX
{
    PhysicsSetupColliderTranslationManipulators::PhysicsSetupColliderTranslationManipulators()
        : m_translationManipulators(
              AzToolsFramework::TranslationManipulators::Dimensions::Three, AZ::Transform::CreateIdentity(), AZ::Vector3::CreateOne())
    {
        m_adjustColliderCallback = AZStd::make_unique<DataChangedCallback>(this, false);
        EMStudio::GetCommandManager()->RegisterCommandCallback("AdjustCollider", m_adjustColliderCallback.get());
    }

    PhysicsSetupColliderTranslationManipulators::~PhysicsSetupColliderTranslationManipulators()
    {
        EMStudio::GetCommandManager()->RemoveCommandCallback(m_adjustColliderCallback.get(), false);
    }

    void PhysicsSetupColliderTranslationManipulators::Setup(PhysicsSetupManipulatorData& physicsSetupManipulatorData)
    {
        m_physicsSetupManipulatorData = physicsSetupManipulatorData;

        if (!m_physicsSetupManipulatorData.m_valid || !m_physicsSetupManipulatorData.m_colliderNodeConfiguration ||
            m_physicsSetupManipulatorData.m_colliderNodeConfiguration->m_shapes.empty())
        {
            return;
        }

        m_translationManipulators.SetSpace(m_physicsSetupManipulatorData.m_nodeWorldTransform);
        m_translationManipulators.SetLocalPosition(
            m_physicsSetupManipulatorData.m_colliderNodeConfiguration->m_shapes[0].first->m_position);
        m_translationManipulators.Register(EMStudio::g_animManipulatorManagerId);
        AzToolsFramework::ConfigureTranslationManipulatorAppearance3d(&m_translationManipulators);

        // mouse down callbacks
        m_translationManipulators.InstallLinearManipulatorMouseDownCallback(
            [this](const AzToolsFramework::LinearManipulator::Action& action)
            {
                BeginEditing(action.m_start.m_localPosition, action.m_current.m_localPositionOffset);
            });

        m_translationManipulators.InstallPlanarManipulatorMouseDownCallback(
            [this](const AzToolsFramework::PlanarManipulator::Action& action)
            {
                BeginEditing(action.m_start.m_localPosition, action.m_current.m_localOffset);
            });

        m_translationManipulators.InstallSurfaceManipulatorMouseDownCallback(
            [this](const AzToolsFramework::SurfaceManipulator::Action& action)
            {
                BeginEditing(action.m_start.m_localPosition, action.m_current.m_localOffset);
            });

        // mouse move callbacks
        m_translationManipulators.InstallLinearManipulatorMouseMoveCallback(
            [this](const AzToolsFramework::LinearManipulator::Action& action)
            {
                OnManipulatorMoved(action.m_start.m_localPosition, action.m_current.m_localPositionOffset);
            });

        m_translationManipulators.InstallPlanarManipulatorMouseMoveCallback(
            [this](const AzToolsFramework::PlanarManipulator::Action& action)
            {
                OnManipulatorMoved(action.m_start.m_localPosition, action.m_current.m_localOffset);
            });

        m_translationManipulators.InstallSurfaceManipulatorMouseMoveCallback(
            [this](const AzToolsFramework::SurfaceManipulator::Action& action)
            {
                OnManipulatorMoved(action.m_start.m_localPosition, action.m_current.m_localOffset);
            });

        // mouse up callbacks
        m_translationManipulators.InstallLinearManipulatorMouseUpCallback(
            [this](const AzToolsFramework::LinearManipulator::Action& action)
            {
                FinishEditing(action.m_start.m_localPosition, action.m_current.m_localPositionOffset);
            });

        m_translationManipulators.InstallPlanarManipulatorMouseUpCallback(
            [this](const AzToolsFramework::PlanarManipulator::Action& action)
            {
                FinishEditing(action.m_start.m_localPosition, action.m_current.m_localOffset);
            });

        m_translationManipulators.InstallSurfaceManipulatorMouseUpCallback(
            [this](const AzToolsFramework::SurfaceManipulator::Action& action)
            {
                FinishEditing(action.m_start.m_localPosition, action.m_current.m_localOffset);
            });
    }

    void PhysicsSetupColliderTranslationManipulators::Refresh()
    {
        if (!m_physicsSetupManipulatorData.m_valid || !m_physicsSetupManipulatorData.m_colliderNodeConfiguration ||
            m_physicsSetupManipulatorData.m_colliderNodeConfiguration->m_shapes.empty())
        {
            return;
        }

        m_translationManipulators.SetLocalPosition(
            m_physicsSetupManipulatorData.m_colliderNodeConfiguration->m_shapes[0].first->m_position);
    }

    void PhysicsSetupColliderTranslationManipulators::Teardown()
    {
        m_translationManipulators.Unregister();
    }

    void PhysicsSetupColliderTranslationManipulators::ResetValues()
    {
        if (!m_physicsSetupManipulatorData.m_valid || !m_physicsSetupManipulatorData.m_colliderNodeConfiguration ||
            m_physicsSetupManipulatorData.m_colliderNodeConfiguration->m_shapes.empty())
        {
            return;
        }
        m_physicsSetupManipulatorData.m_colliderNodeConfiguration->m_shapes[0].first->m_position = AZ::Vector3::CreateZero();
        m_translationManipulators.SetLocalPosition(AZ::Vector3::CreateZero());
    }

    AZ::Vector3 PhysicsSetupColliderTranslationManipulators::GetPosition(const AZ::Vector3& startPosition, const AZ::Vector3& offset) const
    {
        const float scale = AZ::GetMax(AZ::MinTransformScale, m_physicsSetupManipulatorData.m_nodeWorldTransform.GetUniformScale());
        return startPosition + offset / scale;
    }

    void PhysicsSetupColliderTranslationManipulators::OnManipulatorMoved(const AZ::Vector3& startPosition, const AZ::Vector3& offset)
    {
        AZ::Vector3 newPosition = GetPosition(startPosition, offset);
        if (m_physicsSetupManipulatorData.m_valid && m_physicsSetupManipulatorData.m_colliderNodeConfiguration &&
            !m_physicsSetupManipulatorData.m_colliderNodeConfiguration->m_shapes.empty())
        {
            m_physicsSetupManipulatorData.m_colliderNodeConfiguration->m_shapes[0].first->m_position = newPosition;
        }
        m_translationManipulators.SetLocalPosition(newPosition);
        m_physicsSetupManipulatorData.m_collidersWidget->Update();
    }

    void PhysicsSetupColliderTranslationManipulators::BeginEditing(const AZ::Vector3& startPosition, const AZ::Vector3& offset)
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
        command->SetOldPosition(GetPosition(startPosition, offset));
    }

    void PhysicsSetupColliderTranslationManipulators::FinishEditing(const AZ::Vector3& startPosition, const AZ::Vector3& offset)
    {
        if (m_commandGroup.IsEmpty())
        {
            return;
        }

        if (CommandAdjustCollider* command = azdynamic_cast<CommandAdjustCollider*>(m_commandGroup.GetCommand(0)))
        {
            command->SetPosition(GetPosition(startPosition, offset));
        }
        AZStd::string result;
        CommandSystem::GetCommandManager()->ExecuteCommandGroup(m_commandGroup, result);
        m_commandGroup.Clear();
    }

    bool PhysicsSetupColliderTranslationManipulators::DataChangedCallback::Execute(
        [[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        m_manipulators->Refresh();
        return true;
    }

    bool PhysicsSetupColliderTranslationManipulators::DataChangedCallback::Undo(
        [[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        m_manipulators->Refresh();
        return true;
    }
} // namespace EMotionFX
