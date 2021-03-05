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

#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphNodeGroup.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Source/AnimGraphReferenceNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphActionManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphViewWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NodePaletteWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionSetsWindow/MotionSetsWindowPlugin.h>
#include <QMenu>


namespace EMStudio
{
    // Fill the given menu with anim graph objects that can be created inside the current given object and category.
    void BlendGraphWidget::AddAnimGraphObjectCategoryMenu(AnimGraphPlugin* plugin, QMenu* parentMenu,
        EMotionFX::AnimGraphObject::ECategory category, EMotionFX::AnimGraphObject* focusedGraphObject)
    {
        // Check if any object of the given category can be created in the currently focused and shown graph.
        bool isEmpty = true;
        const AZStd::vector<EMotionFX::AnimGraphObject*>& objectPrototypes = plugin->GetAnimGraphObjectFactory()->GetUiObjectPrototypes();
        for (EMotionFX::AnimGraphObject* objectPrototype : objectPrototypes)
        {
            if (mPlugin->CheckIfCanCreateObject(focusedGraphObject, objectPrototype, category))
            {
                isEmpty = false;
                break;
            }
        }

        // If the category will be empty, return directly and skip adding a category sub-menu.
        if (isEmpty)
        {
            return;
        }

        const char* categoryName = EMotionFX::AnimGraphObject::GetCategoryName(category);
        QMenu* menu = parentMenu->addMenu(categoryName);

        for (const EMotionFX::AnimGraphObject* objectPrototype : objectPrototypes)
        {
            if (mPlugin->CheckIfCanCreateObject(focusedGraphObject, objectPrototype, category))
            {
                const EMotionFX::AnimGraphNode* nodePrototype = static_cast<const EMotionFX::AnimGraphNode*>(objectPrototype);
                QAction* action = menu->addAction(nodePrototype->GetPaletteName());
                action->setWhatsThis(azrtti_typeid(nodePrototype).ToString<QString>());
                action->setData(QVariant(nodePrototype->GetPaletteName()));
                connect(action, &QAction::triggered, plugin->GetGraphWidget(), &BlendGraphWidget::OnContextMenuCreateNode);
            }
        }
    }


    void BlendGraphWidget::AddNodeGroupSubmenu(QMenu* menu, EMotionFX::AnimGraph* animGraph, const AZStd::vector<EMotionFX::AnimGraphNode*>& selectedNodes)
    {
        // node group sub menu
        QMenu* nodeGroupMenu = new QMenu("Node Group", menu);
        bool isNodeInNoneGroup = true;
        QAction* noneNodeGroupAction = nodeGroupMenu->addAction("None");
        noneNodeGroupAction->setData(0); // this index is there to know it's the real none action in case one node group is also called like that
        connect(noneNodeGroupAction, &QAction::triggered, this, &BlendGraphWidget::OnNodeGroupSelected);
        noneNodeGroupAction->setCheckable(true);

        const uint32 numNodeGroups = animGraph->GetNumNodeGroups();
        for (uint32 i = 0; i < numNodeGroups; ++i)
        {
            EMotionFX::AnimGraphNodeGroup* nodeGroup = animGraph->GetNodeGroup(i);

            QAction* nodeGroupAction = nodeGroupMenu->addAction(nodeGroup->GetName());
            nodeGroupAction->setData(i + 1); // index of the menu added, not used
            connect(nodeGroupAction, &QAction::triggered, this, &BlendGraphWidget::OnNodeGroupSelected);
            nodeGroupAction->setCheckable(true);

            if (selectedNodes.size() == 1)
            {
                const EMotionFX::AnimGraphNode* animGraphNode = selectedNodes[0];
                if (nodeGroup->Contains(animGraphNode->GetId()))
                {
                    nodeGroupAction->setChecked(true);
                    isNodeInNoneGroup = false;
                }
                else
                {
                    nodeGroupAction->setChecked(false);
                }
            }
        }

        if (selectedNodes.size() == 1)
        {
            if (isNodeInNoneGroup)
            {
                noneNodeGroupAction->setChecked(true);
            }
            else
            {
                noneNodeGroupAction->setChecked(false);
            }
        }

        menu->addMenu(nodeGroupMenu);
    }

    void BlendGraphWidget::AddPreviewMotionSubmenu(QMenu* menu, AnimGraphActionManager* actionManager, const EMotionFX::AnimGraphNode* selectedNode)
    {
        // Preview motion only supporting motion node as of now.
        if (azrtti_typeid(selectedNode) == azrtti_typeid<EMotionFX::AnimGraphMotionNode>())
        {
            const EMotionFX::AnimGraphMotionNode* motionNode = static_cast<const EMotionFX::AnimGraphMotionNode*>(selectedNode);
            const size_t numMotions = motionNode->GetNumMotions();
            if (numMotions == 0)
            {
                return;
            }
            else if (numMotions == 1)
            {
                const char* motionId = motionNode->GetMotionId(0);
                EMotionFX::MotionSet::MotionEntry* motionEntry = MotionSetsWindowPlugin::FindBestMatchMotionEntryById(motionId);
                if (!motionEntry)
                {
                    return;
                }

                QAction* previewMotionAction = menu->addAction(AZStd::string::format("Preview %s", motionId).c_str());
                previewMotionAction->setWhatsThis(QString("PreviewMotion"));
                previewMotionAction->setData(QVariant(motionId));
                connect(previewMotionAction, &QAction::triggered, [actionManager, motionId]() { actionManager->PreviewMotionSelected(motionId); });
            }
            else
            {
                QMenu* previewMotionMenu = new QMenu("Preview Motions", menu);
                for (uint32 i = 0; i < numMotions; ++i)
                {
                    const char* motionId = motionNode->GetMotionId(i);
                    QAction* previewMotionAction = previewMotionMenu->addAction(motionId);
                    connect(previewMotionAction, &QAction::triggered, [actionManager, motionId]() { actionManager->PreviewMotionSelected(motionId); });
                }
                menu->addMenu(previewMotionMenu);
            }
        }
    }


    void BlendGraphWidget::OnContextMenuEvent(QWidget* parentWidget, QPoint localMousePos, QPoint globalMousePos, AnimGraphPlugin* plugin,
        const AZStd::vector<EMotionFX::AnimGraphNode*>& selectedNodes, bool graphWidgetOnlyMenusEnabled, bool selectingAnyReferenceNodeFromNavigation,
        const AnimGraphActionFilter& actionFilter)
    {
        BlendGraphWidget* blendGraphWidget = plugin->GetGraphWidget();
        NodeGraph* nodeGraph = blendGraphWidget->GetActiveGraph();
        if (!nodeGraph)
        {
            return;
        }

        BlendGraphViewWidget* viewWidget = plugin->GetViewWidget();
        AnimGraphActionManager& actionManager = plugin->GetActionManager();
        const bool inReferenceGraph = nodeGraph->IsInReferencedGraph() || selectingAnyReferenceNodeFromNavigation;

        // only show the paste and the create node menu entries in case the function got called from the graph widget
        if (!inReferenceGraph && graphWidgetOnlyMenusEnabled)
        {
            // check if we have actually clicked a node and if it is selected, only show the menu if both cases are true
            GraphNode* graphNode = nodeGraph->FindNode(localMousePos);
            if (graphNode == nullptr)
            {
                QMenu* menu = new QMenu(parentWidget);
                if (actionFilter.m_copyAndPaste && actionManager.GetIsReadyForPaste())
                {
                    const QModelIndex modelIndex = nodeGraph->GetModelIndex();
                    if (modelIndex.isValid())
                    {
                        localMousePos = SnapLocalToGrid(LocalToGlobal(localMousePos));

                        QAction* pasteAction = menu->addAction("Paste");
                        connect(pasteAction, &QAction::triggered, [&actionManager, modelIndex, localMousePos]() { actionManager.Paste(modelIndex, localMousePos); });
                        menu->addSeparator();
                    }
                }

                if (actionFilter.m_createNodes)
                {
                    QMenu* createNodeMenu = menu->addMenu("Create Node");

                    const AZStd::vector<EMotionFX::AnimGraphNode::ECategory> categories =
                    {
                        EMotionFX::AnimGraphNode::CATEGORY_SOURCES,
                        EMotionFX::AnimGraphNode::CATEGORY_BLENDING,
                        EMotionFX::AnimGraphNode::CATEGORY_CONTROLLERS,
                        EMotionFX::AnimGraphNode::CATEGORY_PHYSICS,
                        EMotionFX::AnimGraphNode::CATEGORY_LOGIC,
                        EMotionFX::AnimGraphNode::CATEGORY_MATH,
                        EMotionFX::AnimGraphNode::CATEGORY_MISC
                    };

                    // Create sub-menus for each of the categories that are possible to be added to the currently focused graph.
                    EMotionFX::AnimGraphNode* currentNode = nodeGraph->GetModelIndex().data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
                    for (const EMotionFX::AnimGraphNode::ECategory category: categories)
                    {
                        AddAnimGraphObjectCategoryMenu(plugin, createNodeMenu, category, currentNode);
                    }
                }

                connect(menu, &QMenu::triggered, menu, &QMenu::deleteLater);

                if (!menu->isEmpty())
                {
                    menu->popup(globalMousePos);
                    return;
                }
            }
        }

        const size_t numSelectedNodes = selectedNodes.size();

        // Is only one anim graph node selected?
        if (numSelectedNodes == 1 && selectedNodes[0])
        {
            QMenu* menu = new QMenu(parentWidget);
            menu->setObjectName("BlendGraphWidget.SelectedNodeMenu");
            EMotionFX::AnimGraphNode* animGraphNode = selectedNodes[0];
            if (animGraphNode->GetSupportsPreviewMotion())
            {
                AddPreviewMotionSubmenu(menu, &actionManager, animGraphNode);
                menu->addSeparator();
            }

            if (animGraphNode->GetParentNode())
            {
                // if the parent is a state machine
                if (azrtti_typeid(animGraphNode->GetParentNode()) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
                {
                    if (actionFilter.m_activateState)
                    {
                        QAction* activateNodeAction = menu->addAction("Activate State");
                        connect(activateNodeAction, &QAction::triggered, viewWidget, &BlendGraphViewWidget::OnActivateState);
                    }

                    if (!inReferenceGraph)
                    {
                        EMotionFX::AnimGraphStateMachine* stateMachine = (EMotionFX::AnimGraphStateMachine*)animGraphNode->GetParentNode();
                        if (actionFilter.m_setEntryNode &&
                            stateMachine->GetEntryState() != animGraphNode &&
                            animGraphNode->GetCanBeEntryNode())
                        {
                            QAction* setAsEntryStateAction = menu->addAction("Set As Entry State");
                            connect(setAsEntryStateAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::SetEntryState);
                        }

                        // action for adding a wildcard transition
                        if (actionFilter.m_createConnections)
                        {
                            QAction* addWildcardAction = menu->addAction("Add Wildcard Transition");
                            connect(addWildcardAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::AddWildCardTransition);
                        }
                    }
                } // parent is a state machine

                // if the parent is a state blend tree
                if (actionFilter.m_editNodes &&
                    azrtti_typeid(animGraphNode->GetParentNode()) == azrtti_typeid<EMotionFX::BlendTree>())
                {
                    if (animGraphNode->GetSupportsDisable())
                    {
                        // enable or disable the node
                        if (animGraphNode->GetIsEnabled() == false)
                        {
                            QAction* enableAction = menu->addAction("Enable Node");
                            connect(enableAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::EnableSelected);
                        }
                        else
                        {
                            QAction* disableAction = menu->addAction("Disable Node");
                            connect(disableAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::DisableSelected);
                        }
                    }

                    if (animGraphNode->GetSupportsVisualization())
                    {
                        menu->addSeparator();
                        QAction* action = menu->addAction("Adjust Visualization Color");
                        connect(action, &QAction::triggered, [this, animGraphNode](bool) { mPlugin->GetActionManager().ShowNodeColorPicker(animGraphNode); });
                    }
                }
            }

            if (!menu->isEmpty())
            {
                menu->addSeparator();
            }

            if (azrtti_typeid(animGraphNode) == azrtti_typeid<EMotionFX::AnimGraphReferenceNode>())
            {
                EMotionFX::AnimGraphReferenceNode* referenceNode = static_cast<EMotionFX::AnimGraphReferenceNode*>(animGraphNode);
                EMotionFX::AnimGraph* referencedGraph = referenceNode->GetReferencedAnimGraph();
                if (referencedGraph)
                {
                    AZStd::string filename;
                    AzFramework::StringFunc::Path::GetFullFileName(referencedGraph->GetFileName(), filename);
                    QAction* openAnimGraphAction = menu->addAction(QString("Open '%1' file").arg(filename.c_str()));
                    connect(openAnimGraphAction, &QAction::triggered, [&actionManager, referenceNode]() { actionManager.OpenReferencedAnimGraph(referenceNode); });
                    menu->addSeparator();
                }
            }

            // we can only go to the selected node in case the selected node has a visual graph (state machine / blend tree)
            if (animGraphNode->GetHasVisualGraph())
            {
                QAction* goToNodeAction = menu->addAction("Open Selected Node");
                connect(goToNodeAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::NavigateToNode);
                menu->addSeparator();
            }

            // make the node a virtual final node
            if (animGraphNode->GetHasOutputPose())
            {
                if (animGraphNode->GetParentNode() && azrtti_typeid(animGraphNode->GetParentNode()) == azrtti_typeid<EMotionFX::BlendTree>())
                {
                    EMotionFX::BlendTree* blendTree = static_cast<EMotionFX::BlendTree*>(animGraphNode->GetParentNode());
                    if (blendTree->GetVirtualFinalNode() != animGraphNode)
                    {
                        QAction* virtualAction = menu->addAction("Make Final Output");
                        connect(virtualAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::MakeVirtualFinalNode);
                        menu->addSeparator();
                    }
                    else
                    {
                        if (blendTree->GetFinalNode() != animGraphNode)
                        {
                            QAction* virtualAction = menu->addAction("Restore Final Output");
                            connect(virtualAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::RestoreVirtualFinalNode);
                            menu->addSeparator();
                        }
                    }
                }
            }

            if (animGraphNode->GetIsDeletable())
            {
                if (actionFilter.m_copyAndPaste)
                {
                    if (!inReferenceGraph)
                    {
                        // cut and copy actions
                        QAction* cutAction = menu->addAction("Cut");
                        connect(cutAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::Cut);
                    }

                    QAction* ccopyAction = menu->addAction("Copy");
                    connect(ccopyAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::Copy);
                    menu->addSeparator();
                }

                if (actionFilter.m_delete &&
                    !inReferenceGraph)
                {
                    QAction* removeNodeAction = menu->addAction("Delete Node");
                    connect(removeNodeAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::DeleteSelectedNodes);
                    menu->addSeparator();
                }
            }

            // add the node group sub menu
            if (actionFilter.m_editNodeGroups &&
                !inReferenceGraph && animGraphNode->GetParentNode())
            {
                AddNodeGroupSubmenu(menu, animGraphNode->GetAnimGraph(), selectedNodes);
            }

            connect(menu, &QMenu::triggered, menu, &QMenu::deleteLater);

            // show the menu at the given position
            if (menu->isEmpty() == false)
            {
                menu->popup(globalMousePos);
                return;
            }
        }

        // check if we are dealing with multiple selected nodes
        if (numSelectedNodes > 1)
        {
            QMenu menu(parentWidget);

            if (actionFilter.m_editNodes &&
                !inReferenceGraph)
            {
                QAction* alignLeftAction = menu.addAction("Align Left");
                QAction* alignRightAction = menu.addAction("Align Right");
                QAction* alignTopAction = menu.addAction("Align Top");
                QAction* alignBottomAction = menu.addAction("Align Bottom");


                connect(alignLeftAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::AlignLeft);
                connect(alignRightAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::AlignRight);
                connect(alignTopAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::AlignTop);
                connect(alignBottomAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::AlignBottom);

                menu.addSeparator();
            }

            QAction* zoomSelectionAction = menu.addAction("Zoom Selection");
            connect(zoomSelectionAction, &QAction::triggered, viewWidget, &BlendGraphViewWidget::ZoomSelected);

            menu.addSeparator();

            // check if all nodes selected have a blend tree as parent and if they all support disabling/enabling
            bool allBlendTreeNodes = true;
            for (const EMotionFX::AnimGraphNode* selectedNode : selectedNodes)
            {
                // make sure its a node inside a blend tree and that it supports disabling
                if (!selectedNode->GetParentNode() ||
                    (azrtti_typeid(selectedNode->GetParentNode()) != azrtti_typeid<EMotionFX::BlendTree>()))
                {
                    allBlendTreeNodes = false;
                    break;
                }
            }

            // if we only deal with blend tree nodes
            if (allBlendTreeNodes)
            {
                // check if we have at least one enabled or disabled node
                bool oneDisabled = false;
                bool oneEnabled = false;
                bool oneSupportsDelete = false;
                for (const EMotionFX::AnimGraphNode* selectedNode : selectedNodes)
                {
                    if (selectedNode->GetSupportsDisable())
                    {
                        oneSupportsDelete = true;
                        if (selectedNode->GetIsEnabled())
                        {
                            oneEnabled = true;
                        }
                        else
                        {
                            oneDisabled = true;
                        }
                    }
                }

                // enable all nodes
                if (actionFilter.m_editNodes &&
                    oneDisabled && oneSupportsDelete)
                {
                    QAction* enableAction = menu.addAction("Enable Nodes");
                    connect(enableAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::EnableSelected);
                }

                // disable all nodes
                if (actionFilter.m_editNodes &&
                    oneEnabled && oneSupportsDelete)
                {
                    QAction* disableAction = menu.addAction("Disable Nodes");
                    connect(disableAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::DisableSelected);
                }

                menu.addSeparator();
            }

            // check if there is deletable node selected
            const AZStd::vector<GraphNode*> selectedGraphNodes = nodeGraph->GetSelectedGraphNodes();
            const bool canDelete = AZStd::any_of(
                begin(selectedGraphNodes),
                end(selectedGraphNodes),
                [](const GraphNode* graphNode) {
                    return graphNode->GetIsDeletable();
                });

            if (canDelete)
            {
                if (actionFilter.m_copyAndPaste)
                {
                    menu.addSeparator();

                    if (!inReferenceGraph)
                    {
                        QAction* cutAction = menu.addAction("Cut");
                        connect(cutAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::Cut);
                    }

                    QAction* ccopyAction = menu.addAction("Copy");
                    connect(ccopyAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::Copy);
                }

                menu.addSeparator();

                if (actionFilter.m_delete &&
                    !inReferenceGraph)
                {
                    QAction* removeNodesAction = menu.addAction("Delete Nodes");
                    connect(removeNodesAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::DeleteSelectedNodes);

                    menu.addSeparator();
                }
            }

            if (actionFilter.m_editNodeGroups &&
                !inReferenceGraph)
            {
                AddNodeGroupSubmenu(&menu, selectedNodes[0]->GetAnimGraph(), selectedNodes);
            }

            if (!menu.isEmpty())
            {
                menu.exec(globalMousePos);
            }
        }
    }
}
