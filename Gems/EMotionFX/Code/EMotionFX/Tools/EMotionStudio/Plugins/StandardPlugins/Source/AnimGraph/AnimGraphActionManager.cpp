/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MCore/Source/LogManager.h>
#include <MCore/Source/StringConversions.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphNodeCommands.h>
#include <EMotionFX/CommandSystem/Source/MotionCommands.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/AnimGraphReferenceNode.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/MotionSet.h>
#include <Editor/AnimGraphEditorBus.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphActionManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphModel.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionWindow/MotionWindowPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionWindow/MotionListWindow.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionSetsWindow/MotionSetsWindowPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TimeViewPlugin.h>
#include <AzQtComponents/Components/Widgets/ColorPicker.h>

namespace EMStudio
{
    AnimGraphActionFilter AnimGraphActionFilter::CreateDisallowAll()
    {
        AnimGraphActionFilter result;

        result.m_createNodes = false;
        result.m_editNodes = false;
        result.m_createConnections = false;
        result.m_editConnections = false;
        result.m_copyAndPaste = false;
        result.m_setEntryNode = false;
        result.m_activateState = false;
        result.m_delete = false;
        result.m_editNodeGroups = false;

        return result;
    }


    AnimGraphActionManager::AnimGraphActionManager(AnimGraphPlugin* plugin)
        : m_plugin(plugin)
        , m_pasteOperation(PasteOperation::None)
    {
    }

    AnimGraphActionManager::~AnimGraphActionManager()
    {
    }

    bool AnimGraphActionManager::GetIsReadyForPaste() const
    {
        return m_pasteOperation != PasteOperation::None;
    }

    void AnimGraphActionManager::ShowNodeColorPicker(EMotionFX::AnimGraphNode* animGraphNode)
    {
        const AZ::Color color = animGraphNode->GetVisualizeColor();
        const AZ::Color originalColor = color;

        AzQtComponents::ColorPicker dialog(AzQtComponents::ColorPicker::Configuration::RGBA);
        dialog.setCurrentColor(originalColor);
        dialog.setSelectedColor(originalColor);
        auto changeNodeColor = [animGraphNode](const AZ::Color& color)
        {
            animGraphNode->SetVisualizeColor(color);
        };

        // Show live the color the user is choosing
        connect(&dialog, &AzQtComponents::ColorPicker::currentColorChanged, changeNodeColor);
        if (dialog.exec() != QDialog::Accepted)
        {
            changeNodeColor(originalColor);
        }
    }

    void AnimGraphActionManager::Copy()
    {
        m_pasteItems.clear();
        const QModelIndexList selectedIndexes = m_plugin->GetAnimGraphModel().GetSelectionModel().selectedRows();
        for (const QModelIndex& selectedIndex : selectedIndexes)
        {
            m_pasteItems.emplace_back(selectedIndex);
        }
        if (!m_pasteItems.empty())
        {
            SetPasteOperation(PasteOperation::Copy);
        }
    }

    void AnimGraphActionManager::Cut()
    {
        m_pasteItems.clear();
        const QModelIndexList selectedIndexes = m_plugin->GetAnimGraphModel().GetSelectionModel().selectedRows();

        for (const QModelIndex& selectedIndex : selectedIndexes)
        {
            if (selectedIndex.isValid())
            {
                const AnimGraphModel::ModelItemType itemType = selectedIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>();
                if (itemType == AnimGraphModel::ModelItemType::NODE)
                {
                    const EMotionFX::AnimGraphNode* node = selectedIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
                    if (!node->GetIsDeletable())
                    {
                        continue;
                    }
                }
            }
            m_pasteItems.emplace_back(selectedIndex);
        }
        if (!m_pasteItems.empty())
        {
            SetPasteOperation(PasteOperation::Cut);
        }
    }

    void AnimGraphActionManager::Paste(const QModelIndex& parentIndex, const QPoint& pos)
    {
        if (!GetIsReadyForPaste() || !parentIndex.isValid())
        {
            return;
        }

        AZStd::vector<EMotionFX::AnimGraphNode*> nodesToCopy;
        for (const QPersistentModelIndex& modelIndex : m_pasteItems)
        {
            // User could have deleted nodes in between the copy/cut and the paste operation, check that they are valid.
            if (modelIndex.isValid())
            {
                const AnimGraphModel::ModelItemType itemType = modelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>();
                if (itemType == AnimGraphModel::ModelItemType::NODE)
                {
                    EMotionFX::AnimGraphNode* node = modelIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
                    nodesToCopy.emplace_back(node);
                }
            }
        }

        EMotionFX::AnimGraphNode* targetParentNode = parentIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();

        if (!nodesToCopy.empty())
        {
            MCore::CommandGroup commandGroup(AZStd::string(m_pasteOperation == PasteOperation::Copy ? "Copy" : "Cut") + " and paste nodes");
            
            CommandSystem::AnimGraphCopyPasteData copyPasteData;
            CommandSystem::ConstructCopyAnimGraphNodesCommandGroup(&commandGroup, targetParentNode, nodesToCopy, pos.x(), pos.y(), m_pasteOperation == PasteOperation::Cut, copyPasteData, false);
            
            AZStd::string result;
            if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        }

        m_pasteItems.clear();
        SetPasteOperation(PasteOperation::None);
    }
    
    void AnimGraphActionManager::SetEntryState()
    {
        QModelIndexList selectedIndexes = m_plugin->GetAnimGraphModel().GetSelectionModel().selectedRows();

        if (!selectedIndexes.empty())
        {
            const QModelIndex firstSelectedNode = selectedIndexes.front();

            const EMotionFX::AnimGraphNode* node = firstSelectedNode.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();

            const AZStd::string command = AZStd::string::format("AnimGraphSetEntryState -animGraphID %i -entryNodeName \"%s\"",
                node->GetAnimGraph()->GetID(),
                node->GetName());
            AZStd::string commandResult;
            if (!GetCommandManager()->ExecuteCommand(command, commandResult))
            {
                MCore::LogError(commandResult.c_str());
            }
        }
    }

    void AnimGraphActionManager::PreviewMotionSelected(const char* motionId)
    {
        GetMainWindow()->DisableUndoRedo();
        CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        MCore::CommandGroup commandGroup("Preview Motion");
        AZStd::string commandString, commandParameters, result;

        EMotionFX::MotionSet::MotionEntry* motionEntry = MotionSetsWindowPlugin::FindBestMatchMotionEntryById(motionId);
        
        if (!motionEntry || !motionEntry->GetMotion())
        {
            return;
        }

        // If found motion entry, add select and play motion command strings to command group.
        EMotionFX::Motion* motion = motionEntry->GetMotion();
        EMotionFX::PlayBackInfo* defaultPlayBackInfo = motion->GetDefaultPlayBackInfo();
        defaultPlayBackInfo->m_blendInTime = 0.0f;
        defaultPlayBackInfo->m_blendOutTime = 0.0f;
        commandParameters = CommandSystem::CommandPlayMotion::PlayBackInfoToCommandParameters(defaultPlayBackInfo);

        const size_t motionIndex = EMotionFX::GetMotionManager().FindMotionIndexByName(motion->GetName());
        commandString = AZStd::string::format("Select -motionIndex %zu", motionIndex);
        commandGroup.AddCommandString(commandString);

        commandString = AZStd::string::format("PlayMotion -filename \"%s\" %s", motion->GetFileName(), commandParameters.c_str());
        commandGroup.AddCommandString(commandString);
        selectionList.ClearMotionSelection();

        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        // Update motion list window to select motion.
        EMStudioPlugin* motionBasePlugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionWindowPlugin::CLASS_ID);
        MotionWindowPlugin* motionWindowPlugin = static_cast<MotionWindowPlugin*>(motionBasePlugin);
        if (motionWindowPlugin)
        {
            motionWindowPlugin->ReInit();
        }

        // Update time view plugin with new motion related data.
        EMStudioPlugin* timeViewBasePlugin = EMStudio::GetPluginManager()->FindActivePlugin(TimeViewPlugin::CLASS_ID);
        TimeViewPlugin* timeViewPlugin = static_cast<TimeViewPlugin*>(timeViewBasePlugin);
        if (timeViewPlugin)
        {
            timeViewPlugin->SetMode(TimeViewMode::Motion);
        }
    }

    void AnimGraphActionManager::AddWildCardTransition()
    {
        QModelIndexList selectedIndexes = m_plugin->GetAnimGraphModel().GetSelectionModel().selectedRows();
        if (selectedIndexes.empty())
        {
            return;
        }

        MCore::CommandGroup commandGroup;

        for (QModelIndex selectedModelIndex : selectedIndexes)
        {
            if (selectedModelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() != AnimGraphModel::ModelItemType::NODE)
            {
                continue;
            }

            EMotionFX::AnimGraphNode* node = selectedModelIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
            
            // Only handle states, skip blend tree nodes.
            EMotionFX::AnimGraphNode* parentNode = node->GetParentNode();
            if (!parentNode || azrtti_typeid(parentNode) != azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
            {
                continue;
            }

            EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(parentNode);

            const uint32 numWildcardTransitions = stateMachine->CalcNumWildcardTransitions(node);
            const bool isEven = (numWildcardTransitions % 2 == 0);
            const uint32 evenTransitions = numWildcardTransitions / 2;

            // space the wildcard transitions in case we're dealing with multiple ones
            uint32 endOffsetX = 0;
            uint32 endOffsetY = 0;

            // if there is no wildcard transition yet, we can skip directly
            if (numWildcardTransitions > 0)
            {
                if (isEven)
                {
                    endOffsetY = evenTransitions * 15;
                }
                else
                {
                    endOffsetX = (evenTransitions + 1) * 15;
                }
            }

            const AZStd::string commandString = AZStd::string::format("AnimGraphCreateConnection -animGraphID %i -sourceNode \"\" -targetNode \"%s\" -sourcePort 0 -targetPort 0 -startOffsetX 0 -startOffsetY 0 -endOffsetX %i -endOffsetY %i -transitionType \"%s\"",
                node->GetAnimGraph()->GetID(),
                node->GetName(),
                endOffsetX,
                endOffsetY,
                azrtti_typeid<EMotionFX::AnimGraphStateTransition>().ToString<AZStd::string>().c_str());

            commandGroup.AddCommandString(commandString);
        }

        if (commandGroup.GetNumCommands() > 0)
        {
            commandGroup.SetGroupName(AZStd::string::format("Add wildcard transition%s", commandGroup.GetNumCommands() > 1 ? "s" : ""));

            AZStd::string commandResult;
            AZ_Error("EMotionFX", GetCommandManager()->ExecuteCommandGroup(commandGroup, commandResult), commandResult.c_str());
        }
    }

    void AnimGraphActionManager::SetSelectedEnabled(bool enabled)
    {
        const QModelIndexList selectedIndexes = m_plugin->GetAnimGraphModel().GetSelectionModel().selectedRows();
        if (selectedIndexes.empty())
        {
            return;
        }

        MCore::CommandGroup commandGroup(enabled ? "Enable nodes" : "Disable nodes");

        for (const QModelIndex& modelIndex : selectedIndexes)
        {
            if (modelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() == AnimGraphModel::ModelItemType::NODE)
            {
                const EMotionFX::AnimGraphNode* node = modelIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();

                if (node->GetSupportsDisable())
                {
                    commandGroup.AddCommandString(AZStd::string::format("AnimGraphAdjustNode -animGraphID %i -name \"%s\" -enabled %s",
                        node->GetAnimGraph()->GetID(),
                        node->GetName(),
                        AZStd::to_string(enabled).c_str()));
                }
            }
        }

        if (commandGroup.GetNumCommands() > 0)
        {
            AZStd::string commandResult;
            if (!GetCommandManager()->ExecuteCommandGroup(commandGroup, commandResult))
            {
                MCore::LogError(commandResult.c_str());
            }
        }
    }

    void AnimGraphActionManager::EnableSelected()
    {
        SetSelectedEnabled(true);
    }

    void AnimGraphActionManager::DisableSelected()
    {
        SetSelectedEnabled(false);
    }

    void AnimGraphActionManager::MakeVirtualFinalNode()
    {
        const QModelIndexList selectedIndexes = m_plugin->GetAnimGraphModel().GetSelectionModel().selectedRows();

        if (selectedIndexes.size() == 1)
        {
            const QModelIndex firstSelectedNode = selectedIndexes.front();
            EMotionFX::AnimGraphNode* node = firstSelectedNode.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
            if (azrtti_typeid(node->GetParentNode()) == azrtti_typeid<EMotionFX::BlendTree>())
            {
                EMotionFX::BlendTree* blendTree = static_cast<EMotionFX::BlendTree*>(node->GetParentNode());
                if (node == blendTree->GetFinalNode())
                {
                    blendTree->SetVirtualFinalNode(nullptr);
                }
                else
                {
                    blendTree->SetVirtualFinalNode(node);
                }

                // update the virtual final node
                m_plugin->GetGraphWidget()->SetVirtualFinalNode(firstSelectedNode);
            }
        }
    }

    void AnimGraphActionManager::RestoreVirtualFinalNode()
    {
        const QModelIndexList selectedIndexes = m_plugin->GetAnimGraphModel().GetSelectionModel().selectedRows();

        if (selectedIndexes.size() == 1)
        {
            const QModelIndex firstSelectedNode = selectedIndexes.front();
            EMotionFX::AnimGraphNode* node = firstSelectedNode.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
            if (azrtti_typeid(node->GetParentNode()) == azrtti_typeid<EMotionFX::BlendTree>())
            {
                // set its new virtual final node
                EMotionFX::BlendTree* blendTree = static_cast<EMotionFX::BlendTree*>(node->GetParentNode());
                blendTree->SetVirtualFinalNode(nullptr);

                // update the virtual final node
                const QModelIndexList finalNodes = m_plugin->GetAnimGraphModel().FindModelIndexes(blendTree->GetFinalNode());
                m_plugin->GetGraphWidget()->SetVirtualFinalNode(finalNodes.front());
            }
        }
    }

    void AnimGraphActionManager::DeleteSelectedNodes()
    {
        if (m_plugin->GetAnimGraphModel().CheckAnySelectedNodeBelongsToReferenceGraph())
        {
            return;
        }

        const AZStd::unordered_map<EMotionFX::AnimGraph*, AZStd::vector<EMotionFX::AnimGraphNode*>> nodesByAnimGraph = m_plugin->GetAnimGraphModel().GetSelectedObjectsOfType<EMotionFX::AnimGraphNode>();
        if (!nodesByAnimGraph.empty())
        {
            MCore::CommandGroup commandGroup("Delete anim graph nodes");

            for (const AZStd::pair<EMotionFX::AnimGraph*, AZStd::vector<EMotionFX::AnimGraphNode*>>& animGraphAndNodes : nodesByAnimGraph)
            {
                CommandSystem::DeleteNodes(&commandGroup, animGraphAndNodes.first, animGraphAndNodes.second, true);
            }

            AZStd::string result;
            if (!GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        }
    }

    void AnimGraphActionManager::NavigateToNode()
    {
        const QModelIndexList selectedIndexes = m_plugin->GetAnimGraphModel().GetSelectionModel().selectedRows();

        if (selectedIndexes.size() == 1)
        {
            const QModelIndex firstSelectedNode = selectedIndexes.front();
            m_plugin->GetAnimGraphModel().Focus(firstSelectedNode);
        }
    }

    void AnimGraphActionManager::NavigateToParent()
    {
        const QModelIndex parentFocus = m_plugin->GetAnimGraphModel().GetParentFocus();
        if (parentFocus.isValid())
        {
            QModelIndex newParentFocus = parentFocus.model()->parent(parentFocus);
            if (newParentFocus.isValid())
            {
                m_plugin->GetAnimGraphModel().Focus(newParentFocus);
            }
        }
    }

    void AnimGraphActionManager::OpenReferencedAnimGraph(EMotionFX::AnimGraphReferenceNode* referenceNode)
    {
        EMotionFX::AnimGraph* referencedGraph = referenceNode->GetReferencedAnimGraph();
        if (referencedGraph)
        {
            EMotionFX::MotionSet* motionSet = referenceNode->GetMotionSet();
            ActivateGraphForSelectedActors(referencedGraph, motionSet);
        }
    }

    void AnimGraphActionManager::ActivateAnimGraph()
    {
        // Activate the anim graph for visual feedback of button click.
        EMotionFX::MotionSet* motionSet = nullptr;
        EMotionFX::AnimGraphEditorRequestBus::BroadcastResult(motionSet, &EMotionFX::AnimGraphEditorRequests::GetSelectedMotionSet);
        if (!motionSet)
        {
            const EMotionFX::MotionManager& motionManager = EMotionFX::GetMotionManager();

            // In case no motion set was selected yet, use the first available. The activate graph callback will update the UI.
            const size_t numMotionSets = motionManager.GetNumMotionSets();
            for (size_t i = 0; i < numMotionSets; ++i)
            {
                EMotionFX::MotionSet* currentMotionSet = motionManager.GetMotionSet(i);
                if (!currentMotionSet->GetIsOwnedByRuntime())
                {
                    motionSet = currentMotionSet;
                    break;
                }
            }
        }

        EMotionFX::AnimGraph* animGraph = m_plugin->GetAnimGraphModel().GetFocusedAnimGraph();
        if (animGraph)
        {
            ActivateGraphForSelectedActors(animGraph, motionSet);
        }
    }

    void AnimGraphActionManager::ActivateGraphForSelectedActors(EMotionFX::AnimGraph* animGraph, EMotionFX::MotionSet* motionSet)
    {
        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        const size_t numActorInstances = selectionList.GetNumSelectedActorInstances();

        if (numActorInstances == 0)
        {
            // No need to issue activation commands
            m_plugin->SetActiveAnimGraph(animGraph);
            return;
        }

        MCore::CommandGroup commandGroup("Activate anim graph");
        commandGroup.AddCommandString("RecorderClear -force true");

        // Activate the anim graph each selected actor instance.
        for (size_t i = 0; i < numActorInstances; ++i)
        {
            EMotionFX::ActorInstance* actorInstance = selectionList.GetActorInstance(i);
            if (actorInstance->GetIsOwnedByRuntime())
            {
                continue;
            }

            EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();

            // Use the given motion set in case it is valid, elsewise use the one previously set to the actor instance.
            uint32 motionSetId = MCORE_INVALIDINDEX32;
            if (motionSet)
            {
                motionSetId = motionSet->GetID();
            }
            else
            {
                // if a motionSet is not passed, then we try to get the one from the anim graph instance
                if (animGraphInstance)
                {
                    EMotionFX::MotionSet* animGraphInstanceMotionSet = animGraphInstance->GetMotionSet();
                    if (animGraphInstanceMotionSet)
                    {
                        motionSetId = animGraphInstanceMotionSet->GetID();
                    }
                }
            }

            commandGroup.AddCommandString(AZStd::string::format("ActivateAnimGraph -actorInstanceID %d -animGraphID %d -motionSetID %d", actorInstance->GetID(), animGraph->GetID(), motionSetId));
        }

        if (commandGroup.GetNumCommands() > 0)
        {
            AZStd::string result;
            if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
            m_plugin->SetActiveAnimGraph(animGraph);
        }
    }

    void AnimGraphActionManager::AlignNodes(AlignMode alignMode)
    {
        if (!m_plugin->GetActionFilter().m_editNodes)
        {
            return;
        }

        NodeGraph* nodeGraph = m_plugin->GetGraphWidget()->GetActiveGraph();
        if (!nodeGraph)
        {
            return;
        }

        int32 alignedXPos = 0;
        int32 alignedYPos = 0;
        int32 maxGraphNodeHeight = 0;
        int32 maxGraphNodeWidth = 0;

        bool firstSelectedNode = true;
        const QModelIndexList selectedItems = m_plugin->GetAnimGraphModel().GetSelectionModel().selectedRows();
        AZStd::vector<GraphNode*> alignedGraphNodes;

        for (const QModelIndex& selected : selectedItems)
        {
            if (selected.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() == AnimGraphModel::ModelItemType::NODE)
            {
                GraphNode* graphNode = nodeGraph->FindGraphNode(selected);
                if (graphNode) // otherwise it does not belong to the current active graph
                {
                    alignedGraphNodes.push_back(graphNode);

                    EMotionFX::AnimGraphNode* animGraphNode = selected.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
                    const int32 xPos = animGraphNode->GetVisualPosX();
                    const int32 yPos = animGraphNode->GetVisualPosY();
                    const int32 graphNodeHeight = graphNode->CalcRequiredHeight();
                    const int32 graphNodeWidth = graphNode->CalcRequiredWidth();

                    if (firstSelectedNode)
                    {
                        alignedXPos = xPos;
                        alignedYPos = yPos;
                        maxGraphNodeHeight = graphNodeHeight;
                        maxGraphNodeWidth = graphNodeWidth;
                        firstSelectedNode = false;
                    }

                    if (graphNodeHeight > maxGraphNodeHeight)
                    {
                        maxGraphNodeHeight = graphNodeHeight;
                    }

                    if (graphNodeWidth > maxGraphNodeWidth)
                    {
                        maxGraphNodeWidth = graphNodeWidth;
                    }

                    switch (alignMode)
                    {
                        case Left:
                        {
                            if (xPos < alignedXPos)
                            {
                                alignedXPos = xPos;
                            }
                            break;
                        }
                        case Right:
                        {
                            if (xPos + graphNodeWidth > alignedXPos)
                            {
                                alignedXPos = xPos + graphNodeWidth;
                            }
                            break;
                        }
                        case Top:
                        {
                            if (yPos < alignedYPos)
                            {
                                alignedYPos = yPos;
                            }
                            break;
                        }
                        case Bottom:
                        {
                            if (yPos + graphNodeHeight > alignedYPos)
                            {
                                alignedYPos = yPos + graphNodeHeight;
                            }
                            break;
                        }
                        default:
                            MCORE_ASSERT(false);
                    }
                }
            }
        }

        if (alignedGraphNodes.size() > 1)
        {
            AZStd::string command;
            AZStd::string outResult;
            MCore::CommandGroup commandGroup("Align anim graph nodes");

            const QModelIndex& modelIndex = nodeGraph->GetModelIndex();
            EMotionFX::AnimGraphNode* parentNode = modelIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
            AZ_Assert(parentNode, "Expected the parent to be a node");
            EMotionFX::AnimGraph* animGraph = parentNode->GetAnimGraph();

            for (GraphNode* node : alignedGraphNodes)
            {
                command = AZStd::string::format("AnimGraphAdjustNode -animGraphID %i -name \"%s\" ", animGraph->GetID(), node->GetName());

                switch (alignMode)
                {
                    case Left:
                        command += AZStd::string::format("-xPos %i", alignedXPos);
                        break;
                    case Right:
                        command += AZStd::string::format("-xPos %i", alignedXPos - node->CalcRequiredWidth());
                        break;
                    case Top:
                        command += AZStd::string::format("-yPos %i", alignedYPos);
                        break;
                    case Bottom:
                        command += AZStd::string::format("-yPos %i", alignedYPos - node->CalcRequiredHeight());
                        break;
                    default:
                        MCORE_ASSERT(false);
                }

                commandGroup.AddCommandString(command);
            }

            GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult);
        }
    }

    void AnimGraphActionManager::SetPasteOperation(PasteOperation newOperation)
    {
        m_pasteOperation = newOperation;
        emit PasteStateChanged();
    }
} // namespace EMStudio
