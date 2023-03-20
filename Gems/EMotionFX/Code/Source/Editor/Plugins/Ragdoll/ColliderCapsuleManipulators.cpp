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

    void ColliderCapsuleManipulators::InstallCapsuleViewportEditFunctions()
    {
        if (!m_capsuleViewportEdit)
        {
            return;
        }

        m_capsuleViewportEdit->InstallGetManipulatorSpace(
            [this]()
            {
                return m_physicsSetupManipulatorData.m_nodeWorldTransform;
            });
        m_capsuleViewportEdit->InstallGetNonUniformScale(
            []()
            {
                return AZ::Vector3::CreateOne();
            });
        m_capsuleViewportEdit->InstallGetTranslationOffset(
            [this]()
            {
                if (!m_physicsSetupManipulatorData.HasCapsuleCollider())
                {
                    return AZ::Vector3::CreateZero();
                }

                const Physics::ColliderConfiguration* colliderConfiguration =
                    m_physicsSetupManipulatorData.m_colliderNodeConfiguration->m_shapes[0].first.get();

                return colliderConfiguration->m_position;
            });
        m_capsuleViewportEdit->InstallGetRotationOffset(
            [this]()
            {
                if (!m_physicsSetupManipulatorData.HasCapsuleCollider())
                {
                    return AZ::Quaternion::CreateIdentity();
                }

                const Physics::ColliderConfiguration* colliderConfiguration =
                    m_physicsSetupManipulatorData.m_colliderNodeConfiguration->m_shapes[0].first.get();

                return colliderConfiguration->m_rotation;
            });
        m_capsuleViewportEdit->InstallGetCapsuleRadius(
            [this]()
            {
                if (auto* capsuleShapeConfiguration = GetCapsuleShapeConfiguration(m_physicsSetupManipulatorData))
                {
                    return capsuleShapeConfiguration->m_radius;
                }

                return Physics::ShapeConstants::DefaultCapsuleRadius;
            });
        m_capsuleViewportEdit->InstallGetCapsuleHeight(
            [this]()
            {
                if (const auto* capsuleShapeConfiguration = GetCapsuleShapeConfiguration(m_physicsSetupManipulatorData))
                {
                    return capsuleShapeConfiguration->m_height;
                }

                return Physics::ShapeConstants::DefaultCapsuleHeight;
            });
        m_capsuleViewportEdit->InstallSetCapsuleRadius(
            [this](float radius)
            {
                if (auto* capsuleShapeConfiguration = GetCapsuleShapeConfiguration(m_physicsSetupManipulatorData))
                {
                    capsuleShapeConfiguration->m_radius = radius;
                    m_physicsSetupManipulatorData.m_collidersWidget->Update();
                }
            });
        m_capsuleViewportEdit->InstallSetCapsuleHeight(
            [this](float height)
            {
                if (auto* capsuleShapeConfiguration = GetCapsuleShapeConfiguration(m_physicsSetupManipulatorData))
                {
                    capsuleShapeConfiguration->m_height = height;
                    m_physicsSetupManipulatorData.m_collidersWidget->Update();
                }
            });
        m_capsuleViewportEdit->InstallSetTranslationOffset(
            [this](const AZ::Vector3& translationOffset)
            {
                if (m_physicsSetupManipulatorData.HasCapsuleCollider())
                {
                    Physics::ColliderConfiguration* colliderConfiguration =
                        m_physicsSetupManipulatorData.m_colliderNodeConfiguration->m_shapes[0].first.get();

                    colliderConfiguration->m_position = translationOffset;
                }
            });
        m_capsuleViewportEdit->InstallBeginEditing(
            [this]()
            {
                BeginEditing();
            });
        m_capsuleViewportEdit->InstallEndEditing(
            [this]()
            {
                EndEditing();
            });
    }

    void ColliderCapsuleManipulators::Setup(const PhysicsSetupManipulatorData& physicsSetupManipulatorData)
    {
        m_physicsSetupManipulatorData = physicsSetupManipulatorData;

        if (!m_physicsSetupManipulatorData.HasCapsuleCollider())
        {
            return;
        }

        const bool allowAsymmetricalEditing = true;
        m_capsuleViewportEdit = AZStd::make_unique<AzToolsFramework::CapsuleViewportEdit>(allowAsymmetricalEditing);
        InstallCapsuleViewportEditFunctions();
        m_capsuleViewportEdit->Setup(EMStudio::g_animManipulatorManagerId);

        AZ::TickBus::Handler::BusConnect();
        PhysicsSetupManipulatorRequestBus::Handler::BusConnect();
        m_adjustColliderCallback = AZStd::make_unique<PhysicsSetupManipulatorCommandCallback>(this, false);
        EMStudio::GetCommandManager()->RegisterCommandCallback("AdjustCollider", m_adjustColliderCallback.get());
    }

    void ColliderCapsuleManipulators::Refresh()
    {
        if (m_capsuleViewportEdit)
        {
            m_capsuleViewportEdit->UpdateManipulators();
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
        if (m_capsuleViewportEdit)
        {
            m_capsuleViewportEdit->Teardown();
        }
    }

    void ColliderCapsuleManipulators::ResetValues()
    {
        if (m_capsuleViewportEdit)
        {
            m_capsuleViewportEdit->ResetValues();
            m_capsuleViewportEdit->UpdateManipulators();
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

    void ColliderCapsuleManipulators::EndEditing()
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
        if (m_capsuleViewportEdit)
        {
            const AzFramework::CameraState cameraState = AzToolsFramework::GetCameraState(m_viewportId);
            m_capsuleViewportEdit->OnCameraStateChanged(cameraState);
        }
    }

    void ColliderCapsuleManipulators::OnUnderlyingPropertiesChanged()
    {
        Refresh();
    }
} // namespace EMotionFX
