/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QMenu>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>
#include <Editor/View/Widgets/NodePalette/CreateNodeMimeEvent.h>

#include <ScriptCanvas/Bus/RequestBus.h>
#include <Editor/Include/ScriptCanvas/Bus/NodeIdPair.h>

#include <ScriptCanvasDeveloperEditor/AutomationActions/NodePaletteFullCreation.h>
#include <ScriptCanvasDeveloperEditor/DeveloperUtils.h>

namespace ScriptCanvasDeveloperEditor
{    
    namespace NodePaletteFullCreation
    {
        class FullCreationNodePaletteInterface
            : public ProcessNodePaletteInterface
        {
        public:
        
            void SetupInterface(const AZ::EntityId& activeGraphCanvasGraphId, const ScriptCanvas::ScriptCanvasId& scriptCanvasId)
            {
                m_graphCanvasGraphId = activeGraphCanvasGraphId;
                m_scriptCanvasId = scriptCanvasId;
                
                GraphCanvas::SceneRequestBus::EventResult(m_viewId, activeGraphCanvasGraphId, &GraphCanvas::SceneRequests::GetViewId);
                GraphCanvas::SceneRequestBus::EventResult(m_gridId, activeGraphCanvasGraphId, &GraphCanvas::SceneRequests::GetGrid);

                GraphCanvas::GridRequestBus::EventResult(m_minorPitch, m_gridId, &GraphCanvas::GridRequests::GetMinorPitch);

                GraphCanvas::ViewRequestBus::EventResult(m_nodeCreationPos, m_viewId, &GraphCanvas::ViewRequests::GetViewSceneCenter);

                GraphCanvas::GraphCanvasGraphicsView* graphicsView = nullptr;
                GraphCanvas::ViewRequestBus::EventResult(graphicsView, m_viewId, &GraphCanvas::ViewRequests::AsGraphicsView);

                m_viewportRectangle = graphicsView->mapToScene(graphicsView->viewport()->geometry()).boundingRect();
            }
            
            bool ShouldProcessItem([[maybe_unused]] const GraphCanvas::NodePaletteTreeItem* nodePaletteTreeItem) const
            {
                return true;
            }
            
            void ProcessItem(const GraphCanvas::NodePaletteTreeItem* nodePaletteTreeItem)
            {
                GraphCanvas::GraphCanvasMimeEvent* mimeEvent = nodePaletteTreeItem->CreateMimeEvent();

                if (ScriptCanvasEditor::MultiCreateNodeMimeEvent* multiCreateMimeEvent = azrtti_cast<ScriptCanvasEditor::MultiCreateNodeMimeEvent*>(mimeEvent))
                {
                    AZStd::vector< GraphCanvas::GraphCanvasMimeEvent* > mimeEvents = multiCreateMimeEvent->CreateMimeEvents();

                    for (GraphCanvas::GraphCanvasMimeEvent* currentEvent : mimeEvents)
                    {
                        DeveloperUtils::HandleMimeEvent(currentEvent, m_graphCanvasGraphId, m_viewportRectangle, m_widthOffset, m_heightOffset, m_maxRowHeight, m_minorPitch);
                        delete currentEvent;
                    }
                }
                else if (mimeEvent)
                {
                    DeveloperUtils::HandleMimeEvent(mimeEvent, m_graphCanvasGraphId, m_viewportRectangle, m_widthOffset, m_heightOffset, m_maxRowHeight, m_minorPitch);
                }
            }
        
        private:
        
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
        
        void NodePaletteFullCreationAction()
        {   
            ScriptCanvasEditor::AutomationRequestBus::Broadcast(&ScriptCanvasEditor::AutomationRequests::SignalAutomationBegin);
            
            FullCreationNodePaletteInterface nodePaletteInterface;
            DeveloperUtils::ProcessNodePalette(nodePaletteInterface);

            ScriptCanvasEditor::AutomationRequestBus::Broadcast(&ScriptCanvasEditor::AutomationRequests::SignalAutomationEnd);
        }

        QAction* CreateNodePaletteFullCreationAction(QMenu* mainMenu)
        {
            QAction* createNodePaletteAction = nullptr;

            if (mainMenu)
            {
                createNodePaletteAction = mainMenu->addAction(QAction::tr("Create Node Palette"));
                createNodePaletteAction->setAutoRepeat(false);
                createNodePaletteAction->setToolTip("Tries to create every node in the node palette. All of them. At once.");
                createNodePaletteAction->setShortcut(QKeySequence(QAction::tr("Ctrl+Shift+h", "Debug|Create Node Palette")));

                QObject::connect(createNodePaletteAction, &QAction::triggered, &NodePaletteFullCreationAction);
            }

            return createNodePaletteAction;
        }
    }
}
