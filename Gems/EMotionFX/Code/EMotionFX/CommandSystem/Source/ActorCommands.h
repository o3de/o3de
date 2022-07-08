/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
    size_t                                          m_oldMotionExtractionNodeIndex;
    size_t                                          m_oldRetargetRootNodeIndex;
    size_t                                          m_oldTrajectoryNodeIndex;
    AZStd::string                                   m_oldAttachmentNodes;
    AZStd::string                                   m_oldExcludedFromBoundsNodes;
    AZStd::string                                   m_oldName;
    AZStd::vector<EMotionFX::Actor::NodeMirrorInfo>  m_oldMirrorSetup;
    bool                                            m_oldDirtyFlag;

    void                                            SetIsAttachmentNode(EMotionFX::Actor* actor, bool isAttachmentNode);
    void                                            SetIsExcludedFromBoundsNode(EMotionFX::Actor* actor, bool excludedFromBounds);
    MCORE_DEFINECOMMAND_END

    // Set the collision meshes of the given actor.
    MCORE_DEFINECOMMAND_START(CommandActorSetCollisionMeshes, "Actor set collison meshes", true)
    AZStd::string   m_oldNodeList;
    bool            m_oldDirtyFlag;
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
        uint32          m_previouslyUsedId;
        AZStd::string   m_oldFileName;
        bool            m_oldDirtyFlag;
        bool            m_oldWorkspaceDirtyFlag;
    MCORE_DEFINECOMMAND_END


    // Scale actor data.
    MCORE_DEFINECOMMAND_START(CommandScaleActorData, "Scale actor data", true)
public:
    AZStd::string   m_oldUnitType;
    uint32          m_actorId;
    float           m_scaleFactor;
    bool            m_oldActorDirtyFlag;
    bool            m_useUnitType;
    MCORE_DEFINECOMMAND_END

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper functions
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    void COMMANDSYSTEM_API ClearScene(bool deleteActors = true, bool deleteActorInstances = true, MCore::CommandGroup* commandGroup = nullptr);
    void COMMANDSYSTEM_API PrepareCollisionMeshesNodesString(EMotionFX::Actor* actor, size_t lod, AZStd::string* outNodeNames);
    void COMMANDSYSTEM_API PrepareExcludedNodesString(EMotionFX::Actor* actor, AZStd::string* outNodeNames);
} // namespace CommandSystem
