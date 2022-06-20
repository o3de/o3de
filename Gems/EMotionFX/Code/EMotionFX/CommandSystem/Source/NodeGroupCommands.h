/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include "CommandSystemConfig.h"
#include <MCore/Source/Command.h>
#include <MCore/Source/CommandGroup.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/NodeGroup.h>
#include <EMotionFX/CommandSystem/Source/ParameterMixins.h>

EMFX_FORWARD_DECLARE(Actor);
EMFX_FORWARD_DECLARE(NodeGroup);

namespace CommandSystem
{
    // adjust a node group
    class CommandAdjustNodeGroup
        : public MCore::Command
        , public EMotionFX::ParameterMixinActorId
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        enum class NodeAction
        {
            Add,
            Remove,
            Replace
        };

        static constexpr inline AZStd::string_view s_commandName = "AdjustNodeGroup";

        CommandAdjustNodeGroup(
            MCore::Command* orgCommand = nullptr,
            uint32 actorId = MCORE_INVALIDINDEX32,
            const AZStd::string& name = {},
            AZStd::optional<AZStd::string> newName = AZStd::nullopt,
            AZStd::optional<bool> enabledOnDefault = AZStd::nullopt,
            AZStd::optional<AZStd::vector<AZStd::string>> nodeNames = AZStd::nullopt,
            AZStd::optional<NodeAction> nodeAction = AZStd::nullopt
        );
        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;
        bool GetIsUndoable() const override
        {
            return true;
        }
        const char* GetHistoryName() const override
        {
            return "Adjust node group";
        }
        const char* GetDescription() const override;
        MCore::Command* Create() override
        {
            return aznew CommandAdjustNodeGroup(this);
        }

    private:
        AZStd::string m_name;
        AZStd::optional<AZStd::string> m_newName;
        AZStd::optional<bool> m_enabledOnDefault;
        AZStd::optional<AZStd::vector<AZStd::string>> m_nodeNames;
        AZStd::optional<NodeAction> m_nodeAction;

        bool m_oldDirtyFlag = false;
        AZStd::unique_ptr<EMotionFX::NodeGroup> m_oldNodeGroup = nullptr;
    };

    // add node group
    MCORE_DEFINECOMMAND_START(CommandAddNodeGroup, "Add node group", true)
    bool                    m_oldDirtyFlag;
    MCORE_DEFINECOMMAND_END


    // remove a node group
    MCORE_DEFINECOMMAND_START(CommandRemoveNodeGroup, "Remove node group", true)
    EMotionFX::NodeGroup *   m_oldNodeGroup;
    bool                    m_oldDirtyFlag;
    MCORE_DEFINECOMMAND_END


    // helper functions
    void COMMANDSYSTEM_API ClearNodeGroupsCommand(EMotionFX::Actor* actor, MCore::CommandGroup* commandGroup = nullptr);
} // namespace CommandSystem
