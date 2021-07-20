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
    {
    public:
        CommandAdjustNodeGroup(MCore::Command* orgCommand = nullptr);
        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        void InitSyntax() override;
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
            return new CommandAdjustNodeGroup(this);
        }

    protected:
        bool mOldDirtyFlag = false;
        AZStd::unique_ptr<EMotionFX::NodeGroup> mOldNodeGroup = nullptr;
    };

    // add node group
    MCORE_DEFINECOMMAND_START(CommandAddNodeGroup, "Add node group", true)
    bool                    mOldDirtyFlag;
    MCORE_DEFINECOMMAND_END


    // remove a node group
    MCORE_DEFINECOMMAND_START(CommandRemoveNodeGroup, "Remove node group", true)
    EMotionFX::NodeGroup *   mOldNodeGroup;
    bool                    mOldDirtyFlag;
    MCORE_DEFINECOMMAND_END


    // helper functions
    void COMMANDSYSTEM_API ClearNodeGroupsCommand(EMotionFX::Actor* actor, MCore::CommandGroup* commandGroup = nullptr);
} // namespace CommandSystem
