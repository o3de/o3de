/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteWidget.h>
#include <QMenu>
#include <QWidgetAction>


namespace EMStudio
{
    // Fill the node palette tree view with the anim graph objects that can be created inside the current given object and category.
    void BlendGraphWidget::AddCategoryToNodePalette(EMotionFX::AnimGraphNode::ECategory category, GraphCanvas::NodePaletteTreeItem* rootNode,
        EMotionFX::AnimGraphObject* focusedGraphObject)
    {
        const AZStd::vector<EMotionFX::AnimGraphObject*>& objectPrototypes = m_plugin->GetAnimGraphObjectFactory()->GetUiObjectPrototypes();
        // If the category is empty, don't add a node for it
        if (objectPrototypes.empty())
        {
            return;
        }
        const char* categoryName = EMotionFX::AnimGraphObject::GetCategoryName(category);

        auto* categoryNode = rootNode->CreateChildNode<GraphCanvas::NodePaletteTreeItem>(categoryName, AnimGraphEditorId);
        bool categoryEnabled = false;
        for (const EMotionFX::AnimGraphObject* objectPrototype : objectPrototypes)
        {
            if (objectPrototype->GetPaletteCategory() != category)
            {
                continue;
            }

            bool active = m_plugin->CheckIfCanCreateObject(focusedGraphObject, objectPrototype, category);
            categoryEnabled |= active;
            const EMotionFX::AnimGraphNode* nodePrototype = static_cast<const EMotionFX::AnimGraphNode*>(objectPrototype);

            const QString typeString = azrtti_typeid(nodePrototype).ToString<QString>();
            auto* node = categoryNode->CreateChildNode<BlendGraphNodePaletteTreeItem>(nodePrototype->GetPaletteName(), typeString, AnimGraphEditorId);
            node->SetEnabled(active);
        }
        categoryNode->SetEnabled(categoryEnabled);
    }

    void BlendGraphWidget::AddNodeGroupSubmenu(QMenu* menu, EMotionFX::AnimGraph* animGraph, const AZStd::vector<EMotionFX::AnimGraphNode*>& selectedNodes)
    {
        // node group sub menu
        QMenu* nodeGroupMenu = new QMenu("Node Group", menu);
        bool isNodeInNoneGroup = true;
        QAction* noneNodeGroupAction = nodeGroupMenu->addAction("None");
        noneNodeGroupAction->setData(qulonglong(0)); // this index is there to know it's the real none action in case one node group is also called like that
        connect(noneNodeGroupAction, &QAction::triggered, this, &BlendGraphWidget::OnNodeGroupSelected);
        noneNodeGroupAction->setCheckable(true);

        const size_t numNodeGroups = animGraph->GetNumNodeGroups();
        for (size_t i = 0; i < numNodeGroups; ++i)
        {
            EMotionFX::AnimGraphNodeGroup* nodeGroup = animGraph->GetNodeGroup(i);

            QAction* nodeGroupAction = nodeGroupMenu->addAction(nodeGroup->GetName());
            nodeGroupAction->setData(qulonglong(i + 1)); // index of the menu added, not used
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
                for (size_t i = 0; i < numMotions; ++i)
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

                if (actionFilter.m_copyAndPaste && actionManager.GetIsReadyForPaste() && nodeGraph->GetModelIndex().isValid())
                {
                    menu->addAction(viewWidget->GetAction(BlendGraphViewWidget::EDIT_PASTE));
                    menu->addSeparator();
                }

                if (actionFilter.m_createNodes)
                {
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

                    // Create nodes in palette view for each of the categories that are possible to be added to the currently focused graph.
                    EMotionFX::AnimGraphNode* currentNode = nodeGraph->GetModelIndex().data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();

                    auto* action = new QWidgetAction(menu);
                    auto* rootItem = new GraphCanvas::NodePaletteTreeItem("root", AnimGraphEditorId);
                    GraphCanvas::NodePaletteConfig config;
                    config.m_rootTreeItem = rootItem;
                    config.m_isInContextMenu = true;
                    auto* paletteWidget = new GraphCanvas::NodePaletteWidget(nullptr);
                    paletteWidget->SetupNodePalette(config);
                    action->setDefaultWidget(paletteWidget);
                    menu->addAction(action);

                    for (const EMotionFX::AnimGraphNode::ECategory category: categories)
                    {
                        AddCategoryToNodePalette(category, rootItem, currentNode);
                    }

                    connect(menu, &QMenu::aboutToShow, paletteWidget, [=](){
                        paletteWidget->FocusOnSearchFilter();
                    });
                    connect(paletteWidget, &GraphCanvas::NodePaletteWidget::OnCreateSelection, paletteWidget, [=]() {
                            auto* event = static_cast<BlendGraphMimeEvent*>(paletteWidget->GetContextMenuEvent());
                            if (event)
                            {
                                m_plugin->GetGraphWidget()->OnContextMenuCreateNode(event);
                                menu->close();
                            }
                    });
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
                        connect(action, &QAction::triggered, [this, animGraphNode](bool) { m_plugin->GetActionManager().ShowNodeColorPicker(animGraphNode); });
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
                menu->addAction(viewWidget->GetAction(BlendGraphViewWidget::NAVIGATION_OPEN_SELECTED));
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
                        menu->addAction(viewWidget->GetAction(BlendGraphViewWidget::EDIT_CUT));
                    }

                    menu->addAction(viewWidget->GetAction(BlendGraphViewWidget::EDIT_COPY));
                    menu->addSeparator();
                }

                if (actionFilter.m_delete &&
                    !inReferenceGraph)
                {
                    menu->addAction(viewWidget->GetAction(BlendGraphViewWidget::EDIT_DELETE));
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
                menu.addAction(viewWidget->GetAction(BlendGraphViewWidget::SELECTION_ALIGNLEFT));
                menu.addAction(viewWidget->GetAction(BlendGraphViewWidget::SELECTION_ALIGNRIGHT));
                menu.addAction(viewWidget->GetAction(BlendGraphViewWidget::SELECTION_ALIGNTOP));
                menu.addAction(viewWidget->GetAction(BlendGraphViewWidget::SELECTION_ALIGNBOTTOM));
                menu.addSeparator();
            }

            menu.addAction(viewWidget->GetAction(BlendGraphViewWidget::NAVIGATION_ZOOMSELECTION));

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
                        menu.addAction(viewWidget->GetAction(BlendGraphViewWidget::EDIT_CUT));
                    }

                    menu.addAction(viewWidget->GetAction(BlendGraphViewWidget::EDIT_COPY));
                }

                menu.addSeparator();

                if (actionFilter.m_delete &&
                    !inReferenceGraph)
                {
                    menu.addAction(viewWidget->GetAction(BlendGraphViewWidget::EDIT_DELETE));

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
} // namespace EMStudio
