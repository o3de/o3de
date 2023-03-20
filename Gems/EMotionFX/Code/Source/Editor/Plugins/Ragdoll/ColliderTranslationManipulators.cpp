/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/CommandSystem/Source/ColliderCommands.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <Editor/ColliderContainerWidget.h>
#include <Editor/Plugins/Ragdoll/ColliderTranslationManipulators.h>

namespace EMotionFX
{
    ColliderTranslationManipulators::ColliderTranslationManipulators()
        : m_translationManipulators(
              AzToolsFramework::TranslationManipulators::Dimensions::Three, AZ::Transform::CreateIdentity(), AZ::Vector3::CreateOne())
    {
    }

    void ColliderTranslationManipulators::Setup(const PhysicsSetupManipulatorData& physicsSetupManipulatorData)
    {
        m_physicsSetupManipulatorData = physicsSetupManipulatorData;

        if (!m_physicsSetupManipulatorData.HasColliders())
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
                EndEditing(action.m_start.m_localPosition, action.m_current.m_localPositionOffset);
            });

        m_translationManipulators.InstallPlanarManipulatorMouseUpCallback(
            [this](const AzToolsFramework::PlanarManipulator::Action& action)
            {
                EndEditing(action.m_start.m_localPosition, action.m_current.m_localOffset);
            });

        m_translationManipulators.InstallSurfaceManipulatorMouseUpCallback(
            [this](const AzToolsFramework::SurfaceManipulator::Action& action)
            {
                EndEditing(action.m_start.m_localPosition, action.m_current.m_localOffset);
            });

        PhysicsSetupManipulatorRequestBus::Handler::BusConnect();
        m_adjustColliderCallback = AZStd::make_unique<PhysicsSetupManipulatorCommandCallback>(this, false);
        EMStudio::GetCommandManager()->RegisterCommandCallback("AdjustCollider", m_adjustColliderCallback.get());
    }

    void ColliderTranslationManipulators::Refresh()
    {
        if (m_physicsSetupManipulatorData.HasColliders())
        {
            m_translationManipulators.SetLocalPosition(
                m_physicsSetupManipulatorData.m_colliderNodeConfiguration->m_shapes[0].first->m_position);
        }
    }

    void ColliderTranslationManipulators::Teardown()
    {
        if (!m_physicsSetupManipulatorData.HasColliders())
        {
            return;
        }

        EMStudio::GetCommandManager()->RemoveCommandCallback(m_adjustColliderCallback.get(), false);
        m_adjustColliderCallback.reset();
        PhysicsSetupManipulatorRequestBus::Handler::BusDisconnect();
        m_translationManipulators.Unregister();
    }

    void ColliderTranslationManipulators::ResetValues()
    {
        if (m_physicsSetupManipulatorData.HasColliders())
        {
            BeginEditing(
                m_physicsSetupManipulatorData.m_colliderNodeConfiguration->m_shapes[0].first->m_position, AZ::Vector3::CreateZero());
            EndEditing(AZ::Vector3::CreateZero(), AZ::Vector3::CreateZero());
            Refresh();
        }
    }

    AZ::Vector3 ColliderTranslationManipulators::GetPosition(const AZ::Vector3& startPosition, const AZ::Vector3& offset) const
    {
        const float scale = AZ::GetMax(AZ::MinTransformScale, m_physicsSetupManipulatorData.m_nodeWorldTransform.GetUniformScale());
        return startPosition + offset / scale;
    }

    void ColliderTranslationManipulators::OnManipulatorMoved(const AZ::Vector3& startPosition, const AZ::Vector3& offset)
    {
        AZ::Vector3 newPosition = GetPosition(startPosition, offset);
        if (m_physicsSetupManipulatorData.HasColliders())
        {
            m_physicsSetupManipulatorData.m_colliderNodeConfiguration->m_shapes[0].first->m_position = newPosition;
        }
        m_translationManipulators.SetLocalPosition(newPosition);
        m_physicsSetupManipulatorData.m_collidersWidget->Update();
    }

    void ColliderTranslationManipulators::BeginEditing(const AZ::Vector3& startPosition, const AZ::Vector3& offset)
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

    void ColliderTranslationManipulators::EndEditing(const AZ::Vector3& startPosition, const AZ::Vector3& offset)
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

    void ColliderTranslationManipulators::OnUnderlyingPropertiesChanged()
    {
        Refresh();
    }
} // namespace EMotionFX
