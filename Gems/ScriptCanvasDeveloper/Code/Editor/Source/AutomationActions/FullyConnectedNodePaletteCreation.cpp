/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <QMenu>

#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/Utils/GraphUtils.h>
#include <GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteWidget.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteDockWidget.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>

#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>
#include <Editor/Include/ScriptCanvas/Bus/NodeIdPair.h>
#include <Editor/View/Widgets/NodePalette/CreateNodeMimeEvent.h>
#include <Editor/View/Widgets/NodePalette/GeneralNodePaletteTreeItemTypes.h>

#include <ScriptCanvasDeveloperEditor/AutomationActions/NodePaletteFullCreation.h>
#include <ScriptCanvasDeveloperEditor/DeveloperUtils.h>

namespace ScriptCanvasDeveloperEditor
{    
    namespace NodePaletteFullCreation
    {
        class CreateFullyConnectedNodePaletteInterface
            : public ProcessNodePaletteInterface
        {
        public:

            CreateFullyConnectedNodePaletteInterface(DeveloperUtils::ConnectionStyle connectionStyle, bool skipHandlers = false)
            {
                m_chainConfig.m_connectionStyle = connectionStyle;
                m_chainConfig.m_skipHandlers = skipHandlers;
            }

            void SetupInterface(const AZ::EntityId& activeGraphCanvasGraphId, const ScriptCanvas::ScriptCanvasId& scriptCanvasId)
            {
                m_graphCanvasGraphId = activeGraphCanvasGraphId;
                m_scriptCanvasId = scriptCanvasId;
                
                GraphCanvas::SceneRequestBus::EventResult(m_viewId, activeGraphCanvasGraphId, &GraphCanvas::SceneRequests::GetViewId);
                GraphCanvas::SceneRequestBus::EventResult(m_gridId, activeGraphCanvasGraphId, &GraphCanvas::SceneRequests::GetGrid);

                GraphCanvas::GridRequestBus::EventResult(m_minorPitch, m_gridId, &GraphCanvas::GridRequests::GetMinorPitch);

                QGraphicsScene* graphicsScene = nullptr;
                GraphCanvas::SceneRequestBus::EventResult(graphicsScene, activeGraphCanvasGraphId, &GraphCanvas::SceneRequests::AsQGraphicsScene);

                if (graphicsScene)
                {
                    QRectF sceneArea = graphicsScene->sceneRect();
                    sceneArea.adjust(m_minorPitch.GetX(), m_minorPitch.GetY(), -m_minorPitch.GetX(), -m_minorPitch.GetY());
                    GraphCanvas::ViewRequestBus::Event(m_viewId, &GraphCanvas::ViewRequests::CenterOnArea, sceneArea);
                    QApplication::processEvents();
                }

                GraphCanvas::ViewRequestBus::EventResult(m_nodeCreationPos, m_viewId, &GraphCanvas::ViewRequests::GetViewSceneCenter);

                GraphCanvas::GraphCanvasGraphicsView* graphicsView = nullptr;
                GraphCanvas::ViewRequestBus::EventResult(graphicsView, m_viewId, &GraphCanvas::ViewRequests::AsGraphicsView);

                m_viewportRectangle = graphicsView->mapToScene(graphicsView->viewport()->geometry()).boundingRect();

                // Temporary work around until the extra automation tools can be merged over that have better ways of doing this.
                const GraphCanvas::GraphCanvasTreeItem* treeItem = nullptr;
                ScriptCanvasEditor::AutomationRequestBus::BroadcastResult(treeItem, &ScriptCanvasEditor::AutomationRequests::GetNodePaletteRoot);

                const GraphCanvas::NodePaletteTreeItem* onGraphStartItem = nullptr;

                if (treeItem)
                {
                    AZStd::unordered_set< const GraphCanvas::GraphCanvasTreeItem* > unexploredSet = { treeItem };

                    while (!unexploredSet.empty())
                    {
                        const GraphCanvas::GraphCanvasTreeItem* treeItem = (*unexploredSet.begin());
                        unexploredSet.erase(unexploredSet.begin());

                        const GraphCanvas::NodePaletteTreeItem* nodePaletteTreeItem = azrtti_cast<const GraphCanvas::NodePaletteTreeItem*>(treeItem);

                        if (nodePaletteTreeItem && nodePaletteTreeItem->GetName().compare("On Graph Start") == 0)
                        {
                            onGraphStartItem = nodePaletteTreeItem;
                            break;
                        }

                        for (int i = 0; i < treeItem->GetChildCount(); ++i)
                        {
                            const GraphCanvas::GraphCanvasTreeItem* childItem = treeItem->FindChildByRow(i);

                            if (childItem)
                            {
                                unexploredSet.insert(childItem);
                            }
                        }
                    }
                }

                if (onGraphStartItem)
                {
                    ProcessItem(onGraphStartItem);
                }
            }

            int m_counter = 60;
            
            bool ShouldProcessItem(const GraphCanvas::NodePaletteTreeItem* nodePaletteTreeItem) const
            {
                return m_counter > 0;
            }
            
            void ProcessItem(const GraphCanvas::NodePaletteTreeItem* nodePaletteTreeItem)
            {
                GraphCanvas::GraphCanvasMimeEvent* mimeEvent = nodePaletteTreeItem->CreateMimeEvent();

                if (ScriptCanvasEditor::MultiCreateNodeMimeEvent* multiCreateMimeEvent = azrtti_cast<ScriptCanvasEditor::MultiCreateNodeMimeEvent*>(mimeEvent))
                {
                    --m_counter;
                    AZStd::vector< GraphCanvas::GraphCanvasMimeEvent* > mimeEvents = multiCreateMimeEvent->CreateMimeEvents();

                    for (GraphCanvas::GraphCanvasMimeEvent* currentEvent : mimeEvents)
                    {
                        ScriptCanvasEditor::NodeIdPair createdPair = DeveloperUtils::HandleMimeEvent(currentEvent, m_graphCanvasGraphId, m_viewportRectangle, m_widthOffset, m_heightOffset, m_maxRowHeight, m_minorPitch);
                        delete currentEvent;

                        if (DeveloperUtils::CreateConnectedChain(createdPair, m_chainConfig))
                        {
                            m_createdNodes.emplace_back(createdPair);
                        }
                        else
                        {
                            m_nodesToDelete.insert(GraphCanvas::GraphUtils::FindOutermostNode(createdPair.m_graphCanvasId));
                        }
                    }
                }
                else if (mimeEvent)
                {
                    --m_counter;
                    ScriptCanvasEditor::NodeIdPair createdPair = DeveloperUtils::HandleMimeEvent(mimeEvent, m_graphCanvasGraphId, m_viewportRectangle, m_widthOffset, m_heightOffset, m_maxRowHeight, m_minorPitch);

                    if (DeveloperUtils::CreateConnectedChain(createdPair, m_chainConfig))
                    {
                        m_createdNodes.emplace_back(createdPair);
                    }
                    else
                    {
                        m_nodesToDelete.insert(GraphCanvas::GraphUtils::FindOutermostNode(createdPair.m_graphCanvasId));
                    }
                }

                delete mimeEvent;
            }
        
        private:

            void OnProcessingComplete() override
            {
                GraphCanvas::SceneRequestBus::Event(m_graphCanvasGraphId, &GraphCanvas::SceneRequests::Delete, m_nodesToDelete);
            }

            DeveloperUtils::CreateConnectedChainConfig m_chainConfig;

            AZStd::vector<ScriptCanvasEditor::NodeIdPair> m_createdNodes;
            AZStd::unordered_set<AZ::EntityId> m_nodesToDelete;
        
            AZ::EntityId m_graphCanvasGraphId;
            ScriptCanvas::ScriptCanvasId m_scriptCanvasId;
        
            AZ::Vector2 m_nodeCreationPos = AZ::Vector2::CreateZero();
            
            AZ::EntityId m_viewId;
            AZ::EntityId m_gridId;
            
            AZ::Vector2 m_minorPitch = AZ::Vector2::CreateZero();
            
            QRectF m_viewportRectangle;
            
            int m_widthOffset = 0;
            int m_heightOffset = 0;
            
            int m_maxRowHeight = 0;
        };
        
        void CreateSingleExecutionConnectedNodePaletteAction()
        {   
            ScriptCanvasEditor::AutomationRequestBus::Broadcast(&ScriptCanvasEditor::AutomationRequests::SignalAutomationBegin);
            
            CreateFullyConnectedNodePaletteInterface nodePaletteInterface(DeveloperUtils::ConnectionStyle::SingleExecutionConnection);
            DeveloperUtils::ProcessNodePalette(nodePaletteInterface);

            ScriptCanvasEditor::AutomationRequestBus::Broadcast(&ScriptCanvasEditor::AutomationRequests::SignalAutomationEnd);
        }

        void CreateSingleExecutionConnectedNodePaletteExcludeHandlersAction()
        {
            ScriptCanvasEditor::AutomationRequestBus::Broadcast(&ScriptCanvasEditor::AutomationRequests::SignalAutomationBegin);

            CreateFullyConnectedNodePaletteInterface nodePaletteInterface(DeveloperUtils::ConnectionStyle::SingleExecutionConnection, true);
            DeveloperUtils::ProcessNodePalette(nodePaletteInterface);

            ScriptCanvasEditor::AutomationRequestBus::Broadcast(&ScriptCanvasEditor::AutomationRequests::SignalAutomationEnd);
        }

        QAction* FullyConnectedNodePaletteCreation(QMenu* mainMenu)
        {
            QAction* createNodePaletteAction = nullptr;

            if (mainMenu)
            {
                {
                    createNodePaletteAction = mainMenu->addAction(QAction::tr("Create Execution Connected Node Palette"));
                    createNodePaletteAction->setAutoRepeat(false);
                    createNodePaletteAction->setToolTip("Tries to create every node in the node palette and will attempt to create an execution path through them.");

                    QObject::connect(createNodePaletteAction, &QAction::triggered, &CreateSingleExecutionConnectedNodePaletteAction);
                }

                {
                    createNodePaletteAction = mainMenu->addAction(QAction::tr("Create Execution Connected Node Palette sans Handlers"));
                    createNodePaletteAction->setAutoRepeat(false);
                    createNodePaletteAction->setToolTip("Tries to create every node in the node palette(except EBus Handlers) and attempt to create an execution path through them..");

                    QObject::connect(createNodePaletteAction, &QAction::triggered, &CreateSingleExecutionConnectedNodePaletteExcludeHandlersAction);
                }


            }

            return createNodePaletteAction;
        }
    }
}
