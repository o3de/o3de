/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/CommandSystem/Source/AnimGraphCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphConditionCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphConnectionCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphNodeCommands.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/MotionCommands.h>
#include <EMotionFX/CommandSystem/Source/ParameterMixins.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphModel.h>
#include <Source/Editor/AnimGraphEditorBus.h>


namespace EMStudio
{
    bool AnimGraphModel::CommandDidLoadAnimGraphCallback::Execute(MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        CommandSystem::CommandLoadAnimGraph* commandLoadAnimGraph = static_cast<CommandSystem::CommandLoadAnimGraph*>(command);

        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(commandLoadAnimGraph->m_oldAnimGraphId);
        if (animGraph)
        {
            m_animGraphModel.Add(animGraph);

            EMotionFX::AnimGraphStateMachine* rootStateMachine = animGraph->GetRootStateMachine();
            m_animGraphModel.Focus(m_animGraphModel.FindFirstModelIndex(rootStateMachine));
        }
        
        return true;
    }
    
    bool AnimGraphModel::CommandDidLoadAnimGraphCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);

        // We are doing nothing in this case because the LoadAnimGraph command uses a RemoveAnimGraph
        // while undoing, that will be processed by the callbacks below
        return true;
    }

    bool AnimGraphModel::CommandDidCreateAnimGraphCallback::Execute(MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        CommandSystem::CommandCreateAnimGraph* commandCreateAnimGraph = static_cast<CommandSystem::CommandCreateAnimGraph*>(command);

        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(commandCreateAnimGraph->m_previouslyUsedId);
        m_animGraphModel.Add(animGraph);

        EMotionFX::AnimGraphStateMachine* rootStateMachine = animGraph->GetRootStateMachine();
        m_animGraphModel.Focus(m_animGraphModel.FindFirstModelIndex(rootStateMachine));

        return true;
    }

    bool AnimGraphModel::CommandDidCreateAnimGraphCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);

        // CreateAnimGraph command uses a RemoveAnimGraph while undoing, that will be processed by the callbacks below
        // We just need to find the first available anim graph and set the focus to it to prevent losing focus.
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetFirstAnimGraph();   
        if (animGraph)
        {
            EMotionFX::AnimGraphStateMachine* rootStateMachine = animGraph->GetRootStateMachine();
            m_animGraphModel.Focus(m_animGraphModel.FindFirstModelIndex(rootStateMachine));
        }

        return true;
    }

    bool AnimGraphModel::CommandWillRemoveAnimGraphCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        CommandSystem::CommandRemoveAnimGraph* commandRemoveAnimGraph = static_cast<CommandSystem::CommandRemoveAnimGraph*>(command);

        // Handle the loading case
        AZStd::unordered_set<EMotionFX::AnimGraph*> animGraphs;
        for (const AZStd::pair<AZStd::string, uint32>& filenameAndId : commandRemoveAnimGraph->m_oldFileNamesAndIds)
        {
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(filenameAndId.second);
            if (animGraph)
            {
                animGraphs.emplace(animGraph);
            }
        }

        // Handle the case where an AnimGraph was created
        AZ::Outcome<AZStd::string> animGraphID = commandLine.GetValueIfExists("animGraphID", command);
        if (!animGraphID.IsSuccess() || animGraphID.GetValue() != "SELECT_ALL")
        {
            AZStd::string outResult;
            EMotionFX::AnimGraph* animGraph = CommandSystem::CommandsGetAnimGraph(commandLine, command, outResult);
            if (animGraph)
            {
                animGraphs.emplace(animGraph);
            }
        }

        if (!animGraphs.empty())
        {
            QModelIndexList modelIndexes;
            const size_t rootModelItemCount = m_animGraphModel.m_rootModelItemData.size();
            for (size_t i = 0; i < rootModelItemCount; ++i)
            {
                ModelItemData* modelItemData = m_animGraphModel.m_rootModelItemData[i];
                // The root is always a node
                AZ_Assert(modelItemData->m_type == ModelItemType::NODE, "Expected root to be a node");
                if (animGraphs.find(modelItemData->m_object.m_node->GetAnimGraph()) != animGraphs.end())
                {
                    modelIndexes.push_back(m_animGraphModel.createIndex(static_cast<int>(i), 0, modelItemData));
                }
            }

            if (!modelIndexes.empty())
            {
                m_animGraphModel.RemoveIndices(modelIndexes);
            }
        }
        return true;
    }

    bool AnimGraphModel::CommandWillRemoveAnimGraphCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);
        return true;
    }

    bool AnimGraphModel::CommandDidRemoveAnimGraphCallback::Execute([[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        return true;
    }

    bool AnimGraphModel::CommandDidRemoveAnimGraphCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);

        // We are doing nothing in this case because the RemoveAnimGraph command uses a LoadAnimGraph/CreateAnimGraph
        // while undoing, that will be processed by the callbacks above
        return true;
    }

    bool AnimGraphModel::CommandDidActivateAnimGraphPostUndoCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);

        return true;
    }

    bool AnimGraphModel::CommandDidActivateAnimGraphPostUndoCallback::Undo(MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        CommandSystem::CommandActivateAnimGraph* commandActivateAnimGraph = static_cast<CommandSystem::CommandActivateAnimGraph*>(command);
        EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(commandActivateAnimGraph->m_actorInstanceId);

        if (actorInstance)
        {
            EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
            if (animGraphInstance)
            {
                m_animGraphModel.SetAnimGraphInstance(animGraphInstance->GetAnimGraph(), nullptr, animGraphInstance);
            }
        }

        return true;
    }

    bool AnimGraphModel::CommandDidActivateAnimGraphCallback::Execute(MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        CommandSystem::CommandActivateAnimGraph* commandActivateAnimGraph = static_cast<CommandSystem::CommandActivateAnimGraph*>(command);
        EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(commandActivateAnimGraph->m_actorInstanceId);
        
        if (actorInstance)
        {
            EMotionFX::AnimGraphInstance* currentAnimGraphInstance = actorInstance->GetAnimGraphInstance();
            EMotionFX::AnimGraph* currentAnimGraph = currentAnimGraphInstance->GetAnimGraph();
            EMotionFX::AnimGraph* oldAnimGraph = nullptr;
            oldAnimGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(commandActivateAnimGraph->m_oldAnimGraphUsed);

            if (currentAnimGraphInstance)
            {
                m_animGraphModel.SetAnimGraphInstance(currentAnimGraph, nullptr, currentAnimGraphInstance);

                // Focus on the new anim graph after activation if the old anim graph is different than the new one.
                if (oldAnimGraph != currentAnimGraph)
                {
                    EMotionFX::AnimGraphStateMachine* rootStateMachine = currentAnimGraph->GetRootStateMachine();
                    m_animGraphModel.Focus(m_animGraphModel.FindFirstModelIndex(rootStateMachine));
                }
            }
        }
        
        return true;
    }

    bool AnimGraphModel::CommandDidActivateAnimGraphCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        CommandSystem::CommandActivateAnimGraph* commandActivateAnimGraph = static_cast<CommandSystem::CommandActivateAnimGraph*>(command);
        EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(commandActivateAnimGraph->m_actorInstanceId);

        // TODO: do this better, we need to find the animgraphinstance that we are undoing after the undo finishes
        if (actorInstance)
        {
            EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
            if (animGraphInstance)
            {
                EMotionFX::AnimGraph* animGraph = nullptr;
                if (commandLine.CheckIfHasParameter("animGraphID"))
                {
                    const uint32 animGraphID = commandLine.GetValueAsInt("animGraphID", command);
                    if (animGraphID != MCORE_INVALIDINDEX32)
                    {
                        animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
                    }
                }
                else if (commandLine.CheckIfHasParameter("animGraphIndex"))
                {
                    const uint32 animGraphIndex = commandLine.GetValueAsInt("animGraphIndex", command);
                    if (animGraphIndex < EMotionFX::GetAnimGraphManager().GetNumAnimGraphs())
                    {
                        animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(animGraphIndex);
                    }
                }
                if (animGraph)
                {
                    m_animGraphModel.SetAnimGraphInstance(animGraph, animGraphInstance, nullptr);
                }
            }
        }

        return true;
    }

    bool AnimGraphModel::CommandDidCreateNodeCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZStd::string outResult;
        EMotionFX::AnimGraph* animGraph = CommandSystem::CommandsGetAnimGraph(commandLine, command, outResult);
        if (!animGraph)
        {
            return true;
        }

        CommandSystem::CommandAnimGraphCreateNode* commandCreateNode = static_cast<CommandSystem::CommandAnimGraphCreateNode*>(command);
        EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNodeById(commandCreateNode->m_nodeId);
        return m_animGraphModel.NodeAdded(node);
    }

    bool AnimGraphModel::CommandDidCreateNodeCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);

        // We are doing nothing in this case because the AnimGraphCreateNode command uses an AnimGraphRemoveNode
        // while undoing, that will be processed by the callbacks below
        return true;
    }

    bool AnimGraphModel::CommandWillRemoveNodeCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZStd::string outResult;
        EMotionFX::AnimGraph* animGraph = CommandSystem::CommandsGetAnimGraph(commandLine, command, outResult);
        if (!animGraph)
        {
            return true;
        }

        AZStd::string name;
        commandLine.GetValue("name", "", name);
        EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNodeByName(name.c_str());
        if (!node)
        {
            return true;
        }

        m_animGraphModel.RemoveIndices(m_animGraphModel.FindModelIndexes(node));
        
        return true;
    }

    bool AnimGraphModel::CommandWillRemoveNodeCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);
        return true;
    }

    bool AnimGraphModel::CommandDidRemoveNodeCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);
        
        return true;
    }

    bool AnimGraphModel::CommandDidRemoveNodeCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);

        // We are doing nothing in this case because the AnimGraphRemoveNode command uses an AnimGraphCreateNode
        // while undoing, that will be processed by the callbacks above
        return true;
    }

    bool AnimGraphModel::CommandRemoveActorInstanceCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);

        return true;
    }

    bool AnimGraphModel::CommandRemoveActorInstanceCallback::Execute([[maybe_unused]] MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        const uint32 actorInstanceID = commandLine.GetValueAsInt("actorInstanceID", MCORE_INVALIDINDEX32);

        EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);
        if (!actorInstance)
        {
            // The command will check the actorinstance validity
            return true;
        }
        EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
        if (animGraphInstance)
        {
            m_animGraphModel.SetAnimGraphInstance(animGraphInstance->GetAnimGraph(), animGraphInstance, nullptr);
        }

        return true;
    }

    bool AnimGraphModel::CommandDidAdjustNodeCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZStd::string outResult;
        EMotionFX::AnimGraph* animGraph = CommandSystem::CommandsGetAnimGraph(commandLine, command, outResult);
        if (!animGraph)
        {
            return true;
        }

        CommandSystem::CommandAnimGraphAdjustNode* commandAdjustNode = static_cast<CommandSystem::CommandAnimGraphAdjustNode*>(command);
        EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNodeById(commandAdjustNode->GetNodeId());

        m_animGraphModel.Edited(node);

        return true;
    }

    bool AnimGraphModel::CommandDidAdjustNodeCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZStd::string outResult;
        EMotionFX::AnimGraph* animGraph = CommandSystem::CommandsGetAnimGraph(commandLine, command, outResult);
        if (!animGraph)
        {
            return true;
        }

        CommandSystem::CommandAnimGraphAdjustNode* commandAdjustNode = static_cast<CommandSystem::CommandAnimGraphAdjustNode*>(command);
        EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNodeById(commandAdjustNode->GetNodeId());

        m_animGraphModel.Edited(node);

        return true;
    }

    bool AnimGraphModel::CommandDidCreateConnectionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZStd::string outResult;
        EMotionFX::AnimGraph* animGraph = CommandSystem::CommandsGetAnimGraph(commandLine, command, outResult);
        if (!animGraph)
        {
            return true;
        }

        CommandSystem::CommandAnimGraphCreateConnection* commandCreateConnection = static_cast<CommandSystem::CommandAnimGraphCreateConnection*>(command);
        EMotionFX::AnimGraphNode* targetNode = animGraph->RecursiveFindNodeById(commandCreateConnection->GetTargetNodeId());
        if (targetNode)
        {
            if (azrtti_typeid(targetNode->GetParentNode()) != azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
            {
                // In this case is a BlendTreeConnection, we dont keep items in the model for it. We just
                // need to mark the target node as changed
                EMotionFX::BlendTreeConnection* connection = targetNode->FindConnection(aznumeric_caster(commandCreateConnection->GetTargetPort()));
                return m_animGraphModel.ConnectionAdded(targetNode, connection);
            }
            else
            {
                EMotionFX::AnimGraphStateMachine* parentStateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(targetNode->GetParentNode());
                EMotionFX::AnimGraphStateTransition* transition = parentStateMachine->FindTransitionById(commandCreateConnection->GetConnectionId());
                if (transition)
                {
                    m_animGraphModel.TransitionAdded(transition);
                }
            }
        }

        return true;
    }

    bool AnimGraphModel::CommandDidCreateConnectionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);

        // We are doing nothing in this case because the AnimGraphCreateConnection command uses an AnimGraphRemoveConnection
        // while undoing, that will be processed by the callbacks below
        return true;
    }

    bool AnimGraphModel::CommandWillRemoveConnectionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZStd::string outResult;
        EMotionFX::AnimGraph* animGraph = CommandSystem::CommandsGetAnimGraph(commandLine, command, outResult);
        if (!animGraph)
        {
            return true;
        }

        AZStd::string targetNodeName;
        commandLine.GetValue("targetNode", "", targetNodeName);

        EMotionFX::AnimGraphNode* targetNode = animGraph->RecursiveFindNodeByName(targetNodeName.c_str());
        if (!targetNode)
        {
            // command will fail
            return true;
        }

        const EMotionFX::AnimGraphConnectionId connectionId = EMotionFX::AnimGraphConnectionId::CreateFromString(commandLine.GetValue("id", command));
        if (!connectionId.IsValid())
        {
            // This is the case when it was a BlendTreeConnection
            AZStd::string sourceNodeName;
            commandLine.GetValue("sourceNode", "", sourceNodeName);

            EMotionFX::AnimGraphNode* sourceNode = animGraph->RecursiveFindNodeByName(sourceNodeName.c_str());

            const int sourcePort = commandLine.GetValueAsInt("sourcePort", 0);
            const int targetPort = commandLine.GetValueAsInt("targetPort", 0);

            EMotionFX::BlendTreeConnection* blendTreeConnection = targetNode->FindConnection(sourceNode, static_cast<uint16>(sourcePort), static_cast<uint16>(targetPort));
            if (blendTreeConnection)
            {
                m_animGraphModel.RemoveIndices(m_animGraphModel.FindModelIndexes(blendTreeConnection));
            }
        }
        else
        {
            EMotionFX::AnimGraphNode* parentNode = targetNode->GetParentNode();
            
            if (azrtti_typeid(parentNode) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
            {
                EMotionFX::AnimGraphStateMachine* parentStateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(parentNode);
                EMotionFX::AnimGraphStateTransition* transition = parentStateMachine->FindTransitionById(connectionId);
                if (transition)
                {
                    m_animGraphModel.RemoveIndices(m_animGraphModel.FindModelIndexes(transition));
                }
            }
            else
            {
                EMotionFX::BlendTreeConnection* blendTreeConnection = targetNode->FindConnectionById(connectionId);
                if (blendTreeConnection)
                {
                    m_animGraphModel.RemoveIndices(m_animGraphModel.FindModelIndexes(blendTreeConnection));
                }
            }
            
        }

        return true;
    }

    bool AnimGraphModel::CommandWillRemoveConnectionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);
        return true;
    }

    bool AnimGraphModel::CommandDidRemoveConnectionCallback::Execute([[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        return true;
    }

    bool AnimGraphModel::CommandDidRemoveConnectionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);

        // We are doing nothing in this case because the AnimGraphRemoveConnection command uses an AnimGraphCreateConnection
        // while undoing, that will be processed by the callbacks above
        return true;
    }

    bool AnimGraphModel::CommandDidAdjustConnectionCallback::Execute(MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        AZStd::string outResult;
        CommandSystem::CommandAnimGraphAdjustTransition* adjustTransitionCommand = static_cast<CommandSystem::CommandAnimGraphAdjustTransition*>(command);
        EMotionFX::AnimGraphStateTransition* transition = adjustTransitionCommand->GetTransition(outResult);
        if (!transition)
        {
            return false;
        }

        EMotionFX::AnimGraphNode* targetNode = transition->GetTargetNode();
        if (!targetNode)
        {
            return false;
        }

        EMotionFX::AnimGraphNode* parentTargetNode = targetNode->GetParentNode();
        if (azrtti_typeid(parentTargetNode) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
        {
            m_animGraphModel.Edited(parentTargetNode);
            m_animGraphModel.Edited(transition);
        }
        else
        {
            // if the parent is not a state machine, just assume that this is a regular connection
            m_animGraphModel.Edited(parentTargetNode);
        }

        return true;
    }

    bool AnimGraphModel::CommandDidAdjustConnectionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);

        // We are doing nothing in this case because the CommandAnimGraphAdjustTransition command calls itself
        // for undo, that will be processed by the callbacks above.
        return true;
    }

    bool AnimGraphModel::CommandDidConditionChangeCallbackHelper(AnimGraphModel& animGraphModel, MCore::Command* command)
    {
        if (EMotionFX::ParameterMixinTransitionId* transitionIdMixin = azdynamic_cast<EMotionFX::ParameterMixinTransitionId*>(command))
        {
            AZStd::string resultString;
            EMotionFX::AnimGraphStateTransition* transition = transitionIdMixin->GetTransition(resultString);
            if (transition)
            {
                static const QVector<int> transitionConditionRole = { AnimGraphModel::ROLE_TRANSITION_CONDITIONS };
                animGraphModel.Edited(transition, transitionConditionRole);
            }
        }

        return true;
    }

    bool AnimGraphModel::CommandDidAddRemoveConditionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(commandLine);
        return AnimGraphModel::CommandDidConditionChangeCallbackHelper(m_animGraphModel, command);
    }

    bool AnimGraphModel::CommandDidAddRemoveConditionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);

        // We are doing nothing in this case because the AnimGraphAddCondition command uses an AnimGraphRemoveCondition
        // while undoing, that will be processed by the callbacks below.
        // The same will be applied for AnimGraphRemoveCondition.
        return true;
    }

    bool AnimGraphModel::CommandDidAdjustConditionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(commandLine);
        return AnimGraphModel::CommandDidConditionChangeCallbackHelper(m_animGraphModel, command);
    }

    bool AnimGraphModel::CommandDidAdjustConditionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(commandLine);
        return AnimGraphModel::CommandDidConditionChangeCallbackHelper(m_animGraphModel, command);
    }

    bool AnimGraphModel::CommandDidEditActionCallback::Execute(MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        static const QVector<int> transitionActionRole = { AnimGraphModel::ROLE_TRIGGER_ACTIONS };

        EMotionFX::ParameterMixinAnimGraphId* animGraphIdMixin = azdynamic_cast<EMotionFX::ParameterMixinAnimGraphId*>(command);
        if (animGraphIdMixin)
        {
            AZStd::string outResult;
            const EMotionFX::AnimGraph* animGraph = animGraphIdMixin->GetAnimGraph(outResult);
            if (animGraph)
            {
                if (EMotionFX::ParameterMixinTransitionId* transitionIdMixin = azdynamic_cast<EMotionFX::ParameterMixinTransitionId*>(command))
                {
                    EMotionFX::AnimGraphStateTransition* transition = transitionIdMixin->GetTransition(animGraph, outResult);
                    if (transition)
                    {
                        m_animGraphModel.Edited(transition, transitionActionRole);
                    }
                }
                else if (EMotionFX::ParameterMixinAnimGraphNodeId* nodeIdMixin = azdynamic_cast<EMotionFX::ParameterMixinAnimGraphNodeId*>(command))
                {
                    EMotionFX::AnimGraphNode* node = nodeIdMixin->GetNode(animGraph, command, outResult);
                    if (node)
                    {
                        m_animGraphModel.Edited(node, transitionActionRole);
                    }
                }
            }
        }

        return true;
    }

    bool AnimGraphModel::CommandDidEditActionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);
        return true;
    }

    bool AnimGraphModel::CommandDidSetEntryStateCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZStd::string outResult;
        EMotionFX::AnimGraph* animGraph = CommandSystem::CommandsGetAnimGraph(commandLine, command, outResult);
        if (animGraph)
        {
            // find the entry anim graph node
            AZStd::string entryNodeName;
            commandLine.GetValue("entryNodeName", "", entryNodeName);
            EMotionFX::AnimGraphNode* entryNode = animGraph->RecursiveFindNodeByName(entryNodeName.c_str());
            if (entryNode)
            {
                static const QVector<int> entryStateRole = { AnimGraphModel::ROLE_NODE_ENTRY_STATE };
                m_animGraphModel.Edited(entryNode, entryStateRole);
            }
        }
        
        return true;
    }

    bool AnimGraphModel::CommandDidSetEntryStateCallback::Undo(MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        CommandSystem::CommandAnimGraphSetEntryState* commandSetEntryState = static_cast<CommandSystem::CommandAnimGraphSetEntryState*>(command);
        
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(commandSetEntryState->m_animGraphId);
        if (animGraph)
        {
            EMotionFX::AnimGraphNode* entryNode = animGraph->RecursiveFindNodeById(commandSetEntryState->m_oldEntryStateNodeId);
            if (entryNode)
            {
                static const QVector<int> entryStateRole = { AnimGraphModel::ROLE_NODE_ENTRY_STATE };
                m_animGraphModel.Edited(entryNode, entryStateRole);
            }
        }

        return true;
    }

    bool AnimGraphModel::OnParameterChangedCallback(AnimGraphModel& animGraphModel, MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZStd::string outResult;
        EMotionFX::AnimGraph* animGraph = CommandSystem::CommandsGetAnimGraph(commandLine, command, outResult);
        if (animGraph && commandLine.GetValueAsBool("updateUI", true))
        {
            return animGraphModel.ParameterEdited(animGraph);
        }
        return true;
    }

    bool AnimGraphModel::CommandDidCreateParameterCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        return OnParameterChangedCallback(m_animGraphModel, command, commandLine);
    }

    bool AnimGraphModel::CommandDidCreateParameterCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);

        return true;
    }

    bool AnimGraphModel::CommandDidAdjustParameterCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        return OnParameterChangedCallback(m_animGraphModel, command, commandLine);
    }

    bool AnimGraphModel::CommandDidAdjustParameterCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        return OnParameterChangedCallback(m_animGraphModel, command, commandLine);
    }

    bool AnimGraphModel::CommandDidRemoveParameterCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        return OnParameterChangedCallback(m_animGraphModel, command, commandLine);
    }

    bool AnimGraphModel::CommandDidRemoveParameterCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);

        return true;
    }

    bool AnimGraphModel::CommandDidMoveParameterCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        return OnParameterChangedCallback(m_animGraphModel, command, commandLine);
    }

    bool AnimGraphModel::CommandDidMoveParameterCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        return OnParameterChangedCallback(m_animGraphModel, command, commandLine);
    }

    bool AnimGraphModel::CommandDidAddGroupParameterCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        return OnParameterChangedCallback(m_animGraphModel, command, commandLine);
    }

    bool AnimGraphModel::CommandDidAddGroupParameterCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);

        return true;
    }

    bool AnimGraphModel::CommandDidRemoveGroupParameterCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        return OnParameterChangedCallback(m_animGraphModel, command, commandLine);
    }

    bool AnimGraphModel::CommandDidRemoveGroupParameterCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);

        return true;
    }

    bool AnimGraphModel::CommandDidAdjustGroupParameterCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        return OnParameterChangedCallback(m_animGraphModel, command, commandLine);
    }

    bool AnimGraphModel::CommandDidAdjustGroupParameterCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);

        return true;
    }

    bool AnimGraphModel::CommandDidCreateMotionSetCallback::Execute([[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        EMotionFX::AnimGraphEditorRequestBus::Broadcast(&EMotionFX::AnimGraphEditorRequests::UpdateMotionSetComboBox);
        return true;
    }

    bool AnimGraphModel::CommandDidCreateMotionSetCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);

        return true;
    }

    bool AnimGraphModel::CommandDidRemoveMotionSetCallback::Execute([[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        EMotionFX::AnimGraphEditorRequestBus::Broadcast(&EMotionFX::AnimGraphEditorRequests::UpdateMotionSetComboBox);
        return true;
    }

    bool AnimGraphModel::CommandDidRemoveMotionSetCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);

        return true;
    }

    bool AnimGraphModel::CommandDidAdjustMotionSetCallback::Execute([[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        EMotionFX::AnimGraphEditorRequestBus::Broadcast(&EMotionFX::AnimGraphEditorRequests::UpdateMotionSetComboBox);
        return true;
    }

    bool AnimGraphModel::CommandDidAdjustMotionSetCallback::Undo([[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        EMotionFX::AnimGraphEditorRequestBus::Broadcast(&EMotionFX::AnimGraphEditorRequests::UpdateMotionSetComboBox);
        return true;
    }

    bool AnimGraphModel::CommandDidMotionSetAddMotionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);

        return m_animGraphModel.MotionEdited();
    }

    bool AnimGraphModel::CommandDidMotionSetAddMotionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);

        return true;
    }

    bool AnimGraphModel::CommandDidMotionSetRemoveMotionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);

        return m_animGraphModel.MotionEdited();
    }

    bool AnimGraphModel::CommandDidMotionSetRemoveMotionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);

        return true;
    }

    bool AnimGraphModel::CommandDidMotionSetAdjustMotionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);

        return m_animGraphModel.MotionEdited();
    }

    bool AnimGraphModel::CommandDidMotionSetAdjustMotionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);

        return m_animGraphModel.MotionEdited();
    }

    bool AnimGraphModel::CommandDidLoadMotionSetCallback::Execute([[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        EMotionFX::AnimGraphEditorRequestBus::Broadcast(&EMotionFX::AnimGraphEditorRequests::UpdateMotionSetComboBox);
        return true;
    }

    bool AnimGraphModel::CommandDidLoadMotionSetCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);

        return true;
    }

    bool AnimGraphModel::CommandDidSaveMotionSetCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);

        return true;
    }

    bool AnimGraphModel::CommandDidSaveMotionSetCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);

        return true;
    }

    bool AnimGraphModel::CommandDidPlayMotionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(commandLine);
        CommandSystem::CommandPlayMotion* commandPlayMotion = static_cast<CommandSystem::CommandPlayMotion*>(command);

        for (const CommandSystem::CommandPlayMotion::UndoObject& undoObject : commandPlayMotion->m_oldData)
        {
            EMotionFX::ActorInstance* actorInstance = undoObject.m_actorInstance;

            if (!actorInstance || !EMotionFX::GetActorManager().CheckIfIsActorInstanceRegistered(actorInstance))
            {
                continue;
            }

            EMotionFX::AnimGraph* oldAnimGraph = undoObject.m_animGraph;
            EMotionFX::AnimGraphInstance* oldAnimGraphInstance = undoObject.m_animGraphInstance;
            if (oldAnimGraph && oldAnimGraphInstance)
            {
                m_animGraphModel.SetAnimGraphInstance(oldAnimGraph, oldAnimGraphInstance, nullptr);
            }
        }

        return true;
    }

    bool AnimGraphModel::CommandDidPlayMotionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);

        return true;
    }
} // namespace EMStudio
