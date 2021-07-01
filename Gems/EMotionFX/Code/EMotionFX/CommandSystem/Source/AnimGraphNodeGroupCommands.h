/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "CommandSystemConfig.h"
#include <MCore/Source/Command.h>
#include <MCore/Source/CommandGroup.h>
#include <EMotionFX/Source/AnimGraphNodeGroup.h>
#include <EMotionFX/Source/AnimGraph.h>

namespace CommandSystem
{
    // adjust a node group
    MCORE_DEFINECOMMAND_START(CommandAnimGraphAdjustNodeGroup, "Adjust anim graph node group", true)
public:
    static AZStd::string GenerateNodeNameString(EMotionFX::AnimGraph* animGraph, const AZStd::vector<EMotionFX::AnimGraphNodeId>& nodeIDs);
    static AZStd::vector<EMotionFX::AnimGraphNodeId> CollectNodeIdsFromGroup(EMotionFX::AnimGraphNodeGroup* nodeGroup);

    AZStd::string                               mOldName;
    bool                                        mOldIsVisible;
    AZ::u32                                     mOldColor;
    AZStd::vector<EMotionFX::AnimGraphNodeId>   mOldNodeIds;
    bool                                        mOldDirtyFlag;
    MCORE_DEFINECOMMAND_END


    // add node group
    MCORE_DEFINECOMMAND_START(CommandAnimGraphAddNodeGroup, "Add anim graph node group", true)
    bool                    mOldDirtyFlag;
    AZStd::string           mOldName;
    MCORE_DEFINECOMMAND_END


    // remove a node group
    MCORE_DEFINECOMMAND_START(CommandAnimGraphRemoveNodeGroup, "Remove anim graph node group", true)
    AZStd::string                               mOldName;
    bool                                        mOldIsVisible;
    AZ::u32                                     mOldColor;
    AZStd::vector<EMotionFX::AnimGraphNodeId>   mOldNodeIds;
    bool                                        mOldDirtyFlag;
    MCORE_DEFINECOMMAND_END

    // helper function
    COMMANDSYSTEM_API void ClearNodeGroups(EMotionFX::AnimGraph* animGraph, MCore::CommandGroup* commandGroup = nullptr);
} // namespace CommandSystem
