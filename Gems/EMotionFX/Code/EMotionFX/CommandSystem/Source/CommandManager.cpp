/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/CommandSystem/Source/ActorCommands.h>
#include <EMotionFX/CommandSystem/Source/ActorInstanceCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphConditionCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphConnectionCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphGroupParameterCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphNodeCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphNodeGroupCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphParameterCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphTriggerActionCommands.h>
#include <EMotionFX/CommandSystem/Source/AttachmentCommands.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/ColliderCommands.h>
#include <EMotionFX/CommandSystem/Source/ImporterCommands.h>
#include <EMotionFX/CommandSystem/Source/MiscCommands.h>
#include <EMotionFX/CommandSystem/Source/MorphTargetCommands.h>
#include <EMotionFX/CommandSystem/Source/MotionCommands.h>
#include <EMotionFX/CommandSystem/Source/MotionEventCommands.h>
#include <EMotionFX/CommandSystem/Source/MotionSetCommands.h>
#include <EMotionFX/CommandSystem/Source/NodeGroupCommands.h>
#include <EMotionFX/CommandSystem/Source/RagdollCommands.h>
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include <EMotionFX/CommandSystem/Source/SimulatedObjectCommands.h>


namespace CommandSystem
{
    // the global command manager object
    CommandManager* gCommandManager = nullptr;

    CommandManager* GetCommandManager()
    {
        return gCommandManager;
    }

    CommandManager::CommandManager()
        : MCore::CommandManager()
    {
        RegisterCommand(new CommandImportActor());
        RegisterCommand(new CommandRemoveActor());
        RegisterCommand(new CommandScaleActorData());
        RegisterCommand(new CommandCreateActorInstance());
        RegisterCommand(new CommandRemoveActorInstance());
        RegisterCommand(new CommandAdjustMorphTarget());
        RegisterCommand(new CommandAdjustActorInstance());
        RegisterCommand(new CommandResetToBindPose());
        RegisterCommand(new CommandAddAttachment());
        RegisterCommand(new CommandRemoveAttachment());
        RegisterCommand(new CommandClearAttachments());
        RegisterCommand(new CommandAddDeformableAttachment());
        RegisterCommand(new CommandAdjustActor());
        RegisterCommand(new CommandActorSetCollisionMeshes());
        RegisterCommand(new CommandReInitRenderActors());
        RegisterCommand(new CommandUpdateRenderActors());
        RegisterCommand(aznew EMotionFX::CommandAddCollider());
        RegisterCommand(aznew EMotionFX::CommandAdjustCollider());
        RegisterCommand(aznew EMotionFX::CommandRemoveCollider());
        RegisterCommand(aznew EMotionFX::CommandAddRagdollJoint());
        RegisterCommand(aznew EMotionFX::CommandAdjustRagdollJoint());
        RegisterCommand(aznew EMotionFX::CommandRemoveRagdollJoint());

        // register simulated object related commands.
        RegisterCommand(aznew EMotionFX::CommandAddSimulatedObject());
        RegisterCommand(aznew EMotionFX::CommandAdjustSimulatedObject());
        RegisterCommand(aznew EMotionFX::CommandAddSimulatedJoints());
        RegisterCommand(aznew EMotionFX::CommandRemoveSimulatedObject());
        RegisterCommand(aznew EMotionFX::CommandRemoveSimulatedJoints());
        RegisterCommand(aznew EMotionFX::CommandAdjustSimulatedJoint());

        // register motion commands
        RegisterCommand(new CommandImportMotion());
        RegisterCommand(new CommandRemoveMotion());
        RegisterCommand(new CommandScaleMotionData());
        RegisterCommand(new CommandPlayMotion());
        RegisterCommand(new CommandAdjustMotionInstance());
        RegisterCommand(new CommandAdjustDefaultPlayBackInfo());
        RegisterCommand(new CommandStopMotionInstances());
        RegisterCommand(new CommandStopAllMotionInstances());
        RegisterCommand(aznew CommandAdjustMotion());

        // register motion event commands
        RegisterCommand(aznew CommandCreateMotionEvent());
        RegisterCommand(new CommandRemoveMotionEvent());
        RegisterCommand(aznew CommandAdjustMotionEvent());
        RegisterCommand(aznew CommandClearMotionEvents());
        RegisterCommand(aznew CommandCreateMotionEventTrack());
        RegisterCommand(new CommandRemoveMotionEventTrack());
        RegisterCommand(aznew CommandAdjustMotionEventTrack());

        // register motion set commands
        RegisterCommand(new CommandCreateMotionSet());
        RegisterCommand(new CommandRemoveMotionSet());
        RegisterCommand(new CommandAdjustMotionSet());
        RegisterCommand(new CommandMotionSetAddMotion());
        RegisterCommand(new CommandMotionSetRemoveMotion());
        RegisterCommand(new CommandMotionSetAdjustMotion());

        // register node group commands
        RegisterCommand(aznew CommandAdjustNodeGroup());
        RegisterCommand(new CommandAddNodeGroup());
        RegisterCommand(new CommandRemoveNodeGroup());

        // register selection commands
        RegisterCommand(new CommandSelect());
        RegisterCommand(new CommandUnselect());
        RegisterCommand(new CommandClearSelection());
        RegisterCommand(new CommandToggleLockSelection());

        // anim graph commands
        RegisterCommand(new CommandAnimGraphCreateNode());
        RegisterCommand(new CommandAnimGraphAdjustNode());
        RegisterCommand(new CommandAnimGraphCreateConnection());
        RegisterCommand(new CommandAnimGraphRemoveConnection());
        RegisterCommand(aznew CommandAnimGraphAdjustTransition());
        RegisterCommand(new CommandAnimGraphRemoveNode());
        RegisterCommand(new CommandAnimGraphCreateParameter());
        RegisterCommand(new CommandAnimGraphRemoveParameter());
        RegisterCommand(new CommandAnimGraphAdjustParameter());
        RegisterCommand(new CommandAnimGraphMoveParameter());
        RegisterCommand(new CommandCreateAnimGraph());
        RegisterCommand(new CommandRemoveAnimGraph());
        RegisterCommand(new CommandActivateAnimGraph());
        RegisterCommand(new CommandAnimGraphSetEntryState());
        RegisterCommand(aznew CommandAddTransitionCondition());
        RegisterCommand(aznew CommandRemoveTransitionCondition());
        RegisterCommand(aznew CommandAdjustTransitionCondition());
        RegisterCommand(new CommandAnimGraphAddNodeGroup());
        RegisterCommand(new CommandAnimGraphRemoveNodeGroup());
        RegisterCommand(aznew CommandAnimGraphAdjustNodeGroup());
        RegisterCommand(new CommandAnimGraphAddGroupParameter());
        RegisterCommand(new CommandAnimGraphRemoveGroupParameter());
        RegisterCommand(new CommandAnimGraphAdjustGroupParameter());
        RegisterCommand(aznew CommandAnimGraphAddTransitionAction());
        RegisterCommand(aznew CommandAnimGraphRemoveTransitionAction());
        RegisterCommand(aznew CommandAnimGraphAddStateAction());
        RegisterCommand(aznew CommandAnimGraphRemoveStateAction());

        // register misc commands
        RegisterCommand(new CommandRecorderClear());

        gCommandManager     = this;
        m_lockSelection      = false;
        m_workspaceDirtyFlag = false;
    }

    CommandManager::~CommandManager()
    {
    }

    void CommandManager::SetUserOpenedWorkspaceFlag(bool flag)
    {
        m_userOpenedWorkspaceFlag = flag;
    }
} // namespace CommandSystem
