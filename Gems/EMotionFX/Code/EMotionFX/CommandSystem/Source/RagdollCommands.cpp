/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/iterator.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Physics/Ragdoll.h>
#include <AzFramework/Physics/SystemBus.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/ColliderCommands.h>
#include <EMotionFX/CommandSystem/Source/RagdollCommands.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/PhysicsSetup.h>
#include <MCore/Source/AzCoreConversions.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(CommandAddRagdollJoint, EMotionFX::CommandAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(CommandRemoveRagdollJoint, EMotionFX::CommandAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(CommandAdjustRagdollJoint, EMotionFX::CommandAllocator)

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    const Physics::RagdollNodeConfiguration* CommandRagdollHelpers::GetNodeConfig(const Actor * actor, const AZStd::string& jointName,
        const Physics::RagdollConfiguration& ragdollConfig, AZStd::string& outResult)
    {
        const Skeleton* skeleton = actor->GetSkeleton();
        const Node* joint = skeleton->FindNodeByName(jointName);
        if (!joint)
        {
            outResult = AZStd::string::format("Cannot get node config. Joint with name '%s' does not exist.", jointName.c_str());
            return nullptr;
        }

        return ragdollConfig.FindNodeConfigByName(jointName);
    }

    Physics::RagdollNodeConfiguration* CommandRagdollHelpers::GetCreateNodeConfig(const Actor* actor, const AZStd::string& jointName,
        Physics::RagdollConfiguration& ragdollConfig, const AZStd::optional<size_t>& index, AZStd::string& outResult)
    {
        const Skeleton* skeleton = actor->GetSkeleton();
        const Node* joint = skeleton->FindNodeByName(jointName);
        if (!joint)
        {
            outResult = AZStd::string::format("Cannot add node config. Joint with name '%s' does not exist.", jointName.c_str());
            return nullptr;
        }

        Physics::RagdollNodeConfiguration* nodeConfig = ragdollConfig.FindNodeConfigByName(jointName);
        if (nodeConfig)
        {
            return nodeConfig;
        }

        Physics::RagdollNodeConfiguration newNodeConfig;
        newNodeConfig.m_debugName = jointName;

        // Create joint limit on default.
        newNodeConfig.m_jointConfig = CommandRagdollHelpers::CreateJointLimitByType(AzPhysics::JointType::D6Joint, skeleton, joint);

        if (index)
        {
            const auto iterator = ragdollConfig.m_nodes.begin() + index.value();
            ragdollConfig.m_nodes.insert(iterator, newNodeConfig);
            return iterator;
        }
        else
        {
            ragdollConfig.m_nodes.push_back(newNodeConfig);
            return &ragdollConfig.m_nodes.back();
        }
    }

    AZStd::unique_ptr<AzPhysics::JointConfiguration> CommandRagdollHelpers::CreateJointLimitByType(
        AzPhysics::JointType jointType, const Skeleton* skeleton, const Node* node)
    {
        const Pose* bindPose = skeleton->GetBindPose();
        const Transform& nodeBindTransform = bindPose->GetModelSpaceTransform(node->GetNodeIndex());
        const Transform& parentBindTransform = node->GetParentNode()
            ? bindPose->GetModelSpaceTransform(node->GetParentIndex())
            : Transform::CreateIdentity();
        const AZ::Quaternion& nodeBindRotationWorld = nodeBindTransform.m_rotation;
        const AZ::Quaternion& parentBindRotationWorld = parentBindTransform.m_rotation;

        AZ::Vector3 boneDirection = GetBoneDirection(skeleton, node);
        AZStd::vector<AZ::Quaternion> locaRotationSamples;

        if (auto* jointHelpers = AZ::Interface<AzPhysics::JointHelpersInterface>::Get())
        {
            if (AZStd::optional<const AZ::TypeId> jointTypeId = jointHelpers->GetSupportedJointTypeId(jointType);
                jointTypeId.has_value())
            {
                AZStd::unique_ptr<AzPhysics::JointConfiguration> jointLimitConfig = jointHelpers->ComputeInitialJointLimitConfiguration(
                    *jointTypeId, parentBindRotationWorld, nodeBindRotationWorld, boneDirection, locaRotationSamples);

                AZ_Assert(jointLimitConfig, "Could not create joint limit configuration.");
                jointLimitConfig->SetPropertyVisibility(AzPhysics::JointConfiguration::PropertyVisibility::ParentLocalRotation, true);
                jointLimitConfig->SetPropertyVisibility(AzPhysics::JointConfiguration::PropertyVisibility::ChildLocalRotation, true);
                return jointLimitConfig;
            }
        }
        AZ_Assert(false, "Could not create joint limit configuration.");
        return nullptr;
    }

    void CommandRagdollHelpers::AddJointsToRagdoll(AZ::u32 actorId, const AZStd::vector<AZStd::string>& jointNames,
        MCore::CommandGroup* commandGroup, bool executeInsideCommand, bool addDefaultCollider)
    {
        const Actor* actor = GetEMotionFX().GetActorManager()->FindActorByID(actorId);
        if (!actor)
        {
            return;
        }
        const Physics::RagdollConfiguration& ragdollConfig = actor->GetPhysicsSetup()->GetRagdollConfig();

        AZStd::unordered_set<AZStd::string> newJointNames;
        AZStd::remove_copy_if(begin(jointNames), end(jointNames), AZStd::inserter(newJointNames, newJointNames.end()), [&ragdollConfig](const AZStd::string& jointName) {
            return ragdollConfig.FindNodeConfigByName(jointName);
        });
        if (newJointNames.empty())
        {
            // These joints are already in the ragdoll
            return;
        }

        AZStd::unordered_set<AZStd::string> jointsToAdd {newJointNames};
        // keep track of the root and its immediate children, to avoid adding colliders to those joints later
        AZStd::unordered_set<AZStd::string> rootAndImmediateChildren;

        // The new joints being added are leaf joints in the ragdoll. Find all
        // parent joints that are not currently in the ragdoll, and add them as
        // well.
        for (const AZStd::string& jointToAdd : newJointNames)
        {
            AZStd::unordered_set<AZStd::string> parents;

            Node* node = actor->GetSkeleton()->FindNodeByName(jointToAdd);
            if (node->GetIsRootNode() || node->GetParentNode()->GetIsRootNode())
            {
                rootAndImmediateChildren.emplace(node->GetNameString());
            }
            node = node->GetParentNode();

            while (node)
            {
                if (jointsToAdd.contains(node->GetNameString()))
                {
                    break;
                }

                if (!ragdollConfig.FindNodeConfigByName(node->GetNameString()))
                {
                    parents.emplace(node->GetNameString());
                }
                else
                {
                    // Ideally we could stop here, but we continue the traversal
                    // all the way up to the root joint to fix any existing asset
                    // with bad data
                }

                Node* parentNode = node->GetParentNode();
                if (node->GetIsRootNode() || parentNode->GetIsRootNode())
                {
                    rootAndImmediateChildren.emplace(node->GetNameString());
                }
                node = parentNode;
            }

            jointsToAdd.insert(AZStd::make_move_iterator(parents.begin()), AZStd::make_move_iterator(parents.end()));
        }

        for(const AZStd::string& jointToAdd : jointsToAdd)
        {
            AddJointToRagdoll(
                actorId,
                jointToAdd,
                AZStd::nullopt,
                AZStd::nullopt,
                commandGroup,
                executeInsideCommand,
                rootAndImmediateChildren.contains(jointToAdd) ? false : addDefaultCollider);
        }
    }

    void CommandRagdollHelpers::AddJointToRagdoll(AZ::u32 actorId, const AZStd::string& jointName,
        const AZStd::optional<AZStd::string>& contents, const AZStd::optional<size_t>& index,
        MCore::CommandGroup* commandGroup, bool executeInsideCommand, bool addDefaultCollider)
    {
        // 1. Add joint to ragdoll.
        CommandAddRagdollJoint* command = aznew CommandAddRagdollJoint(actorId, jointName);
        if (contents)
        {
            command->SetContents(contents.value());
        }
        if (index)
        {
            command->SetJointIndex(index.value());
        }

        CommandSystem::GetCommandManager()->ExecuteCommandOrAddToGroup(command, commandGroup, executeInsideCommand);

        // 2. Create default capsule collider.
        if (addDefaultCollider)
        {
            const AZ::TypeId defaultColliderType = azrtti_typeid<Physics::CapsuleShapeConfiguration>();
            CommandColliderHelpers::AddCollider(actorId, jointName,
                PhysicsSetup::Ragdoll, defaultColliderType,
                commandGroup, executeInsideCommand);
        }
    }

    void CommandRagdollHelpers::RemoveJointsFromRagdoll(AZ::u32 actorId, const AZStd::vector<AZStd::string>& jointNames,
        MCore::CommandGroup* commandGroup, bool executeInsideCommand)
    {
        const Actor* actor = GetEMotionFX().GetActorManager()->FindActorByID(actorId);
        if (!actor)
        {
            return;
        }
        const Physics::RagdollConfiguration& ragdollConfig = actor->GetPhysicsSetup()->GetRagdollConfig();

        const auto getChildJointsInRagdoll = [](const Skeleton* skeleton, const Node* joint, const Physics::RagdollConfiguration& ragdollConfig, AZStd::unordered_set<AZStd::string>& result)
        {
            // This nested lambda takes a lambda as its first argument,
            // allowing for it to call itself recursively
            const auto getChildJointsInRagdoll_ = [](const auto getChildJointsInRagdoll, const Skeleton* skeleton, const Node* joint, const Physics::RagdollConfiguration& ragdollConfig, AZStd::unordered_set<AZStd::string>& result) -> void
            {
                if (ragdollConfig.FindNodeConfigByName(joint->GetNameString()))
                {
                    result.emplace(joint->GetNameString());
                }

                // We examine the child joints as well, even if this joint is
                // not in the ragdoll, in case invalid setups were made somehow
                for (uint32 i = 0; i < joint->GetNumChildNodes(); ++i)
                {
                    getChildJointsInRagdoll(getChildJointsInRagdoll, skeleton, skeleton->GetNode(joint->GetChildIndex(i)), ragdollConfig, result);
                }
            };

            getChildJointsInRagdoll_(getChildJointsInRagdoll_, skeleton, joint, ragdollConfig, result);
        };

        // Find the joints to remove, and their children, recursively
        AZStd::unordered_set<AZStd::string> jointsToRemove;
        for (const AZStd::string& jointToRemove : jointNames)
        {
            getChildJointsInRagdoll(actor->GetSkeleton(), actor->GetSkeleton()->FindNodeByName(jointToRemove), ragdollConfig, jointsToRemove);
        }

        for (const AZStd::string& jointToRemove : jointsToRemove)
        {
            RemoveJointFromRagdoll(actorId, jointToRemove, commandGroup, executeInsideCommand);
        }
    }

    void CommandRagdollHelpers::RemoveJointFromRagdoll(AZ::u32 actorId, const AZStd::string& jointName, MCore::CommandGroup* commandGroup, bool executeInsideCommand)
    {
        // 1. Clear all ragdoll colliders for this joint.
        CommandColliderHelpers::ClearColliders(actorId, jointName,
            PhysicsSetup::Ragdoll,
            commandGroup);

        // 2. Remove joint from ragdoll.
        auto command = aznew CommandRemoveRagdollJoint(actorId, jointName);

        CommandSystem::GetCommandManager()->ExecuteCommandOrAddToGroup(command, commandGroup, executeInsideCommand);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    const char* CommandAddRagdollJoint::s_commandName = "AddRagdollJoint";
    const char* CommandAddRagdollJoint::s_contentsParameterName = "contents";
    const char* CommandAddRagdollJoint::s_indexParameterName = "index";

    CommandAddRagdollJoint::CommandAddRagdollJoint(MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
        , m_oldIsDirty(false)
    {
    }

    CommandAddRagdollJoint::CommandAddRagdollJoint(AZ::u32 actorId, const AZStd::string& jointName, MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
        , ParameterMixinActorId(actorId)
        , ParameterMixinJointName(jointName)
    {
    }

    void CommandAddRagdollJoint::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<CommandAddRagdollJoint, MCore::Command, ParameterMixinActorId, ParameterMixinJointName>()
            ->Version(1)
        ;
    }

    bool CommandAddRagdollJoint::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        Actor* actor = GetActor(this, outResult);
        if (!actor)
        {
            return false;
        }

        const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
        Physics::RagdollConfiguration& ragdollConfig = physicsSetup->GetRagdollConfig();

        Physics::RagdollNodeConfiguration* nodeConfig = CommandRagdollHelpers::GetCreateNodeConfig(actor, m_jointName, ragdollConfig, m_index, outResult);
        if (!nodeConfig)
        {
            return false;
        }

        // Either in case the contents got specified via a command parameter or in case of redo.
        if (m_contents)
        {
            const AZStd::string contents = m_contents.value();
            MCore::ReflectionSerializer::Deserialize(nodeConfig, contents);
        }

        m_oldIsDirty = actor->GetDirtyFlag();
        actor->SetDirtyFlag(true);
        return true;
    }

    bool CommandAddRagdollJoint::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        Actor* actor = GetActor(this, outResult);
        if (!actor)
        {
            return false;
        }

        const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
        Physics::RagdollConfiguration& ragdollConfig = physicsSetup->GetRagdollConfig();

        const Physics::RagdollNodeConfiguration* nodeConfig = CommandRagdollHelpers::GetNodeConfig(actor, m_jointName, ragdollConfig, outResult);
        if (!nodeConfig)
        {
            return false;
        }

        m_contents = MCore::ReflectionSerializer::Serialize(nodeConfig).GetValue();

        CommandRagdollHelpers::RemoveJointFromRagdoll(m_actorId, m_jointName, /*commandGroup*/ nullptr, true);

        actor->SetDirtyFlag(m_oldIsDirty);
        return true;
    }

    void CommandAddRagdollJoint::InitSyntax()
    {
        MCore::CommandSyntax& syntax = GetSyntax();
        syntax.ReserveParameters(3);
        ParameterMixinActorId::InitSyntax(syntax);
        ParameterMixinJointName::InitSyntax(syntax);

        syntax.AddParameter(s_contentsParameterName, "The serialized contents (in reflected XML).", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        syntax.AddParameter(s_indexParameterName, "The index of the ragdoll node config.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
    }

    bool CommandAddRagdollJoint::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        ParameterMixinActorId::SetCommandParameters(parameters);
        ParameterMixinJointName::SetCommandParameters(parameters);

        if (parameters.CheckIfHasParameter(s_contentsParameterName))
        {
            m_contents = parameters.GetValue(s_contentsParameterName, this);
        }

        if (parameters.CheckIfHasParameter(s_indexParameterName))
        {
            m_index = parameters.GetValueAsInt(s_indexParameterName, this);
        }

        return true;
    }

    const char* CommandAddRagdollJoint::GetDescription() const
    {
        return "Add node to ragdoll.";
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    const char* CommandRemoveRagdollJoint::s_commandName = "RemoveRagdollJoint";

    CommandRemoveRagdollJoint::CommandRemoveRagdollJoint(MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
        , m_oldIsDirty(false)
    {
    }

    CommandRemoveRagdollJoint::CommandRemoveRagdollJoint(AZ::u32 actorId, const AZStd::string& jointName, MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
        , ParameterMixinActorId(actorId)
        , ParameterMixinJointName(jointName)
    {
    }

    void CommandRemoveRagdollJoint::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<CommandRemoveRagdollJoint, MCore::Command, ParameterMixinActorId, ParameterMixinJointName>()
            ->Version(1)
        ;
    }

    bool CommandRemoveRagdollJoint::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        Actor* actor = GetActor(this, outResult);
        if (!actor)
        {
            return false;
        }

        const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
        Physics::RagdollConfiguration& ragdollConfig = physicsSetup->GetRagdollConfig();

        const Physics::RagdollNodeConfiguration* nodeConfig = CommandRagdollHelpers::GetNodeConfig(actor, m_jointName, ragdollConfig, outResult);
        if (!nodeConfig)
        {
            return false;
        }

        m_oldContents = MCore::ReflectionSerializer::Serialize(nodeConfig).GetValue();
        m_oldIndex = ragdollConfig.FindNodeConfigIndexByName(m_jointName).GetValue();
        m_oldIsDirty = actor->GetDirtyFlag();

        ragdollConfig.RemoveNodeConfigByName(m_jointName);

        actor->SetDirtyFlag(true);
        return true;
    }

    bool CommandRemoveRagdollJoint::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        Actor* actor = GetActor(this, outResult);
        if (!actor)
        {
            return false;
        }

        CommandRagdollHelpers::AddJointToRagdoll(m_actorId, m_jointName, m_oldContents, m_oldIndex, /*commandGroup*/ nullptr, /*executeInsideCommand*/ true, /*addDefaultCollider*/ false);
        actor->SetDirtyFlag(m_oldIsDirty);
        return true;
    }

    void CommandRemoveRagdollJoint::InitSyntax()
    {
        MCore::CommandSyntax& syntax = GetSyntax();
        syntax.ReserveParameters(2);
        ParameterMixinActorId::InitSyntax(syntax);
        ParameterMixinJointName::InitSyntax(syntax);
    }

    bool CommandRemoveRagdollJoint::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        ParameterMixinActorId::SetCommandParameters(parameters);
        ParameterMixinJointName::SetCommandParameters(parameters);
        return true;
    }

    const char* CommandRemoveRagdollJoint::GetDescription() const
    {
        return "Remove the given joint from the ragdoll.";
    }

    // ------------------------------------------------------------------------

    const char* CommandAdjustRagdollJoint::s_commandName = "AdjustRagdollJoint";

    CommandAdjustRagdollJoint::CommandAdjustRagdollJoint(MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
    {
    }

    CommandAdjustRagdollJoint::CommandAdjustRagdollJoint(AZ::u32 actorId, const AZStd::string& jointName, AZStd::optional<AZStd::string> serializedJointLimits, MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
        , ParameterMixinActorId(actorId)
        , ParameterMixinJointName(jointName)
        , m_serializedJointLimits(AZStd::move(serializedJointLimits))
    {
    }

    void CommandAdjustRagdollJoint::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<CommandAdjustRagdollJoint, MCore::Command, ParameterMixinActorId, ParameterMixinJointName>()
            ->Version(1)
        ;
    }

    bool CommandAdjustRagdollJoint::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);
        AZ_UNUSED(outResult);

        Actor* actor = GetActor(this, outResult);
        if (!actor)
        {
            return false;
        }

        Physics::RagdollConfiguration& ragdollConfig = actor->GetPhysicsSetup()->GetRagdollConfig();

        Physics::RagdollNodeConfiguration* nodeConfig = ragdollConfig.FindNodeConfigByName(m_jointName);

        if (!nodeConfig)
        {
            return false;
        }

        bool success = true;

        if (m_serializedJointLimits)
        {
            AZ::Outcome<AZStd::string> oldSerializedJointLimits = SerializeJointLimits(nodeConfig);
            success |= MCore::ReflectionSerializer::DeserializeMembers(nodeConfig->m_jointConfig.get(), m_serializedJointLimits.value());
            if (success && oldSerializedJointLimits.IsSuccess())
            {
                m_oldSerializedJointLimits = oldSerializedJointLimits.GetValue();
            }
        }

        return success;
    }

    bool CommandAdjustRagdollJoint::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        return CommandAdjustRagdollJoint(m_actorId, m_jointName, m_oldSerializedJointLimits).Execute(parameters, outResult);
    }

    void CommandAdjustRagdollJoint::InitSyntax()
    {
        MCore::CommandSyntax& syntax = GetSyntax();
        syntax.ReserveParameters(2);
        ParameterMixinActorId::InitSyntax(syntax);
        ParameterMixinJointName::InitSyntax(syntax);
    }

    bool CommandAdjustRagdollJoint::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        ParameterMixinActorId::SetCommandParameters(parameters);
        ParameterMixinJointName::SetCommandParameters(parameters);
        return true;
    }

    AZ::Outcome<AZStd::string> CommandAdjustRagdollJoint::SerializeJointLimits(const Physics::RagdollNodeConfiguration* ragdollNodeConfig)
    {
        return MCore::ReflectionSerializer::SerializeMembersExcept(
            ragdollNodeConfig->m_jointConfig.get(),
            {"ParentLocalRotation", "ParentLocalPosition", "ChildLocalRotation", "ChildLocalPosition", }
        );
    }

} // namespace EMotionFX
