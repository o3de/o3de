/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionWindow/MotionWindowPlugin.h>
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

        mPlugin = plugin;
        mFontMetrics = new QFontMetrics(mFont);

        // update the active graph
        SetActiveGraph(activeGraph);

        // enable mouse tracking to capture mouse movements over the widget
        setMouseTracking(true);

        // init members
        mShowFPS            = false;
        mLeftMousePressed   = false;
        mPanning            = false;
        mMiddleMousePressed = false;
        mRightMousePressed  = false;
        mRectSelecting      = false;
        mShiftPressed       = false;
        mControlPressed     = false;
        mAltPressed         = false;
        mMoveNode           = nullptr;
        mMouseLastPos       = QPoint(0, 0);
        mMouseLastPressPos  = QPoint(0, 0);
        mMousePos           = QPoint(0, 0);

        // setup to get focus when we click or use the mouse wheel
        setFocusPolicy((Qt::FocusPolicy)(Qt::ClickFocus | Qt::WheelFocus));

        // accept drops
        setAcceptDrops(true);

        setAutoFillBackground(false);
        setAttribute(Qt::WA_OpaquePaintEvent);

        mCurWidth       = geometry().width();
        mCurHeight      = geometry().height();
        mPrevWidth      = mCurWidth;
        mPrevHeight     = mCurHeight;
    }


    // destructor
    NodeGraphWidget::~NodeGraphWidget()
    {
        // delete the overlay font metrics
        delete mFontMetrics;
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

        mCurWidth = w;
        mCurHeight = h;

        // specify the center of the window, so that that is the origin
        if (mActiveGraph)
        {
            mActiveGraph->SetScalePivot(QPoint(w / 2, h / 2));

            QPoint  scrollOffset    = mActiveGraph->GetScrollOffset();
            int32   scrollOffsetX   = scrollOffset.x();
            int32   scrollOffsetY   = scrollOffset.y();

            // calculate the size delta
            QPoint  oldSize         = QPoint(mPrevWidth, mPrevHeight);
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

            mActiveGraph->SetScrollOffset(QPoint(scrollOffsetX, scrollOffsetY));
            //MCore::LOG("%d, %d", scrollOffsetX, scrollOffsetY);
        }

        QOpenGLWidget::resizeGL(w, h);

        mPrevWidth = w;
        mPrevHeight = h;
    }


    // set the active graph
    void NodeGraphWidget::SetActiveGraph(NodeGraph* graph)
    {
        if (mActiveGraph == graph)
        {
            return;
        }

        if (mActiveGraph)
        {
            mActiveGraph->StopCreateConnection();
            mActiveGraph->StopRelinkConnection();
            mActiveGraph->StopReplaceTransitionHead();
            mActiveGraph->StopReplaceTransitionTail();
        }

        mActiveGraph = graph;
        mMoveNode = nullptr;

        emit ActiveGraphChanged();
    }


    // get the active graph
    NodeGraph* NodeGraphWidget::GetActiveGraph() const
    {
        return mActiveGraph;
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
        const float timePassedInSeconds = mRenderTimer.StampAndGetDeltaTimeInSeconds();

        // start painting
        QPainter painter(this);

        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::TextAntialiasing);

        // get the width and height
        const uint32 width  = mCurWidth;
        const uint32 height = mCurHeight;

        // fill the background
        //painter.fillRect( event->rect(), QColor(30, 30, 30) );
        painter.setBrush(QColor(47, 47, 47));
        painter.setPen(Qt::NoPen);
        painter.drawRect(QRect(0, 0, width, height));

        // render the active graph
        if (mActiveGraph)
        {
            mActiveGraph->Render(mPlugin->GetAnimGraphModel().GetSelectionModel(), painter, width, height, mMousePos, timePassedInSeconds);
        }

        // render selection rect
        if (mRectSelecting)
        {
            painter.resetTransform();
            QRect selectRect;
            CalcSelectRect(selectRect);

            if (mAltPressed)
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
        if (mActiveGraph)
        {
            painter.resetTransform();
            mActiveGraph->DrawOverlay(painter);
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
        if (mShowFPS)
        {
            // get the time delta between the current time and the last frame
            static AZ::Debug::Timer perfTimer;
            static float perfTimeDelta = perfTimer.StampAndGetDeltaTimeInSeconds();
            perfTimeDelta = perfTimer.StampAndGetDeltaTimeInSeconds();

            static float fpsTimeElapsed = 0.0f;
            static uint32 fpsNumFrames = 0;
            static uint32 lastFPS = 0;
            fpsTimeElapsed += perfTimeDelta;
            const float renderTime = mRenderTimer.StampAndGetDeltaTimeInSeconds() * 1000.0f;
            fpsNumFrames++;
            if (fpsTimeElapsed > 1.0f)
            {
                lastFPS         = fpsNumFrames;
                fpsTimeElapsed  = 0.0f;
                fpsNumFrames    = 0;
            }

            static AZStd::string perfTempString;
            perfTempString = AZStd::string::format("%i FPS (%.1f ms)", lastFPS, renderTime);

            GraphNode::RenderText(painter, perfTempString.c_str(), QColor(150, 150, 150), mFont, *mFontMetrics, Qt::AlignRight, QRect(width - 55, height - 20, 50, 20));
        }

        // show the info to which actor the currently rendered graph belongs to
        const CommandSystem::SelectionList& selectionList = CommandSystem::GetCommandManager()->GetCurrentSelection();

        if ((EMotionFX::GetActorManager().GetNumActorInstances() > 1) && (selectionList.GetNumSelectedActorInstances() > 0))
        {
            // get the first of the multiple selected actor instances
            EMotionFX::ActorInstance* firstActorInstance = selectionList.GetFirstActorInstance();

            // update the stored short filename without path
            if (mFullActorName != firstActorInstance->GetActor()->GetFileName())
            {
                AzFramework::StringFunc::Path::GetFileName(firstActorInstance->GetActor()->GetFileNameString().c_str(), mActorName);
            }

            mTempString = AZStd::string::format("Showing graph for ActorInstance with ID %d and Actor file \"%s\"", firstActorInstance->GetID(), mActorName.c_str());
            GraphNode::RenderText(painter, mTempString.c_str(), QColor(150, 150, 150), mFont, *mFontMetrics, Qt::AlignLeft, QRect(8, 0, 50, 20));
        }
    }


    // convert to a global position
    QPoint NodeGraphWidget::LocalToGlobal(const QPoint& inPoint) const
    {
        if (mActiveGraph)
        {
            return mActiveGraph->GetTransform().inverted().map(inPoint);
        }

        return inPoint;
    }


    // convert to a local position
    QPoint NodeGraphWidget::GlobalToLocal(const QPoint& inPoint) const
    {
        if (mActiveGraph)
        {
            return mActiveGraph->GetTransform().map(inPoint);
        }

        return inPoint;
    }


    QPoint NodeGraphWidget::SnapLocalToGrid(const QPoint& inPoint, uint32 cellSize) const
    {
        MCORE_UNUSED(cellSize);

        //QPoint scaledMouseDelta = (mousePos - mMouseLastPos) * (1.0f / mActiveGraph->GetScale());
        //QPoint unSnappedTopRight = oldTopRight + scaledMouseDelta;
        QPoint snapped;
        snapped.setX(inPoint.x() - aznumeric_cast<int>(MCore::Math::FMod(aznumeric_cast<float>(inPoint.x()), 10.0f)));
        snapped.setY(inPoint.y() - aznumeric_cast<int>(MCore::Math::FMod(aznumeric_cast<float>(inPoint.y()), 10.0f)));
        //snapDelta = snappedTopRight - unSnappedTopRight;
        return snapped;
    }

    // mouse is moving over the widget
    void NodeGraphWidget::mouseMoveEvent(QMouseEvent* event)
    {
        if (mActiveGraph == nullptr)
        {
            return;
        }

        // get the mouse position, calculate the global mouse position and update the relevant data
        QPoint mousePos = event->pos();

        QPoint snapDelta(0, 0);
        if (mMoveNode && mLeftMousePressed && mPanning == false && mRectSelecting == false)
        {
            QPoint oldTopRight = mMoveNode->GetRect().topRight();
            QPoint scaledMouseDelta = (mousePos - mMouseLastPos) * (1.0f / mActiveGraph->GetScale());
            QPoint unSnappedTopRight = oldTopRight + scaledMouseDelta;
            QPoint snappedTopRight = SnapLocalToGrid(unSnappedTopRight, 10);
            snapDelta = snappedTopRight - unSnappedTopRight;
        }

        mousePos += snapDelta * mActiveGraph->GetScale();
        QPoint delta        = (mousePos - mMouseLastPos) * (1.0f / mActiveGraph->GetScale());
        mMouseLastPos       = mousePos;
        QPoint globalPos    = LocalToGlobal(mousePos);
        SetMousePos(globalPos);

        //if (delta.x() > 0 || delta.x() < -0 || delta.y() > 0 || delta.y() < -0)
        if (delta.x() != 0 || delta.y() != 0)
        {
            mAllowContextMenu = false;
        }

        // update modifiers
        mAltPressed     = event->modifiers() & Qt::AltModifier;
        mShiftPressed   = event->modifiers() & Qt::ShiftModifier;
        mControlPressed = event->modifiers() & Qt::ControlModifier;

        /*GraphNode* node = */ UpdateMouseCursor(mousePos, globalPos);
        if (mRectSelecting == false && mMoveNode == nullptr &&
            mPlugin->GetActionFilter().m_editConnections &&
            !mActiveGraph->IsInReferencedGraph())
        {
            // check if we are clicking on a port
            GraphNode*  portNode    = nullptr;
            NodePort*   port        = nullptr;
            uint32      portNr      = MCORE_INVALIDINDEX32;
            bool        isInputPort = true;
            port = mActiveGraph->FindPort(globalPos.x(), globalPos.y(), &portNode, &portNr, &isInputPort);

            // check if we are adjusting a transition head or tail
            if (mActiveGraph->GetIsRepositioningTransitionHead() || mActiveGraph->GetIsRepositioningTransitionTail())
            {
                NodeConnection* connection = mActiveGraph->GetRepositionedTransitionHead();
                if (connection == nullptr)
                {
                    connection = mActiveGraph->GetRepositionedTransitionTail();
                }

                MCORE_ASSERT(connection->GetType() == StateConnection::TYPE_ID);
                StateConnection* stateConnection = static_cast<StateConnection*>(connection);

                EMotionFX::AnimGraphStateTransition* transition = connection->GetModelIndex().data(AnimGraphModel::ROLE_TRANSITION_POINTER).value<EMotionFX::AnimGraphStateTransition*>();
                if (transition)
                {
                    // check if our mouse is-over a node
                    GraphNode* hoveredNode = mActiveGraph->FindNode(mousePos);
                    if (hoveredNode == nullptr && portNode)
                    {
                        hoveredNode = portNode;
                    }

                    if (mActiveGraph->GetIsRepositioningTransitionHead())
                    {
                        // when adjusting the arrow head and we are over the source node with the mouse
                        if (hoveredNode
                            && hoveredNode != stateConnection->GetSourceNode()
                            && CheckIfIsValidTransition(stateConnection->GetSourceNode(), hoveredNode))
                        {
                            stateConnection->SetTargetNode(hoveredNode);
                            mActiveGraph->SetReplaceTransitionValid(true);
                        }
                        else
                        {
                            mActiveGraph->SetReplaceTransitionValid(false);
                        }

                        GraphNode* targetNode = stateConnection->GetTargetNode();
                        if (targetNode)
                        {
                            const QPoint newEndOffset = globalPos - targetNode->GetRect().topLeft();
                            transition->SetVisualOffsets(transition->GetVisualStartOffsetX(), transition->GetVisualStartOffsetY(),
                                newEndOffset.x(), newEndOffset.y());
                        }
                    }
                    else if (mActiveGraph->GetIsRepositioningTransitionTail())
                    {
                        // when adjusting the arrow tail and we are over the target node with the mouse
                        if (hoveredNode
                            && hoveredNode != stateConnection->GetTargetNode()
                            && CheckIfIsValidTransition(hoveredNode, stateConnection->GetTargetNode()))
                        {
                            stateConnection->SetSourceNode(hoveredNode);
                            mActiveGraph->SetReplaceTransitionValid(true);
                        }
                        else
                        {
                            mActiveGraph->SetReplaceTransitionValid(false);
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
                if (mActiveGraph->GetIsCreatingConnection())
                {
                    const bool isValid = CheckIfIsCreateConnectionValid(portNr, portNode, port, isInputPort);
                    mActiveGraph->SetCreateConnectionIsValid(isValid);
                    mActiveGraph->SetTargetPort(port);
                    //update();
                    return;
                }
                else if (mActiveGraph->GetIsRelinkingConnection())
                {
                    bool isValid = NodeGraph::CheckIfIsRelinkConnectionValid(mActiveGraph->GetRelinkConnection(), portNode, portNr, isInputPort);
                    mActiveGraph->SetCreateConnectionIsValid(isValid);
                    mActiveGraph->SetTargetPort(port);
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
                mActiveGraph->SetTargetPort(nullptr);
            }
        }

        // if we are panning
        if (mPanning)
        {
            // handle mouse wrapping, to enable smoother panning
            bool mouseWrapped = false;
            if (event->x() > (int32)width())
            {
                mouseWrapped = true;
                QCursor::setPos(QPoint(event->globalX() - width(), event->globalY()));
                mMouseLastPos = QPoint(event->x() - width(), event->y());
            }
            else if (event->x() < 0)
            {
                mouseWrapped = true;
                QCursor::setPos(QPoint(event->globalX() + width(), event->globalY()));
                mMouseLastPos = QPoint(event->x() + width(), event->y());
            }

            if (event->y() > (int32)height())
            {
                mouseWrapped = true;
                QCursor::setPos(QPoint(event->globalX(), event->globalY() - height()));
                mMouseLastPos = QPoint(event->x(), event->y() - height());
            }
            else if (event->y() < 0)
            {
                mouseWrapped = true;
                QCursor::setPos(QPoint(event->globalX(), event->globalY() + height()));
                mMouseLastPos = QPoint(event->x(), event->y() + height());
            }

            // don't apply the delta, if mouse has been wrapped
            if (mouseWrapped)
            {
                delta = QPoint(0, 0);
            }

            if (mActiveGraph)
            {
                // scrolling
                if (mAltPressed == false)
                {
                    QPoint newOffset = mActiveGraph->GetScrollOffset();
                    newOffset += delta;
                    mActiveGraph->SetScrollOffset(newOffset);
                    mActiveGraph->StopAnimatedScroll();
                    UpdateMouseCursor(mousePos, globalPos);
                    //update();
                    return;
                }
                // zooming
                else
                {
                    // stop the automated zoom
                    mActiveGraph->StopAnimatedZoom();

                    // calculate the new scale value
                    const float scaleDelta = (delta.y() / 120.0f) * 0.2f;
                    float newScale = MCore::Clamp<float>(mActiveGraph->GetScale() + scaleDelta, mActiveGraph->GetLowestScale(), 1.0f);
                    mActiveGraph->SetScale(newScale);

                    // redraw the viewport
                    //update();
                }
            }
        }

        // if the left mouse button is pressed
        if (mLeftMousePressed)
        {
            if (mMoveNode)
            {
                if (mActiveGraph &&
                    mPlugin->GetActionFilter().m_editNodes &&
                    !mActiveGraph->IsInReferencedGraph())
                {
                    const AZStd::vector<GraphNode*> selectedGraphNodes = mActiveGraph->GetSelectedGraphNodes();
                    if (!selectedGraphNodes.empty())
                    {
                        // move all selected nodes
                        for (GraphNode* graphNode : selectedGraphNodes)
                        {
                            graphNode->MoveRelative(delta);
                        }
                    }
                    else if (mMoveNode)
                    {
                        mMoveNode->MoveRelative(delta);
                    }
                    return;
                }
            }
            else
            {
                //setCursor( Qt::ArrowCursor );

                if (mRectSelecting)
                {
                    mSelectEnd = mousePos;
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
        if (!mActiveGraph)
        {
            return;
        }

        GetMainWindow()->DisableUndoRedo();

        mAllowContextMenu = true;

        // get the mouse position, calculate the global mouse position and update the relevant data
        QPoint mousePos     = event->pos();
        mMouseLastPos       = mousePos;
        mMouseLastPressPos  = mousePos;
        QPoint globalPos    = LocalToGlobal(mousePos);
        SetMousePos(globalPos);

        // update modifiers
        mAltPressed     = event->modifiers() & Qt::AltModifier;
        mShiftPressed   = event->modifiers() & Qt::ShiftModifier;
        mControlPressed = event->modifiers() & Qt::ControlModifier;
        const AnimGraphActionFilter& actionFilter = mPlugin->GetActionFilter();

        // check if we can start panning
        if ((event->buttons() & Qt::RightButton && event->buttons() & Qt::LeftButton) || event->button() == Qt::RightButton || event->button() == Qt::MidButton)
        {
            // update button booleans
            if (event->buttons() & Qt::RightButton && event->buttons() & Qt::LeftButton)
            {
                mLeftMousePressed  = true;
                mRightMousePressed = true;
            }

            if (event->button() == Qt::RightButton)
            {
                mRightMousePressed = true;

                GraphNode* node = UpdateMouseCursor(mousePos, globalPos);
                if (node && node->GetCanVisualize() && node->GetIsInsideVisualizeRect(globalPos))
                {
                    OnSetupVisualizeOptions(node);
                    mRectSelecting = false;
                    mPanning = false;
                    mMoveNode = nullptr;
                    //update();
                    return;
                }

                // Right click on the node will trigger a single selection, if the node is not already selected.
                if (node && !node->GetIsSelected() && !node->GetIsInsideArrowRect(globalPos))
                {
                    mPlugin->GetAnimGraphModel().GetSelectionModel().select(QItemSelection(node->GetModelIndex(), node->GetModelIndex()),
                        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                    return;
                }
            }

            if (event->button() == Qt::MidButton)
            {
                mMiddleMousePressed = true;
            }

            mPanning = true;
            mRectSelecting = false;
            setCursor(Qt::ClosedHandCursor);
            //mMoveNode = nullptr;

            // update viewport and return
            //update();
            return;
        }

        // if we press the left mouse button
        if (event->button() == Qt::LeftButton)
        {
            mLeftMousePressed = true;

            // get the node we click on
            GraphNode* node = UpdateMouseCursor(mousePos, globalPos);

            // if we pressed the visualize icon
            GraphNode* orgNode = mActiveGraph->FindNode(mousePos);
            if (orgNode && orgNode->GetCanVisualize() && orgNode->GetIsInsideVisualizeRect(globalPos))
            {
                const bool viz = !orgNode->GetIsVisualized();
                orgNode->SetIsVisualized(viz);
                OnVisualizeToggle(orgNode, viz);
                mRectSelecting = false;
                mPanning = false;
                mMoveNode = nullptr;
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
                    if (animGraphNode->GetSupportsPreviewMotion())
                    {
                        MCore::CommandGroup commandGroup("Preview Motion Time View");
                        AZStd::string commandString, result;
                        EMotionFX::AnimGraphMotionNode* motionNode = static_cast<EMotionFX::AnimGraphMotionNode*>(animGraphNode);

                        if (motionNode->GetNumMotions() == 1)
                        {
                            GetCommandManager()->GetCurrentSelection().ClearMotionSelection();
                            const char* motionId = motionNode->GetMotionId(0);
                            EMotionFX::MotionSet::MotionEntry* motionEntry = MotionSetsWindowPlugin::FindBestMatchMotionEntryById(motionId);
                            const EMotionFX::MotionManager& motionManager = EMotionFX::GetMotionManager();

                            if (motionEntry && motionEntry->GetMotion())
                            {
                                EMotionFX::Motion* motion = motionEntry->GetMotion();
                                uint32 motionIndex = motionManager.FindMotionIndexByName(motion->GetName());
                                commandString = AZStd::string::format("Select -motionIndex %d", motionIndex);
                                commandGroup.AddCommandString(commandString);
                            }
                        }

                        if (commandGroup.GetNumCommands() > 0)
                        {
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
                            timeViewPlugin->SetMode(TimeViewMode::Motion);
                        }
                    }
                    else
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
            
            if (!mActiveGraph->IsInReferencedGraph())
            {
                // check if we are clicking on an input port
                GraphNode*  portNode = nullptr;
                NodePort*   port = nullptr;
                uint32      portNr = MCORE_INVALIDINDEX32;
                bool        isInputPort = true;
                port = mActiveGraph->FindPort(globalPos.x(), globalPos.y(), &portNode, &portNr, &isInputPort);
                if (port)
                {
                    mMoveNode = nullptr;
                    mPanning = false;
                    mRectSelecting = false;

                    // relink existing connection
                    NodeConnection* connection = mActiveGraph->FindInputConnection(portNode, portNr);
                    if (actionFilter.m_editConnections &&
                        isInputPort && connection && portNode->GetType() != StateGraphNode::TYPE_ID)
                    {
                        //connection->SetColor();
                        //MCore::LOG("%s(%i)->%s(%i)", connection->GetSourceNode()->GetName(), connection->GetOutputPortNr(), connection->GetTargetNode()->GetName(), connection->GetInputPortNr());
                        //GraphNode*    createConNode   = connection->GetSourceNode();
                        //uint32        createConPortNr = connection->GetOutputPortNr();
                        //NodePort* createConPort   = createConNode->GetOutputPort( createConPortNr );
                        //QPoint        createConOffset = QPoint(0,0);//globalPos - createConNode->GetRect().topLeft();
                        connection->SetIsDashed(true);

                        UpdateMouseCursor(mousePos, globalPos);
                        //mActiveGraph->StartCreateConnection( createConPortNr, !isInputPort, createConNode, createConPort, createConOffset );
                        mActiveGraph->StartRelinkConnection(connection, portNr, portNode);
                        //update();
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
                            mActiveGraph->StartCreateConnection(portNr, isInputPort, portNode, port, offset);
                            //update();
                            return;
                        }
                    }
                }

                // check if we click on an transition arrow head or tail
                NodeConnection* connection = mActiveGraph->FindConnection(globalPos);
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
                            mMoveNode = nullptr;
                            mPanning = false;
                            mRectSelecting = false;

                            mActiveGraph->StartReplaceTransitionHead(stateConnection, startOffset, endOffset, stateConnection->GetSourceNode(), stateConnection->GetTargetNode());
                            return;
                        }

                        if (!stateConnection->GetIsWildcardTransition() && stateConnection->CheckIfIsCloseToTail(globalPos))
                        {
                            mMoveNode = nullptr;
                            mPanning = false;
                            mRectSelecting = false;

                            mActiveGraph->StartReplaceTransitionTail(stateConnection, startOffset, endOffset, stateConnection->GetSourceNode(), stateConnection->GetTargetNode());
                            return;
                        }
                    }
                }
            }

            // get the node we click on
            node = UpdateMouseCursor(mousePos, globalPos);
            if (node && mShiftPressed)
            {
                OnShiftClickedNode(node);
            }
            else
            {
                if (node && mShiftPressed == false && mControlPressed == false && mAltPressed == false &&
                    actionFilter.m_editNodes &&
                    !mActiveGraph->IsInReferencedGraph())
                {
                    mMoveNode   = node;
                    mPanning    = false;
                    setCursor(Qt::ClosedHandCursor);
                }
                else
                {
                    mMoveNode       = nullptr;
                    mPanning        = false;
                    mRectSelecting  = true;
                    mSelectStart    = mousePos;
                    mSelectEnd      = mSelectStart;
                    setCursor(Qt::ArrowCursor);
                }
            }

            if (mActiveGraph)
            {
                // shift is used to activate a state, disable all selection behavior in case we press shift!
                // check if we clicked a node and additionally not clicked its arrow rect
                bool nodeClicked = false;
                if (node && !node->GetIsInsideArrowRect(globalPos))
                {
                    nodeClicked = true;
                }
                if (!mShiftPressed)
                {
                    // check the node we're clicking on
                    if (!mControlPressed)
                    {
                        // only reset the selection in case we clicked in empty space or in case the node we clicked on is not part of
                        if (!node || (node && !node->GetIsSelected()))
                        {
                            mPlugin->GetAnimGraphModel().GetSelectionModel().clear();
                        }
                    }

                    // node clicked with shift only
                    if (nodeClicked && mControlPressed)
                    {
                        mPlugin->GetAnimGraphModel().GetSelectionModel().select(QItemSelection(node->GetModelIndex(), node->GetModelIndex()),
                            QItemSelectionModel::Toggle | QItemSelectionModel::Rows);
                    }
                    // node clicked with ctrl only
                    else if (nodeClicked && !mControlPressed)
                    {
                        mPlugin->GetAnimGraphModel().GetSelectionModel().select(QItemSelection(node->GetModelIndex(), node->GetModelIndex()),
                            QItemSelectionModel::Select | QItemSelectionModel::Rows);
                    }
                    // in case we didn't click on a node, check if we click on a connection
                    else if (!nodeClicked)
                    {
                        mActiveGraph->SelectConnectionCloseTo(LocalToGlobal(event->pos()), mControlPressed == false, true);
                    }
                }
                else
                {
                    // in case shift and control are both pressed, special case!
                    if (mControlPressed && node)
                    {
                        mPlugin->GetAnimGraphModel().GetSelectionModel().select(QItemSelection(node->GetModelIndex(), node->GetModelIndex()),
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

        if (mActiveGraph == nullptr)
        {
            return;
        }

        GetMainWindow()->UpdateUndoRedo();

        // update modifiers
        mAltPressed     = event->modifiers() & Qt::AltModifier;
        mShiftPressed   = event->modifiers() & Qt::ShiftModifier;
        mControlPressed = event->modifiers() & Qt::ControlModifier;

        const AnimGraphActionFilter& actionFilter = mPlugin->GetActionFilter();

        // both left and right released at the same time
        if (event->buttons() & Qt::RightButton && event->buttons() & Qt::LeftButton)
        {
            mRightMousePressed  = false;
            mLeftMousePressed   = false;
        }

        // right mouse button
        if (event->button() == Qt::RightButton)
        {
            mRightMousePressed  = false;
            mPanning            = false;
            UpdateMouseCursor(mousePos, globalPos);
            //update();
            return;
        }

        // middle mouse button
        if (event->button() == Qt::MidButton)
        {
            mMiddleMousePressed = false;
            mPanning            = false;
        }

        // if we release the left mouse button
        if (event->button() == Qt::LeftButton)
        {
            const bool mouseMoved = (event->pos() != mMouseLastPressPos);

            // if we pressed the visualize icon
            GraphNode* node = UpdateMouseCursor(mousePos, globalPos);
            if (node && node->GetCanVisualize() && node->GetIsInsideVisualizeRect(globalPos))
            {
                mRectSelecting = false;
                mPanning = false;
                mMoveNode = nullptr;
                mLeftMousePressed = false;
                UpdateMouseCursor(mousePos, globalPos);
                //update();
                return;
            }

            if (node && node->GetIsInsideArrowRect(globalPos))
            {
                mRectSelecting = false;
                mPanning = false;
                mMoveNode = nullptr;
                mLeftMousePressed = false;
                UpdateMouseCursor(mousePos, globalPos);
                //update();
                return;
            }

            // if we were creating a connection
            if (mActiveGraph->GetIsCreatingConnection())
            {
                AZ_Assert(!mActiveGraph->IsInReferencedGraph(), "Expected to not be in a referenced graph");

                // create the connection if needed
                if (mActiveGraph->GetTargetPort())
                {
                    if (mActiveGraph->GetIsCreateConnectionValid())
                    {
                        uint32      targetPortNr;
                        bool        targetIsInputPort;
                        GraphNode*  targetNode;

                        NodePort* targetPort = mActiveGraph->FindPort(globalPos.x(), globalPos.y(), &targetNode, &targetPortNr, &targetIsInputPort);
                        if (targetPort && mActiveGraph->GetTargetPort() == targetPort
                            && targetNode != mActiveGraph->GetCreateConnectionNode())
                        {
#ifndef MCORE_DEBUG
                            MCORE_UNUSED(targetPort);
#endif

                            QPoint endOffset = globalPos - targetNode->GetRect().topLeft();
                            mActiveGraph->SetCreateConnectionEndOffset(endOffset);

                            // trigger the callback
                            OnCreateConnection(mActiveGraph->GetCreateConnectionPortNr(),
                                mActiveGraph->GetCreateConnectionNode(),
                                mActiveGraph->GetCreateConnectionIsInputPort(),
                                targetPortNr,
                                targetNode,
                                targetIsInputPort,
                                mActiveGraph->GetCreateConnectionStartOffset(),
                                mActiveGraph->GetCreateConnectionEndOffset());
                        }
                    }
                }

                mActiveGraph->StopCreateConnection();
                mLeftMousePressed = false;
                UpdateMouseCursor(mousePos, globalPos);
                //update();
                return;
            }

            // if we were relinking a connection
            if (mActiveGraph->GetIsRelinkingConnection())
            {
                AZ_Assert(actionFilter.m_editConnections, "Expected edit connections being enabled.");
                AZ_Assert(!mActiveGraph->IsInReferencedGraph(), "Expected to not be in a referenced graph");

                // get the information from the current mouse position
                uint32      newTargetPortNr;
                bool        newTargetIsInputPort;
                GraphNode*  newTargetNode;
                NodePort*   newTargetPort = mActiveGraph->FindPort(globalPos.x(), globalPos.y(), &newTargetNode, &newTargetPortNr, &newTargetIsInputPort);

                // relink existing connection
                NodeConnection* connection = nullptr;
                if (newTargetPort)
                {
                    connection = mActiveGraph->FindInputConnection(newTargetNode, newTargetPortNr);
                }

                NodeConnection* relinkedConnection = mActiveGraph->GetRelinkConnection();
                if (relinkedConnection)
                {
                    relinkedConnection->SetIsDashed(false);
                }

                if (newTargetNode && newTargetPort)
                {
                    // get the information from the old connection which we want to relink
                    GraphNode*      sourceNode          = relinkedConnection->GetSourceNode();
                    AZStd::string   sourceNodeName      = sourceNode->GetName();
                    uint32          sourcePortNr        = relinkedConnection->GetOutputPortNr();
                    GraphNode*      oldTargetNode       = relinkedConnection->GetTargetNode();
                    AZStd::string   oldTargetNodeName   = oldTargetNode->GetName();
                    uint32          oldTargetPortNr     = relinkedConnection->GetInputPortNr();

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
                        mActiveGraph->StopRelinkConnection();

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

                //mActiveGraph->StopCreateConnection();
                mActiveGraph->StopRelinkConnection();
                mLeftMousePressed = false;
                UpdateMouseCursor(mousePos, globalPos);
                //update();
                return;
            }

            // in case we adjusted a transition start or end point
            if (mActiveGraph->GetIsRepositioningTransitionHead() || mActiveGraph->GetIsRepositioningTransitionTail())
            {
                AZ_Assert(actionFilter.m_editConnections, "Expected edit connections being enabled.");
                AZ_Assert(!mActiveGraph->IsInReferencedGraph(), "Expected to not be in a referenced graph");

                NodeConnection* connection;
                QPoint oldStartOffset, oldEndOffset;
                GraphNode* oldSourceNode;
                GraphNode* oldTargetNode;
                mActiveGraph->GetReplaceTransitionInfo(&connection, &oldStartOffset, &oldEndOffset, &oldSourceNode, &oldTargetNode);
                GraphNode* newDropNode = mActiveGraph->FindNode(event->pos());

                if (newDropNode && newDropNode != oldSourceNode)
                {
                    if (mActiveGraph->GetIsRepositioningTransitionHead())
                    {
                        ReplaceTransition(connection, oldStartOffset, oldEndOffset, oldSourceNode, oldTargetNode, oldSourceNode, newDropNode);
                        mActiveGraph->StopReplaceTransitionHead();
                    }
                    else if (mActiveGraph->GetIsRepositioningTransitionTail() && newDropNode != oldTargetNode)
                    {
                        ReplaceTransition(connection, oldStartOffset, oldEndOffset, oldSourceNode, oldTargetNode, newDropNode, oldTargetNode);
                        mActiveGraph->StopReplaceTransitionTail();
                    }
                }
                else
                {
                    ReplaceTransition(connection, oldStartOffset, oldEndOffset, oldSourceNode, oldTargetNode, oldSourceNode, oldTargetNode);
                    if (mActiveGraph->GetIsRepositioningTransitionHead())
                    {
                        mActiveGraph->StopReplaceTransitionHead();
                    }
                    else if (mActiveGraph->GetIsRepositioningTransitionTail())
                    {
                        mActiveGraph->StopReplaceTransitionTail();
                    }
                }
                return;
            }

            // if we are finished moving, trigger the OnMoveNode callbacks
            if (mMoveNode && mouseMoved &&
                actionFilter.m_editNodes &&
                !mActiveGraph->IsInReferencedGraph())
            {
                OnMoveStart();

                bool moveNodeSelected = false; // prevent moving the same node twice
                const AZStd::vector<GraphNode*> selectedNodes = mActiveGraph->GetSelectedGraphNodes();
                for (GraphNode* currentNode : selectedNodes)
                {
                    OnMoveNode(currentNode, currentNode->GetRect().topLeft().x(), currentNode->GetRect().topLeft().y());
                    if (currentNode == mMoveNode)
                    {
                        moveNodeSelected = true;
                    }
                }
                if (!moveNodeSelected && !mMoveNode)
                {
                    OnMoveNode(mMoveNode, mMoveNode->GetRect().topLeft().x(), mMoveNode->GetRect().topLeft().y());
                }

                OnMoveEnd();
            }

            mPanning = false;
            mMoveNode = nullptr;
            UpdateMouseCursor(mousePos, globalPos);

            // get the node we click on
            node = UpdateMouseCursor(mousePos, globalPos);

            if (mRectSelecting && mouseMoved)
            {
                // calc the selection rect
                QRect selectRect;
                CalcSelectRect(selectRect);

                // select things inside it
                if (selectRect.isEmpty() == false && mActiveGraph)
                {
                    selectRect = mActiveGraph->GetTransform().inverted().mapRect(selectRect);

                    // select nodes when alt is not pressed
                    if (mAltPressed == false)
                    {
                        const bool overwriteSelection = (mControlPressed == false);
                        mActiveGraph->SelectNodesInRect(selectRect, overwriteSelection, mControlPressed);
                    }
                    else // zoom into the selected rect
                    {
                        mActiveGraph->ZoomOnRect(selectRect, geometry().width(), geometry().height(), true);
                    }
                }
            }

            mLeftMousePressed = false;
            mRectSelecting = false;
        }
    }


    // double click
    void NodeGraphWidget::mouseDoubleClickEvent(QMouseEvent* event)
    {
        // only do things when a graph is active
        if (mActiveGraph == nullptr)
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
            GraphNode* node = mActiveGraph->FindNode(mousePos);
            if (node == nullptr)
            {
                // if we didn't double click on a node zoom in to the clicked area
                mActiveGraph->ScrollTo(-LocalToGlobal(mousePos) + geometry().center());
                mActiveGraph->ZoomIn();
            }
        }

        // right mouse button double clicked
        if (event->button() == Qt::RightButton)
        {
            // check if double clicked on a node
            GraphNode* node = mActiveGraph->FindNode(mousePos);
            if (node == nullptr)
            {
                mActiveGraph->ScrollTo(-LocalToGlobal(mousePos) + geometry().center());
                mActiveGraph->ZoomOut();
            }
        }

        setCursor(Qt::ArrowCursor);

        // reset flags
        mRectSelecting = false;

        // redraw the viewport
        //update();
    }


    // when the mouse wheel changes
    void NodeGraphWidget::wheelEvent(QWheelEvent* event)
    {
        // only do things when a graph is active
        if (mActiveGraph == nullptr)
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
        mActiveGraph->StopAnimatedZoom();

        // calculate the new scale value
        const float scaleDelta = (event->angleDelta().y() / 120.0f) * 0.05f;
        float newScale = MCore::Clamp<float>(mActiveGraph->GetScale() + scaleDelta, mActiveGraph->GetLowestScale(), 1.0f);
        mActiveGraph->SetScale(newScale);

        // redraw the viewport
        //update();
    }


    // update the mouse cursor, based on if we hover over a given node or not
    GraphNode* NodeGraphWidget::UpdateMouseCursor(const QPoint& localMousePos, const QPoint& globalMousePos)
    {
        // if there is no active graph
        if (mActiveGraph == nullptr)
        {
            setCursor(Qt::ArrowCursor);
            return nullptr;
        }

        if (mPanning || mMoveNode)
        {
            setCursor(Qt::ClosedHandCursor);
            return nullptr;
        }

        // check if we hover above a node
        GraphNode* node = mActiveGraph->FindNode(localMousePos);

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
            uint32      portNr;
            GraphNode*  portNode;
            bool        isInputPort;
            NodePort* nodePort = mActiveGraph->FindPort(globalMousePos.x(), globalMousePos.y(), &portNode, &portNr, &isInputPort);
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
            uint32      portNr;
            GraphNode*  portNode;
            bool        isInputPort;
            NodePort* nodePort = mActiveGraph->FindPort(globalMousePos.x(), globalMousePos.y(), &portNode, &portNr, &isInputPort);
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
        const int32 startX = MCore::Min<int32>(mSelectStart.x(), mSelectEnd.x());
        const int32 startY = MCore::Min<int32>(mSelectStart.y(), mSelectEnd.y());
        const int32 width  = abs(mSelectEnd.x() - mSelectStart.x());
        const int32 height = abs(mSelectEnd.y() - mSelectStart.y());

        outRect = QRect(startX, startY, width, height);
    }


    // on keypress
    void NodeGraphWidget::keyPressEvent(QKeyEvent* event)
    {
        switch (event->key())
        {
        case Qt::Key_Shift:
        {
            mShiftPressed     = true;
            break;
        }
        case Qt::Key_Control:
        {
            mControlPressed   = true;
            break;
        }
        case Qt::Key_Alt:
        {
            mAltPressed       = true;
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
            mShiftPressed     = false;
            break;
        }
        case Qt::Key_Control:
        {
            mControlPressed   = false;
            break;
        }
        case Qt::Key_Alt:
        {
            mAltPressed       = false;
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
        mShiftPressed   = false;
        mControlPressed = false;
        mAltPressed     = false;
        releaseKeyboard();

        if (mActiveGraph && mActiveGraph->GetIsCreatingConnection())
        {
            mActiveGraph->StopCreateConnection();
            mLeftMousePressed = false;
        }
    }


    // return the number of selected nodes
    uint32 NodeGraphWidget::CalcNumSelectedNodes() const
    {
        if (mActiveGraph)
        {
            return mActiveGraph->CalcNumSelectedNodes();
        }
        else
        {
            return 0;
        }
    }


    // is the given connection valid
    bool NodeGraphWidget::CheckIfIsCreateConnectionValid(uint32 portNr, GraphNode* portNode, NodePort* port, bool isInputPort)
    {
        MCORE_UNUSED(portNr);
        MCORE_UNUSED(port);

        if (mActiveGraph == nullptr)
        {
            return false;
        }

        GraphNode* sourceNode = mActiveGraph->GetCreateConnectionNode();
        GraphNode* targetNode = portNode;

        // don't allow connection to itself
        if (sourceNode == targetNode)
        {
            return false;
        }

        // dont allow to connect an input port to another input port or output port to another output port
        if (isInputPort == mActiveGraph->GetCreateConnectionIsInputPort())
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

    void NodeGraphWidget::OnCreateConnection(uint32 sourcePortNr, GraphNode* sourceNode, bool sourceIsInputPort, uint32 targetPortNr, GraphNode* targetNode, bool targetIsInputPort, const QPoint& startOffset, const QPoint& endOffset)
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
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/moc_NodeGraphWidget.cpp>
