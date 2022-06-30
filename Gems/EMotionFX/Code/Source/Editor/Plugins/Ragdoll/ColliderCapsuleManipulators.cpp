/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <EMotionFX/CommandSystem/Source/ColliderCommands.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/RenderPlugin/ViewportPluginBus.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <Editor/ColliderContainerWidget.h>
#include <Editor/Plugins/Ragdoll/ColliderCapsuleManipulators.h>

namespace EMotionFX
{
    const Physics::CapsuleShapeConfiguration* GetCapsuleShapeConfiguration(const PhysicsSetupManipulatorData& physicsSetupManipulatorData)
    {
        if (!physicsSetupManipulatorData.HasCapsuleCollider())
        {
            return nullptr;
        }

        return azdynamic_cast<Physics::CapsuleShapeConfiguration*>(
            physicsSetupManipulatorData.m_colliderNodeConfiguration->m_shapes[0].second.get());
    }

    Physics::CapsuleShapeConfiguration* GetCapsuleShapeConfiguration(PhysicsSetupManipulatorData& physicsSetupManipulatorData)
    {
        return const_cast<Physics::CapsuleShapeConfiguration*>(
            GetCapsuleShapeConfiguration(const_cast<const PhysicsSetupManipulatorData&>(physicsSetupManipulatorData)));
    }

    void ColliderCapsuleManipulators::Setup(const PhysicsSetupManipulatorData& physicsSetupManipulatorData)
    {
        m_physicsSetupManipulatorData = physicsSetupManipulatorData;

        if (!m_physicsSetupManipulatorData.HasCapsuleCollider())
        {
            return;
        }

        SetupCapsuleManipulators(EMStudio::g_animManipulatorManagerId);

        AZ::TickBus::Handler::BusConnect();
        PhysicsSetupManipulatorRequestBus::Handler::BusConnect();
        m_adjustColliderCallback = AZStd::make_unique<PhysicsSetupManipulatorCommandCallback>(this, false);
        EMStudio::GetCommandManager()->RegisterCommandCallback("AdjustCollider", m_adjustColliderCallback.get());
    }

    void ColliderCapsuleManipulators::Refresh()
    {
        if (m_physicsSetupManipulatorData.HasCapsuleCollider())
        {
            UpdateCapsuleManipulators();
        }
    }

    void ColliderCapsuleManipulators::Teardown()
    {
        if (!m_physicsSetupManipulatorData.HasCapsuleCollider())
        {
            return;
        }

        EMStudio::GetCommandManager()->RemoveCommandCallback(m_adjustColliderCallback.get(), false);
        m_adjustColliderCallback.reset();
        PhysicsSetupManipulatorRequestBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
        if (m_radiusManipulator)
        {
            m_radiusManipulator->Unregister();
        }
        if (m_heightManipulator)
        {
            m_heightManipulator->Unregister();
        }
        TeardownCapsuleManipulators();
    }

    void ColliderCapsuleManipulators::ResetValues()
    {
        if (m_physicsSetupManipulatorData.HasCapsuleCollider())
        {
            BeginEditing();
            ResetCapsuleManipulators();
            FinishEditing();
            Refresh();
        }
    }

    void ColliderCapsuleManipulators::BeginEditing()
    {
        if (!m_commandGroup.IsEmpty())
        {
            return;
        }
        m_commandGroup.SetGroupName("Adjust collider");

        const AZ::u32 actorId = m_physicsSetupManipulatorData.m_actor->GetID();
        const AZStd::string& nodeName = m_physicsSetupManipulatorData.m_node->GetNameString();
        const auto colliderType = PhysicsSetup::ColliderConfigType::Ragdoll;
        const size_t colliderIndex = 0;
        CommandAdjustCollider* command = aznew CommandAdjustCollider(actorId, nodeName, colliderType, colliderIndex);
        m_commandGroup.AddCommand(command);

        if (const auto* capsuleShapeConfiguration = GetCapsuleShapeConfiguration(m_physicsSetupManipulatorData))
        {
            command->SetOldHeight(capsuleShapeConfiguration->m_height);
            command->SetOldRadius(capsuleShapeConfiguration->m_radius);
        }
    }

    void ColliderCapsuleManipulators::FinishEditing()
    {
        if (m_commandGroup.IsEmpty())
        {
            return;
        }

        if (CommandAdjustCollider* command = azdynamic_cast<CommandAdjustCollider*>(m_commandGroup.GetCommand(0)))
        {
            if (const auto* capsuleShapeConfiguration = GetCapsuleShapeConfiguration(m_physicsSetupManipulatorData))
            {
                command->SetHeight(capsuleShapeConfiguration->m_height);
                command->SetRadius(capsuleShapeConfiguration->m_radius);
            }
        }
        AZStd::string result;
        CommandSystem::GetCommandManager()->ExecuteCommandGroup(m_commandGroup, result);
        m_commandGroup.Clear();
    }

    void ColliderCapsuleManipulators::OnTick([[maybe_unused]] float delta, [[maybe_unused]] AZ::ScriptTimePoint timePoint)
    {
        const AzFramework::CameraState cameraState = AzToolsFramework::GetCameraState(m_viewportId);
        OnCameraStateChanged(cameraState);
    }

    void ColliderCapsuleManipulators::OnUnderlyingPropertiesChanged()
    {
        Refresh();
    }

    AZ::Transform ColliderCapsuleManipulators::GetCapsuleWorldTransform() const
    {
        return m_physicsSetupManipulatorData.m_nodeWorldTransform;
    }

    AZ::Transform ColliderCapsuleManipulators::GetCapsuleLocalTransform() const
    {
        if (!m_physicsSetupManipulatorData.HasCapsuleCollider())
        {
            return AZ::Transform::CreateIdentity();
        }

        const Physics::ColliderConfiguration* colliderConfiguration =
            m_physicsSetupManipulatorData.m_colliderNodeConfiguration->m_shapes[0].first.get();

        return AZ::Transform::CreateFromQuaternionAndTranslation(colliderConfiguration->m_rotation, colliderConfiguration->m_position);
    }

    AZ::Vector3 ColliderCapsuleManipulators::GetCapsuleNonUniformScale() const
    {
        return AZ::Vector3::CreateOne();
    }

    float ColliderCapsuleManipulators::GetCapsuleRadius() const
    {
        if (auto* capsuleShapeConfiguration = GetCapsuleShapeConfiguration(m_physicsSetupManipulatorData))
        {
            return capsuleShapeConfiguration->m_radius;
        }

        return Physics::ShapeConstants::DefaultCapsuleRadius;
    }

    float ColliderCapsuleManipulators::GetCapsuleHeight() const
    {
        if (const auto* capsuleShapeConfiguration = GetCapsuleShapeConfiguration(m_physicsSetupManipulatorData))
        {
            return capsuleShapeConfiguration->m_height;
        }

        return Physics::ShapeConstants::DefaultCapsuleHeight;
    }

    void ColliderCapsuleManipulators::SetCapsuleRadius(float radius)
    {
        if (auto* capsuleShapeConfiguration = GetCapsuleShapeConfiguration(m_physicsSetupManipulatorData))
        {
            capsuleShapeConfiguration->m_radius = radius;
            m_physicsSetupManipulatorData.m_collidersWidget->Update();
        }
    }

    void ColliderCapsuleManipulators::SetCapsuleHeight(float height)
    {
        if (auto* capsuleShapeConfiguration = GetCapsuleShapeConfiguration(m_physicsSetupManipulatorData))
        {
            capsuleShapeConfiguration->m_height = height;
            m_physicsSetupManipulatorData.m_collidersWidget->Update();
        }
    }
} // namespace EMotionFX
