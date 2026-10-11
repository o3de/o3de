/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/string/string.h>
#include <EMotionFX/CommandSystem/Source/ParameterMixins.h>
#include <EMotionFX/Source/SocketSetup.h>
#include <MCore/Source/Command.h>
#include <MCore/Source/CommandGroup.h>


namespace EMotionFX
{
    class CommandSocketHelpers
    {
    public:
        // An empty name asks for a generated unique one; the joint is matched case insensitively.
        static bool AddSocket(AZ::u32 actorId, const AZStd::string& name, const AZStd::string& jointName,
            const AZ::Vector3& position, const AZ::Quaternion& rotation, MCore::CommandGroup* commandGroup = nullptr);
        static bool RemoveSocket(AZ::u32 actorId, const AZStd::string& name, MCore::CommandGroup* commandGroup = nullptr);
        static bool AdjustSocket(AZ::u32 actorId, const AZStd::string& name, const AZStd::optional<AZStd::string>& newName,
            const AZStd::optional<AZStd::string>& jointName, const AZStd::optional<AZ::Vector3>& position,
            const AZStd::optional<AZ::Quaternion>& rotation, MCore::CommandGroup* commandGroup = nullptr);
    };

    class CommandAddSocket
        : public MCore::Command
        , public ParameterMixinActorId
    {
    public:
        AZ_RTTI(CommandAddSocket, "{C4AA8BF7-38D8-4841-AD90-AD1B4D95ED23}", MCore::Command, ParameterMixinActorId)
        AZ_CLASS_ALLOCATOR_DECL

        explicit CommandAddSocket(MCore::Command* orgCommand = nullptr);

        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;

        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Add a socket to an actor"; }
        const char* GetDescription() const override { return "Add a socket to an actor"; }
        MCore::Command* Create() override { return aznew CommandAddSocket(this); }

        // The name the socket ended up with; only valid after Execute().
        const AZStd::string& GetSocketName() const { return m_name; }

        static const char* s_commandName;
        static const char* s_nameParameterName;
        static const char* s_jointNameParameterName;
        static const char* s_positionParameterName;
        static const char* s_rotationParameterName;

    private:
        AZStd::string m_name;
        AZStd::string m_jointName;
        AZ::Vector3 m_position = AZ::Vector3::CreateZero();
        AZ::Quaternion m_rotation = AZ::Quaternion::CreateIdentity();
        bool m_oldDirtyFlag = false;
    };

    class CommandRemoveSocket
        : public MCore::Command
        , public ParameterMixinActorId
    {
    public:
        AZ_RTTI(CommandRemoveSocket, "{724A4C42-5AAA-4AEF-8615-AE9B5571B6F5}", MCore::Command, ParameterMixinActorId)
        AZ_CLASS_ALLOCATOR_DECL

        explicit CommandRemoveSocket(MCore::Command* orgCommand = nullptr);

        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;

        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Remove a socket from an actor"; }
        const char* GetDescription() const override { return "Remove a socket from an actor"; }
        MCore::Command* Create() override { return aznew CommandRemoveSocket(this); }

        static const char* s_commandName;
        static const char* s_nameParameterName;

    private:
        AZStd::string m_name;
        ActorSocket m_oldSocket;
        size_t m_oldIndex = 0;
        bool m_oldDirtyFlag = false;
    };

    class CommandAdjustSocket
        : public MCore::Command
        , public ParameterMixinActorId
    {
    public:
        AZ_RTTI(CommandAdjustSocket, "{5E4ED4F3-84CD-45D4-A891-D8023A4CF244}", MCore::Command, ParameterMixinActorId)
        AZ_CLASS_ALLOCATOR_DECL

        explicit CommandAdjustSocket(MCore::Command* orgCommand = nullptr);

        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;

        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Adjust socket attributes"; }
        const char* GetDescription() const override { return "Adjust the attributes of an actor socket"; }
        MCore::Command* Create() override { return aznew CommandAdjustSocket(this); }

        static const char* s_commandName;
        static const char* s_nameParameterName;
        static const char* s_newNameParameterName;
        static const char* s_jointNameParameterName;
        static const char* s_positionParameterName;
        static const char* s_rotationParameterName;

    private:
        AZStd::string m_name;
        AZStd::optional<AZStd::string> m_newName;
        AZStd::optional<AZStd::string> m_jointName;
        AZStd::optional<AZ::Vector3> m_position;
        AZStd::optional<AZ::Quaternion> m_rotation;

        AZStd::optional<ActorSocket> m_oldSocket;
        AZStd::string m_appliedName;
        bool m_oldDirtyFlag = false;
    };
} // namespace EMotionFX
