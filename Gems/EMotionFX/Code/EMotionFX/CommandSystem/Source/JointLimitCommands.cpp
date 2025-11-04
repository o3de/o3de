/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/make_shared.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/JointLimitCommands.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/PhysicsSetup.h>
#include <MCore/Source/ReflectionSerializer.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(ParameterMixinActorIdJointName, EMotionFX::CommandAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(CommandAdjustJointLimit, EMotionFX::CommandAllocator);

    ParameterMixinActorIdJointName::ParameterMixinActorIdJointName(AZ::u32 actorId, const AZStd::string& jointName)
        : ParameterMixinActorId(actorId)
        , ParameterMixinJointName(jointName)
    {
    }

    void ParameterMixinActorIdJointName::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<ParameterMixinActorIdJointName, ParameterMixinActorId, ParameterMixinJointName>()->Version(1);
    }

    AzPhysics::JointConfiguration* CommandAdjustJointLimit::GetJointConfiguration(Actor** outActor, AZStd::string& outResult)
    {
        Actor* actor = GetActor(this, outResult);
        if (!actor)
        {
            *outActor = nullptr;
            outResult = "Could not get actor.";
            return nullptr;
        }
        *outActor = actor;

        const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
        if (physicsSetup)
        {
            const Physics::RagdollConfiguration& ragdollConfig = physicsSetup->GetRagdollConfig();
            const Physics::RagdollNodeConfiguration* ragdollNodeConfig = ragdollConfig.FindNodeConfigByName(m_jointName);
            return ragdollNodeConfig ? ragdollNodeConfig->m_jointConfig.get() : nullptr;
        }
        outResult = "Could not get physics setup.";
        return nullptr;
    }

    CommandAdjustJointLimit::CommandAdjustJointLimit(MCore::Command* orgCommand)
        : MCore::Command(CommandName, orgCommand)
    {
    }

    CommandAdjustJointLimit::CommandAdjustJointLimit(AZ::u32 actorId, const AZStd::string& jointName, MCore::Command* orgCommand)
        : MCore::Command(CommandName, orgCommand)
        , ParameterMixinActorIdJointName(actorId, jointName)
        , m_oldIsDirty(false)
    {
    }

    void CommandAdjustJointLimit::Reflect(AZ::ReflectContext* context)
    {
        auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<CommandAdjustJointLimit, MCore::Command, ParameterMixinActorIdJointName, ParameterMixinSerializedContents>()
            ->Version(1);
    }

    bool CommandAdjustJointLimit::Execute([[maybe_unused]] const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        Actor* actor = nullptr;
        AzPhysics::JointConfiguration* jointConfiguration = GetJointConfiguration(&actor, outResult);

        if (!jointConfiguration)
        {
            return false;
        }

        m_oldIsDirty = actor->GetDirtyFlag();

        if (m_contents.has_value())
        {
            if (!m_oldContents.has_value())
            {
                m_oldContents = MCore::ReflectionSerializer::Serialize(jointConfiguration).GetValue();
            }
            MCore::ReflectionSerializer::Deserialize(jointConfiguration, m_contents.value());
        }

        return true;
    }

    bool CommandAdjustJointLimit::Undo([[maybe_unused]] const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        Actor* actor = nullptr;
        AzPhysics::JointConfiguration* jointConfiguration = GetJointConfiguration(&actor, outResult);

        if (!jointConfiguration)
        {
            return false;
        }

        if (m_oldContents.has_value())
        {
            MCore::ReflectionSerializer::Deserialize(jointConfiguration, m_oldContents.value());
        }

        actor->SetDirtyFlag(m_oldIsDirty);
        return true;
    }

    void CommandAdjustJointLimit::SetJointConfiguration(const AzPhysics::JointConfiguration* jointConfiguration)
    {
        m_contents = MCore::ReflectionSerializer::Serialize(jointConfiguration).GetValue();
    }

    void CommandAdjustJointLimit::SetOldJointConfiguration(const AzPhysics::JointConfiguration* jointConfiguration)
    {
        m_oldContents = MCore::ReflectionSerializer::Serialize(jointConfiguration).GetValue();
    }
} // namespace EMotionFX
