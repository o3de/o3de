/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <MCore/Source/Command.h>
#include <EMotionFX/Source/AnimGraphObjectIds.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/BlendTreeConnection.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphCopyPasteData.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>


namespace CommandSystem
{
    // create a blend node
    MCORE_DEFINECOMMAND_START(CommandAnimGraphCreateNode, "Create a anim graph node", true)
public:
    EMotionFX::AnimGraphNodeId GetNodeId(const MCore::CommandLine& parameters);
    void DeleteGraphNode(EMotionFX::AnimGraphNode* node);

    uint32  mAnimGraphID;
    bool    mOldDirtyFlag;
    EMotionFX::AnimGraphNodeId mNodeId;
    MCORE_DEFINECOMMAND_END


    // adjust a node
    MCORE_DEFINECOMMAND_START(CommandAnimGraphAdjustNode, "Adjust a anim graph node", true)
    EMotionFX::AnimGraphNodeId mNodeId;
    int32               mOldPosX;
    int32               mOldPosY;
    AZStd::string       mOldName;
    AZStd::string       mOldParameterMask;
    bool                mOldDirtyFlag;
    bool                mOldEnabled;
    bool                mOldVisualized;
    AZStd::string       mNodeGroupName;

public:
    EMotionFX::AnimGraphNodeId GetNodeId() const    { return mNodeId; }
    const AZStd::string& GetOldName() const         { return mOldName; }
    uint32              mAnimGraphID;
    MCORE_DEFINECOMMAND_END


    // remove a node
    MCORE_DEFINECOMMAND_START(CommandAnimGraphRemoveNode, "Remove a anim graph node", true)
    EMotionFX::AnimGraphNodeId mNodeId;
    uint32          mAnimGraphID;
    EMotionFX::AnimGraphNodeId mParentNodeId;
    AZ::TypeId      mType;
    AZStd::string   mParentName;
    AZStd::string   mName;
    AZStd::string   mNodeGroupName;
    int32           mPosX;
    int32           mPosY;
    AZStd::string   mOldContents;
    bool            mCollapsed;
    bool            mOldDirtyFlag;
    bool            mIsEntryNode;

public:
    EMotionFX::AnimGraphNodeId GetNodeId() const        { return mNodeId; }
    EMotionFX::AnimGraphNodeId GetParentNodeId() const  { return mParentNodeId; }
    MCORE_DEFINECOMMAND_END


    // set the entry state of a state machine
    MCORE_DEFINECOMMAND_START(CommandAnimGraphSetEntryState, "Set entry state", true)
public:
    uint32                      mAnimGraphID;
    EMotionFX::AnimGraphNodeId  mOldEntryStateNodeId;
    EMotionFX::AnimGraphNodeId  mOldStateMachineNodeId;
    bool                        mOldDirtyFlag;
    MCORE_DEFINECOMMAND_END


    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper functions
    //////////////////////////////////////////////////////////////////////////////////////////////////////////

    COMMANDSYSTEM_API void CreateAnimGraphNode(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraph* animGraph, const AZ::TypeId& type, const AZStd::string& namePrefix, EMotionFX::AnimGraphNode* parentNode, int32 offsetX, int32 offsetY, const AZStd::string& serializedContents = "");

    COMMANDSYSTEM_API void DeleteNodes(EMotionFX::AnimGraph* animGraph, const AZStd::vector<AZStd::string>& nodeNames);
    COMMANDSYSTEM_API void DeleteNodes(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraph* animGraph, const AZStd::vector<AZStd::string>& nodeNames, AZStd::vector<EMotionFX::AnimGraphNode*>& nodeList, AZStd::vector<EMotionFX::BlendTreeConnection*>& connectionList, AZStd::vector<EMotionFX::AnimGraphStateTransition*>& transitionList, bool autoChangeEntryStates = true);
    COMMANDSYSTEM_API void DeleteNodes(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraph* animGraph, const AZStd::vector<EMotionFX::AnimGraphNode*>& nodes, bool autoChangeEntryStates = true);

    COMMANDSYSTEM_API void ConstructCopyAnimGraphNodesCommandGroup(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraphNode* targetNode, AZStd::vector<EMotionFX::AnimGraphNode*>& inOutNodesToCopy, int32 posX, int32 posY, bool cutMode, AnimGraphCopyPasteData& copyPasteData, bool ignoreTopLevelConnections);
} // namespace CommandSystem
