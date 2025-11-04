/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/optional.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/string/string.h>
#include <AzFramework/Physics/Character.h>
#include <AzFramework/Physics/Ragdoll.h>
#include <MCore/Source/Command.h>
#include <MCore/Source/CommandGroup.h>
#include <EMotionFX/CommandSystem/Source/ParameterMixins.h>


namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    class Actor;
    class Node;
    class Skeleton;
    class CommandAddRagdollJoint;
    class CommandRemoveRagdollJoint;

    class CommandRagdollHelpers
    {
    public:
        static const Physics::RagdollNodeConfiguration* GetNodeConfig(const Actor* actor, const AZStd::string& jointName,
            const Physics::RagdollConfiguration& ragdollConfig, AZStd::string& outResult);

        static Physics::RagdollNodeConfiguration* GetCreateNodeConfig(const Actor* actor, const AZStd::string& jointName,
            Physics::RagdollConfiguration& ragdollConfig, const AZStd::optional<size_t>& index,
            AZStd::string& outResult);

        static AZStd::unique_ptr<AzPhysics::JointConfiguration> CreateJointLimitByType(
            AzPhysics::JointType jointType, const Skeleton* skeleton, const Node* node);

        static void AddJointsToRagdoll(AZ::u32 actorId, const AZStd::vector<AZStd::string>& jointNames,
            MCore::CommandGroup* commandGroup = nullptr, bool executeInsideCommand = false, bool addDefaultCollider = true);

        static void RemoveJointsFromRagdoll(AZ::u32 actorId, const AZStd::vector<AZStd::string>& jointNames,
            MCore::CommandGroup* commandGroup = nullptr, bool executeInsideCommand = false);

    private:
        static void AddJointToRagdoll(AZ::u32 actorId, const AZStd::string& jointName, const AZStd::optional<AZStd::string>& contents, const AZStd::optional<size_t>& index,
            MCore::CommandGroup* commandGroup = nullptr, bool executeInsideCommand = false, bool addDefaultCollider = true);

        static void RemoveJointFromRagdoll(AZ::u32 actorId, const AZStd::string& jointName,
            MCore::CommandGroup* commandGroup = nullptr, bool executeInsideCommand = false);

        friend class CommandAddRagdollJoint;
        friend class CommandRemoveRagdollJoint;
    };

    class CommandAddRagdollJoint
        : public MCore::Command
        , public ParameterMixinActorId
        , public ParameterMixinJointName
    {
    public:
        AZ_RTTI(CommandAddRagdollJoint, "{70F21A7D-8EC9-4F7B-B0FE-E63205E1F3FF}", MCore::Command, ParameterMixinActorId, ParameterMixinJointName)
        AZ_CLASS_ALLOCATOR_DECL

        explicit CommandAddRagdollJoint(MCore::Command* orgCommand = nullptr);
        CommandAddRagdollJoint(AZ::u32 actorId, const AZStd::string& jointName, MCore::Command* orgCommand = nullptr);

        void SetContents(const AZStd::string& newContents) { m_contents = newContents; }
        void SetJointIndex(size_t newIndex) { m_index = newIndex;}

        static void Reflect(AZ::ReflectContext* context);

        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;

        static const char* s_commandName;
        static const char* s_contentsParameterName;
        static const char* s_indexParameterName;
        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;

        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Add joint to ragdoll"; }
        const char* GetDescription() const override;
        MCore::Command* Create() override { return aznew CommandAddRagdollJoint(this); }

    private:
        AZStd::optional<AZStd::string> m_contents;
        AZStd::optional<size_t> m_index;

        bool m_oldIsDirty;
    };

    class CommandRemoveRagdollJoint
        : public MCore::Command
        , public ParameterMixinActorId
        , public ParameterMixinJointName
    {
    public:
        AZ_RTTI(CommandRemoveRagdollJoint, "{AC5DAC4D-89EF-49CD-8E0C-DBA94147CED0}", MCore::Command, ParameterMixinActorId, ParameterMixinJointName)
        AZ_CLASS_ALLOCATOR_DECL

        explicit CommandRemoveRagdollJoint(MCore::Command* orgCommand = nullptr);
        CommandRemoveRagdollJoint(AZ::u32 actorId, const AZStd::string& jointName, MCore::Command* orgCommand = nullptr);
        static void Reflect(AZ::ReflectContext* context);

        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;

        static const char* s_commandName;
        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;

        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Remove joint from ragdoll"; }
        const char* GetDescription() const override;
        MCore::Command* Create() override { return aznew CommandRemoveRagdollJoint(this); }

    private:
        AZStd::string m_oldContents;
        size_t m_oldIndex;
        bool m_oldIsDirty;
    };

    class CommandAdjustRagdollJoint
        : public MCore::Command
        , public ParameterMixinActorId
        , public ParameterMixinJointName
    {
    public:
        AZ_RTTI(CommandAdjustRagdollJoint, "{B2ADE3E1-3A6C-40F2-8D12-BF17F5B3FDB2}", MCore::Command, ParameterMixinActorId, ParameterMixinJointName);
        AZ_CLASS_ALLOCATOR_DECL

        explicit CommandAdjustRagdollJoint(MCore::Command* orgCommand = nullptr);
        CommandAdjustRagdollJoint(AZ::u32 actorId, const AZStd::string& jointName, AZStd::optional<AZStd::string> serializedJointLimits = AZStd::nullopt, MCore::Command* orgCommand = nullptr);
        static void Reflect(AZ::ReflectContext* context);

        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;

        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;

        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Adjust joint in ragdoll"; }
        const char* GetDescription() const override { return GetHistoryName(); }

        MCore::Command* Create() override { return aznew CommandAdjustRagdollJoint(this); }

        static AZ::Outcome<AZStd::string> SerializeJointLimits(const Physics::RagdollNodeConfiguration* ragdollNodeConfig);

        static const char* s_commandName;
    private:
        AZStd::optional<AZStd::string> m_serializedJointLimits;
        AZStd::optional<AZStd::string> m_oldSerializedJointLimits;
    };
} // namespace EMotionFX
