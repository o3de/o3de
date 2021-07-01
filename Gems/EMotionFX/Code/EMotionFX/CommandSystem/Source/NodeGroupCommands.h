/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include "CommandSystemConfig.h"
#include <MCore/Source/Command.h>
#include <MCore/Source/CommandGroup.h>
#include <EMotionFX/Source/EMotionFXConfig.h>

EMFX_FORWARD_DECLARE(Actor);
EMFX_FORWARD_DECLARE(NodeGroup);

namespace CommandSystem
{
    // adjust a node group
    MCORE_DEFINECOMMAND_START(CommandAdjustNodeGroup, "Adjust node group", true)
    bool                    mOldDirtyFlag;
    EMotionFX::NodeGroup*   mOldNodeGroup;
    MCORE_DEFINECOMMAND_END


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
