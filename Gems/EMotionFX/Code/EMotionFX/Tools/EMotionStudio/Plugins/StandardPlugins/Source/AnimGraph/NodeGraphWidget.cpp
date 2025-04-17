/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/AnimGraphNodeGroup.h>
#include <MCore/Source/LogManager.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphConnectionCommands.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionManager.h>
#include <Editor/AnimGraphEditorBus.h>
#include <EMotionStudio/EMStudioSDK/Source/MainWindow.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendTreeVisualNode.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NodeConnection.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NodeGraph.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NodeGraphWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/StateGraphNode.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionSetsWindow/MotionSetsWindowPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TimeViewPlugin.h>
#include <QMouseEvent>
#include <QPainter>


namespace EMStudio
{
    //NodeGraphWidget::NodeGraphWidget(NodeGraph* activeGraph, QWidget* parent) : QGLWidget(QGLFormat(QGL::SampleBuffers), parent)
    NodeGraphWidget::NodeGraphWidget(AnimGraphPlugin* plugin, NodeGraph* activeGraph, QWidget* parent)
        : QOpenGLWidget(parent)
        , QOpenGLFunctions()
    {
        setObjectName("NodeGraphWidget");

        m_plugin = plugin;
        m_fontMetrics = new QFontMetrics(m_font);

        // update the active graph
        SetActiveGraph(activeGraph);

        // enable mouse tracking to capture mouse movements over the widget
        setMouseTracking(true);

        // init members
        m_showFps            = false;
        m_leftMousePressed   = false;
        m_panning            = false;
        m_middleMousePressed = false;
        m_rightMousePressed  = false;
        m_rectSelecting      = false;
        m_shiftPressed       = false;
        m_controlPressed     = false;
        m_altPressed         = false;
        m_moveNode           = nullptr;
        m_mouseLastPos       = QPoint(0, 0);
        m_mouseLastPressPos  = QPoint(0, 0);
        m_mousePos           = QPoint(0, 0);

        // setup to get focus when we click or use the mouse wheel
        setFocusPolicy((Qt::FocusPolicy)(Qt::ClickFocus | Qt::WheelFocus));

        // accept drops
        setAcceptDrops(true);

        setAutoFillBackground(false);
        setAttribute(Qt::WA_OpaquePaintEvent);

        m_curWidth       = geometry().width();
        m_curHeight      = geometry().height();
        m_prevWidth      = m_curWidth;
        m_prevHeight     = m_curHeight;
    }


    // destructor
    NodeGraphWidget::~NodeGraphWidget()
    {
        // delete the overlay font metrics
        delete m_fontMetrics;
    }


    // initialize opengl
    void NodeGraphWidget::initializeGL()
    {
        initializeOpenGLFunctions();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    }


    //
    void NodeGraphWidget::resizeGL(int w, int h)
    {
        static QPoint sizeDiff(0, 0);

        m_curWidth = w;
        m_curHeight = h;

        // specify the center of the window, so that that is the origin
        if (m_activeGraph)
        {
            m_activeGraph->SetScalePivot(QPoint(w / 2, h / 2));

            QPoint  scrollOffset    = m_activeGraph->GetScrollOffset();
            int32   scrollOffsetX   = scrollOffset.x();
            int32   scrollOffsetY   = scrollOffset.y();

            // calculate the size delta
            QPoint  oldSize         = QPoint(m_prevWidth, m_prevHeight);
            QPoint  size            = QPoint(w, h);
            QPoint  diff            = oldSize - size;
            sizeDiff += diff;

            const int32 sizeDiffX       = sizeDiff.x();
            const int32 halfSizeDiffX   = sizeDiffX / 2;

            const int32 sizeDiffY       = sizeDiff.y();
            const int32 halfSizeDiffY   = sizeDiffY / 2;

            if (halfSizeDiffX != 0)
            {
                scrollOffsetX -= halfSizeDiffX;
                const int32 modRes = halfSizeDiffX % 2;
                sizeDiff.setX(modRes);
            }

            if (halfSizeDiffY != 0)
            {
                scrollOffsetY -= halfSizeDiffY;
                const int32 modRes = halfSizeDiffY % 2;
                sizeDiff.setY(modRes);
            }

            m_activeGraph->SetScrollOffset(QPoint(scrollOffsetX, scrollOffsetY));
            //MCore::LOG("%d, %d", scrollOffsetX, scrollOffsetY);
        }

        QOpenGLWidget::resizeGL(w, h);

        m_prevWidth = w;
        m_prevHeight = h;
    }


    // set the active graph
    void NodeGraphWidget::SetActiveGraph(NodeGraph* graph)
    {
        if (m_activeGraph == graph)
        {
            return;
        }

        if (m_activeGraph)
        {
            m_activeGraph->StopCreateConnection();
            m_activeGraph->StopRelinkConnection();
            m_activeGraph->StopReplaceTransitionHead();
            m_activeGraph->StopReplaceTransitionTail();
        }

        m_activeGraph = graph;
        m_moveNode = nullptr;

        emit ActiveGraphChanged();
    }


    // get the active graph
    NodeGraph* NodeGraphWidget::GetActiveGraph() const
    {
        return m_activeGraph;
    }


    // paint event
    void NodeGraphWidget::paintGL()
    {
        if (visibleRegion().isEmpty())
        {
            return;
        }

        if (PreparePainting() == false)
        {
            return;
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // calculate the time passed since the last render
        const float timePassedInSeconds = m_renderTimer.StampAndGetDeltaTimeInSeconds();

        // start painting
        QPainter painter(this);

        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::TextAntialiasing);

        // get the width and height
        const uint32 width  = m_curWidth;
        const uint32 height = m_curHeight;

        // fill the background
        //painter.fillRect( event->rect(), QColor(30, 30, 30) );
        painter.setBrush(QColor(47, 47, 47));
        painter.setPen(Qt::NoPen);
        painter.drawRect(QRect(0, 0, width, height));

        // render the active graph
        if (m_activeGraph)
        {
            m_activeGraph->Render(m_plugin->GetAnimGraphModel().GetSelectionModel(), painter, width, height, m_mousePos, timePassedInSeconds);
        }

        // render selection rect
        if (m_rectSelecting)
        {
            painter.resetTransform();
            QRect selectRect;
            CalcSelectRect(selectRect);

            if (m_altPressed)
            {
                painter.setBrush(QColor(0, 100, 200, 75));
                painter.setPen(QColor(0, 100, 255));
            }
            else
            {
                painter.setBrush(QColor(200, 120, 0, 75));
                painter.setPen(QColor(255, 128, 0));
            }

            painter.drawRect(selectRect);
        }

        // draw the overlay
        OnDrawOverlay(painter);

        // render the callback overlay
        if (m_activeGraph)
        {
            painter.resetTransform();
            m_activeGraph->DrawOverlay(painter);
        }

        // draw the border
        QColor borderColor(0, 0, 0);
        float borderWidth = 2.0f;
        if (hasFocus())
        {
            borderColor = QColor(244, 156, 28);
            borderWidth = 3.0f;
        }
        if (m_borderOverwrite)
        {
            borderColor = m_borderOverwriteColor;
            borderWidth = m_borderOverwriteWidth;
        }

        QPen pen(borderColor, borderWidth);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);

        painter.resetTransform();
        painter.drawLine(0, 0, width, 0);
        painter.drawLine(width, 0, width, height);
        painter.drawLine(0, 0, 0, height);
        painter.drawLine(0, height, width, height);

        //painter.setPen( Qt::NoPen );
        //painter.setBrush( Qt::NoBrush );
        //painter.setBackgroundMode(Qt::TransparentMode);

        // render FPS counter
        if (m_showFps)
        {
            // get the time delta between the current time and the last frame
            static AZ::Debug::Timer perfTimer;
            static float perfTimeDelta = perfTimer.StampAndGetDeltaTimeInSeconds();
            perfTimeDelta = perfTimer.StampAndGetDeltaTimeInSeconds();

            static float fpsTimeElapsed = 0.0f;
            static uint32 fpsNumFrames = 0;
            static uint32 lastFPS = 0;
            fpsTimeElapsed += perfTimeDelta;
            const float renderTime = m_renderTimer.StampAndGetDeltaTimeInSeconds() * 1000.0f;
            fpsNumFrames++;
            if (fpsTimeElapsed > 1.0f)
            {
                lastFPS         = fpsNumFrames;
                fpsTimeElapsed  = 0.0f;
                fpsNumFrames    = 0;
            }

            static AZStd::string perfTempString;
            perfTempString = AZStd::string::format("%i FPS (%.1f ms)", lastFPS, renderTime);

            GraphNode::RenderText(painter, perfTempString.c_str(), QColor(150, 150, 150), m_font, *m_fontMetrics, Qt::AlignRight, QRect(width - 55, height - 20, 50, 20));
        }

        // show the info to which actor the currently rendered graph belongs to
        const CommandSystem::SelectionList& selectionList = CommandSystem::GetCommandManager()->GetCurrentSelection();

        if ((EMotionFX::GetActorManager().GetNumActorInstances() > 1) && (selectionList.GetNumSelectedActorInstances() > 0))
        {
            // get the first of the multiple selected actor instances
            EMotionFX::ActorInstance* firstActorInstance = selectionList.GetFirstActorInstance();

            // update the stored short filename without path
            if (m_fullActorName != firstActorInstance->GetActor()->GetFileName())
            {
                AzFramework::StringFunc::Path::GetFileName(firstActorInstance->GetActor()->GetFileNameString().c_str(), m_actorName);
            }

            m_tempString = AZStd::string::format("Showing graph for ActorInstance with ID %d and Actor file \"%s\"", firstActorInstance->GetID(), m_actorName.c_str());
            GraphNode::RenderText(painter, m_tempString.c_str(), QColor(150, 150, 150), m_font, *m_fontMetrics, Qt::AlignLeft, QRect(8, 0, 50, 20));
        }
    }


    // convert to a global position
    QPoint NodeGraphWidget::LocalToGlobal(const QPoint& inPoint) const
    {
        if (m_activeGraph)
        {
            return m_activeGraph->GetTransform().inverted().map(inPoint);
        }

        return inPoint;
    }


    // convert to a local position
    QPoint NodeGraphWidget::GlobalToLocal(const QPoint& inPoint) const
    {
        if (m_activeGraph)
        {
            return m_activeGraph->GetTransform().map(inPoint);
        }

        return inPoint;
    }


    QPoint NodeGraphWidget::SnapLocalToGrid(const QPoint& inPoint) const
    {
        QPoint snapped;
        snapped.setX(inPoint.x() - inPoint.x() % s_snapCellSize);
        snapped.setY(inPoint.y() - inPoint.y() % s_snapCellSize);
        return snapped;
    }

    // mouse is moving over the widget
    void NodeGraphWidget::mouseMoveEvent(QMouseEvent* event)
    {
        if (m_activeGraph == nullptr)
        {
            return;
        }

        // get the mouse position, calculate the global mouse position and update the relevant data
        QPoint mousePos = event->pos();

        QPoint snapDelta(0, 0);
        if (m_moveNode && m_leftMousePressed && m_panning == false && m_rectSelecting == false)
        {
            QPoint oldTopRight = m_moveNode->GetRect().topRight();
            QPoint scaledMouseDelta = (mousePos - m_mouseLastPos) * (1.0f / m_activeGraph->GetScale());
            QPoint unSnappedTopRight = oldTopRight + scaledMouseDelta;
            QPoint snappedTopRight = SnapLocalToGrid(unSnappedTopRight);
            snapDelta = snappedTopRight - unSnappedTopRight;
        }

        mousePos += snapDelta * m_activeGraph->GetScale();
        QPoint delta        = (mousePos - m_mouseLastPos) * (1.0f / m_activeGraph->GetScale());
        m_mouseLastPos       = mousePos;
        QPoint globalPos    = LocalToGlobal(mousePos);
        SetMousePos(globalPos);

        //if (delta.x() > 0 || delta.x() < -0 || delta.y() > 0 || delta.y() < -0)
        if (delta.x() != 0 || delta.y() != 0)
        {
            m_allowContextMenu = false;
        }

        // update modifiers
        m_altPressed     = event->modifiers() & Qt::AltModifier;
        m_shiftPressed   = event->modifiers() & Qt::ShiftModifier;
        m_controlPressed = event->modifiers() & Qt::ControlModifier;

        /*GraphNode* node = */ UpdateMouseCursor(mousePos, globalPos);
        if (m_rectSelecting == false && m_moveNode == nullptr &&
            m_plugin->GetActionFilter().m_editConnections &&
            !m_activeGraph->IsInReferencedGraph())
        {
            // check if we are clicking on a port
            GraphNode*  portNode    = nullptr;
            NodePort*   port        = nullptr;
            AZ::u16     portNr      = InvalidIndex16;
            bool        isInputPort = true;
            port = m_activeGraph->FindPort(globalPos.x(), globalPos.y(), &portNode, &portNr, &isInputPort);

            // check if we are adjusting a transition head or tail
            if (m_activeGraph->GetIsRepositioningTransitionHead() || m_activeGraph->GetIsRepositioningTransitionTail())
            {
                NodeConnection* connection = m_activeGraph->GetRepositionedTransitionHead();
                if (connection == nullptr)
                {
                    connection = m_activeGraph->GetRepositionedTransitionTail();
                }

                MCORE_ASSERT(connection->GetType() == StateConnection::TYPE_ID);
                StateConnection* stateConnection = static_cast<StateConnection*>(connection);

                EMotionFX::AnimGraphStateTransition* transition = connection->GetModelIndex().data(AnimGraphModel::ROLE_TRANSITION_POINTER).value<EMotionFX::AnimGraphStateTransition*>();
                if (transition)
                {
                    // check if our mouse is-over a node
                    GraphNode* hoveredNode = m_activeGraph->FindNode(mousePos);
                    if (hoveredNode == nullptr && portNode)
                    {
                        hoveredNode = portNode;
                    }

                    if (m_activeGraph->GetIsRepositioningTransitionHead())
                    {
                        // when adjusting the arrow head and we are over the source node with the mouse
                        if (hoveredNode
                            && hoveredNode != stateConnection->GetSourceNode()
                            && CheckIfIsValidTransition(stateConnection->GetSourceNode(), hoveredNode))
                        {
                            stateConnection->SetTargetNode(hoveredNode);
                            m_activeGraph->SetReplaceTransitionValid(true);
                        }
                        else
                        {
                            m_activeGraph->SetReplaceTransitionValid(false);
                        }

                        GraphNode* targetNode = stateConnection->GetTargetNode();
                        if (targetNode)
                        {
                            const QPoint newEndOffset = globalPos - targetNode->GetRect().topLeft();
                            transition->SetVisualOffsets(transition->GetVisualStartOffsetX(), transition->GetVisualStartOffsetY(),
                                newEndOffset.x(), newEndOffset.y());
                        }
                    }
                    else if (m_activeGraph->GetIsRepositioningTransitionTail())
                    {
                        // when adjusting the arrow tail and we are over the target node with the mouse
                        if (hoveredNode
                            && hoveredNode != stateConnection->GetTargetNode()
                            && CheckIfIsValidTransition(hoveredNode, stateConnection->GetTargetNode()))
                        {
                            stateConnection->SetSourceNode(hoveredNode);
                            m_activeGraph->SetReplaceTransitionValid(true);
                        }
                        else
                        {
                            m_activeGraph->SetReplaceTransitionValid(false);
                        }

                        GraphNode* sourceNode = stateConnection->GetSourceNode();
                        if (sourceNode)
                        {
                            const QPoint newStartOffset = globalPos - sourceNode->GetRect().topLeft();
                            transition->SetVisualOffsets(newStartOffset.x(), newStartOffset.y(),
                                transition->GetVisualEndOffsetX(), transition->GetVisualEndOffsetY());
                        }
                    }
                }
            }

            // connection relinking or creation
            if (port)
            {
                if (m_activeGraph->GetIsCreatingConnection())
                {
                    const bool isValid = CheckIfIsCreateConnectionValid(portNr, portNode, port, isInputPort);
                    m_activeGraph->SetCreateConnectionIsValid(isValid);
                    m_activeGraph->SetTargetPort(port);
                    //update();
                    return;
                }
                else if (m_activeGraph->GetIsRelinkingConnection())
                {
                    bool isValid = NodeGraph::CheckIfIsRelinkConnectionValid(m_activeGraph->GetRelinkConnection(), portNode, portNr, isInputPort);
                    m_activeGraph->SetCreateConnectionIsValid(isValid);
                    m_activeGraph->SetTargetPort(port);
                    //update();
                    return;
                }
                else
                {
                    if ((isInputPort && portNode->GetCreateConFromOutputOnly() == false) || isInputPort == false)
                    {
                        UpdateMouseCursor(mousePos, globalPos);
                        //update();
                        return;
                    }
                }
            }
            else
            {
                m_activeGraph->SetTargetPort(nullptr);
            }
        }

        // if we are panning
        if (m_panning)
        {
            // handle mouse wrapping, to enable smoother panning
            bool mouseWrapped = false;
            if (event->x() > (int32)width())
            {
                mouseWrapped = true;
                QCursor::setPos(QPoint(event->globalX() - width(), event->globalY()));
                m_mouseLastPos = QPoint(event->x() - width(), event->y());
            }
            else if (event->x() < 0)
            {
                mouseWrapped = true;
                QCursor::setPos(QPoint(event->globalX() + width(), event->globalY()));
                m_mouseLastPos = QPoint(event->x() + width(), event->y());
            }

            if (event->y() > (int32)height())
            {
                mouseWrapped = true;
                QCursor::setPos(QPoint(event->globalX(), event->globalY() - height()));
                m_mouseLastPos = QPoint(event->x(), event->y() - height());
            }
            else if (event->y() < 0)
            {
                mouseWrapped = true;
                QCursor::setPos(QPoint(event->globalX(), event->globalY() + height()));
                m_mouseLastPos = QPoint(event->x(), event->y() + height());
            }

            // don't apply the delta, if mouse has been wrapped
            if (mouseWrapped)
            {
                delta = QPoint(0, 0);
            }

            if (m_activeGraph)
            {
                // scrolling
                if (m_altPressed == false)
                {
                    QPoint newOffset = m_activeGraph->GetScrollOffset();
                    newOffset += delta;
                    m_activeGraph->SetScrollOffset(newOffset);
                    m_activeGraph->StopAnimatedScroll();
                    UpdateMouseCursor(mousePos, globalPos);
                    //update();
                    return;
                }
                // zooming
                else
                {
                    // stop the automated zoom
                    m_activeGraph->StopAnimatedZoom();

                    // calculate the new scale value
                    const float scaleDelta = (delta.y() / 120.0f) * 0.2f;
                    float newScale = MCore::Clamp<float>(m_activeGraph->GetScale() + scaleDelta, m_activeGraph->GetLowestScale(), 1.0f);
                    m_activeGraph->SetScale(newScale);

                    // redraw the viewport
                    //update();
                }
            }
        }

        // if the left mouse button is pressed
        if (m_leftMousePressed)
        {
            if (m_moveNode)
            {
                if (m_activeGraph &&
                    m_plugin->GetActionFilter().m_editNodes &&
                    !m_activeGraph->IsInReferencedGraph())
                {
                    const AZStd::vector<GraphNode*> selectedGraphNodes = m_activeGraph->GetSelectedGraphNodes();
                    if (!selectedGraphNodes.empty())
                    {
                        // move all selected nodes
                        for (GraphNode* graphNode : selectedGraphNodes)
                        {
                            graphNode->MoveRelative(delta);
                        }
                    }
                    else if (m_moveNode)
                    {
                        m_moveNode->MoveRelative(delta);
                    }
                    return;
                }
            }
            else
            {
                //setCursor( Qt::ArrowCursor );

                if (m_rectSelecting)
                {
                    m_selectEnd = mousePos;
                }
            }

            // redraw viewport
            //update();
            return;
        }

        // if we're just moving around without pressing the left mouse button, change the cursor
        //UpdateMouseCursor( mousePos, globalPos );
        //update();
    }


    // mouse button has been pressed
    void NodeGraphWidget::mousePressEvent(QMouseEvent* event)
    {
        if (!m_activeGraph)
        {
            return;
        }

        GetMainWindow()->DisableUndoRedo();

        m_allowContextMenu = true;

        // get the mouse position, calculate the global mouse position and update the relevant data
        QPoint mousePos     = event->pos();
        m_mouseLastPos       = mousePos;
        m_mouseLastPressPos  = mousePos;
        QPoint globalPos    = LocalToGlobal(mousePos);
        SetMousePos(globalPos);

        // update modifiers
        m_altPressed     = event->modifiers() & Qt::AltModifier;
        m_shiftPressed   = event->modifiers() & Qt::ShiftModifier;
        m_controlPressed = event->modifiers() & Qt::ControlModifier;
        const AnimGraphActionFilter& actionFilter = m_plugin->GetActionFilter();

        // check if we can start panning
        if ((event->buttons() & Qt::RightButton && event->buttons() & Qt::LeftButton) || event->button() == Qt::RightButton || event->button() == Qt::MidButton)
        {
            // update button booleans
            if (event->buttons() & Qt::RightButton && event->buttons() & Qt::LeftButton)
            {
                m_leftMousePressed  = true;
                m_rightMousePressed = true;
            }

            if (event->button() == Qt::RightButton)
            {
                m_rightMousePressed = true;

                GraphNode* node = UpdateMouseCursor(mousePos, globalPos);
                if (node && node->GetCanVisualize() && node->GetIsInsideVisualizeRect(globalPos))
                {
                    OnSetupVisualizeOptions(node);
                    m_rectSelecting = false;
                    m_panning = false;
                    m_moveNode = nullptr;
                    //update();
                    return;
                }

                // Right click on the node will trigger a single selection, if the node is not already selected.
                if (node && !node->GetIsSelected() && !node->GetIsInsideArrowRect(globalPos))
                {
                    m_plugin->GetAnimGraphModel().GetSelectionModel().select(QItemSelection(node->GetModelIndex(), node->GetModelIndex()),
                        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                    return;
                }
            }

            if (event->button() == Qt::MidButton)
            {
                m_middleMousePressed = true;
            }

            m_panning = true;
            m_rectSelecting = false;
            setCursor(Qt::ClosedHandCursor);
            return;
        }

        // get the node we click on
        GraphNode* node = UpdateMouseCursor(mousePos, globalPos);

        // if we press the left mouse button
        if (event->button() == Qt::LeftButton)
        {
            m_leftMousePressed = true;

            // if we pressed the visualize icon
            GraphNode* orgNode = m_activeGraph->FindNode(mousePos);
            if (orgNode && orgNode->GetCanVisualize() && orgNode->GetIsInsideVisualizeRect(globalPos))
            {
                const bool viz = !orgNode->GetIsVisualized();
                orgNode->SetIsVisualized(viz);
                OnVisualizeToggle(orgNode, viz);
                m_rectSelecting = false;
                m_panning = false;
                m_moveNode = nullptr;
                //update();
                return;
            }

            // Get time view plugin.
            EMStudioPlugin* timeViewBasePlugin = EMStudio::GetPluginManager()->FindActivePlugin(TimeViewPlugin::CLASS_ID);
            TimeViewPlugin* timeViewPlugin = static_cast<TimeViewPlugin*>(timeViewBasePlugin);

            if (node)
            {
                // Cast the node.
                BlendTreeVisualNode* blendNode = static_cast<BlendTreeVisualNode*>(node);
                EMotionFX::AnimGraphNode* animGraphNode = blendNode->GetEMFXNode();

                // Collapse the node if possible (not possible in a state machine).
                if (azrtti_typeid(animGraphNode->GetParentNode()) != azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
                {
                    if (node->GetIsInsideArrowRect(globalPos))
                    {
                        node->SetIsCollapsed(!node->GetIsCollapsed());
                        OnNodeCollapsed(node, node->GetIsCollapsed());
                        UpdateMouseCursor(mousePos, globalPos);
                        //update();
                        return;
                    }
                }

                // Update time view if time view window is opened and animgraph node supports preview motion.
                if (timeViewPlugin)
                {
                    bool motionSelected = false;
                    if (animGraphNode->GetSupportsPreviewMotion())
                    {
                        EMotionFX::AnimGraphMotionNode* motionNode = static_cast<EMotionFX::AnimGraphMotionNode*>(animGraphNode);
                        if (motionNode->GetNumMotions() == 1)
                        {
                            const char* motionId = motionNode->GetMotionId(0);
                            EMotionFX::MotionSet::MotionEntry* motionEntry = MotionSetsWindowPlugin::FindBestMatchMotionEntryById(motionId);
                            if (motionEntry && motionEntry->GetMotion())
                            {
                                // Update motion list window to select motion.
                                EMStudioPlugin* motionSetBasePlugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionSetsWindowPlugin::CLASS_ID);
                                MotionSetsWindowPlugin* motionSetWindowPlugin = static_cast<MotionSetsWindowPlugin*>(motionSetBasePlugin);
                                if (motionSetWindowPlugin)
                                {
                                    motionSetWindowPlugin->GetMotionSetWindow()->Select(motionEntry);

                                    // Update time view plugin with new motion related data.
                                    timeViewPlugin->SetMode(TimeViewMode::Motion);

                                    motionSelected = true;
                                }
                            }
                        }
                    }

                    if (!motionSelected)
                    {
                        // If not clicked on another animgraph node, clear time view window.
                        timeViewPlugin->SetMode(TimeViewMode::AnimGraph);
                    }
                }
            }
            else if (timeViewPlugin)
            {
                // If clicked away from nodes, set time view window to animgraph mode.
                timeViewPlugin->SetMode(TimeViewMode::AnimGraph);
            }

            if (!m_activeGraph->IsInReferencedGraph())
            {
                // check if we are clicking on an input port
                GraphNode*  portNode = nullptr;
                NodePort*   port = nullptr;
                AZ::u16     portNr = InvalidIndex16;
                bool        isInputPort = true;
                port = m_activeGraph->FindPort(globalPos.x(), globalPos.y(), &portNode, &portNr, &isInputPort);
                if (port)
                {
                    m_moveNode = nullptr;
                    m_panning = false;
                    m_rectSelecting = false;

                    // relink existing connection
                    NodeConnection* connection = m_activeGraph->FindInputConnection(portNode, portNr);
                    if (actionFilter.m_editConnections &&
                        isInputPort && connection && portNode->GetType() != StateGraphNode::TYPE_ID)
                    {
                        connection->SetIsDashed(true);

                        UpdateMouseCursor(mousePos, globalPos);
                        m_activeGraph->StartRelinkConnection(connection, portNr, portNode);
                        return;
                    }

                    // create new connection
                    if ((isInputPort && portNode->GetCreateConFromOutputOnly() == false) || isInputPort == false)
                    {
                        if (actionFilter.m_createConnections &&
                            CheckIfIsValidTransitionSource(portNode))
                        {
                            QPoint offset = globalPos - portNode->GetRect().topLeft();
                            UpdateMouseCursor(mousePos, globalPos);
                            m_activeGraph->StartCreateConnection(portNr, isInputPort, portNode, port, offset);
                            //update();
                            return;
                        }
                    }
                }

                // check if we click on an transition arrow head or tail
                NodeConnection* connection = m_activeGraph->FindConnection(globalPos);
                if (actionFilter.m_editConnections &&
                    connection && connection->GetType() == StateConnection::TYPE_ID)
                {
                    StateConnection* stateConnection = static_cast<StateConnection*>(connection);
                    EMotionFX::AnimGraphStateTransition* transition = connection->GetModelIndex().data(AnimGraphModel::ROLE_TRANSITION_POINTER).value<EMotionFX::AnimGraphStateTransition*>();
                    if (transition)
                    {
                        const QPoint startOffset(transition->GetVisualStartOffsetX(), transition->GetVisualStartOffsetY());
                        const QPoint endOffset(transition->GetVisualEndOffsetX(), transition->GetVisualEndOffsetY());

                        if (!stateConnection->GetIsWildcardTransition() && stateConnection->CheckIfIsCloseToHead(globalPos))
                        {
                            m_moveNode = nullptr;
                            m_panning = false;
                            m_rectSelecting = false;

                            m_activeGraph->StartReplaceTransitionHead(stateConnection, startOffset, endOffset, stateConnection->GetSourceNode(), stateConnection->GetTargetNode());
                            return;
                        }

                        if (!stateConnection->GetIsWildcardTransition() && stateConnection->CheckIfIsCloseToTail(globalPos))
                        {
                            m_moveNode = nullptr;
                            m_panning = false;
                            m_rectSelecting = false;

                            m_activeGraph->StartReplaceTransitionTail(stateConnection, startOffset, endOffset, stateConnection->GetSourceNode(), stateConnection->GetTargetNode());
                            return;
                        }
                    }
                }
            }

            EMotionFX::AnimGraphNodeGroup* nodeGroup = m_activeGraph->FindNodeGroup(mousePos);

            if (node && m_shiftPressed)
            {
                OnShiftClickedNode(node);
            }
            else
            {
                // Start dragging the node when the mouse is moved
                if (node && m_shiftPressed == false && m_controlPressed == false && m_altPressed == false && actionFilter.m_editNodes &&
                    !m_activeGraph->IsInReferencedGraph())
                {
                    m_moveNode   = node;
                    m_panning    = false;
                    setCursor(Qt::ClosedHandCursor);
                }
                // Start dragging all nodes in the group when the mouse is moved
                else if (nodeGroup)
                {
                    // The node within the group which is assigned to m_moveNode is arbitrary,
                    // so pick the first one because it should always exist if the group exists
                    // (otherwise something has gone very wrong if there is a group without any nodes).
                    EMotionFX::AnimGraphNode* nodeInGroup = m_plugin->GetActiveAnimGraph()->RecursiveFindNodeById(nodeGroup->GetNode(0));
                    AZ_Assert(nodeInGroup, "No AnimGraphNode in clicked group");
                    m_moveNode = m_activeGraph->FindGraphNode(nodeInGroup);
                    m_panning = false;
                    setCursor(Qt::ClosedHandCursor);
                }
                // Not dragging any nodes to move
                else
                {
                    m_moveNode       = nullptr;
                    m_panning        = false;
                    m_rectSelecting  = true;
                    m_selectStart    = mousePos;
                    m_selectEnd      = m_selectStart;
                    setCursor(Qt::ArrowCursor);
                }
            }

            if (m_activeGraph)
            {
                // shift is used to activate a state, disable all selection behavior in case we press shift!
                // check if we clicked a node and additionally not clicked its arrow rect
                bool nodeClicked = false;
                if (node && !node->GetIsInsideArrowRect(globalPos))
                {
                    nodeClicked = true;
                }
                if (!m_shiftPressed)
                {
                    // check the node we're clicking on
                    if (!m_controlPressed)
                    {
                        // Reset the selection if either:
                        //   * Clicked on empty background
                        //   * Clicked node is not already selected
                        //   * Clicked node is in group
                        //
                        // When multiple nodes are selected, normally clicking and dragging one of them
                        // moves them all together, while clicking outside of a node clears the selection.
                        //
                        // However, this is a bit different in groups. With groups, clicking the group
                        // background area selects all nodes of the group, and dragging moves all the nodes
                        // together. After selecting a group, selecting a single node within the group
                        // requires clearing the selection first. Otherwise, the user would need to click
                        // empty space outside the group to clear the selection before being able to select
                        // the single node.
                        bool allNodesInGroupSelected = false;
                        if (nodeGroup && node)
                        {
                            allNodesInGroupSelected = true;
                            for (size_t n = 0; n < nodeGroup->GetNumNodes(); n++)
                            {
                                EMotionFX::AnimGraphNode* animGraphNode =
                                    m_plugin->GetActiveAnimGraph()->RecursiveFindNodeById(nodeGroup->GetNode(n));
                                AZ_Assert(animGraphNode, "No AnimGraphNode in group");

                                const EMStudio::GraphNode* graphNode = m_activeGraph->FindGraphNode(animGraphNode);
                                if (!m_plugin->GetAnimGraphModel().GetSelectionModel().isSelected(graphNode->GetModelIndex()))
                                {
                                    allNodesInGroupSelected = false;
                                    break;
                                }
                            }
                        }
                        if (!node || (node && !node->GetIsSelected()) || allNodesInGroupSelected)
                        {
                            m_plugin->GetAnimGraphModel().GetSelectionModel().clear();
                        }
                    }

                    if (nodeClicked && m_controlPressed)
                    {
                        m_plugin->GetAnimGraphModel().GetSelectionModel().select(
                            node->GetModelIndex(), QItemSelectionModel::Toggle | QItemSelectionModel::Rows);
                    }
                    else if (nodeClicked && !m_controlPressed)
                    {
                        m_plugin->GetAnimGraphModel().GetSelectionModel().select(
                            node->GetModelIndex(), QItemSelectionModel::Select | QItemSelectionModel::Rows);
                    }
                    else if (nodeGroup)
                    {
                        SelectNodesInGroup(nodeGroup);
                    }
                    // in case we didn't click on a node, check if we click on a connection
                    else if (!nodeClicked)
                    {
                        m_activeGraph->SelectConnectionCloseTo(LocalToGlobal(event->pos()), m_controlPressed == false, true);
                    }
                }
                else
                {
                    // in case shift and control are both pressed, special case!
                    if (m_controlPressed && node)
                    {
                        m_plugin->GetAnimGraphModel().GetSelectionModel().select(QItemSelection(node->GetModelIndex(), node->GetModelIndex()),
                            QItemSelectionModel::Current | QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                    }
                }
            }
        }
    }


    // mouse button has been released
    void NodeGraphWidget::mouseReleaseEvent(QMouseEvent* event)
    {
        // get the mouse position, calculate the global mouse position and update the relevant data
        const QPoint mousePos   = event->pos();
        const QPoint globalPos  = LocalToGlobal(mousePos);
        SetMousePos(globalPos);

        if (m_activeGraph == nullptr)
        {
            return;
        }

        GetMainWindow()->UpdateUndoRedo();

        // update modifiers
        m_altPressed     = event->modifiers() & Qt::AltModifier;
        m_shiftPressed   = event->modifiers() & Qt::ShiftModifier;
        m_controlPressed = event->modifiers() & Qt::ControlModifier;

        const AnimGraphActionFilter& actionFilter = m_plugin->GetActionFilter();

        // both left and right released at the same time
        if (event->buttons() & Qt::RightButton && event->buttons() & Qt::LeftButton)
        {
            m_rightMousePressed  = false;
            m_leftMousePressed   = false;
        }

        // right mouse button
        if (event->button() == Qt::RightButton)
        {
            m_rightMousePressed  = false;
            m_panning            = false;
            UpdateMouseCursor(mousePos, globalPos);
            //update();
            return;
        }

        // middle mouse button
        if (event->button() == Qt::MidButton)
        {
            m_middleMousePressed = false;
            m_panning            = false;
        }

        // if we release the left mouse button
        if (event->button() == Qt::LeftButton)
        {
            const bool mouseMoved = (event->pos() != m_mouseLastPressPos);

            // if we pressed the visualize icon
            GraphNode* node = UpdateMouseCursor(mousePos, globalPos);
            if (node && node->GetCanVisualize() && node->GetIsInsideVisualizeRect(globalPos))
            {
                m_rectSelecting = false;
                m_panning = false;
                m_moveNode = nullptr;
                m_leftMousePressed = false;
                UpdateMouseCursor(mousePos, globalPos);
                //update();
                return;
            }

            if (node && node->GetIsInsideArrowRect(globalPos))
            {
                m_rectSelecting = false;
                m_panning = false;
                m_moveNode = nullptr;
                m_leftMousePressed = false;
                UpdateMouseCursor(mousePos, globalPos);
                //update();
                return;
            }

            // if we were creating a connection
            if (m_activeGraph->GetIsCreatingConnection())
            {
                AZ_Assert(!m_activeGraph->IsInReferencedGraph(), "Expected to not be in a referenced graph");

                // create the connection if needed
                if (m_activeGraph->GetTargetPort())
                {
                    if (m_activeGraph->GetIsCreateConnectionValid())
                    {
                        AZ::u16     targetPortNr;
                        bool        targetIsInputPort;
                        GraphNode*  targetNode;

                        NodePort* targetPort = m_activeGraph->FindPort(globalPos.x(), globalPos.y(), &targetNode, &targetPortNr, &targetIsInputPort);
                        if (targetPort && m_activeGraph->GetTargetPort() == targetPort
                            && targetNode != m_activeGraph->GetCreateConnectionNode())
                        {
#ifndef MCORE_DEBUG
                            MCORE_UNUSED(targetPort);
#endif

                            QPoint endOffset = globalPos - targetNode->GetRect().topLeft();
                            m_activeGraph->SetCreateConnectionEndOffset(endOffset);

                            // trigger the callback
                            OnCreateConnection(m_activeGraph->GetCreateConnectionPortNr(),
                                m_activeGraph->GetCreateConnectionNode(),
                                m_activeGraph->GetCreateConnectionIsInputPort(),
                                targetPortNr,
                                targetNode,
                                targetIsInputPort,
                                m_activeGraph->GetCreateConnectionStartOffset(),
                                m_activeGraph->GetCreateConnectionEndOffset());
                        }
                    }
                }

                m_activeGraph->StopCreateConnection();
                m_leftMousePressed = false;
                UpdateMouseCursor(mousePos, globalPos);
                //update();
                return;
            }

            // if we were relinking a connection
            if (m_activeGraph->GetIsRelinkingConnection())
            {
                AZ_Assert(actionFilter.m_editConnections, "Expected edit connections being enabled.");
                AZ_Assert(!m_activeGraph->IsInReferencedGraph(), "Expected to not be in a referenced graph");

                // get the information from the current mouse position
                AZ::u16     newTargetPortNr;
                bool        newTargetIsInputPort;
                GraphNode*  newTargetNode;
                NodePort*   newTargetPort = m_activeGraph->FindPort(globalPos.x(), globalPos.y(), &newTargetNode, &newTargetPortNr, &newTargetIsInputPort);

                // relink existing connection
                NodeConnection* connection = nullptr;
                if (newTargetPort)
                {
                    connection = m_activeGraph->FindInputConnection(newTargetNode, newTargetPortNr);
                }

                NodeConnection* relinkedConnection = m_activeGraph->GetRelinkConnection();
                if (relinkedConnection)
                {
                    relinkedConnection->SetIsDashed(false);
                }

                if (newTargetNode && newTargetPort)
                {
                    // get the information from the old connection which we want to relink
                    GraphNode*      sourceNode          = relinkedConnection->GetSourceNode();
                    AZStd::string   sourceNodeName      = sourceNode->GetName();
                    AZ::u16         sourcePortNr        = relinkedConnection->GetOutputPortNr();
                    GraphNode*      oldTargetNode       = relinkedConnection->GetTargetNode();
                    AZStd::string   oldTargetNodeName   = oldTargetNode->GetName();
                    AZ::u16         oldTargetPortNr     = relinkedConnection->GetInputPortNr();

                    if (NodeGraph::CheckIfIsRelinkConnectionValid(relinkedConnection, newTargetNode, newTargetPortNr, newTargetIsInputPort))
                    {
                        const EMotionFX::AnimGraphNode* parentNode = newTargetNode->GetModelIndex().data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
                        EMotionFX::AnimGraph* animGraph = parentNode->GetAnimGraph();
                        AZ_Assert(animGraph, "Invalid anim graph");

                        // create the relink command group
                        MCore::CommandGroup commandGroup("Relink blend tree connection");

                        // check if there already is a connection plugged into the port where we want to put our new connection in
                        if (connection)
                        {
                            // construct the command
                            AZStd::string commandString;
                            commandString = AZStd::string::format("AnimGraphRemoveConnection -animGraphID %i -sourceNode \"%s\" -sourcePort %d -targetNode \"%s\" -targetPort %d",
                                    animGraph->GetID(),
                                    connection->GetSourceNode()->GetName(),
                                    connection->GetOutputPortNr(),
                                    connection->GetTargetNode()->GetName(),
                                    connection->GetInputPortNr());

                            // add it to the command group
                            commandGroup.AddCommandString(commandString.c_str());
                        }

                        MCORE_ASSERT(newTargetIsInputPort);
                        AZStd::string newTargetNodeName = newTargetNode->GetName();
                        CommandSystem::RelinkConnectionTarget(&commandGroup, animGraph->GetID(), sourceNodeName.c_str(), sourcePortNr, oldTargetNodeName.c_str(), oldTargetPortNr, newTargetNodeName.c_str(), newTargetPortNr);

                        // call this before calling the commands as the commands will trigger a graph update
                        m_activeGraph->StopRelinkConnection();

                        // execute the command group
                        AZStd::string commandResult;
                        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, commandResult) == false)
                        {
                            if (commandResult.empty() == false)
                            {
                                MCore::LogError(commandResult.c_str());
                            }
                        }
                    }
                }

                m_activeGraph->StopRelinkConnection();
                m_leftMousePressed = false;
                UpdateMouseCursor(mousePos, globalPos);
                return;
            }

            // in case we adjusted a transition start or end point
            if (m_activeGraph->GetIsRepositioningTransitionHead() || m_activeGraph->GetIsRepositioningTransitionTail())
            {
                AZ_Assert(actionFilter.m_editConnections, "Expected edit connections being enabled.");
                AZ_Assert(!m_activeGraph->IsInReferencedGraph(), "Expected to not be in a referenced graph");

                NodeConnection* connection;
                QPoint oldStartOffset, oldEndOffset;
                GraphNode* oldSourceNode;
                GraphNode* oldTargetNode;
                m_activeGraph->GetReplaceTransitionInfo(&connection, &oldStartOffset, &oldEndOffset, &oldSourceNode, &oldTargetNode);
                GraphNode* newDropNode = m_activeGraph->FindNode(event->pos());

                if (newDropNode && newDropNode != oldSourceNode)
                {
                    if (m_activeGraph->GetIsRepositioningTransitionHead())
                    {
                        ReplaceTransition(connection, oldStartOffset, oldEndOffset, oldSourceNode, oldTargetNode, oldSourceNode, newDropNode);
                        m_activeGraph->StopReplaceTransitionHead();
                    }
                    else if (m_activeGraph->GetIsRepositioningTransitionTail() && newDropNode != oldTargetNode)
                    {
                        ReplaceTransition(connection, oldStartOffset, oldEndOffset, oldSourceNode, oldTargetNode, newDropNode, oldTargetNode);
                        m_activeGraph->StopReplaceTransitionTail();
                    }
                }
                else
                {
                    ReplaceTransition(connection, oldStartOffset, oldEndOffset, oldSourceNode, oldTargetNode, oldSourceNode, oldTargetNode);
                    if (m_activeGraph->GetIsRepositioningTransitionHead())
                    {
                        m_activeGraph->StopReplaceTransitionHead();
                    }
                    else if (m_activeGraph->GetIsRepositioningTransitionTail())
                    {
                        m_activeGraph->StopReplaceTransitionTail();
                    }
                }
                return;
            }

            // if we are finished moving, trigger the OnMoveNode callbacks
            if (m_moveNode && mouseMoved &&
                actionFilter.m_editNodes &&
                !m_activeGraph->IsInReferencedGraph())
            {
                OnMoveStart();

                bool moveNodeSelected = false; // prevent moving the same node twice
                const AZStd::vector<GraphNode*> selectedNodes = m_activeGraph->GetSelectedGraphNodes();
                for (GraphNode* currentNode : selectedNodes)
                {
                    OnMoveNode(currentNode, currentNode->GetRect().topLeft().x(), currentNode->GetRect().topLeft().y());
                    if (currentNode == m_moveNode)
                    {
                        moveNodeSelected = true;
                    }
                }
                if (!moveNodeSelected && !m_moveNode)
                {
                    OnMoveNode(m_moveNode, m_moveNode->GetRect().topLeft().x(), m_moveNode->GetRect().topLeft().y());
                }

                OnMoveEnd();
            }

            m_panning = false;
            m_moveNode = nullptr;
            UpdateMouseCursor(mousePos, globalPos);

            // get the node we click on
            node = UpdateMouseCursor(mousePos, globalPos);

            if (m_rectSelecting && mouseMoved)
            {
                // calc the selection rect
                QRect selectRect;
                CalcSelectRect(selectRect);

                // select things inside it
                if (selectRect.isEmpty() == false && m_activeGraph)
                {
                    selectRect = m_activeGraph->GetTransform().inverted().mapRect(selectRect);

                    // select nodes when alt is not pressed
                    if (m_altPressed == false)
                    {
                        const bool overwriteSelection = (m_controlPressed == false);
                        m_activeGraph->SelectNodesInRect(selectRect, overwriteSelection, m_controlPressed);
                    }
                    else // zoom into the selected rect
                    {
                        m_activeGraph->ZoomOnRect(selectRect, geometry().width(), geometry().height(), true);
                    }
                }
            }

            m_leftMousePressed = false;
            m_rectSelecting = false;
        }
    }


    // double click
    void NodeGraphWidget::mouseDoubleClickEvent(QMouseEvent* event)
    {
        // only do things when a graph is active
        if (m_activeGraph == nullptr)
        {
            return;
        }

        GetMainWindow()->DisableUndoRedo();

        // get the mouse position, calculate the global mouse position and update the relevant data
        QPoint mousePos     = event->pos();
        QPoint globalPos    = LocalToGlobal(mousePos);
        SetMousePos(globalPos);

        // left mouse button double clicked
        if (event->button() == Qt::LeftButton)
        {
            // check if double clicked on a node
            GraphNode* node = m_activeGraph->FindNode(mousePos);
            EMotionFX::AnimGraphNodeGroup* nodeGroup = m_activeGraph->FindNodeGroup(mousePos);

            if (nodeGroup)
            {
                if (m_activeGraph->CheckInsideNodeGroupTitleRect(nodeGroup, mousePos) && !nodeGroup->IsNameEditOngoing())
                {
                    m_activeGraph->EnableNameEditForNodeGroup(nodeGroup);
                }
            }
            else if (node == nullptr)
            {
                // if we didn't double click on a node zoom in to the clicked area
                m_activeGraph->ScrollTo(-LocalToGlobal(mousePos) + geometry().center());
                m_activeGraph->ZoomIn();
            }
        }

        // right mouse button double clicked
        if (event->button() == Qt::RightButton)
        {
            // check if double clicked on a node
            GraphNode* node = m_activeGraph->FindNode(mousePos);
            if (node == nullptr)
            {
                m_activeGraph->ScrollTo(-LocalToGlobal(mousePos) + geometry().center());
                m_activeGraph->ZoomOut();
            }
        }

        setCursor(Qt::ArrowCursor);

        // reset flags
        m_rectSelecting = false;

        // redraw the viewport
        //update();
    }


    // when the mouse wheel changes
    void NodeGraphWidget::wheelEvent(QWheelEvent* event)
    {
        // only do things when a graph is active
        if (m_activeGraph == nullptr)
        {
            return;
        }

        // get the mouse position, calculate the global mouse position and update the relevant data

        // For some reason this call fails to get the correct mouse position
        // It might be relative to the window?  Jira generated.
        //        QPoint mousePos     = event->pos();

        QPoint globalQtMousePos = event->globalPosition().toPoint();
        QPoint globalQtMousePosInWidget = this->mapFromGlobal(globalQtMousePos);
        QPoint globalPos    = LocalToGlobal(globalQtMousePosInWidget);

        SetMousePos(globalPos);

        // stop the automated zoom
        m_activeGraph->StopAnimatedZoom();

        // calculate the new scale value
        const float scaleDelta = (event->angleDelta().y() / 120.0f) * 0.05f;
        float newScale = MCore::Clamp<float>(m_activeGraph->GetScale() + scaleDelta, m_activeGraph->GetLowestScale(), 1.0f);
        m_activeGraph->SetScale(newScale);

        // redraw the viewport
        //update();
    }


    // update the mouse cursor, based on if we hover over a given node or not
    GraphNode* NodeGraphWidget::UpdateMouseCursor(const QPoint& localMousePos, const QPoint& globalMousePos)
    {
        // if there is no active graph
        if (m_activeGraph == nullptr)
        {
            setCursor(Qt::ArrowCursor);
            return nullptr;
        }

        if (m_panning || m_moveNode)
        {
            setCursor(Qt::ClosedHandCursor);
            return nullptr;
        }

        // check if we hover above a node
        GraphNode* node = m_activeGraph->FindNode(localMousePos);

        // check if the node is valid
        // we test firstly the node to have the visualize cursor correct
        if (node)
        {
            // cast the node
            BlendTreeVisualNode* blendNode = static_cast<BlendTreeVisualNode*>(node);
            EMotionFX::AnimGraphNode* animGraphNode = blendNode->GetEMFXNode();

            // check if the node is part of a state machine, on this case it's not collapsible, the arrow is not needed to be checked
            if (azrtti_typeid(animGraphNode->GetParentNode()) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
            {
                // if the mouse is over a node but NOT over the visualize icon
                if (node->GetIsInsideVisualizeRect(globalMousePos))
                {
                    setCursor(Qt::ArrowCursor);
                    return node;
                }
            }
            else
            {
                // if the mouse is over a node but NOT over the arrow rect or the visualize icon
                if (node->GetIsInsideArrowRect(globalMousePos) || (node->GetCanVisualize() && node->GetIsInsideVisualizeRect(globalMousePos)))
                {
                    setCursor(Qt::ArrowCursor);
                    return node;
                }
            }

            // check if we're hovering over a port
            AZ::u16     portNr;
            GraphNode*  portNode;
            bool        isInputPort;
            NodePort* nodePort = m_activeGraph->FindPort(globalMousePos.x(), globalMousePos.y(), &portNode, &portNr, &isInputPort);
            if (nodePort)
            {
                if ((isInputPort && portNode->GetCreateConFromOutputOnly() == false) || isInputPort == false)
                {
                    setCursor(Qt::PointingHandCursor);
                    return nullptr;
                }
            }

            // set the hand cursor if we are only hovering a node
            setCursor(Qt::OpenHandCursor);
            return node;
        }
        else // not hovering a node, simply check for ports
        {
            // check if we're hovering over a port
            AZ::u16     portNr;
            GraphNode*  portNode;
            bool        isInputPort;
            NodePort* nodePort = m_activeGraph->FindPort(globalMousePos.x(), globalMousePos.y(), &portNode, &portNr, &isInputPort);
            if (nodePort)
            {
                if ((isInputPort && portNode->GetCreateConFromOutputOnly() == false) || isInputPort == false)
                {
                    setCursor(Qt::PointingHandCursor);
                    return nullptr;
                }
            }
        }

        // the default cursor is the arrow
        setCursor(Qt::ArrowCursor);
        return node;
    }


    // resize
    void NodeGraphWidget::resizeEvent(QResizeEvent* event)
    {
        QOpenGLWidget::resizeEvent(event);
    }


    // calculate the selection rect
    void NodeGraphWidget::CalcSelectRect(QRect& outRect)
    {
        const int32 startX = MCore::Min<int32>(m_selectStart.x(), m_selectEnd.x());
        const int32 startY = MCore::Min<int32>(m_selectStart.y(), m_selectEnd.y());
        const int32 width  = abs(m_selectEnd.x() - m_selectStart.x());
        const int32 height = abs(m_selectEnd.y() - m_selectStart.y());

        outRect = QRect(startX, startY, width, height);
    }


    // on keypress
    void NodeGraphWidget::keyPressEvent(QKeyEvent* event)
    {
        switch (event->key())
        {
        case Qt::Key_Shift:
        {
            m_shiftPressed     = true;
            break;
        }
        case Qt::Key_Control:
        {
            m_controlPressed   = true;
            break;
        }
        case Qt::Key_Alt:
        {
            m_altPressed       = true;
            break;
        }
        }

        event->ignore();
    }

    // on key release
    void NodeGraphWidget::keyReleaseEvent(QKeyEvent* event)
    {
        switch (event->key())
        {
        case Qt::Key_Shift:
        {
            m_shiftPressed     = false;
            break;
        }
        case Qt::Key_Control:
        {
            m_controlPressed   = false;
            break;
        }
        case Qt::Key_Alt:
        {
            m_altPressed       = false;
            break;
        }
        }

        event->ignore();
    }


    // receiving focus
    void NodeGraphWidget::focusInEvent(QFocusEvent* event)
    {
        MCORE_UNUSED(event);
        grabKeyboard();
        //update();

        EMotionFX::AnimGraphEditorNotificationBus::Broadcast(&EMotionFX::AnimGraphEditorNotificationBus::Events::OnFocusIn);
    }


    // out of focus
    void NodeGraphWidget::focusOutEvent(QFocusEvent* event)
    {
        MCORE_UNUSED(event);
        m_shiftPressed   = false;
        m_controlPressed = false;
        m_altPressed     = false;
        releaseKeyboard();

        if (m_activeGraph && m_activeGraph->GetIsCreatingConnection())
        {
            m_activeGraph->StopCreateConnection();
            m_leftMousePressed = false;
        }
    }


    // return the number of selected nodes
    size_t NodeGraphWidget::CalcNumSelectedNodes() const
    {
        if (m_activeGraph)
        {
            return m_activeGraph->CalcNumSelectedNodes();
        }
        return 0;
    }


    // is the given connection valid
    bool NodeGraphWidget::CheckIfIsCreateConnectionValid(AZ::u16 portNr, GraphNode* portNode, NodePort* port, bool isInputPort)
    {
        MCORE_UNUSED(portNr);
        MCORE_UNUSED(port);

        if (m_activeGraph == nullptr)
        {
            return false;
        }

        GraphNode* sourceNode = m_activeGraph->GetCreateConnectionNode();
        GraphNode* targetNode = portNode;

        // don't allow connection to itself
        if (sourceNode == targetNode)
        {
            return false;
        }

        // dont allow to connect an input port to another input port or output port to another output port
        if (isInputPort == m_activeGraph->GetCreateConnectionIsInputPort())
        {
            return false;
        }

        return true;
    }

    bool NodeGraphWidget::CheckIfIsValidTransition(GraphNode* sourceState, GraphNode* targetState)
    {
        AZ_UNUSED(sourceState);
        AZ_UNUSED(targetState);

        return true;
    }

    bool NodeGraphWidget::CheckIfIsValidTransitionSource(GraphNode* sourceState)
    {
        AZ_UNUSED(sourceState);

        return true;
    }

    void NodeGraphWidget::OnCreateConnection(AZ::u16 sourcePortNr, GraphNode* sourceNode, bool sourceIsInputPort, AZ::u16 targetPortNr, GraphNode* targetNode, bool targetIsInputPort, const QPoint& startOffset, const QPoint& endOffset)
    {
        AZ_UNUSED(sourcePortNr);
        AZ_UNUSED(sourceNode);
        AZ_UNUSED(sourceIsInputPort);
        AZ_UNUSED(targetPortNr);
        AZ_UNUSED(targetNode);
        AZ_UNUSED(targetIsInputPort);
        AZ_UNUSED(startOffset);
        AZ_UNUSED(endOffset);
    }

    void NodeGraphWidget::ReplaceTransition(NodeConnection* connection, QPoint oldStartOffset, QPoint oldEndOffset,
        GraphNode* oldSourceNode, GraphNode* oldTargetNode, GraphNode* newSourceNode, GraphNode* newTargetNode)
    {
        AZ_UNUSED(connection);
        AZ_UNUSED(oldStartOffset);
        AZ_UNUSED(oldEndOffset);
        AZ_UNUSED(oldSourceNode);
        AZ_UNUSED(oldTargetNode);
        AZ_UNUSED(newSourceNode);
        AZ_UNUSED(newTargetNode);
    }

    void NodeGraphWidget::SelectNodesInGroup(EMotionFX::AnimGraphNodeGroup* nodeGroup)
    {
        AZ_Assert(nodeGroup->GetNumNodes() > 0, "No nodes in selected group");
        for (size_t n = 0; n < nodeGroup->GetNumNodes(); n++)
        {
            EMotionFX::AnimGraphNode* animGraphNode = m_plugin->GetActiveAnimGraph()->RecursiveFindNodeById(nodeGroup->GetNode(n));
            AZ_Assert(animGraphNode, "No AnimGraphNode in selected group");

            const EMStudio::GraphNode* graphNode = m_activeGraph->FindGraphNode(animGraphNode);
            m_plugin->GetAnimGraphModel().GetSelectionModel().select(
                graphNode->GetModelIndex(), QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/moc_NodeGraphWidget.cpp>
