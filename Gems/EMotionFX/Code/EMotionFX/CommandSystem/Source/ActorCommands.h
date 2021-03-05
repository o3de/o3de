/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include "CommandSystemConfig.h"
#include <MCore/Source/Command.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/ActorInstance.h>
#include "CommandManager.h"


namespace CommandSystem
{
    // Adjust the given actor.
    MCORE_DEFINECOMMAND_START(CommandAdjustActor, "Adjust actor", true)
    uint32                                          mOldMotionExtractionNodeIndex;
    uint32                                          mOldRetargetRootNodeIndex;
    uint32                                          mOldTrajectoryNodeIndex;
    AZStd::string                                   mOldAttachmentNodes;
    AZStd::string                                   mOldExcludedFromBoundsNodes;
    AZStd::string                                   mOldName;
    MCore::Array<EMotionFX::Actor::NodeMirrorInfo>  mOldMirrorSetup;
    bool                                            mOldDirtyFlag;

    void                                            SetIsAttachmentNode(EMotionFX::Actor* actor, bool isAttachmentNode);
    void                                            SetIsExcludedFromBoundsNode(EMotionFX::Actor* actor, bool excludedFromBounds);
    MCORE_DEFINECOMMAND_END

    // Set the collision meshes of the given actor.
    MCORE_DEFINECOMMAND_START(CommandActorSetCollisionMeshes, "Actor set collison meshes", true)
    AZStd::string   mOldNodeList;
    bool            mOldDirtyFlag;
    MCORE_DEFINECOMMAND_END

    // Reset actor instance to bind pose.
    MCORE_DEFINECOMMAND(CommandResetToBindPose, "ResetToBindPose", "Reset actor instance to bind pose", false)

    // this will be called in case all render actors need to get removed and reconstructed completely
    MCORE_DEFINECOMMAND(CommandReInitRenderActors, "ReInitRenderActors", "Reinit render actors", false)

    // Will be called in case an actor got removed and we have to remove a render actor or in case there is a new actor we need to create a render actor for, all current render actors won't get touched.
    MCORE_DEFINECOMMAND(CommandUpdateRenderActors, "UpdateRenderActors", "Update render actors", false)

    // Remove actor.
    MCORE_DEFINECOMMAND_START(CommandRemoveActor, "Remove actor", true)
    public:
        uint32          mPreviouslyUsedID;
        AZStd::string   mOldFileName;
        bool            mOldDirtyFlag;
        bool            mOldWorkspaceDirtyFlag;
    MCORE_DEFINECOMMAND_END


    // Scale actor data.
    MCORE_DEFINECOMMAND_START(CommandScaleActorData, "Scale actor data", true)
public:
    AZStd::string   mOldUnitType;
    uint32          mActorID;
    float           mScaleFactor;
    bool            mOldActorDirtyFlag;
    bool            mUseUnitType;
    MCORE_DEFINECOMMAND_END

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper functions
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    void COMMANDSYSTEM_API ClearScene(bool deleteActors = true, bool deleteActorInstances = true, MCore::CommandGroup* commandGroup = nullptr);
    void COMMANDSYSTEM_API PrepareCollisionMeshesNodesString(EMotionFX::Actor* actor, uint32 lod, AZStd::string* outNodeNames);
    void COMMANDSYSTEM_API PrepareExcludedNodesString(EMotionFX::Actor* actor, AZStd::string* outNodeNames);
} // namespace CommandSystem