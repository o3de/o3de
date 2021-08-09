/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __EMFX_ACTORINSTANCECOMMANDS_H
#define __EMFX_ACTORINSTANCECOMMANDS_H

// include the required headers
#include "CommandSystemConfig.h"
#include <MCore/Source/Command.h>
#include "CommandManager.h"


namespace CommandSystem
{
    // create a new actor instance
    MCORE_DEFINECOMMAND_START(CommandCreateActorInstance, "Create actor instance", true)
public:
    uint32  mPreviouslyUsedID;
    bool    mOldWorkspaceDirtyFlag;
    MCORE_DEFINECOMMAND_END


    // adjust a given actor instance
        MCORE_DEFINECOMMAND_START(CommandAdjustActorInstance, "Adjust actor instance", true)
public:
    AZ::Vector3         mOldPosition;
    AZ::Quaternion      mOldRotation;
    AZ::Vector3         mOldScale;
    size_t              mOldLODLevel;
    bool                mOldIsVisible;
    bool                mOldDoRender;
    bool                mOldWorkspaceDirtyFlag;
    MCORE_DEFINECOMMAND_END


    // remove an actor instance
        MCORE_DEFINECOMMAND_START(CommandRemoveActorInstance, "Remove actor instance", true)
    uint32              mOldActorID;
    AZ::Vector3         mOldPosition;
    AZ::Quaternion      mOldRotation;
    AZ::Vector3         mOldScale;
    size_t              mOldLODLevel;
    bool                mOldIsVisible;
    bool                mOldDoRender;
    bool                mOldWorkspaceDirtyFlag;
    MCORE_DEFINECOMMAND_END

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper functions
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    void COMMANDSYSTEM_API CloneActorInstance(EMotionFX::ActorInstance* actorInstance, MCore::CommandGroup* commandGroup = nullptr);
    void COMMANDSYSTEM_API CloneSelectedActorInstances();
    void COMMANDSYSTEM_API ResetToBindPose();
    void COMMANDSYSTEM_API RemoveSelectedActorInstances();
    void COMMANDSYSTEM_API MakeSelectedActorInstancesInvisible();
    void COMMANDSYSTEM_API MakeSelectedActorInstancesVisible();
    void COMMANDSYSTEM_API UnselectSelectedActorInstances();
} // namespace CommandSystem


#endif
