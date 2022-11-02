/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/CommandSystem/Source/AnimGraphConnectionCommands.h>
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
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NodePaletteModelUpdater.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/SolidColorIconEngine.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionSetsWindow/MotionSetsWindowPlugin.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteWidget.h>
#include <QItemSelectionModel>
#include <QMenu>
#include <QWidgetAction>

namespace EMStudio
{
    void BlendGraphWidget::AddAssignNodeToGroupSubmenu(QMenu* menu, EMotionFX::AnimGraph* animGraph)
    {
        const size_t numNodeGroups = animGraph->GetNumNodeGroups();
        if (numNodeGroups == 0)
        {
            return;
        }

        QMenu* nodeGroupMenu = new QMenu(tr("Assign To Node Group"), menu);

        for (size_t i = 0; i < numNodeGroups; ++i)
        {
            EMotionFX::AnimGraphNodeGroup* nodeGroup = animGraph->GetNodeGroup(i);

            AZ::Color nodeGroupColor;
            nodeGroupColor.FromU32(nodeGroup->GetColor());
            QAction* nodeGroupAction = nodeGroupMenu->addAction(QIcon(new SolidColorIconEngine(nodeGroupColor)), nodeGroup->GetName());
            nodeGroupAction->setData(qulonglong(i + 1)); // index of the menu added, not used
            connect(nodeGroupAction, &QAction::triggered, this, &BlendGraphWidget::AssignSelectedNodesToGroup);
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

                QAction* previewMotionAction = menu->addAction(tr("Preview %1").arg(motionId));
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
        EMotionFX::AnimGraphNodeGroup* nodeGroup = nodeGraph->FindNodeGroup(localMousePos);

        // only show the paste and the create node menu entries in case the function got called from the graph widget
        if (!inReferenceGraph && graphWidgetOnlyMenusEnabled)
        {
            // check if we have actually clicked a node and if it is selected, only show the menu if both cases are true
            GraphNode* graphNode = nodeGraph->FindNode(localMousePos);
            if (graphNode == nullptr)
            {
                QMenu* menu = new QMenu(parentWidget);
                menu->setAttribute(Qt::WA_DeleteOnClose);

                if (actionFilter.m_copyAndPaste && actionManager.GetIsReadyForPaste() && nodeGraph->GetModelIndex().isValid())
                {
                    menu->addAction(viewWidget->GetAction(BlendGraphViewWidget::EDIT_PASTE));
                    menu->addSeparator();
                }

                if (actionFilter.m_createNodes && !nodeGroup)
                {
                    // Create nodes in palette view for each of the categories that are possible to be added to the currently focused graph.
                    EMotionFX::AnimGraphNode* currentNode = nodeGraph->GetModelIndex().data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();

                    auto* action = new QWidgetAction(menu);

                    NodePaletteModelUpdater modelUpdater{ plugin };
                    modelUpdater.InitForNode(currentNode);

                    GraphCanvas::NodePaletteConfig config;
                    config.m_rootTreeItem = modelUpdater.GetRootItem();
                    config.m_isInContextMenu = true;
                    auto* paletteWidget = new GraphCanvas::NodePaletteWidget(nullptr);
                    paletteWidget->SetupNodePalette(config);
                    action->setDefaultWidget(paletteWidget);
                    menu->addAction(action);

                    connect(
                        menu,
                        &QMenu::aboutToShow,
                        paletteWidget,
                        [paletteWidget]()
                        {
                            paletteWidget->FocusOnSearchFilter();
                        });
                    connect(
                        paletteWidget,
                        &GraphCanvas::NodePaletteWidget::OnCreateSelection,
                        paletteWidget,
                        [paletteWidget, this, menu]()
                        {
                            auto* event = static_cast<BlendGraphMimeEvent*>(paletteWidget->GetContextMenuEvent());
                            if (event)
                            {
                                m_plugin->GetGraphWidget()->OnContextMenuCreateNode(event);
                                menu->close();
                            }
                        });
                }
                else if (nodeGroup)
                {
                    QAction* renameNodeGroupAction = menu->addAction(QIcon(":/EMotionFX/Rename.svg"), "Rename");
                    connect(
                        renameNodeGroupAction,
                        &QAction::triggered,
                        this,
                        [this, nodeGroup]()
                        {
                            RenameNodeGroup(nodeGroup);
                        });

                    AZ::Color nodeGroupColor;
                    nodeGroupColor.FromU32(nodeGroup->GetColor());
                    QAction* editNodeGroupColor = menu->addAction(QIcon(new SolidColorIconEngine(nodeGroupColor)), "Pick Color");
                    connect(
                        editNodeGroupColor,
                        &QAction::triggered,
                        this,
                        [this, nodeGroup]()
                        {
                            ChangeNodeGroupColor(nodeGroup);
                        });

                    QAction* removeNodeGroupAction = menu->addAction(tr("Delete Group"));
                    connect(
                        removeNodeGroupAction,
                        &QAction::triggered,
                        this,
                        [this, nodeGroup]()
                        {
                            DeleteNodeGroup(nodeGroup);
                        });

                    QAction* deleteSelectedNodesAction = menu->addAction(tr("Delete Group and Nodes"));
                    connect(
                        deleteSelectedNodesAction,
                        &QAction::triggered,
                        this,
                        [this, nodeGroup]()
                        {
                            DeleteNodeGroupAndNodes(nodeGroup);
                        });
                }

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
                        QAction* activateNodeAction = menu->addAction(tr("Activate State"));
                        connect(activateNodeAction, &QAction::triggered, viewWidget, &BlendGraphViewWidget::OnActivateState);
                    }

                    if (!inReferenceGraph)
                    {
                        EMotionFX::AnimGraphStateMachine* stateMachine = (EMotionFX::AnimGraphStateMachine*)animGraphNode->GetParentNode();
                        if (actionFilter.m_setEntryNode &&
                            stateMachine->GetEntryState() != animGraphNode &&
                            animGraphNode->GetCanBeEntryNode())
                        {
                            QAction* setAsEntryStateAction = menu->addAction(tr("Set As Entry State"));
                            connect(setAsEntryStateAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::SetEntryState);
                        }

                        // action for adding a wildcard transition
                        if (actionFilter.m_createConnections)
                        {
                            QAction* addWildcardAction = menu->addAction(tr("Add Wildcard Transition"));
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
                            QAction* enableAction = menu->addAction(tr("Enable Node"));
                            connect(enableAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::EnableSelected);
                        }
                        else
                        {
                            QAction* disableAction = menu->addAction(tr("Disable Node"));
                            connect(disableAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::DisableSelected);
                        }
                    }

                    if (animGraphNode->GetSupportsVisualization())
                    {
                        menu->addSeparator();
                        QAction* action = menu->addAction(tr("Adjust Visualization Color"));
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
                    QAction* openAnimGraphAction = menu->addAction(QString(tr("Open '%1' file")).arg(filename.c_str()));
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
                        QAction* virtualAction = menu->addAction(tr("Make Final Output"));
                        connect(virtualAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::MakeVirtualFinalNode);
                        menu->addSeparator();
                    }
                    else
                    {
                        if (blendTree->GetFinalNode() != animGraphNode)
                        {
                            QAction* virtualAction = menu->addAction(tr("Restore Final Output"));
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

            if (!nodeGroup)
            {
                QAction* createNodeGroupAction = menu->addAction(tr("Create Node Group"));
                connect(createNodeGroupAction, &QAction::triggered, this, &BlendGraphWidget::CreateNodeGroup);
                if (actionFilter.m_editNodeGroups && !inReferenceGraph && animGraphNode->GetParentNode())
                {
                    AddAssignNodeToGroupSubmenu(menu, animGraphNode->GetAnimGraph());
                }
            }
            else
            {
                QAction* removeFromGroupAction = menu->addAction(tr("Remove From Node Group"));
                connect(
                    removeFromGroupAction,
                    &QAction::triggered,
                    this,
                    [this, nodeGroup, selectedNodes]
                    {
                        nodeGroup->RemoveNodeById(selectedNodes[0]->GetId());
                        if (nodeGroup->GetNumNodes() == 0)
                        {
                            DeleteNodeGroup(nodeGroup);
                        }
                    });
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
                    QAction* enableAction = menu.addAction(tr("Enable Nodes"));
                    connect(enableAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::EnableSelected);
                }

                // disable all nodes
                if (actionFilter.m_editNodes &&
                    oneEnabled && oneSupportsDelete)
                {
                    QAction* disableAction = menu.addAction(tr("Disable Nodes"));
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

            bool allNodesAreUngrouped = AZStd::all_of(
                selectedNodes.cbegin(),
                selectedNodes.cend(),
                [&](EMotionFX::AnimGraphNode* node)
                {
                    return node->GetAnimGraph()->FindNodeGroupForNode(node) == nullptr;
                });
            if (allNodesAreUngrouped)
            {
                QAction* createNodeGroupAction = menu.addAction(tr("Create Node Group"));
                connect(createNodeGroupAction, &QAction::triggered, this, &BlendGraphWidget::CreateNodeGroup);
                if (actionFilter.m_editNodeGroups && !inReferenceGraph)
                {
                    AddAssignNodeToGroupSubmenu(&menu, selectedNodes[0]->GetAnimGraph());
                }
            }
            else
            {
                QAction* removeFromGroupAction = menu.addAction(tr("Remove From Node Group"));
                connect(
                    removeFromGroupAction,
                    &QAction::triggered,
                    this,
                    [this, &selectedNodes]
                    {
                        for (EMotionFX::AnimGraphNode* node : selectedNodes)
                        {
                            auto* group = node->GetAnimGraph()->FindNodeGroupForNode(node);
                            if (group)
                            {
                                group->RemoveNodeById(node->GetId());
                                if (group->GetNumNodes() == 0)
                                {
                                    DeleteNodeGroup(group);
                                }
                            }
                        }
                    });
            }

            if (!menu.isEmpty())
            {
                menu.exec(globalMousePos);
            }
        }
    }
} // namespace EMStudio
