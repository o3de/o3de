
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <functional>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QGraphicsItem>
#include <QGraphicsSceneEvent>
#include <QGraphicsView>
#include <QScreen>
#include <QMimeData>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/sort.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

#include <Components/SceneComponent.h>

#include <Components/BookmarkManagerComponent.h>
#include <Components/BookmarkAnchor/BookmarkAnchorComponent.h>
#include <Components/Connections/ConnectionComponent.h>
#include <Components/GridComponent.h>
#include <Components/Nodes/NodeComponent.h>
#include <Components/SceneMemberComponent.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/Components/LayerBus.h>
#include <GraphCanvas/Components/Nodes/Comment/CommentBus.h>
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>
#include <GraphCanvas/Components/Nodes/NodeUIBus.h>
#include <GraphCanvas/Components/Nodes/Wrapper/WrapperNodeBus.h>
#include <GraphCanvas/Components/PersistentIdBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Editor/GraphCanvasProfiler.h>
#include <GraphCanvas/Editor/GraphModelBus.h>
#include <GraphCanvas/Styling/Parser.h>
#include <GraphCanvas/tools.h>
#include <GraphCanvas/Types/GraphCanvasGraphSerialization.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ContextMenuAction.h>
#include <GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>
#include <GraphCanvas/Widgets/GraphCanvasMimeContainer.h>
#include <GraphCanvas/Widgets/MimeEvents/CreateSplicingNodeMimeEvent.h>
#include <GraphCanvas/Utils/ConversionUtils.h>
#include <GraphCanvas/Utils/QtVectorMath.h>

namespace GraphCanvas
{
    ////////////////
    // SceneHelper
    ////////////////

    void SceneHelper::SetSceneId(const AZ::EntityId& sceneId)
    {
        m_sceneId = sceneId;
    }

    const AZ::EntityId& SceneHelper::GetSceneId() const
    {
        return m_sceneId;
    }

    void SceneHelper::SetEditorId(const EditorId& editorId)
    {
        m_editorId = editorId;
        OnEditorIdSet();
    }

    const EditorId& SceneHelper::GetEditorId() const
    {
        return m_editorId;
    }

    void SceneHelper::OnEditorIdSet()
    {

    }

    ////////////////////////////
    // MimeDelegateSceneHelper
    ////////////////////////////

    void MimeDelegateSceneHelper::Activate()
    {
        m_pushedUndoBlock = false;
        m_enableConnectionSplicing = false;

        m_spliceTimer.setInterval(500);
        m_spliceTimer.setSingleShot(true);

        QObject::connect(&m_spliceTimer, &QTimer::timeout, [this]() { OnTrySplice(); });        

        SceneMimeDelegateHandlerRequestBus::Handler::BusConnect(GetSceneId());
        SceneMimeDelegateRequestBus::Event(GetSceneId(), &SceneMimeDelegateRequests::AddDelegate, GetSceneId());

        m_nudgingController.SetGraphId(GetSceneId());
    }

    void MimeDelegateSceneHelper::Deactivate()
    {
        SceneMimeDelegateRequestBus::Event(GetSceneId(), &SceneMimeDelegateRequests::RemoveDelegate, GetSceneId());
        SceneMimeDelegateHandlerRequestBus::Handler::BusDisconnect();
    }

    void MimeDelegateSceneHelper::SetMimeType(const char* mimeType)
    {
        m_mimeType = mimeType;
    }

    const QString& MimeDelegateSceneHelper::GetMimeType() const
    {
        return m_mimeType;
    }

    bool MimeDelegateSceneHelper::HasMimeType() const
    {
        return !m_mimeType.isEmpty();
    }
    
    bool MimeDelegateSceneHelper::IsInterestedInMimeData(const AZ::EntityId& graphId, const QMimeData* mimeData)
    {
        bool isInterested = HasMimeType() && mimeData->hasFormat(GetMimeType());
        m_enableConnectionSplicing = false;

        if (isInterested)
        {
            // Need a copy since we are going to try to use
            // this event not in response to a movement, but in response to a timeout.
            m_splicingData = mimeData->data(GetMimeType());

            GraphCanvasMimeContainer mimeContainer;
            if (mimeContainer.FromBuffer(m_splicingData.constData(), m_splicingData.size()))
            {
                isInterested = !mimeContainer.m_mimeEvents.empty();

                for (GraphCanvas::GraphCanvasMimeEvent* mimeEvent : mimeContainer.m_mimeEvents)
                {
                    if (!mimeEvent->CanGraphHandleEvent(graphId))
                    {
                        isInterested = false;
                        break;
                    }
                }

                // Splicing only makes sense when we have a single node.
                if (isInterested
                    && mimeContainer.m_mimeEvents.size() == 1
                    && azrtti_istypeof<CreateSplicingNodeMimeEvent>(mimeContainer.m_mimeEvents.front()))
                {
                    m_enableConnectionSplicing = true;

                    AZ_Error("GraphCanvas", !m_splicingNode.IsValid(), "Splicing node not properly invalidated in between interest calls.");
                    m_splicingNode.SetInvalid();

                    m_splicingPath = QPainterPath();
                }
                else
                {
                    m_splicingData.clear();
                }
            }
            else
            {
                isInterested = false;
            }

            if (!isInterested)
            {
                m_splicingData.clear();
            }
        }

        return isInterested;
    }

    void MimeDelegateSceneHelper::HandleMove(const AZ::EntityId& /*sceneId*/, const QPointF& dragPoint, const QMimeData* /*mimeData*/)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();

        bool enableSplicing = false;
        AssetEditorSettingsRequestBus::EventResult(enableSplicing, GetEditorId(), &AssetEditorSettingsRequests::IsDropConnectionSpliceEnabled);

        if (m_splicingNode.IsValid() || !m_splicingPath.isEmpty())
        {
            if (!m_splicingPath.contains(dragPoint))
            {
                CancelSplice();
            }
            else if (m_splicingNode.IsValid())
            {
                PushUndoBlock();
                GeometryRequestBus::Event(m_splicingNode, &GeometryRequests::SetPosition, ConversionUtils::QPointToVector(dragPoint) + m_positionOffset);
                PopUndoBlock();
            }
        }

        AZ::EntityId targetId;
        AZ::Vector2 targetVector(aznumeric_cast<float>(dragPoint.x()), aznumeric_cast<float>(dragPoint.y()));

        AZStd::vector< AZ::EntityId > entitiesAtCursor;
        SceneRequestBus::EventResult(entitiesAtCursor, GetSceneId(), &SceneRequests::GetEntitiesAt, targetVector);

        AZ::EntityId groupTarget;
        AZStd::unordered_set< AZ::EntityId > parentGroups;

        int groupHitCounter = 0;
        int connectionHitCounter = 0;

        for (const auto& entityId : entitiesAtCursor)
        {
            // Handle targeting for connections
            if (GraphUtils::IsConnection(entityId))
            {
                ++connectionHitCounter;
                QGraphicsItem* connectionObject = nullptr;
                VisualRequestBus::EventResult(connectionObject, entityId, &VisualRequests::AsGraphicsItem);

                m_splicingPath = connectionObject->shape();

                targetId = entityId;
            }
            // Handle Targeting for Groups
            else if (GraphUtils::IsNodeGroup(entityId))
            {
                // If our this element is already in the list of parent nodes it's fine, we have a more specific group to drop to.
                if (parentGroups.find(entityId) != parentGroups.end())
                {
                    continue;
                }

                // Otherwise, we want to walk up out group parent, and if we find out
                AZ::EntityId groupId = entityId;
                parentGroups.clear();

                bool isMoreSpecificGroupTarget = false;

                while (groupId.IsValid())
                {
                    parentGroups.insert(groupId);
                    GroupableSceneMemberRequestBus::EventResult(groupId, groupId, &GroupableSceneMemberRequests::GetGroupId);

                    if (groupId == groupTarget && groupTarget.IsValid())
                    {
                        isMoreSpecificGroupTarget = true;
                    }
                }
                    
                if (isMoreSpecificGroupTarget)
                {
                    groupTarget = entityId;
                    continue;
                }

                // Set our group target, and update the number of unique group chains we've seen.
                // If we see more then one, we don't want to do anything with this.
                groupTarget = entityId;
                ++groupHitCounter;
            }
        }

        // Only want to do the splicing if it's unambiguous which thing they are over.
        if ((enableSplicing || m_enableConnectionSplicing) && !m_splicingNode.IsValid())
        {
            if (connectionHitCounter == 1)
            {
                if (m_targetConnection != targetId)
                {
                    m_targetConnection = targetId;
                    m_targetPosition = dragPoint;

                    StateController<RootGraphicsItemDisplayState>* stateController = nullptr;
                    RootGraphicsItemRequestBus::EventResult(stateController, m_targetConnection, &RootGraphicsItemRequests::GetDisplayStateStateController);

                    if (stateController)
                    {
                        m_displayStateStateSetter.AddStateController(stateController);
                        m_displayStateStateSetter.SetState(RootGraphicsItemDisplayState::Preview);
                    }

                    AZStd::chrono::milliseconds spliceDuration(500);
                    AssetEditorSettingsRequestBus::EventResult(spliceDuration, GetEditorId(), &AssetEditorSettingsRequests::GetDropConnectionSpliceTime);

                    m_spliceTimer.stop();
                    m_spliceTimer.setInterval(aznumeric_cast<int>(spliceDuration.count()));
                    m_spliceTimer.start();
                }
            }
            else
            {
                if (m_targetConnection.IsValid())
                {
                    m_displayStateStateSetter.ResetStateSetter();

                    m_targetConnection.SetInvalid();
                    m_spliceTimer.stop();
                }

                if (connectionHitCounter > 0)
                {
                    m_splicingPath = QPainterPath();
                }
            }
        }

        if (groupTarget.IsValid() && groupHitCounter == 1)
        {
            if (groupTarget != m_groupTarget)
            {
                m_groupTargetStateSetter.ResetStateSetter();

                m_groupTarget = groupTarget;

                StateController<RootGraphicsItemDisplayState>* stateController = nullptr;
                RootGraphicsItemRequestBus::EventResult(stateController, m_groupTarget, &RootGraphicsItemRequests::GetDisplayStateStateController);

                m_groupTargetStateSetter.AddStateController(stateController);
                m_groupTargetStateSetter.SetState(RootGraphicsItemDisplayState::GroupHighlight);
            }
        }
        else
        {
            m_groupTarget.SetInvalid();
            m_groupTargetStateSetter.ResetStateSetter();
        }
    }

    void MimeDelegateSceneHelper::HandleDrop(const AZ::EntityId& /*sceneId*/, const QPointF& dropPoint, const QMimeData* mimeData)
    {
        GRAPH_CANVAS_PROFILE_FUNCTION();

        // Once we finalize the node, we want to release the undo state, and push a new undo.
        ScopedGraphUndoBatch undoBatch(GetSceneId());

        m_spliceTimer.stop();

        m_displayStateStateSetter.ResetStateSetter();

        if (m_splicingNode.IsValid())
        {
            SceneRequestBus::Event(GetSceneId(), &SceneRequests::ClearSelection);
            SceneMemberUIRequestBus::Event(m_splicingNode, &SceneMemberUIRequests::SetSelected, true);

            m_nudgingController.FinalizeNudging();

            m_lastCreationGroup.clear();
            m_lastCreationGroup.insert(m_splicingNode);

            m_splicingData.clear();
            m_splicingNode.SetInvalid();

            m_targetConnection.SetInvalid();

            m_splicingPath = QPainterPath();

            AssignLastCreationToGroup();
            return;
        }

        if (!mimeData->hasFormat(GetMimeType()))
        {
            AZ_Error("SceneMimeDelegate", false, "Handling an event that does not meet our Mime requirements");
            return;
        }

        QByteArray arrayData = mimeData->data(GetMimeType());

        GraphCanvasMimeContainer mimeContainer;

        if (!mimeContainer.FromBuffer(arrayData.constData(), arrayData.size()) || mimeContainer.m_mimeEvents.empty())
        {
            return;
        }

        bool success = false;

        AZ::Vector2 sceneMousePoint = AZ::Vector2(aznumeric_cast<float>(dropPoint.x()), aznumeric_cast<float>(dropPoint.y()));
        AZ::Vector2 sceneDropPoint = sceneMousePoint;
        
        QGraphicsScene* graphicsScene = nullptr;
        SceneRequestBus::EventResult(graphicsScene, GetSceneId(), &SceneRequests::AsQGraphicsScene);
 
        if (graphicsScene)
        {
            graphicsScene->blockSignals(true);
        }

        SceneRequestBus::Event(GetSceneId(), &SceneRequests::ClearSelection);

        m_lastCreationGroup.clear();

        for (GraphCanvasMimeEvent* mimeEvent : mimeContainer.m_mimeEvents)
        {
            if (mimeEvent->ExecuteEvent(sceneMousePoint, sceneDropPoint, GetSceneId()))
            {
                success = true;
            }
        }

        if (success)
        {
            SceneNotificationBus::Event(GetSceneId(), &SceneNotifications::PostCreationEvent);
            AssignLastCreationToGroup();
        }
        
        if (graphicsScene)
        {
            graphicsScene->blockSignals(false);
            emit graphicsScene->selectionChanged();
        }
    }

    void MimeDelegateSceneHelper::HandleLeave(const AZ::EntityId& /*sceneId*/, const QMimeData* /*mimeData*/)
    {
        CancelSplice();
    }

    void MimeDelegateSceneHelper::SignalNodeCreated(const NodeId& nodeId)
    {
        m_lastCreationGroup.insert(nodeId);
    }

    void MimeDelegateSceneHelper::OnTrySplice()
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();

        if (m_targetConnection.IsValid())
        {
            GraphCanvasMimeContainer mimeContainer;

            if (mimeContainer.FromBuffer(m_splicingData.constData(), m_splicingData.size()))
            {
                if (mimeContainer.m_mimeEvents.empty())
                {
                    return;
                }

                ConnectionRequestBus::EventResult(m_spliceSource, m_targetConnection, &ConnectionRequests::GetSourceEndpoint);
                ConnectionRequestBus::EventResult(m_spliceTarget, m_targetConnection, &ConnectionRequests::GetTargetEndpoint);

                m_opportunisticSpliceRemovals.clear();

                CreateSplicingNodeMimeEvent* mimeEvent = azrtti_cast<CreateSplicingNodeMimeEvent*>(mimeContainer.m_mimeEvents.front());

                if (mimeEvent)
                {
                    PushUndoBlock();

                    m_splicingNode = mimeEvent->CreateSplicingNode(GetSceneId());

                    if (m_splicingNode.IsValid())
                    {
                        SceneRequestBus::Event(GetSceneId(), &SceneRequests::AddNode, m_splicingNode, ConversionUtils::QPointToVector(m_targetPosition), false);

                        ConnectionSpliceConfig connectionSpliceConfig;
                        connectionSpliceConfig.m_allowOpportunisticConnections = true;

                        bool allowNode = GraphUtils::SpliceNodeOntoConnection(m_splicingNode, m_targetConnection, connectionSpliceConfig);

                        if (!allowNode)
                        {
                            AZStd::unordered_set<AZ::EntityId> deleteIds = { m_splicingNode };
                            SceneRequestBus::Event(GetSceneId(), &SceneRequests::Delete, deleteIds);

                            m_splicingNode.SetInvalid();
                        }
                        else
                        {
                            m_displayStateStateSetter.ResetStateSetter();

                            m_opportunisticSpliceRemovals = connectionSpliceConfig.m_opportunisticSpliceResult.m_removedConnections;

                            StateController<RootGraphicsItemDisplayState>* stateController = nullptr;

                            RootGraphicsItemRequestBus::EventResult(stateController, m_splicingNode, &RootGraphicsItemRequests::GetDisplayStateStateController);
                            m_displayStateStateSetter.AddStateController(stateController);

                            AZStd::vector< AZ::EntityId > slotIds;
                            NodeRequestBus::EventResult(slotIds, m_splicingNode, &NodeRequests::GetSlotIds);

                            AZ::Vector2 centerPoint(0,0);
                            int totalSamples = 0;

                            for (const AZ::EntityId& slotId : slotIds)
                            {
                                AZStd::vector< AZ::EntityId > connectionIds;
                                SlotRequestBus::EventResult(connectionIds, slotId, &SlotRequests::GetConnections);

                                if (!connectionIds.empty())
                                {
                                    QPointF slotPosition;
                                    SlotUIRequestBus::EventResult(slotPosition, slotId, &SlotUIRequests::GetConnectionPoint);

                                    centerPoint += ConversionUtils::QPointToVector(slotPosition);
                                    ++totalSamples;

                                    for (const AZ::EntityId& connectionId : connectionIds)
                                    {
                                        stateController = nullptr;
                                        RootGraphicsItemRequestBus::EventResult(stateController, connectionId, &RootGraphicsItemRequests::GetDisplayStateStateController);

                                        m_displayStateStateSetter.AddStateController(stateController);
                                    }
                                }
                            }

                            if (totalSamples > 0)
                            {
                                centerPoint /= aznumeric_cast<float>(totalSamples);
                            }

                            m_positionOffset = ConversionUtils::QPointToVector(m_targetPosition) - centerPoint;
                            GeometryRequestBus::Event(m_splicingNode, &GeometryRequests::SetPosition, m_positionOffset + ConversionUtils::QPointToVector(m_targetPosition));

                            m_displayStateStateSetter.SetState(RootGraphicsItemDisplayState::Preview);

                            AnimatedPulseConfiguration pulseConfiguration;

                            pulseConfiguration.m_drawColor = QColor(255, 255, 255);
                            pulseConfiguration.m_durationSec = 0.35f;
                            pulseConfiguration.m_enableGradient = true;

                            QGraphicsItem* item = nullptr;
                            SceneMemberUIRequestBus::EventResult(item, m_splicingNode, &SceneMemberUIRequests::GetRootGraphicsItem);

                            if (item)
                            {
                                pulseConfiguration.m_zValue = item->zValue() - 1;
                            }

                            const int k_squaresToPulse = 4;

                            SceneRequestBus::Event(GetSceneId(), &SceneRequests::CreatePulseAroundSceneMember, m_splicingNode, k_squaresToPulse, pulseConfiguration);

                            bool enableNudging = false;
                            AssetEditorSettingsRequestBus::EventResult(enableNudging, GetEditorId(), &AssetEditorSettingsRequests::IsNodeNudgingEnabled);
                            
                            if (enableNudging)
                            {
                                AZStd::unordered_set< NodeId > elementIds = { m_splicingNode };
                                m_nudgingController.StartNudging(elementIds);
                            }
                        }
                    }

                    PopUndoBlock();
                }
            }
        }
    }

    void MimeDelegateSceneHelper::CancelSplice()
    {
        m_displayStateStateSetter.ResetStateSetter();
        m_spliceTimer.stop();
        m_splicingPath = QPainterPath();

        m_nudgingController.CancelNudging();

        if (m_splicingNode.IsValid())
        {
            PushUndoBlock();
            AZStd::unordered_set< AZ::EntityId > deleteIds = { m_splicingNode };
            SceneRequestBus::Event(GetSceneId(), &SceneRequests::Delete, deleteIds);

            m_splicingNode.SetInvalid();

            AZ::EntityId connectionId;
            SlotRequestBus::EventResult(connectionId, m_spliceSource.GetSlotId(), &SlotRequests::CreateConnectionWithEndpoint, m_spliceTarget);

            AZ_Error("GraphCanvas", connectionId.IsValid(), "Failed to recreate a connection after unsplicing a spliced node.");

            for (const ConnectionEndpoints& removedConnection : m_opportunisticSpliceRemovals)
            {
                AZ::EntityId opportunisticConnectionId;
                SlotRequestBus::EventResult(opportunisticConnectionId, removedConnection.m_sourceEndpoint.GetSlotId(), &SlotRequests::CreateConnectionWithEndpoint, removedConnection.m_targetEndpoint);
                AZ_Error("GraphCanvas", opportunisticConnectionId.IsValid(), "Failed to recreate a connection after unsplicing a spliced node.");
            }

            m_opportunisticSpliceRemovals.clear();

            PopUndoBlock();
        }
    }

    void MimeDelegateSceneHelper::PushUndoBlock()
    {
        if (!m_pushedUndoBlock)
        {
            GraphModelRequestBus::Event(GetSceneId(), &GraphModelRequests::RequestPushPreventUndoStateUpdate);

            m_pushedUndoBlock = true;
        }
    }

    void MimeDelegateSceneHelper::PopUndoBlock()
    {
        if (m_pushedUndoBlock)
        {
            m_pushedUndoBlock = false;

            GraphModelRequestBus::Event(GetSceneId(), &GraphModelRequests::RequestPopPreventUndoStateUpdate);
        }
    }

    void MimeDelegateSceneHelper::AssignLastCreationToGroup()
    {
        if (m_groupTarget.IsValid() && !m_lastCreationGroup.empty())
        {
            AZStd::unordered_set< NodeId > filteredCreationGroup;

            for (const auto& createdNode : m_lastCreationGroup)
            {
                if (GraphUtils::IsNodeWrapped(createdNode))
                {
                    continue;
                }

                filteredCreationGroup.insert(createdNode);
            }

            if (!filteredCreationGroup.empty())
            {
                NodeGroupRequests* nodeGroupRequests = NodeGroupRequestBus::FindFirstHandler(m_groupTarget);

                if (nodeGroupRequests)
                {
                    nodeGroupRequests->AddElementsToGroup(filteredCreationGroup);

                    const bool growGroupOnly = true;
                    nodeGroupRequests->ResizeGroupToElements(growGroupOnly);
                }
            }
        }

        m_groupTarget.SetInvalid();
        m_groupTargetStateSetter.ResetStateSetter();
    }

    ///////////////////////
    // GestureSceneHelper
    ///////////////////////

    void GestureSceneHelper::Activate()
    {        
        m_shakeCounter = 0;
        m_trackingTarget.SetInvalid();
        m_hasDirection = false;        
                
        m_timer.setSingleShot(true);        

        QObject::connect(&m_timer, &QTimer::timeout, [this]() { ResetTracker(); });        
    }

    void GestureSceneHelper::Deactivate()
    {
        GeometryNotificationBus::Handler::BusDisconnect();
        SceneNotificationBus::Handler::BusDisconnect();
        AZ::SystemTickBus::Handler::BusDisconnect();
    }

    void GestureSceneHelper::TrackElement(const AZ::EntityId& elementId)
    {
        if (m_trackShake)
        {
            AZ_Error("GraphCanvas", !m_trackingTarget.IsValid(), "Trying to track a second target for gestures while still tracking the first.");

            if (GeometryNotificationBus::Handler::BusIsConnected())
            {
                GeometryNotificationBus::Handler::BusDisconnect();
            }

            GeometryNotificationBus::Handler::BusConnect(elementId);

            m_trackingTarget = elementId;

            m_shakeCounter = 0;
            m_hasDirection = false;

            m_currentAnchor = QCursor::pos();
            m_lastPoint = m_currentAnchor;

            SceneNotificationBus::Handler::BusConnect(GetSceneId());
        }
    }

    void GestureSceneHelper::ResetTracker()
    {
        m_hasDirection = false;
        m_shakeCounter = 0;
    }

    void GestureSceneHelper::StopTrack()
    {
        SceneNotificationBus::Handler::BusDisconnect();
        GeometryNotificationBus::Handler::BusDisconnect();

        m_trackingTarget.SetInvalid();
    }

    void GestureSceneHelper::OnPositionChanged(const AZ::EntityId& itemId, const AZ::Vector2& position)
    {
        AZ_UNUSED(itemId);
        AZ_UNUSED(position);

        QPointF currentPoint = QCursor::pos();

        QPointF currentDirection = currentPoint - m_lastPoint;

        float length = QtVectorMath::GetLength(currentDirection);        

        if (length >= m_movementTolerance)
        {
            AZ::Vector2 currentVector = ConversionUtils::QPointToVector(currentDirection);
            currentVector.Normalize();

            AZ::Vector2 anchorVector = ConversionUtils::QPointToVector(currentPoint - m_currentAnchor);
            anchorVector.Normalize();

            if (m_hasDirection)
            {
                // Want to keep track of our current moving direction to see if we switched directions
                // Also need to keep track of our ovverall moving direction to see if we strayed too far off course.
                float currentDotProduct = m_lastDirection.Dot(currentVector);
                float anchorDotProduct = m_lastDirection.Dot(anchorVector);

                float totalLengthMoved = ConversionUtils::QPointToVector(m_currentAnchor - m_lastPoint).GetLength();

                // This means we pivoted.
                if (currentDotProduct <= -m_straightnessPercent && totalLengthMoved >= m_minimumDistance)
                {
                    m_shakeCounter++;

                    if (m_shakeCounter >= m_shakeThreshold)
                    {
                        if (!AZ::SystemTickBus::Handler::BusIsConnected())
                        {
                            AZ::SystemTickBus::Handler::BusConnect();
                        }

                        m_handleShakeAction = true;
                        ResetTracker();
                    }

                    m_lastDirection = currentVector;
                    m_currentAnchor = m_lastPoint;
                }
                else if (anchorDotProduct <= m_straightnessPercent)
                {
                    ResetTracker();
                    m_currentAnchor = currentPoint;
                }
            }
            else
            {                
                m_hasDirection = true;
                m_lastDirection = currentVector;
                m_shakeCounter = 0;

                m_timer.stop();
                m_timer.start();
            }

            m_lastPoint = currentPoint;
        }
    }

    void GestureSceneHelper::OnSettingsChanged()
    {
        // We want to make our movement stuff relative so it deals with different resolutions reasonably well.
        // This does not however deal with different monitors with different displays, since that is just
        // sadness incarnate.
        //
        // Also currently doesn't handle screen resolution changing. Probably a signal for that though.
        float movementToleranceAmount = 0.0f;
        AssetEditorSettingsRequestBus::EventResult(movementToleranceAmount, GetEditorId(), &AssetEditorSettingsRequests::GetMinimumShakePercent);

        float precisionTolerance = 0.0f;
        AssetEditorSettingsRequestBus::EventResult(precisionTolerance, GetEditorId(), &AssetEditorSettingsRequests::GetShakeDeadZonePercent);

        QScreen* screen = QApplication::primaryScreen();
        QSize size = screen->size();

        QPointF dimension(size.width(), size.height());

        float length = QtVectorMath::GetLength(dimension);
        m_movementTolerance = length * precisionTolerance;
        m_minimumDistance = length * movementToleranceAmount;
        
        AZStd::chrono::milliseconds duration(500);
        AssetEditorSettingsRequestBus::EventResult(duration, GetEditorId(), &AssetEditorSettingsRequests::GetMaximumShakeDuration);
        m_timer.setInterval(aznumeric_cast<int>(duration.count()));

        AssetEditorSettingsRequestBus::EventResult(m_trackShake, GetEditorId(), &AssetEditorSettingsRequests::IsShakeToDespliceEnabled);        

        AssetEditorSettingsRequestBus::EventResult(m_shakeThreshold, GetEditorId(), &AssetEditorSettingsRequests::GetShakesToDesplice);
        AssetEditorSettingsRequestBus::EventResult(m_straightnessPercent, GetEditorId(), &AssetEditorSettingsRequests::GetShakeStraightnessPercent);
    }

    void GestureSceneHelper::OnSystemTick()
    {
        if (m_handleShakeAction)
        {
            m_handleShakeAction = false;
            HandleDesplice();
        }

        AZ::SystemTickBus::Handler::BusDisconnect();
    }

    void GestureSceneHelper::OnEditorIdSet()
    {
        OnSettingsChanged();
    }

    void GestureSceneHelper::HandleDesplice()
    {
        ScopedGraphUndoBlocker undoBlocker(GetSceneId());

        bool despliced = false;

        AZStd::vector< AZ::EntityId > selectedItems;
        SceneRequestBus::EventResult(selectedItems, GetSceneId(), &SceneRequests::GetSelectedItems);
        
        AZStd::unordered_set< AZ::EntityId > nodeGroups;
        AZStd::unordered_set< NodeId > floatingNodeIds;

        for (const AZ::EntityId& selectedItem : selectedItems)
        {            
            if (GraphUtils::IsNodeGroup(selectedItem))
            {
                nodeGroups.insert(selectedItem);
                floatingNodeIds.insert(selectedItem);
            }
            else if (GraphUtils::IsNode(selectedItem) || GraphUtils::IsCollapsedNodeGroup(selectedItem))
            {
                floatingNodeIds.insert(selectedItem);
            }
        }

        SubGraphParsingConfig subGraphParseConfig;

        for (const AZ::EntityId& nodeGroup : nodeGroups)
        {
            AZStd::vector< AZ::EntityId > groupedItems;
            NodeGroupRequestBus::Event(nodeGroup, &NodeGroupRequests::FindGroupedElements, groupedItems);

            for (const AZ::EntityId& groupedItem : groupedItems)
            {
                if (GraphUtils::IsNode(groupedItem)
                    || GraphUtils::IsCollapsedNodeGroup(groupedItem))
                {
                    floatingNodeIds.erase(groupedItem);
                }
            }

            bool causeBurst = false;

            SubGraphParsingResult subGraphResult = GraphUtils::ParseSceneMembersIntoSubGraphs(groupedItems, subGraphParseConfig);

            for (const GraphSubGraph& subGraph : subGraphResult.m_subGraphs)
            {
                if (subGraph.m_entryConnections.empty() && subGraph.m_exitConnections.empty())
                {
                    continue;
                }

                causeBurst = true;
                GraphUtils::DetachSubGraphAndStitchConnections(subGraph);
            }

            if (causeBurst)
            {
                despliced = true;

                AnimatedPulseConfiguration pulseConfiguration;
                pulseConfiguration.m_durationSec = 0.5;
                pulseConfiguration.m_enableGradient = true;
                pulseConfiguration.m_drawColor = QColor(255, 255, 255);

                QGraphicsItem* rootItem = nullptr;
                SceneMemberUIRequestBus::EventResult(rootItem, nodeGroup, &SceneMemberUIRequests::GetRootGraphicsItem);

                QRectF boundingArea;

                if (rootItem)
                {
                    pulseConfiguration.m_zValue = rootItem->zValue() + 1;
                    boundingArea = rootItem->sceneBoundingRect();
                }

                SceneRequestBus::Event(GetSceneId(), &SceneRequests::CreatePulseAroundArea, boundingArea, 3, pulseConfiguration);
            }
        }

        AZStd::vector< AZ::EntityId > floatingElements(floatingNodeIds.begin(), floatingNodeIds.end());
        SubGraphParsingResult subGraphResult = GraphUtils::ParseSceneMembersIntoSubGraphs(floatingElements, subGraphParseConfig);

        if (subGraphResult.m_subGraphs.size() == 1)
        {
            for (const GraphSubGraph& subGraph : subGraphResult.m_subGraphs)
            {
                if (subGraph.m_entryConnections.empty() && subGraph.m_exitConnections.empty())
                {
                    continue;
                }

                GraphUtils::DetachSubGraphAndStitchConnections(subGraph);

                QRectF boundingRect;

                int maxZValue = 0;

                for (const AZ::EntityId& elementId : subGraph.m_containedNodes)
                {
                    QGraphicsItem* item = nullptr;
                    SceneMemberUIRequestBus::EventResult(item, elementId, &SceneMemberUIRequests::GetRootGraphicsItem);

                    if (item)
                    {
                        if (item->zValue() > maxZValue)
                        {
                            maxZValue = aznumeric_cast<int>(item->zValue());
                        }

                        if (boundingRect.isEmpty())
                        {
                            boundingRect = item->sceneBoundingRect();
                        }
                        else
                        {
                            boundingRect |= item->sceneBoundingRect();
                        }
                    }
                }

                AnimatedPulseConfiguration pulseConfiguration;
                pulseConfiguration.m_durationSec = 0.5;
                pulseConfiguration.m_enableGradient = true;
                pulseConfiguration.m_zValue = maxZValue + 1;
                pulseConfiguration.m_drawColor = QColor(255, 255, 255);

                SceneRequestBus::Event(GetSceneId(), &SceneRequests::CreatePulseAroundArea, boundingRect, 3, pulseConfiguration);

                despliced = true;
            }
        }

        if (despliced)
        {
            SceneRequestBus::Event(GetSceneId(), &SceneRequests::SignalDesplice);
        }
    }

    ///////////////
    // Copy Utils
    ///////////////

    void SerializeToBuffer(const GraphSerialization& serializationTarget, AZStd::vector<char>& buffer)
    {
        AZ::SerializeContext* serializeContext = AZ::EntityUtils::GetApplicationSerializeContext();

        AZ::IO::ByteContainerStream<AZStd::vector<char>> stream(&buffer);
        AZ::Utils::SaveObjectToStream(stream, AZ::DataStream::ST_BINARY, &serializationTarget, serializeContext);
    }

    void SerializeToClipboard(const GraphSerialization& serializationTarget)
    {
        AZ_Error("Graph Canvas", !serializationTarget.GetSerializationKey().empty(), "Serialization Key not server for scene serialization. Cannot push to clipboard.");
        if (serializationTarget.GetSerializationKey().empty())
        {
            return;
        }
       
        AZStd::vector<char> buffer;
        SerializeToBuffer(serializationTarget, buffer);

        QMimeData* mime = new QMimeData();
        mime->setData(serializationTarget.GetSerializationKey().c_str(), QByteArray(buffer.cbegin(), static_cast<int>(buffer.size())));
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setMimeData(mime);
    }

    ///////////////////
    // SceneComponent
    ///////////////////

    constexpr unsigned int k_particleLimit = 250;

    void BuildEndpointMap(GraphData& graphData)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        graphData.m_endpointMap.clear();
        for (auto& connectionEntity : graphData.m_connections)
        {
            auto* connection = connectionEntity ? AZ::EntityUtils::FindFirstDerivedComponent<ConnectionComponent>(connectionEntity) : nullptr;
            if (connection)
            {
                graphData.m_endpointMap.emplace(connection->GetSourceEndpoint(), connection->GetTargetEndpoint());
                graphData.m_endpointMap.emplace(connection->GetTargetEndpoint(), connection->GetSourceEndpoint());
            }
        }
    }

    class GraphCanvasSceneDataEventHandler : public AZ::SerializeContext::IEventHandler
    {
    public:
        /// Called to rebuild the Endpoint map
        void OnWriteEnd(void* classPtr) override
        {
            auto* sceneData = reinterpret_cast<GraphData*>(classPtr);

            BuildEndpointMap((*sceneData));
        }
    };

    /////////////////////////////////
    // GraphCanvasConstructSaveData
    /////////////////////////////////
    bool SceneComponent::GraphCanvasConstructSaveData::VersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() == 1)
        {
            AZ::Crc32 typeId = AZ_CRC_CE("Type");

            AZ::SerializeContext::DataElementNode* dataNode = classElement.FindSubElement(typeId);

            ConstructType constructType = ConstructType::Unknown;

            if (dataNode)
            {
                GraphCanvasConstructType deprecatedType = GraphCanvasConstructType::Unknown;
                dataNode->GetData(deprecatedType);

                if (deprecatedType == GraphCanvasConstructType::BlockCommentNode)
                {
                    constructType = ConstructType::NodeGroup;
                }
                else if (deprecatedType == GraphCanvasConstructType::CommentNode)
                {
                    constructType = ConstructType::CommentNode;
                }
                else if (deprecatedType == GraphCanvasConstructType::BookmarkAnchor)
                {
                    constructType = ConstructType::BookmarkAnchor;
                }
            }

            classElement.RemoveElementByName(typeId);

            classElement.AddElementWithData(serializeContext, "Type", constructType);
        }

        return true;
    }

    ///////////////////
    // SceneComponent
    ///////////////////

    void SceneComponent::Reflect(AZ::ReflectContext* context)
    {
        GraphSerialization::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }
            
        serializeContext->Class<GraphData>()
            ->Version(2)
            ->EventHandler<GraphCanvasSceneDataEventHandler>()
            ->Field("m_nodes", &GraphData::m_nodes)
            ->Field("m_connections", &GraphData::m_connections)
            ->Field("m_userData", &GraphData::m_userData)
            ->Field("m_bookmarkAnchors", &GraphData::m_bookmarkAnchors)
        ;

        serializeContext->Class<GraphCanvasConstructSaveData>()
            ->Version(2, &GraphCanvasConstructSaveData::VersionConverter)
            ->Field("Type", &GraphCanvasConstructSaveData::m_constructType)
            ->Field("DataContainer", &GraphCanvasConstructSaveData::m_saveDataContainer)
        ;

        serializeContext->Class<ViewParams>()
            ->Version(1)
            ->Field("Scale", &ViewParams::m_scale)
            ->Field("AnchorX", &ViewParams::m_anchorPointX)
            ->Field("AnchorY", &ViewParams::m_anchorPointY)
            ;

        serializeContext->Class<SceneComponentSaveData>()
            ->Version(3)
            ->Field("Constructs", &SceneComponentSaveData::m_constructs)
            ->Field("ViewParams", &SceneComponentSaveData::m_viewParams)
            ->Field("BookmarkCounter", &SceneComponentSaveData::m_bookmarkCounter)
        ;

        serializeContext->Class<SceneComponent, GraphCanvasPropertyComponent>()
            ->Version(3)
            ->Field("SceneData", &SceneComponent::m_graphData)
            ->Field("ViewParams", &SceneComponent::m_viewParams)
        ;
    }

    SceneComponent::SceneComponent()
        : m_allowReset(false)
        , m_deleteCount(0)
        , m_dragSelectionType(DragSelectionType::OnRelease)
        , m_isLoading(false)
        , m_isPasting(false)
        , m_activateScene(true)
        , m_isDragSelecting(false)
        , m_originalPosition(0,0)
        , m_forceDragReleaseUndo(false)
        , m_isDraggingEntity(false)
        , m_isDraggingConnection(false)
        , m_enableSpliceTracking(false)
        , m_enableNodeDragConnectionSpliceTracking(false)
        , m_enableNodeDragCouplingTracking(false)
        , m_bookmarkCounter(0)
    {
        m_spliceTimer.setInterval(500);
        m_spliceTimer.setSingleShot(true);

        QObject::connect(&m_spliceTimer, &QTimer::timeout, [this]() { OnTrySplice(); });
    }

    SceneComponent::~SceneComponent()
    {
        DestroyItems(m_graphData.m_nodes);
        DestroyItems(m_graphData.m_connections);
        DestroyItems(m_graphData.m_bookmarkAnchors);
        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::DeleteEntity, m_grid);
    }

    void SceneComponent::Init()
    {
        GRAPH_CANVAS_PROFILE_FUNCTION();
        GraphCanvasPropertyComponent::Init();

        // Make the QGraphicsScene Ui element for managing Qt scene items
        m_graphicsSceneUi = AZStd::make_unique<GraphCanvasGraphicsScene>(*this);

        AZ::EntityBus::Handler::BusConnect(GetEntityId());

        AZ::Entity* gridEntity = GridComponent::CreateDefaultEntity();
        m_grid = gridEntity->GetId();

        InitItems(m_graphData.m_nodes);
        InitConnections();
        InitItems(m_graphData.m_bookmarkAnchors);

        // Grids needs to be active for the save information parsing to work correctly
        ActivateItems(AZStd::initializer_list<AZ::Entity*>{ AzToolsFramework::GetEntity(m_grid) });
        ////

        EntitySaveDataRequestBus::Handler::BusConnect(GetEntityId());
    }

    void SceneComponent::Activate()
    {
        GRAPH_CANVAS_PROFILE_FUNCTION();
        GraphCanvasPropertyComponent::Activate();

        // Need to register this before activating saved nodes. Otherwise data is not properly setup.
        const AZ::EntityId& entityId = GetEntityId();

        SceneRequestBus::Handler::BusConnect(entityId);
        SceneMimeDelegateRequestBus::Handler::BusConnect(entityId);
        SceneBookmarkActionBus::Handler::BusConnect(entityId);
        
        // Only want to activate the scene if we have something to activate
        // Otherwise elements may be repeatedly activated/registered to the scene.
        m_activateScene = !m_graphData.m_nodes.empty() || !m_graphData.m_bookmarkAnchors.empty();

        m_mimeDelegateSceneHelper.SetSceneId(entityId);
        m_gestureSceneHelper.SetSceneId(entityId);

        m_nudgingController.SetGraphId(entityId);
        
        m_mimeDelegateSceneHelper.Activate();
        m_gestureSceneHelper.Activate();
    }

    void SceneComponent::Deactivate()
    {
        GRAPH_CANVAS_PROFILE_FUNCTION();
        m_mimeDelegateSceneHelper.Deactivate();
        m_gestureSceneHelper.Deactivate();

        GraphCanvasPropertyComponent::Deactivate();
        
        SceneBookmarkActionBus::Handler::BusDisconnect();
        SceneMimeDelegateRequestBus::Handler::BusDisconnect();
        SceneRequestBus::Handler::BusDisconnect();
        AssetEditorSettingsNotificationBus::Handler::BusDisconnect();

        m_activeDelegates.clear();

        DeactivateItems(m_graphData.m_connections);
        DeactivateItems(m_graphData.m_nodes);
        DeactivateItems(m_graphData.m_bookmarkAnchors);
        DeactivateItems(AZStd::initializer_list<AZ::Entity*>{ AzToolsFramework::GetEntity(m_grid) });
        SceneMemberRequestBus::Event(m_grid, &SceneMemberRequests::ClearScene, GetEntityId());
    }

    void SceneComponent::OnSystemTick()
    {
        ProcessEnableDisableQueue();
    }

    void SceneComponent::OnEntityExists(const AZ::EntityId& /*entityId*/)
    {
        AZ::Entity* entity = GetEntity();

        // A less then ideal way of doing version control on the scenes
        BookmarkManagerComponent* bookmarkComponent = entity->FindComponent<BookmarkManagerComponent>();

        if (bookmarkComponent == nullptr)
        {
            entity->CreateComponent<BookmarkManagerComponent>();
        }

        AZ::EntityBus::Handler::BusDisconnect(GetEntityId());
    }

    void SceneComponent::WriteSaveData(EntitySaveDataContainer& saveDataContainer) const
    {
        GRAPH_CANVAS_PROFILE_FUNCTION();
        SceneComponentSaveData* saveData = saveDataContainer.FindCreateSaveData<SceneComponentSaveData>();
        saveData->ClearConstructData();
        for (AZ::Entity* currentEntity : m_graphData.m_nodes)
        {
            if (GraphUtils::IsComment(currentEntity->GetId()))
            {
                GraphCanvasConstructSaveData* constructSaveData = aznew GraphCanvasConstructSaveData();
                constructSaveData->m_constructType = ConstructType::CommentNode;
                EntitySaveDataRequestBus::Event(currentEntity->GetId(), &EntitySaveDataRequests::WriteSaveData, constructSaveData->m_saveDataContainer);
                saveData->m_constructs.push_back(constructSaveData);
                continue;
            }

            if (GraphUtils::IsNodeGroup(currentEntity->GetId()))
            {
                GraphCanvasConstructSaveData* constructSaveData = aznew GraphCanvasConstructSaveData();
                constructSaveData->m_constructType = ConstructType::NodeGroup;
                EntitySaveDataRequestBus::Event(currentEntity->GetId(), &EntitySaveDataRequests::WriteSaveData, constructSaveData->m_saveDataContainer);
                saveData->m_constructs.push_back(constructSaveData);
                continue;
            }
        }

        saveData->m_constructs.reserve(saveData->m_constructs.size() + m_graphData.m_bookmarkAnchors.size());

        for (AZ::Entity* currentEntity : m_graphData.m_bookmarkAnchors)
        {
            GraphCanvasConstructSaveData* constructSaveData = aznew GraphCanvasConstructSaveData();
            constructSaveData->m_constructType = ConstructType::BookmarkAnchor;
            EntitySaveDataRequestBus::Event(currentEntity->GetId(), &EntitySaveDataRequests::WriteSaveData, constructSaveData->m_saveDataContainer);
            saveData->m_constructs.push_back(constructSaveData);
        }

        saveData->m_viewParams = m_viewParams;
        saveData->m_bookmarkCounter = m_bookmarkCounter;
    }

    void SceneComponent::ReadSaveData(const EntitySaveDataContainer& saveDataContainer)
    {
        GRAPH_CANVAS_PROFILE_FUNCTION();
        if (const SceneComponentSaveData* saveData = saveDataContainer.FindSaveDataAs<SceneComponentSaveData>())
        {
            for (const GraphCanvasConstructSaveData* currentConstruct : saveData->m_constructs)
            {
                AZ::Entity* constructEntity = nullptr;
                switch (currentConstruct->m_constructType)
                {
                case ConstructType::CommentNode:
                    GraphCanvasRequestBus::BroadcastResult(constructEntity, &GraphCanvasRequests::CreateCommentNode);
                    break;
                case ConstructType::NodeGroup:
                    GraphCanvasRequestBus::BroadcastResult(constructEntity, &GraphCanvasRequests::CreateNodeGroup);
                    break;
                case ConstructType::BookmarkAnchor:
                    GraphCanvasRequestBus::BroadcastResult(constructEntity, &GraphCanvasRequests::CreateBookmarkAnchor);
                    break;
                default:
                    break;
                }

                if (constructEntity)
                {
                    constructEntity->Init();
                    constructEntity->Activate();

                    EntitySaveDataRequestBus::Event(constructEntity->GetId(), &EntitySaveDataRequests::ReadSaveData, currentConstruct->m_saveDataContainer);

                    Add(constructEntity->GetId());
                }
            }

            m_viewParams = saveData->m_viewParams;
            m_bookmarkCounter = saveData->m_bookmarkCounter;
        }
    }

    AZStd::any* SceneComponent::GetUserData()
    {
        return &m_graphData.m_userData;
    }

    const AZStd::any* SceneComponent::GetUserDataConst() const
    {
        return &m_graphData.m_userData;
    }

    void SceneComponent::SetEditorId(const EditorId& editorId)
    {
        if (m_editorId != editorId)
        {
            m_editorId = editorId;
            m_mimeDelegateSceneHelper.SetEditorId(editorId);
            m_gestureSceneHelper.SetEditorId(editorId);

            OnSettingsChanged();

            AssetEditorSettingsNotificationBus::Handler::BusConnect(m_editorId);

            StyleManagerNotificationBus::Handler::BusConnect(m_editorId);
            SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnStylesChanged);
            StyleNotificationBus::Event(m_grid, &StyleNotifications::OnStyleChanged);
        }
    }

    EditorId SceneComponent::GetEditorId() const
    {
        return m_editorId;
    }

    AZ::EntityId SceneComponent::GetGrid() const
    {
        return m_grid;
    }

    GraphicsEffectId SceneComponent::CreatePulse(const AnimatedPulseConfiguration& pulseConfiguration)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AnimatedPulse* animatedPulse = aznew AnimatedPulse(pulseConfiguration);

        ConfigureAndAddGraphicsEffect(animatedPulse);

        return animatedPulse->GetEffectId();
    }

    GraphicsEffectId SceneComponent::CreatePulseAroundArea(const QRectF& area, int gridSteps, AnimatedPulseConfiguration& pulseConfiguration)
    {
        AZ::EntityId gridId = GetGrid();

        AZ::Vector2 minorPitch(0, 0);
        GridRequestBus::EventResult(minorPitch, gridId, &GridRequests::GetMinorPitch);

        pulseConfiguration.m_controlPoints.reserve(4);

        for (QPointF currentPoint : { area.topLeft(), area.topRight(), area.bottomRight(), area.bottomLeft()})
        {
            QPointF directionVector = currentPoint - area.center();

            directionVector = QtVectorMath::Normalize(directionVector);

            QPointF finalPoint(currentPoint.x() + directionVector.x() * minorPitch.GetX() * gridSteps, currentPoint.y() + directionVector.y() * minorPitch.GetY() * gridSteps);

            pulseConfiguration.m_controlPoints.emplace_back(currentPoint, finalPoint);
        }

        return CreatePulse(pulseConfiguration);
    }

    GraphicsEffectId SceneComponent::CreatePulseAroundSceneMember(const AZ::EntityId& memberId, int gridSteps, AnimatedPulseConfiguration pulseConfiguration)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        QGraphicsItem* graphicsItem = nullptr;
        SceneMemberUIRequestBus::EventResult(graphicsItem, memberId, &SceneMemberUIRequests::GetRootGraphicsItem);

        if (graphicsItem)
        {
            QRectF target = graphicsItem->sceneBoundingRect();

            return CreatePulseAroundArea(target, gridSteps, pulseConfiguration);            
        }

        return AZ::EntityId();
    }

    GraphicsEffectId SceneComponent::CreateCircularPulse(const AZ::Vector2& point, float initialRadius, float finalRadius, AnimatedPulseConfiguration pulseConfiguration)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        static const int k_circleSegemnts = 9;

        pulseConfiguration.m_controlPoints.clear();
        pulseConfiguration.m_controlPoints.reserve(k_circleSegemnts);

        float step = AZ::Constants::TwoPi / static_cast<float>(k_circleSegemnts);

        // Start it at some random offset just to hide the staticness of it.
        float currentAngle = AZ::Constants::TwoPi * (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX));

        for (int i = 0; i < k_circleSegemnts; ++i)
        {
            QPointF outerPoint;
            outerPoint.setX(point.GetX() + initialRadius * std::sin(currentAngle));
            outerPoint.setY(point.GetY() + initialRadius * std::cos(currentAngle));

            QPointF innerPoint;
            innerPoint.setX(point.GetX() + finalRadius * std::sin(currentAngle));
            innerPoint.setY(point.GetY() + finalRadius * std::cos(currentAngle));

            currentAngle += step;

            if (currentAngle > AZ::Constants::TwoPi)
            {
                currentAngle -= AZ::Constants::TwoPi;
            }

            pulseConfiguration.m_controlPoints.emplace_back(outerPoint, innerPoint);
        }

        return CreatePulse(pulseConfiguration);
    }

    GraphicsEffectId SceneComponent::CreateOccluder(const OccluderConfiguration& occluderConfiguration)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        Occluder* occluder = aznew Occluder(occluderConfiguration);

        ConfigureAndAddGraphicsEffect(occluder);

        return occluder->GetEffectId();
    }

    GraphicsEffectId SceneComponent::CreateGlow(const FixedGlowOutlineConfiguration& configuration)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        GlowOutlineGraphicsItem* outlineGraphicsItem = aznew GlowOutlineGraphicsItem(configuration);

        ConfigureAndAddGraphicsEffect(outlineGraphicsItem);

        return outlineGraphicsItem->GetEffectId();
    }

    GraphicsEffectId SceneComponent::CreateGlowOnSceneMember(const SceneMemberGlowOutlineConfiguration& configuration)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();

        QGraphicsItem* graphicsItem = nullptr;
        VisualRequestBus::EventResult(graphicsItem, configuration.m_sceneMember, &VisualRequests::AsGraphicsItem);

        if (m_hiddenElements.count(graphicsItem) == 0)
        {
            GlowOutlineGraphicsItem* outlineGraphicsItem = aznew GlowOutlineGraphicsItem(configuration);

            ConfigureAndAddGraphicsEffect(outlineGraphicsItem);

            return outlineGraphicsItem->GetEffectId();
        }
        else
        {
            return GraphicsEffectId();
        }
    }

    GraphicsEffectId SceneComponent::CreateParticle(const ParticleConfiguration& configuration)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();

        if constexpr (k_particleLimit == 0)
        {
            return GraphicsEffectId();
        }

        ParticleGraphicsItem* particleGraphicsItem = aznew ParticleGraphicsItem(configuration);

        ConfigureAndAddGraphicsEffect(particleGraphicsItem);

        m_activeParticles.emplace_back(particleGraphicsItem->GetEffectId());
        
        if (m_activeParticles.size() >= k_particleLimit)
        {
            CancelGraphicsEffect(m_activeParticles.front());
        }

        return particleGraphicsItem->GetEffectId();
    }

    AZStd::vector< GraphicsEffectId > SceneComponent::ExplodeSceneMember(const AZ::EntityId& memberId, float fillPercent)
    {
        AZStd::vector< GraphicsEffectId > effectIds;

        if (GraphUtils::IsNode(memberId) || GraphUtils::IsNodeGroup(memberId))
        {
            const Styling::StyleHelper* styleHelper = nullptr;
            QColor drawColor;

            if (GraphUtils::IsCollapsedNodeGroup(memberId))
            {
                AZ::EntityId sourceGroupId;
                CollapsedNodeGroupRequestBus::EventResult(sourceGroupId, memberId, &CollapsedNodeGroupRequests::GetSourceGroup);

                AZ::Color azColor;
                NodeGroupRequestBus::EventResult(azColor, sourceGroupId, &NodeGroupRequests::GetGroupColor);

                drawColor = ConversionUtils::AZToQColor(azColor);
            }
            else if (GraphUtils::IsNode(memberId))
            {
                PaletteIconConfiguration iconConfiguration;
                NodeTitleRequestBus::Event(memberId, &NodeTitleRequests::ConfigureIconConfiguration, iconConfiguration);

                StyleManagerRequestBus::EventResult(styleHelper, m_editorId, &StyleManagerRequests::FindPaletteIconStyleHelper, iconConfiguration);
            }
            else
            {
                AZ::Color azColor;
                NodeGroupRequestBus::EventResult(azColor, memberId, &NodeGroupRequests::GetGroupColor);

                drawColor = ConversionUtils::AZToQColor(azColor);
            }

            QGraphicsItem* graphicsItem = nullptr;
            SceneMemberUIRequestBus::EventResult(graphicsItem, memberId, &SceneMemberUIRequests::GetRootGraphicsItem);

            if (graphicsItem)
            {
                QRectF boundingRect = graphicsItem->sceneBoundingRect();

                AZ::Vector2 minorPitch;
                GridRequestBus::EventResult(minorPitch, GetGrid(), &GridRequests::GetMinorPitch);

                AZ::Vector2 boxSize = minorPitch;
                AZ::Vector2 impulseVector = minorPitch;

                ParticleConfiguration baseConfiguration;
                baseConfiguration.m_boundingArea = QRectF(0,0, boxSize.GetX(), boxSize.GetY());

                baseConfiguration.m_hasGravity = true;

                baseConfiguration.m_alphaFade = true;
                baseConfiguration.m_alphaStart = 1.0f;
                baseConfiguration.m_alphaEnd = 0.0f;

                baseConfiguration.m_styleHelper = styleHelper;
                baseConfiguration.m_color = drawColor;

                baseConfiguration.m_zValue = aznumeric_cast<int>(graphicsItem->zValue());

                boundingRect.adjust(minorPitch.GetX() * 0.5f, minorPitch.GetY() * 0.5f, -minorPitch.GetX() * 0.5f, -minorPitch.GetY() * 0.5f);

                int yPos = aznumeric_cast<int>(boundingRect.top());
                int xPos = aznumeric_cast<int>(boundingRect.left());

                while (yPos < boundingRect.bottom())
                {
                    while (xPos < boundingRect.right())
                    {
                        float skipChance = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

                        if (skipChance <= fillPercent)
                        {
                            baseConfiguration.m_boundingArea.moveTopLeft(QPointF(xPos, yPos));

                            double impulseVariance = static_cast<double>(rand()) / static_cast<double>(RAND_MAX);

                            double directionSpray = ((boundingRect.center().x() - xPos)/boundingRect.width()) * 2.0;

                            double xImpulse = impulseVector.GetX() * 10.0 * -directionSpray + impulseVector.GetX() * 6.0 * impulseVariance;

                            double yImpulse = -impulseVector.GetY() * 4.0 - impulseVector.GetY() * 2.0 * impulseVariance;

                            baseConfiguration.m_initialImpulse = QPointF(xImpulse, yImpulse);

                            baseConfiguration.m_lifespan = AZStd::chrono::milliseconds(400 + rand() % 125);
                            baseConfiguration.m_fadeTime = baseConfiguration.m_lifespan;

                            effectIds.emplace_back(CreateParticle(baseConfiguration));
                        }

                        xPos += aznumeric_cast<int>(minorPitch.GetX());
                    }

                    yPos += aznumeric_cast<int>(minorPitch.GetY());
                    xPos = aznumeric_cast<int>(boundingRect.left());
                }
            }
        }

        return effectIds;
    }

    void SceneComponent::CancelGraphicsEffect(const GraphicsEffectId& effectId)
    {
        QGraphicsItem* graphicsItem = nullptr;
        GraphicsEffectRequestBus::EventResult(graphicsItem, effectId, &GraphicsEffectRequests::AsQGraphicsItem);

        DestroyGraphicsItem(effectId, graphicsItem);
    }

    bool SceneComponent::AddNode(const AZ::EntityId& nodeId, const AZ::Vector2& position, bool isPaste)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZ::Entity* nodeEntity(AzToolsFramework::GetEntity(nodeId));
        AZ_Assert(nodeEntity, "Node (ID: %s) Entity not found!", nodeId.ToString().data());
        AZ_Assert(nodeEntity->GetState() == AZ::Entity::State::Active, "Only active node entities can be added to a scene");

        QGraphicsItem* item = nullptr;
        SceneMemberUIRequestBus::EventResult(item, nodeId, &SceneMemberUIRequests::GetRootGraphicsItem);
        AZ_Assert(item && !item->parentItem(), "Nodes must have a \"root\", unparented visual/QGraphicsItem");

        auto foundIt = AZStd::find_if(m_graphData.m_nodes.begin(), m_graphData.m_nodes.end(), [&nodeEntity](const AZ::Entity* node) { return node->GetId() == nodeEntity->GetId(); });
        if (foundIt == m_graphData.m_nodes.end())
        {
            m_graphData.m_nodes.emplace(nodeEntity);

            AddSceneMember(nodeId, true, position);
            SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnNodeAdded, nodeId, isPaste);

            m_mimeDelegateSceneHelper.SignalNodeCreated(nodeId);

            return true;
        }

        return false;
    }

    void SceneComponent::AddNodes(const AZStd::vector<AZ::EntityId>& nodeIds)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        for (const auto& nodeId : nodeIds)
        {
            AZ::Vector2 position;
            GeometryRequestBus::EventResult(position, nodeId, &GeometryRequests::GetPosition);
            AddNode(nodeId, position, false);
        }
    }

    bool SceneComponent::RemoveNode(const AZ::EntityId& nodeId)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        auto foundIt = AZStd::find_if(m_graphData.m_nodes.begin(), m_graphData.m_nodes.end(), [&nodeId](const AZ::Entity* node) { return node->GetId() == nodeId; });
        if (foundIt != m_graphData.m_nodes.end())
        {
            VisualNotificationBus::MultiHandler::BusDisconnect(nodeId);
            GeometryNotificationBus::MultiHandler::BusDisconnect(nodeId);
            m_graphData.m_nodes.erase(foundIt);

            SceneMemberNotificationBus::Event(nodeId, &SceneMemberNotifications::PreOnRemovedFromScene, GetEntityId());

            QGraphicsItem* item = nullptr;
            SceneMemberUIRequestBus::EventResult(item, nodeId, &SceneMemberUIRequests::GetRootGraphicsItem);

            RemoveItemFromScene(item);

            UnregisterSelectionItem(nodeId);
            SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnNodeRemoved, nodeId);
            SceneMemberRequestBus::Event(nodeId, &SceneMemberRequests::ClearScene, GetEntityId());

            return true;
        }

        return false;
    }

    AZStd::vector<AZ::EntityId> SceneComponent::GetNodes() const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::vector<AZ::EntityId> result;

        for (const auto& nodeRef : m_graphData.m_nodes)
        {
            result.push_back(nodeRef->GetId());
        }

        return result;
    }

    AZStd::vector<AZ::EntityId> SceneComponent::GetSelectedNodes() const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::vector<AZ::EntityId> result;
        if (m_graphicsSceneUi)
        {
            const QList<QGraphicsItem*> selected(m_graphicsSceneUi->selectedItems());
            result.reserve(selected.size());

            for (QGraphicsItem* item : selected)
            {
                auto entry = m_itemLookup.find(item);
                if (entry != m_itemLookup.end())
                {
                    AZ::Entity* entity = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entry->second);

                    if (m_graphData.m_nodes.count(entity) > 0)
                    {
                        result.push_back(entity->GetId());
                    }
                }
            }
        }

        return result;
    }

    void SceneComponent::DeleteNodeAndStitchConnections(const AZ::EntityId& node)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        if (node.IsValid())
        {
            ScopedGraphUndoBatch undoBatch(GetEntityId());

            float explosionDensity = 0.6f;

            if (GraphUtils::IsNodeGroup(node))
            {
                explosionDensity = 0.3f;
            }

            ExplodeSceneMember(node, explosionDensity);
            GraphUtils::DetachNodeAndStitchConnections(node);
            Delete({ node });
        }
    }

    AZ::EntityId SceneComponent::CreateConnectionBetween(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        if (!sourceEndpoint.IsValid() || !targetEndpoint.IsValid())
        {
            return AZ::EntityId();
        }

        AZ::EntityId connectionId;

        bool isValidConnection = false;
        SlotRequestBus::EventResult(isValidConnection, sourceEndpoint.GetSlotId(), &SlotRequests::CanCreateConnectionTo, targetEndpoint);

        if (isValidConnection)
        {
            SlotRequestBus::EventResult(connectionId, sourceEndpoint.GetSlotId(), &SlotRequests::CreateConnectionWithEndpoint, targetEndpoint);
        }

        return connectionId;
    }

    bool SceneComponent::AddConnection(const AZ::EntityId& connectionId)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZ_Assert(connectionId.IsValid(), "Connection ID %s is not valid!", connectionId.ToString().data());

        AZ::Entity* connectionEntity(AzToolsFramework::GetEntity(connectionId));
        auto connection = connectionEntity ? AZ::EntityUtils::FindFirstDerivedComponent<ConnectionComponent>(connectionEntity) : nullptr;
        AZ_Warning("Graph Canvas", connection->GetEntity()->GetState() == AZ::Entity::State::Active, "Only active connection entities can be added to a scene");
        AZ_Warning("Graph Canvas", connection, "Couldn't find the connection's component (ID: %s)!", connectionId.ToString().data());

        if (connection)
        {
            auto foundIt = AZStd::find_if(m_graphData.m_connections.begin(), m_graphData.m_connections.end(), [&connection](const AZ::Entity* connectionEntity) { return connectionEntity->GetId() == connection->GetEntityId(); });
            if (foundIt == m_graphData.m_connections.end())
            {
                AddSceneMember(connectionId);

                m_graphData.m_connections.emplace(connection->GetEntity());
                Endpoint sourceEndpoint;
                ConnectionRequestBus::EventResult(sourceEndpoint, connectionId, &ConnectionRequests::GetSourceEndpoint);
                Endpoint targetEndpoint;
                ConnectionRequestBus::EventResult(targetEndpoint, connectionId, &ConnectionRequests::GetTargetEndpoint);
                m_graphData.m_endpointMap.emplace(sourceEndpoint, targetEndpoint);
                m_graphData.m_endpointMap.emplace(targetEndpoint, sourceEndpoint);

                SlotRequestBus::Event(sourceEndpoint.GetSlotId(), &SlotRequests::AddConnectionId, connectionId, targetEndpoint);
                SlotRequestBus::Event(targetEndpoint.GetSlotId(), &SlotRequests::AddConnectionId, connectionId, sourceEndpoint);
                SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnConnectionAdded, connectionId);

                return true;
            }
        }

        return false;
    }

    void SceneComponent::AddConnections(const AZStd::vector<AZ::EntityId>& connectionIds)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        for (const auto& connectionId : connectionIds)
        {
            AddConnection(connectionId);
        }
    }

    bool SceneComponent::RemoveConnection(const AZ::EntityId& connectionId)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZ_Assert(connectionId.IsValid(), "Connection ID %s is not valid!", connectionId.ToString().data());

        auto foundIt = AZStd::find_if(m_graphData.m_connections.begin(), m_graphData.m_connections.end(), [&connectionId](const AZ::Entity* connection) { return connection->GetId() == connectionId; });
        if (foundIt != m_graphData.m_connections.end())
        {
            VisualNotificationBus::MultiHandler::BusDisconnect(connectionId);
            GeometryNotificationBus::MultiHandler::BusDisconnect(connectionId);

            Endpoint sourceEndpoint;
            ConnectionRequestBus::EventResult(sourceEndpoint, connectionId, &ConnectionRequests::GetSourceEndpoint);
            Endpoint targetEndpoint;
            ConnectionRequestBus::EventResult(targetEndpoint, connectionId, &ConnectionRequests::GetTargetEndpoint);
            m_graphData.m_endpointMap.erase(sourceEndpoint);
            m_graphData.m_endpointMap.erase(targetEndpoint);
            m_graphData.m_connections.erase(foundIt);

            QGraphicsItem* item{};
            SceneMemberUIRequestBus::EventResult(item, connectionId, &SceneMemberUIRequests::GetRootGraphicsItem);
            AZ_Assert(item, "Connections must have a visual/QGraphicsItem");
            RemoveItemFromScene(item);

            UnregisterSelectionItem(connectionId);

            SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnConnectionRemoved, connectionId);
            SlotRequestBus::Event(targetEndpoint.GetSlotId(), &SlotRequests::RemoveConnectionId, connectionId, sourceEndpoint);
            SlotRequestBus::Event(sourceEndpoint.GetSlotId(), &SlotRequests::RemoveConnectionId, connectionId, targetEndpoint);

            return true;
        }

        return false;
    }

    AZStd::vector<AZ::EntityId> SceneComponent::GetConnections() const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::vector<AZ::EntityId> result;

        for (const auto& connection : m_graphData.m_connections)
        {
            result.push_back(connection->GetId());
        }

        return result;
    }

    AZStd::vector<AZ::EntityId> SceneComponent::GetSelectedConnections() const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::vector<AZ::EntityId> result;
        if (m_graphicsSceneUi)
        {
            const QList<QGraphicsItem*> selected(m_graphicsSceneUi->selectedItems());
            result.reserve(selected.size());

            for (QGraphicsItem* item : selected)
            {
                auto entry = m_itemLookup.find(item);
                if (entry != m_itemLookup.end())
                {
                    AZ::Entity* entity{};
                    AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entry->second);
                    if (entity && AZ::EntityUtils::FindFirstDerivedComponent<ConnectionComponent>(entity))
                    {
                        result.push_back(entity->GetId());
                    }
                }
            }
        }

        return result;
    }

    AZStd::vector<AZ::EntityId> SceneComponent::GetConnectionsForEndpoint(const Endpoint& firstEndpoint) const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::vector<AZ::EntityId> result;
        for (const auto& connection : m_graphData.m_connections)
        {
            Endpoint sourceEndpoint;
            ConnectionRequestBus::EventResult(sourceEndpoint, connection->GetId(), &ConnectionRequests::GetSourceEndpoint);
            Endpoint targetEndpoint;
            ConnectionRequestBus::EventResult(targetEndpoint, connection->GetId(), &ConnectionRequests::GetTargetEndpoint);

            if (sourceEndpoint == firstEndpoint || targetEndpoint == firstEndpoint)
            {
                result.push_back(connection->GetId());
            }
        }

        return result;
    }

    bool SceneComponent::IsEndpointConnected(const Endpoint& endpoint) const
    {
        return m_graphData.m_endpointMap.count(endpoint) > 0;
    }

    AZStd::vector<Endpoint> SceneComponent::GetConnectedEndpoints(const Endpoint& firstEndpoint) const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::vector<Endpoint> connectedEndpoints;
        auto otherEndpointsRange = m_graphData.m_endpointMap.equal_range(firstEndpoint);
        for (auto otherIt = otherEndpointsRange.first; otherIt != otherEndpointsRange.second; ++otherIt)
        {
            connectedEndpoints.push_back(otherIt->second);
        }
        return connectedEndpoints;
    }

    bool SceneComponent::CreateConnection(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZ::Entity* foundEntity = nullptr;
        if (FindConnection(foundEntity, sourceEndpoint, targetEndpoint))
        {
            AZ_Warning("Graph Canvas", false, "Attempting to create duplicate connection between source endpoint (%s, %s) and target endpoint(%s, %s)",
                sourceEndpoint.GetNodeId().ToString().data(), sourceEndpoint.GetSlotId().ToString().data(),
                targetEndpoint.GetNodeId().ToString().data(), targetEndpoint.GetSlotId().ToString().data());
            return false;
        }

        // Hunt through our nodes for both the source and target endpoint at the same time.
        AZStd::pair<bool, bool> findResult(false, false);
        AZStd::find_if(m_graphData.m_nodes.begin(), m_graphData.m_nodes.end(), [&findResult, &sourceEndpoint, &targetEndpoint](const AZ::Entity* node) 
            { 
                findResult.first = findResult.first || node->GetId() == sourceEndpoint.GetNodeId(); 
                findResult.second = findResult.second || node->GetId() == targetEndpoint.GetNodeId();
                return findResult.first && findResult.second;
            }
        );

        if (!findResult.first)
        {
            AZ_Error("Scene", false, "The source node with id %s is not in this scene, a connection cannot be made", sourceEndpoint.GetNodeId().ToString().data());
            return false;
        }
        else if (!findResult.second)
        {
            AZ_Error("Scene", false, "The target node with id %s is not in this scene, a connection cannot be made", targetEndpoint.GetNodeId().ToString().data());
            return false;
        }

        AZ::EntityId connectionEntity;
        SlotRequestBus::EventResult(connectionEntity, sourceEndpoint.GetSlotId(), &SlotRequests::CreateConnectionWithEndpoint, targetEndpoint);

        return true;
    }

    bool SceneComponent::DisplayConnection(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZ::Entity* foundEntity = nullptr;
        if (FindConnection(foundEntity, sourceEndpoint, targetEndpoint))
        {
            AZ_Warning("Graph Canvas", false, "Attempting to create duplicate connection between source endpoint (%s, %s) and target endpoint(%s, %s)",
                sourceEndpoint.GetNodeId().ToString().data(), sourceEndpoint.GetSlotId().ToString().data(),
                targetEndpoint.GetNodeId().ToString().data(), targetEndpoint.GetSlotId().ToString().data());
            return false;
        }

        // Hunt through our nodes for both the source and target endpoint at the same time.
        AZStd::pair<bool, bool> findResult(false, false);
        AZStd::find_if(m_graphData.m_nodes.begin(), m_graphData.m_nodes.end(), [&findResult, &sourceEndpoint, &targetEndpoint](const AZ::Entity* node)
        {
            findResult.first = findResult.first || node->GetId() == sourceEndpoint.GetNodeId();
            findResult.second = findResult.second || node->GetId() == targetEndpoint.GetNodeId();
            return findResult.first && findResult.second;
        }
        );

        if (!findResult.first)
        {
            AZ_Error("Scene", false, "The source node with id %s is not in this scene, a connection cannot be made", sourceEndpoint.GetNodeId().ToString().data());
            return false;
        }
        else if (!findResult.second)
        {
            AZ_Error("Scene", false, "The target node with id %s is not in this scene, a connection cannot be made", targetEndpoint.GetNodeId().ToString().data());
            return false;
        }

        AZ::EntityId connectionEntity;
        SlotRequestBus::EventResult(connectionEntity, sourceEndpoint.GetSlotId(), &SlotRequests::DisplayConnectionWithEndpoint, targetEndpoint);

        return true;
    }

    bool SceneComponent::Disconnect(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint)
    {
        AZ::Entity* connectionEntity = nullptr;
        if (FindConnection(connectionEntity, sourceEndpoint, targetEndpoint) && RemoveConnection(connectionEntity->GetId()))
        {
            delete connectionEntity;
            return true;
        }
        return false;
    }

    bool SceneComponent::DisconnectById(const AZ::EntityId& connectionId)
    {
        if (RemoveConnection(connectionId))
        {
            AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::DeleteEntity, connectionId);
            return true;
        }

        return false;
    }

    bool SceneComponent::FindConnection(AZ::Entity*& connectionEntity, const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint) const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZ::Entity* foundEntity = nullptr;

        AZStd::vector< ConnectionId > connectionIds;
        SlotRequestBus::EventResult(connectionIds, sourceEndpoint.GetSlotId(), &SlotRequests::GetConnections);

        for (const ConnectionId& connectionId : connectionIds)
        {
            Endpoint testTargetEndpoint;
            ConnectionRequestBus::EventResult(testTargetEndpoint, connectionId, &ConnectionRequests::GetTargetEndpoint);

            if (targetEndpoint == testTargetEndpoint)
            {
                AZ::ComponentApplicationBus::BroadcastResult(foundEntity, &AZ::ComponentApplicationRequests::FindEntity, connectionId);
                break;
            }
        }

        if (foundEntity)
        {
            connectionEntity = foundEntity;
            return true;
        }

        return false;
    }

    bool SceneComponent::AddBookmarkAnchor(const AZ::EntityId& bookmarkAnchorId, const AZ::Vector2& position)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZ::Entity* anchorEntity(AzToolsFramework::GetEntity(bookmarkAnchorId));
        AZ_Assert(anchorEntity, "BookmarkAnchor (ID: %s) Entity not found!", bookmarkAnchorId.ToString().data());
        AZ_Assert(anchorEntity->GetState() == AZ::Entity::State::Active, "Only active node entities can be added to a scene");

        QGraphicsItem* item = nullptr;
        SceneMemberUIRequestBus::EventResult(item, bookmarkAnchorId, &SceneMemberUIRequests::GetRootGraphicsItem);
        AZ_Assert(item && !item->parentItem(), "BookmarkAnchors must have a \"root\", unparented visual/QGraphicsItem");

        auto foundIt = AZStd::find_if(m_graphData.m_bookmarkAnchors.begin(), m_graphData.m_bookmarkAnchors.end(), [&anchorEntity](const AZ::Entity* entity) { return entity->GetId() == anchorEntity->GetId(); });
        if (foundIt == m_graphData.m_bookmarkAnchors.end())
        {
            m_graphData.m_bookmarkAnchors.emplace(anchorEntity);
            AddSceneMember(bookmarkAnchorId, true, position);
            return true;
        }

        return false;
    }

    bool SceneComponent::RemoveBookmarkAnchor(const AZ::EntityId& bookmarkAnchorId)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        auto foundIt = AZStd::find_if(m_graphData.m_bookmarkAnchors.begin(), m_graphData.m_bookmarkAnchors.end(), [&bookmarkAnchorId](const AZ::Entity* anchorEntity) { return anchorEntity->GetId() == bookmarkAnchorId; });
        if (foundIt != m_graphData.m_bookmarkAnchors.end())
        {
            VisualNotificationBus::MultiHandler::BusDisconnect(bookmarkAnchorId);
            GeometryNotificationBus::MultiHandler::BusDisconnect(bookmarkAnchorId);
            m_graphData.m_bookmarkAnchors.erase(foundIt);

            QGraphicsItem* item = nullptr;
            SceneMemberUIRequestBus::EventResult(item, bookmarkAnchorId, &SceneMemberUIRequests::GetRootGraphicsItem);

            RemoveItemFromScene(item);

            UnregisterSelectionItem(bookmarkAnchorId);
            SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnSceneMemberRemoved, bookmarkAnchorId);
            SceneMemberRequestBus::Event(bookmarkAnchorId, &SceneMemberRequests::ClearScene, GetEntityId());

            return true;
        }

        return false;
    }

    bool SceneComponent::Add(const AZ::EntityId& entityId, bool isPaste)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZ::Entity* actual = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(actual, &AZ::ComponentApplicationRequests::FindEntity, entityId);
        if (!actual)
        {
            return false;
        }

        if (AZ::EntityUtils::FindFirstDerivedComponent<NodeComponent>(actual))
        {
            AZ::Vector2 position;
            GeometryRequestBus::EventResult(position, entityId, &GeometryRequests::GetPosition);
            return AddNode(entityId, position, isPaste);
        }
        else if (AZ::EntityUtils::FindFirstDerivedComponent<ConnectionComponent>(actual))
        {
            return AddConnection(entityId);
        }
        else if (AZ::EntityUtils::FindFirstDerivedComponent<BookmarkAnchorComponent>(actual))
        {
            AZ::Vector2 position;
            GeometryRequestBus::EventResult(position, entityId, &GeometryRequests::GetPosition);
            return AddBookmarkAnchor(entityId, position);
        }
        else
        {
            AZ_Error("Scene", false, "The entity does not appear to be a valid scene membership candidate (ID: %s)", entityId.ToString().data());
        }

        return false;
    }

    bool SceneComponent::Remove(const AZ::EntityId& entityId)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZ::Entity* actual = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(actual, &AZ::ComponentApplicationRequests::FindEntity, entityId);
        if (!actual)
        {
            return false;
        }

        if (m_pressedEntity == entityId)
        {
            VisualNotificationBus::Event(m_pressedEntity, &VisualNotifications::OnMouseRelease, m_pressedEntity, nullptr);
        }

        if (AZ::EntityUtils::FindFirstDerivedComponent<NodeComponent>(actual))
        {
            return RemoveNode(entityId);
        }
        else if (AZ::EntityUtils::FindFirstDerivedComponent<ConnectionComponent>(actual))
        {
            return RemoveConnection(entityId);
        }
        else if (AZ::EntityUtils::FindFirstDerivedComponent<BookmarkAnchorComponent>(actual))
        {
            return RemoveBookmarkAnchor(entityId);
        }
        else
        {
            AZ_Error("Scene", false, "The entity does not appear to be a valid scene membership candidate (ID: %s)", entityId.ToString().data());
        }

        return false;
    }

    bool SceneComponent::Show(const AZ::EntityId& graphMember)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        if (m_graphicsSceneUi)
        {
            QGraphicsItem* graphicsItem = nullptr;
            SceneMemberUIRequestBus::EventResult(graphicsItem, graphMember, &SceneMemberUIRequests::GetRootGraphicsItem);

            if (graphicsItem)
            {
                auto hiddenIter = m_hiddenElements.find(graphicsItem);

                bool isWrapped = false;
                NodeRequestBus::EventResult(isWrapped, graphMember, &NodeRequests::IsWrapped);

                if (!isWrapped)
                {
                    if (hiddenIter != m_hiddenElements.end())
                    {
                        if (GeometryRequestBus::FindFirstHandler(graphMember) != nullptr)
                        {
                            AZ::Vector2 position(0, 0);
                            GeometryRequestBus::EventResult(position, graphMember, &GeometryRequests::GetPosition);

                            graphicsItem->setPos(ConversionUtils::AZToQPoint(position));
                        }
                        
                        graphicsItem->show();

                        SceneMemberNotificationBus::Event(graphMember, &SceneMemberNotifications::OnSceneMemberShown);

                        m_hiddenElements.erase(hiddenIter);

                        return true;
                    }
                }
                else
                {
                    if (hiddenIter != m_hiddenElements.end())
                    {
                        m_hiddenElements.erase(hiddenIter);
                    }

                    graphicsItem->show();
                    return true;
                }
            }
        }

        return false;
    }

    bool SceneComponent::Hide(const AZ::EntityId& graphMember)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        if (m_graphicsSceneUi)
        {
            QGraphicsItem* graphicsItem = nullptr;
            SceneMemberUIRequestBus::EventResult(graphicsItem, graphMember, &SceneMemberUIRequests::GetRootGraphicsItem);

            if (graphicsItem)
            {
                graphicsItem->hide();

                auto insertResult = m_hiddenElements.insert(graphicsItem);

                if (insertResult.second)
                {
                    SceneMemberNotificationBus::Event(graphMember, &SceneMemberNotifications::OnSceneMemberHidden);
                }

                return insertResult.second;
            }
        }

        return false;
    }

    bool SceneComponent::IsHidden(const AZ::EntityId& graphMember) const
    {
        QGraphicsItem* graphicsItem = nullptr;
        SceneMemberUIRequestBus::EventResult(graphicsItem, graphMember, &SceneMemberUIRequests::GetRootGraphicsItem);

        return m_hiddenElements.find(graphicsItem) != m_hiddenElements.end();
    }

    bool SceneComponent::Enable(const NodeId& nodeId)
    {
        if (!AZ::SystemTickBus::Handler::BusIsConnected())
        {
            AZ::SystemTickBus::Handler::BusConnect();
        }

        m_queuedDisable.erase(nodeId);

        auto insertResult = m_queuedEnable.insert(nodeId);
        
        return insertResult.second;
    }

    void SceneComponent::EnableVisualState(const NodeId& nodeId)
    {
        if (!AZ::SystemTickBus::Handler::BusIsConnected())
        {
            AZ::SystemTickBus::Handler::BusConnect();
        }

        m_queuedVisualDisable.erase(nodeId);
        m_queuedVisualEnable.insert(nodeId);
    }

    void SceneComponent::EnableSelection()
    {
        AZStd::vector< NodeId > selectedNodes = GetSelectedNodes();

        for (NodeId nodeId : selectedNodes)
        {
            Enable(nodeId);
        }
    }

    bool SceneComponent::Disable(const NodeId& nodeId)
    {
        if (!AZ::SystemTickBus::Handler::BusIsConnected())
        {
            AZ::SystemTickBus::Handler::BusConnect();
        }

        m_queuedEnable.erase(nodeId);

        auto insertResult = m_queuedDisable.insert(nodeId);

        return insertResult.second;
    }

    void SceneComponent::DisableVisualState(const NodeId& nodeId)
    {
        if (!AZ::SystemTickBus::Handler::BusIsConnected())
        {
            AZ::SystemTickBus::Handler::BusConnect();
        }

        m_queuedVisualEnable.erase(nodeId);
        m_queuedVisualDisable.insert(nodeId);
    }

    void SceneComponent::DisableSelection()
    {
        AZStd::vector< NodeId > selectedNodes = GetSelectedNodes();

        for (NodeId nodeId : selectedNodes)
        {
            // Temporarily disable collapsed node groups until we figure out how disabled groups should work.
            //
            // Node groups can still be partially disabled if they're connections are disabled, but you won't be able
            // to disable them directly.
            if (!GraphUtils::IsCollapsedNodeGroup(nodeId))
            {
                Disable(nodeId);
            }
        }
    }

    void SceneComponent::ProcessEnableDisableQueue()
    {
        if (!m_queuedDisable.empty())
        {
            GraphModelRequestBus::Event(GetEntityId(), &GraphModelRequests::DisableNodes, m_queuedDisable);
            m_queuedDisable.clear();
        }

        if (!m_queuedEnable.empty())
        {
            GraphModelRequestBus::Event(GetEntityId(), &GraphModelRequests::EnableNodes, m_queuedEnable);
            m_queuedEnable.clear();
        }

        if (!m_queuedVisualDisable.empty())
        {
            GraphUtils::SetNodesEnabledState(m_queuedVisualDisable, RootGraphicsItemEnabledState::ES_Disabled);
            m_queuedVisualDisable.clear();
        }

        if (!m_queuedVisualEnable.empty())
        {
            GraphUtils::SetNodesEnabledState(m_queuedVisualEnable, RootGraphicsItemEnabledState::ES_Enabled);
            m_queuedVisualEnable.clear();
        }

        AZ::SystemTickBus::Handler::BusDisconnect();
    }

    void SceneComponent::ClearSelection()
    {
        if (m_graphicsSceneUi)
        {
            QSignalBlocker signalBlocker(m_graphicsSceneUi.get());
            m_graphicsSceneUi->clearSelection();

            // Always signal the selection change when being told to clear selection.
            // Makes it easier to synchronize selections states across multiple panels.
            OnSelectionChanged();
        }
    }

    void SceneComponent::SetSelectedArea(const AZ::Vector2& topLeft, const AZ::Vector2& topRight)
    {
        if (m_graphicsSceneUi)
        {
            QPainterPath path;
            path.addRect(QRectF{ QPointF {topLeft.GetX(), topLeft.GetY()}, QPointF { topRight.GetX(), topRight.GetY() } });
            m_graphicsSceneUi->setSelectionArea(path);
        }
    }

    void SceneComponent::SelectAll()
    {
        if (m_graphicsSceneUi)
        {
            QPainterPath path;
            path.addRect(m_graphicsSceneUi->sceneRect());
            m_graphicsSceneUi->setSelectionArea(path);
        }
    }

    void SceneComponent::SelectAllRelative(ConnectionType connectionDirection)
    {
        AZStd::vector<AZ::EntityId> seedNodes = GetSelectedNodes();

        AZStd::unordered_set<AZ::EntityId> selectableNodes;

        GraphUtils::FindConnectedNodes(seedNodes, selectableNodes, { connectionDirection });

        AssetEditorRequestBus::Event(GetEditorId(), &AssetEditorRequests::OnSelectionManipulationBegin);

        for (const AZ::EntityId& nodeId : selectableNodes)
        {
            SceneMemberUIRequestBus::Event(nodeId, &SceneMemberUIRequests::SetSelected, true);
        }

        AssetEditorRequestBus::Event(GetEditorId(), &AssetEditorRequests::OnSelectionManipulationEnd);
    }

    void SceneComponent::SelectConnectedNodes()
    {
        AZStd::vector<AZ::EntityId> seedNodes = GetSelectedNodes();

        AZStd::unordered_set<AZ::EntityId> selectableNodes;

        GraphUtils::FindConnectedNodes(seedNodes, selectableNodes, { ConnectionType::CT_Input, ConnectionType::CT_Output });

        AssetEditorRequestBus::Event(GetEditorId(), &AssetEditorRequests::OnSelectionManipulationBegin);

        for (const AZ::EntityId& nodeId : selectableNodes)
        {
            SceneMemberUIRequestBus::Event(nodeId, &SceneMemberUIRequests::SetSelected, true);
        }

        AssetEditorRequestBus::Event(GetEditorId(), &AssetEditorRequests::OnSelectionManipulationEnd);
    }

    bool SceneComponent::HasSelectedItems() const
    {
        return m_graphicsSceneUi ? !m_graphicsSceneUi->selectedItems().isEmpty() : false;
    }

    bool SceneComponent::HasMultipleSelection() const
    {
        return m_graphicsSceneUi ? m_graphicsSceneUi->selectedItems().count() > 1 : false;
    }

    bool SceneComponent::HasCopiableSelection() const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        bool hasCopiableSelection = false;
        if (m_graphicsSceneUi)
        {
            const QList<QGraphicsItem*> selected(m_graphicsSceneUi->selectedItems());

            for (QGraphicsItem* item : selected)
            {
                auto entry = m_itemLookup.find(item);
                if (entry != m_itemLookup.end())
                {
                    AZ::Entity* entity = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entry->second);

                    if (m_graphData.m_nodes.count(entity) > 0)
                    {
                        hasCopiableSelection = true;
                        break;
                    }
                    else if (m_graphData.m_bookmarkAnchors.count(entity) > 0)
                    {
                        hasCopiableSelection = true;
                        break;
                    }
                }
            }
        }

        return hasCopiableSelection;
    }

    bool SceneComponent::HasEntitiesAt(const AZ::Vector2& scenePoint) const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        bool retVal = false;

        if (m_graphicsSceneUi)
        {
            QList<QGraphicsItem*> itemsThere = m_graphicsSceneUi->items(QPointF(scenePoint.GetX(), scenePoint.GetY()));
            for (QGraphicsItem* item : itemsThere)
            {
                auto entry = m_itemLookup.find(item);
                if (entry != m_itemLookup.end() && entry->second != m_grid)
                {
                    retVal = true;
                    break;
                }
            }
        }

        return retVal;
    }

    AZStd::vector<AZ::EntityId> SceneComponent::GetSelectedItems() const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::vector<AZ::EntityId> result;
        if (m_graphicsSceneUi)
        {
            const QList<QGraphicsItem*> selected(m_graphicsSceneUi->selectedItems());
            result.reserve(selected.size());

            for (QGraphicsItem* item : selected)
            {
                auto entry = m_itemLookup.find(item);
                if (entry != m_itemLookup.end())
                {
                    result.push_back(entry->second);
                }
            }
        }
        return result;
    }

    QGraphicsScene* SceneComponent::AsQGraphicsScene()
    {
        return m_graphicsSceneUi.get();
    }

    void SceneComponent::CopySelection() const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::vector<AZ::EntityId> entities = GetSelectedItems();

        Copy(entities);
    }

    void SceneComponent::Copy(const AZStd::vector<AZ::EntityId>& selectedItems) const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnCopyBegin);

        GraphSerialization serializationTarget(m_copyMimeType);
        SerializeEntities({ selectedItems.begin(), selectedItems.end() }, serializationTarget);
        SerializeToClipboard(serializationTarget);

        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnCopyEnd);
    }

    void SceneComponent::CutSelection()
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::vector<AZ::EntityId> entities = GetSelectedItems();
        Cut(entities);
    }

    void SceneComponent::Cut(const AZStd::vector<AZ::EntityId>& selectedItems)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        Copy(selectedItems);

        AZStd::unordered_set<AZ::EntityId> deletedItems(selectedItems.begin(), selectedItems.end());
        Delete(deletedItems);
    }

    void SceneComponent::Paste()
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        QPointF pasteCenter = SignalGenericAddPositionUseBegin();
        PasteAt(pasteCenter);
        SignalGenericAddPositionUseEnd();
    }

    void SceneComponent::PasteAt(const QPointF& scenePos)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnPasteBegin);

        {
            QScopedValueRollback pastingRollback(m_isPasting, true);

            if (m_graphicsSceneUi)
            {
                m_graphicsSceneUi->blockSignals(true);
                m_graphicsSceneUi->clearSelection();
            }

            AZ::Vector2 pastePos{ static_cast<float>(scenePos.x()), static_cast<float>(scenePos.y()) };
            QClipboard* clipboard = QApplication::clipboard();

            // Trying to paste unknown data into our scene.
            if (!clipboard->mimeData()->hasFormat(m_copyMimeType.c_str()))
            {
                SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnUnknownPaste, scenePos);
                SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnPasteEnd);
                return;
            }

            QByteArray byteArray = clipboard->mimeData()->data(m_copyMimeType.c_str());
            GraphSerialization serializationSource(byteArray);
            DeserializeEntities(scenePos, serializationSource);
            
            if (m_graphicsSceneUi)
            {
                m_graphicsSceneUi->blockSignals(false);
            }
        }

        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnPasteEnd);

        OnSelectionChanged();

        ViewRequestBus::Event(m_viewId, &ViewRequests::RefreshView);
    }

    void SceneComponent::SerializeEntities(const AZStd::unordered_set<AZ::EntityId>& itemIds, GraphSerialization& serializationTarget) const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        GraphData& serializedEntities = serializationTarget.GetGraphData();

        GraphUtils::ParseMembersForSerialization(serializationTarget, itemIds);

        if (serializedEntities.m_nodes.empty()
            && serializedEntities.m_bookmarkAnchors.empty())
        {
            return;
        }

        // Calculate the position of selected items relative to the position from either the context menu mouse point or the scene center
        AZ::Vector2 aggregatePos = AZ::Vector2::CreateZero();        

        // Can't do this with the above listing. Because when nodes get serialized, they may add other nodes to the list.
        // So once we are fully added in, we can figure out our positions.
        for (AZ::Entity* entity : serializedEntities.m_nodes)
        {
            QGraphicsItem* graphicsItem = nullptr;
            SceneMemberUIRequestBus::EventResult(graphicsItem, entity->GetId(), &SceneMemberUIRequests::GetRootGraphicsItem);

            AZ::Vector2 itemPos = AZ::Vector2::CreateZero();

            if (graphicsItem)
            {
                QPointF scenePosition = graphicsItem->scenePos();
                itemPos.SetX(aznumeric_cast<float>(scenePosition.x()));
                itemPos.SetY(aznumeric_cast<float>(scenePosition.y()));
            }
            
            aggregatePos += itemPos;
        }

        for (AZ::Entity* entity : serializedEntities.m_bookmarkAnchors)
        {
            QGraphicsItem* graphicsItem = nullptr;
            SceneMemberUIRequestBus::EventResult(graphicsItem, entity->GetId(), &SceneMemberUIRequests::GetRootGraphicsItem);

            AZ::Vector2 itemPos = AZ::Vector2::CreateZero();

            if (graphicsItem)
            {
                QPointF scenePosition = graphicsItem->scenePos();
                itemPos.SetX(aznumeric_cast<float>(scenePosition.x()));
                itemPos.SetY(aznumeric_cast<float>(scenePosition.y()));
            }

            aggregatePos += itemPos;
        }

        AZ::Vector2 averagePos = aggregatePos / aznumeric_cast<float>(serializedEntities.m_nodes.size() + serializedEntities.m_bookmarkAnchors.size());
        serializationTarget.SetAveragePosition(averagePos);

        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnEntitiesSerialized, serializationTarget);
    }

    void SceneComponent::DeserializeEntities(const QPointF& scenePoint, const GraphSerialization& serializationSource)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZ::EntityId groupTarget = FindTopmostGroupAtPoint(scenePoint);

        AZ::Vector2 deserializePoint = AZ::Vector2(aznumeric_cast<float>(scenePoint.x()), aznumeric_cast<float>(scenePoint.y()));
        const AZ::Vector2& averagePos = serializationSource.GetAveragePosition();

        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnEntitiesDeserialized, serializationSource);

        const GraphData& pasteSceneData = serializationSource.GetGraphData();

        AZStd::unordered_map< PersistentGraphMemberId, PersistentGraphMemberId > persistentGraphMemberRemapping;

        AZStd::unordered_set<AZ::EntityId> groupableDeserializedEntities;

        for (auto& nodeRef : pasteSceneData.m_nodes)
        {
            AZStd::unique_ptr<AZ::Entity> entity(nodeRef);
            entity->Init();
            entity->Activate();

            AZ::Vector2 prevNodePos;
            GeometryRequestBus::EventResult(prevNodePos, entity->GetId(), &GeometryRequests::GetPosition);
            GeometryRequestBus::Event(entity->GetId(), &GeometryRequests::SetPosition, (prevNodePos - averagePos) + deserializePoint);

            SceneMemberNotificationBus::Event(entity->GetId(), &SceneMemberNotifications::OnSceneMemberDeserialized, GetEntityId(), serializationSource);

            SceneMemberUIRequestBus::Event(entity->GetId(), &SceneMemberUIRequests::SetSelected, true);

            if (Add(entity->GetId(), true))
            {
                entity.release();

                AZ::EntityId nodeId = nodeRef->GetId();

                PersistentGraphMemberId previousId;
                PersistentMemberRequestBus::EventResult(previousId, nodeId, &PersistentMemberRequests::GetPreviousGraphMemberId);

                PersistentGraphMemberId newId;
                PersistentMemberRequestBus::EventResult(newId, nodeId, &PersistentMemberRequests::GetPersistentGraphMemberId);

                persistentGraphMemberRemapping[previousId] = newId;

                if (GraphUtils::IsGroupableElement(nodeId))
                {
                    groupableDeserializedEntities.insert(nodeId);
                }
            }
        }

        for (auto& bookmarkRef : pasteSceneData.m_bookmarkAnchors)
        {
            AZStd::unique_ptr<AZ::Entity> entity(bookmarkRef);
            entity->Init();
            entity->Activate();

            AZ::Vector2 prevPos;
            GeometryRequestBus::EventResult(prevPos, entity->GetId(), &GeometryRequests::GetPosition);
            GeometryRequestBus::Event(entity->GetId(), &GeometryRequests::SetPosition, (prevPos - averagePos) + deserializePoint);
            SceneMemberNotificationBus::Event(entity->GetId(), &SceneMemberNotifications::OnSceneMemberDeserialized, GetEntityId(), serializationSource);

            SceneMemberUIRequestBus::Event(entity->GetId(), &SceneMemberUIRequests::SetSelected, true);

            if (Add(entity->GetId()))
            {
                entity.release();

                AZ::EntityId bookmarkId = bookmarkRef->GetId();

                PersistentGraphMemberId previousId;
                PersistentMemberRequestBus::EventResult(previousId, bookmarkId, &PersistentMemberRequests::GetPreviousGraphMemberId);

                PersistentGraphMemberId newId;
                PersistentMemberRequestBus::EventResult(newId, bookmarkId, &PersistentMemberRequests::GetPersistentGraphMemberId);

                persistentGraphMemberRemapping[previousId] = newId;

                if (GraphUtils::IsGroupableElement(bookmarkId))
                {
                    groupableDeserializedEntities.insert(bookmarkId);
                }
            }
        }

        // Now go through and recreate all of the connections.
        const AZStd::unordered_multimap<Endpoint, Endpoint>& connectedEndpoints = serializationSource.GetConnectedEndpoints();

        for (const auto& mapPair : connectedEndpoints)
        {
            SlotRequestBus::Event(mapPair.first.GetSlotId(), &SlotRequests::CreateConnectionWithEndpoint, mapPair.second);
        }

        PersistentIdNotificationBus::Event(GetEditorId(), &PersistentIdNotifications::OnPersistentIdsRemapped, persistentGraphMemberRemapping);        

        if (groupTarget.IsValid())
        {
            auto groupableIter = groupableDeserializedEntities.begin();

            while (groupableIter != groupableDeserializedEntities.end())
            {
                // Remove any groupable elements that are apart of another group.
                // And just assign everything that is a 'root' element to our new area.
                AZ::EntityId groupId;
                GroupableSceneMemberRequestBus::EventResult(groupId, (*groupableIter), &GroupableSceneMemberRequests::GetGroupId);

                if (groupId.IsValid())
                {
                    groupableIter = groupableDeserializedEntities.erase(groupableIter);
                }
                else
                {
                    ++groupableIter;
                }
            }

            NodeGroupRequestBus::Event(groupTarget, &NodeGroupRequests::AddElementsToGroup, groupableDeserializedEntities);
        }

        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::PostCreationEvent);
        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnEntitiesDeserializationComplete, serializationSource);
    }

    void SceneComponent::DuplicateSelection()
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::vector<AZ::EntityId> entities = GetSelectedItems();
        Duplicate(entities);
    }

    void SceneComponent::DuplicateSelectionAt(const QPointF& scenePos)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::vector<AZ::EntityId> entities = GetSelectedItems();
        DuplicateAt(entities, scenePos);
    }

    void SceneComponent::Duplicate(const AZStd::vector<AZ::EntityId>& itemIds)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        QPointF duplicateCenter = SignalGenericAddPositionUseBegin();
        DuplicateAt(itemIds, duplicateCenter);
        SignalGenericAddPositionUseEnd();
    }

    void SceneComponent::DuplicateAt(const AZStd::vector<AZ::EntityId>& itemIds, const QPointF& scenePos)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnDuplicateBegin);

        {
            QScopedValueRollback isPastingRollback(m_isPasting, true);

            if (m_graphicsSceneUi)
            {
                m_graphicsSceneUi->blockSignals(true);
                m_graphicsSceneUi->clearSelection();
            }

            GraphSerialization serializationTarget;
            SerializeEntities({ itemIds.begin(), itemIds.end() }, serializationTarget);

            AZStd::vector<char> buffer;
            SerializeToBuffer(serializationTarget, buffer);
            GraphSerialization deserializationTarget(QByteArray(buffer.cbegin(), static_cast<int>(buffer.size())));

            DeserializeEntities(scenePos, deserializationTarget);            

            if (m_graphicsSceneUi)
            {
                m_graphicsSceneUi->blockSignals(false);
            }
        }

        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnDuplicateEnd);

        OnSelectionChanged();
        ViewRequestBus::Event(m_viewId, &ViewRequests::RefreshView);
    }

    void SceneComponent::DeleteSelection()
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        if (m_graphicsSceneUi)
        {
            const QList<QGraphicsItem*> selected(m_graphicsSceneUi->selectedItems());

            AZStd::unordered_set<AZ::EntityId> toDelete;

            for (QGraphicsItem* item : selected)
            {
                auto entry = m_itemLookup.find(item);
                if (entry != m_itemLookup.end())
                {
                    toDelete.insert(entry->second);
                }
            }

            Delete(toDelete);
        }
    }

    void SceneComponent::Delete(const AZStd::unordered_set<AZ::EntityId>& itemIds)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        if (itemIds.empty())
        {
            return;
        }

        // Block the graphics scene from sending selection update events as we remove items.
        if (m_graphicsSceneUi)
        {
            if (m_deleteCount == 0)
            {
                m_graphicsSceneUi->blockSignals(true);
            }
        }

        // Need to deal with recursive deleting because of Wrapper Nodes
        ++m_deleteCount;

        SceneMemberBuckets sceneMembers;

        SieveSceneMembers(itemIds, sceneMembers);

        const bool internalConnectionsOnly = false;
        auto nodeConnections = GraphUtils::FindConnectionsForNodes(sceneMembers.m_nodes, internalConnectionsOnly);
        sceneMembers.m_connections.insert(nodeConnections.begin(), nodeConnections.end());

        for (const auto& connection : sceneMembers.m_connections)
        {
            if (Remove(connection))
            {
                SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnPreConnectionDeleted, connection);
                AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::DeleteEntity, connection);
            }
        }

        for (const auto& node : sceneMembers.m_nodes)
        {
            NodeRequestBus::Event(node, &NodeRequests::SignalNodeAboutToBeDeleted);

            if (Remove(node))
            {
                SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnPreNodeDeleted, node);
                AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::DeleteEntity, node);
            }
        }

        for (const auto& bookmarkAnchor : sceneMembers.m_bookmarkAnchors)
        {
            if (Remove(bookmarkAnchor))
            {
                AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::DeleteEntity, bookmarkAnchor);
            }
        }

        --m_deleteCount;

        if (m_deleteCount == 0)
        {
            if (m_graphicsSceneUi)
            {
                m_graphicsSceneUi->blockSignals(false);
                // Once complete, signal selection is changed
                emit m_graphicsSceneUi->selectionChanged();
            }

            SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::PostDeletionEvent);
        }
    }

    void SceneComponent::DeleteGraphData(const GraphData& graphData)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::unordered_set<AZ::EntityId> itemIds;
        graphData.CollectItemIds(itemIds);

        Delete(itemIds);
    }

    void SceneComponent::ClearScene()
    {
        DeleteGraphData(m_graphData);

        AZStd::unordered_map<GraphicsEffectId, QGraphicsItem*> removalPair;

        auto enumerationCallback = [&removalPair](GraphicsEffectRequests* graphicsInterface) -> bool
        {            
            QGraphicsItem* graphicsItem = graphicsInterface->AsQGraphicsItem();

            if (graphicsItem)
            {
                removalPair[graphicsInterface->GetEffectId()] = graphicsItem;
            }

            // Enumerate over all handlers
            return true;
        };

        GraphicsEffectRequestBus::EnumerateHandlers(enumerationCallback);

        for (auto effectPair : removalPair)
        {
            DestroyGraphicsItem(effectPair.first, effectPair.second);
        }
    }

    void SceneComponent::SuppressNextContextMenu()
    {
        if (m_graphicsSceneUi)
        {
            m_graphicsSceneUi->SuppressNextContextMenu();
        }
    }

    const AZStd::string& SceneComponent::GetCopyMimeType() const
    {
        return m_copyMimeType;
    }

    void SceneComponent::SetMimeType(const char* mimeType)
    {
        m_mimeDelegateSceneHelper.SetMimeType(mimeType);

        m_copyMimeType = AZStd::string::format("%s::copy", mimeType);
    }

    AZStd::vector<AZ::EntityId> SceneComponent::GetEntitiesAt(const AZ::Vector2& position) const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::vector<AZ::EntityId> result;

        if (m_graphicsSceneUi)
        {
            QList<QGraphicsItem*> itemsThere = m_graphicsSceneUi->items(QPointF(position.GetX(), position.GetY()));
            for (QGraphicsItem* item : itemsThere)
            {
                auto entry = m_itemLookup.find(item);
                if (entry != m_itemLookup.end() && entry->second != m_grid)
                {
                    result.emplace_back(AZStd::move(entry->second));
                }
            }
        }

        return result;
    }

    AZStd::vector<AZ::EntityId> SceneComponent::GetEntitiesInRect(const QRectF& rect, Qt::ItemSelectionMode mode) const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::vector<AZ::EntityId> result;

        if (m_graphicsSceneUi)
        {
            QList<QGraphicsItem*> itemsThere = m_graphicsSceneUi->items(rect, mode);
            for (QGraphicsItem* item : itemsThere)
            {
                auto entry = m_itemLookup.find(item);
                if (entry != m_itemLookup.end() && entry->second != m_grid)
                {
                    result.emplace_back(AZStd::move(entry->second));
                }
            }
        }

        return result;
    }

    AZStd::vector<Endpoint> SceneComponent::GetEndpointsInRect(const QRectF& rect) const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::vector<Endpoint> result;

        AZStd::vector<AZ::EntityId> entitiesThere = GetEntitiesInRect(rect, Qt::ItemSelectionMode::IntersectsItemShape);
        for (AZ::EntityId nodeId : entitiesThere)
        {
            if (NodeRequestBus::FindFirstHandler(nodeId) != nullptr)
            {
                AZStd::vector<AZ::EntityId> slotIds;
                NodeRequestBus::EventResult(slotIds, nodeId, &NodeRequestBus::Events::GetSlotIds);
                for (AZ::EntityId slotId : slotIds)
                {
                    QPointF point;
                    SlotUIRequestBus::EventResult(point, slotId, &SlotUIRequestBus::Events::GetConnectionPoint);
                    if (rect.contains(point))
                    {
                        result.emplace_back(Endpoint(nodeId, slotId));
                    }
                }
            }
        }

        AZStd::sort(result.begin(), result.end(), [&rect](Endpoint a, Endpoint b) {
            QPointF pointA;
            SlotUIRequestBus::EventResult(pointA, a.GetSlotId(), &SlotUIRequestBus::Events::GetConnectionPoint);
            QPointF pointB;
            SlotUIRequestBus::EventResult(pointB, b.GetSlotId(), &SlotUIRequestBus::Events::GetConnectionPoint);

            return (rect.center() - pointA).manhattanLength() < (rect.center() - pointB).manhattanLength();
        });

        return result;
    }

    void SceneComponent::RegisterView(const AZ::EntityId& viewId)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        if (m_activateScene)
        {
            m_activateScene = false;

            GraphModelRequestBus::Event(GetEntityId(), &GraphModelRequests::RequestPushPreventUndoStateUpdate);
            
            ActivateItems(m_graphData.m_nodes);
            ActivateItems(m_graphData.m_connections);
            ActivateItems(m_graphData.m_bookmarkAnchors);
            NotifyConnectedSlots();

            SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnStylesChanged); // Forces activated elements to refresh their visual elements.

            GraphModelRequestBus::Event(GetEntityId(), &GraphModelRequests::RequestPopPreventUndoStateUpdate);
        }

        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnViewRegistered);

        if (!m_viewId.IsValid() || m_viewId == viewId)
        {
            m_viewId = viewId;

            EditorId editorId;
            ViewRequestBus::EventResult(editorId, viewId, &ViewRequests::GetEditorId);

            SetEditorId(editorId);
            
            ViewNotificationBus::Handler::BusConnect(m_viewId);
            ViewRequestBus::Event(m_viewId, &ViewRequests::SetViewParams, m_viewParams);
        }
        else
        {
            AZ_Error("Scene", false, "Trying to register scene to two different views.");
        }
    }

    void SceneComponent::RemoveView(const AZ::EntityId& viewId)
    {
        if (m_viewId == viewId)
        {
            m_editorId = EditorId();
            m_viewId.SetInvalid();
            ViewNotificationBus::Handler::BusDisconnect();
        }
        else
        {
            AZ_Error("Scene", !m_viewId.IsValid(), "Trying to unregister scene from the wrong view.");
        }
    }

    ViewId SceneComponent::GetViewId() const
    {
        return m_viewId;
    }

    void SceneComponent::DispatchSceneDropEvent(const AZ::Vector2& scenePos, const QMimeData* mimeData)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        QPointF scenePoint(scenePos.GetX(), scenePos.GetY());

        for (const AZ::EntityId& delegateId : m_delegates)
        {
            bool isInterested;
            SceneMimeDelegateHandlerRequestBus::EventResult(isInterested, delegateId, &SceneMimeDelegateHandlerRequests::IsInterestedInMimeData, GetEntityId(), mimeData);

            if (isInterested)
            {
                SceneMimeDelegateHandlerRequestBus::Event(delegateId, &SceneMimeDelegateHandlerRequests::HandleDrop, GetEntityId(), scenePoint, mimeData);
            }
        }

        // Force the focus onto the GraphicsScene after a drop.
        AZ::EntityId viewId = GetViewId();

        QTimer::singleShot(0, [viewId]()
        {
            GraphCanvasGraphicsView* graphicsView = nullptr;
            ViewRequestBus::EventResult(graphicsView, viewId, &ViewRequests::AsGraphicsView);

            if (graphicsView)
            {
                graphicsView->setFocus(Qt::FocusReason::MouseFocusReason);
            }
        });
    }

    bool SceneComponent::AddGraphData(const GraphData& graphData)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        bool success = true;

        for (const AZStd::unordered_set<AZ::Entity*>& entitySet : { graphData.m_nodes, graphData.m_bookmarkAnchors, graphData.m_connections })
        {
            for (AZ::Entity* itemRef : entitySet)
            {
                if (itemRef->GetState() == AZ::Entity::State::Init)
                {
                    itemRef->Activate();
                }

                success = Add(itemRef->GetId()) && success;
            }
        }

        return success;
    }

    void SceneComponent::RemoveGraphData(const GraphData& graphData)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::unordered_set<AZ::EntityId> itemIds;
        graphData.CollectItemIds(itemIds);

        for (const AZ::EntityId& itemId : itemIds)
        {
            Remove(itemId);
        }
    }

    void SceneComponent::SetDragSelectionType(DragSelectionType selectionType)
    {
        m_dragSelectionType = selectionType;
    }

    void SceneComponent::SignalDragSelectStart()
    {
        m_isDragSelecting = true;
        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnDragSelectStart);
    }

    void SceneComponent::SignalDragSelectEnd()
    {
        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnDragSelectEnd);
        m_isDragSelecting = false;
    }

    bool SceneComponent::IsDragSelecting() const
    {
        return m_isDragSelecting;
    }

    void SceneComponent::SignalConnectionDragBegin()
    {
        // Bit of a hack to get the connections to play nicely with some signalling.
        if (HasSelectedItems())
        {
            ClearSelection();
        }
        else
        {
            OnSelectionChanged();
        }

        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnConnectionDragBegin);
        m_isDraggingConnection = true;
    }

    void SceneComponent::SignalConnectionDragEnd()
    {
        m_isDraggingConnection = false;
        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnConnectionDragEnd);
    }

    bool SceneComponent::IsDraggingConnection() const
    {
        return m_isDraggingConnection;
    }

    void SceneComponent::SignalDesplice()
    {
        CancelNudging();
    }

    QRectF SceneComponent::GetSelectedSceneBoundingArea() const
    {
        QRectF boundingRect;

        for (const auto& sceneMemberList : { m_graphData.m_nodes, m_graphData.m_bookmarkAnchors })
        {
            for (const auto& sceneMember : sceneMemberList)
            {
                QGraphicsItem* sceneItem = nullptr;
                VisualRequestBus::EventResult(sceneItem, sceneMember->GetId(), &VisualRequests::AsGraphicsItem);

                if (sceneItem && sceneItem->isSelected())
                {
                    if (boundingRect.isEmpty())
                    {
                        boundingRect = sceneItem->sceneBoundingRect();
                    }
                    else
                    {
                        boundingRect |= sceneItem->sceneBoundingRect();
                    }
                }
            }
        }

        return boundingRect;
    }

    QRectF SceneComponent::GetSceneBoundingArea() const
    {
        QRectF boundingRect;

        for (const auto& sceneMemberList : { m_graphData.m_nodes, m_graphData.m_bookmarkAnchors })
        {
            for (const auto& sceneMember : sceneMemberList)
            {
                QGraphicsItem* sceneItem = nullptr;
                VisualRequestBus::EventResult(sceneItem, sceneMember->GetId(), &VisualRequests::AsGraphicsItem);

                if (sceneItem)
                {
                    if (boundingRect.isEmpty())
                    {
                        boundingRect = sceneItem->sceneBoundingRect();
                    }
                    else
                    {
                        boundingRect |= sceneItem->sceneBoundingRect();
                    }
                }
            }
        }

        return boundingRect;
    }

    void SceneComponent::SignalLoadStart()
    {
        m_isLoading = true;
        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnGraphLoadBegin);
    }

    void SceneComponent::SignalLoadEnd()
    {
        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnGraphLoadComplete);
        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::PostOnGraphLoadComplete);
        m_isLoading = false;
    }

    bool SceneComponent::IsLoading() const
    {
        return m_isLoading;
    }

    bool SceneComponent::IsPasting() const
    {
        return m_isPasting;
    }

    void SceneComponent::RemoveUnusedNodes()
    {
        AZStd::vector< NodeId > nodeIds = GetNodes();

        AZStd::unordered_set< AZ::EntityId > unusedIds;

        AZStd::unordered_set< NodeId > wrapperNodes;

        for (const NodeId& nodeId : nodeIds)
        {
            // Going to skip node groups for now.
            if (GraphUtils::IsCollapsedNodeGroup(nodeId)
                || GraphUtils::IsNodeGroup(nodeId)
                || GraphUtils::IsComment(nodeId))
            {
                continue;
            }

            bool hasConnections = false;
            NodeRequestBus::EventResult(hasConnections, nodeId, &NodeRequests::HasConnections);

            if (!hasConnections)
            {
                if (GraphUtils::IsWrapperNode(nodeId))
                {
                    wrapperNodes.insert(nodeId);
                }
                else
                {
                    unusedIds.insert(nodeId);
                }
            }
        }

        for (const NodeId& wrapperNodeId : wrapperNodes)
        {
            AZStd::vector< NodeId > wrappedNodes;
            WrapperNodeRequestBus::EventResult(wrappedNodes, wrapperNodeId, &WrapperNodeRequests::GetWrappedNodeIds);

            bool canDelete = true;

            for (const NodeId& wrappedNodeId : wrappedNodes)
            {
                if (unusedIds.count(wrappedNodeId) == 0)
                {
                    canDelete = false;
                    break;
                }
            }

            if (canDelete)
            {
                unusedIds.insert(wrapperNodeId);
            }
        }

        {
            ScopedGraphUndoBlocker undoBlocker(GetEntityId());
            Delete(unusedIds);
            GraphModelRequestBus::Event(GetEntityId(), &GraphModelRequests::OnRemoveUnusedNodes);
        }

        GraphModelRequestBus::Event(GetEntityId(), &GraphModelRequests::RequestUndoPoint);
    }

    void SceneComponent::HandleProposalDaisyChainWithGroup(const NodeId& startNode, SlotType slotType, ConnectionType connectionType, const QPoint& screenPoint, const QPointF& focusPoint, AZ::EntityId groupTarget)
    {
        AZ::Vector2 stepAmount = GraphUtils::FindMinorStep(GetEntityId());

        Endpoint newEndpoint;

        AZ::EntityId connectionId;

        AZStd::vector< SlotId > slotIds;
        NodeRequestBus::EventResult(slotIds, startNode, &NodeRequests::GetVisibleSlotIds);

        for (const SlotId& slotId : slotIds)
        {
            if (!GraphUtils::IsSlot(slotId, slotType, connectionType))
            {
                continue;
            }

            newEndpoint = Endpoint(startNode, slotId);
            break;
        }

        if (newEndpoint.IsValid())
        {
            AZ::EntityId newConnectionId;
            SlotRequestBus::EventResult(newConnectionId, newEndpoint.GetSlotId(), &SlotRequests::DisplayConnection);

            if (newConnectionId.IsValid())
            {
                QPointF connectionPoint;
                SlotUIRequestBus::EventResult(connectionPoint, newEndpoint.GetSlotId(), &SlotUIRequests::GetConnectionPoint);

                QPointF jut;
                SlotUIRequestBus::EventResult(jut, newEndpoint.GetSlotId(), &SlotUIRequests::GetJutDirection);

                connectionPoint.setX(connectionPoint.x() + 2.0f * stepAmount.GetX() * jut.x());

                // Because the size of the nodes are clamped to a size, they don't get resized until they are rendered.
                // This makes doing this sort of fine tuned positioning weird. Since it does it based on the wrong size, then it resizes
                // and ruins everything. Going to just hack this for nowt o give it an extra half step if it's going backwards(which is where this case matters)
                if (jut.x() < 0)
                {
                    connectionPoint.setX(connectionPoint.x() - stepAmount.GetX() * 0.5f);
                }

                connectionPoint.setY(connectionPoint.y() + 2.0f * stepAmount.GetY() * jut.y());

                // Delta vector we need to move the scene by
                QPointF repositioning = connectionPoint - focusPoint;

                ViewRequestBus::Event(GetViewId(), &ViewRequests::PanSceneBy, repositioning, AZStd::chrono::milliseconds(250));

                ConnectionRequestBus::Event(newConnectionId, &ConnectionRequests::ChainProposalCreation, connectionPoint, screenPoint, groupTarget);
            }
        }
    }

    void SceneComponent::StartNudging(const AZStd::unordered_set<AZ::EntityId>& fixedNodes)
    {
        if (m_enableNudging)
        {
            m_nudgingController.StartNudging(fixedNodes);
        }
    }

    void SceneComponent::FinalizeNudging()
    {
        if (m_enableNudging)
        {
            m_nudgingController.FinalizeNudging();
        }
    }

    void SceneComponent::CancelNudging()
    {
        if (m_enableNudging)
        {
            m_nudgingController.CancelNudging();
        }
    }

    AZ::EntityId SceneComponent::FindTopmostGroupAtPoint(QPointF scenePoint)
    {
        return FindGroupTarget(scenePoint);
    }

    void SceneComponent::RemoveUnusedElements()
    {
        {
            ScopedGraphUndoBlocker undoBlocker(GetEntityId());
            RemoveUnusedNodes();
            GraphModelRequestBus::Event(GetEntityId(), &GraphModelRequests::OnRemoveUnusedElements);
        }

        GraphModelRequestBus::Event(GetEntityId(), &GraphModelRequests::RequestUndoPoint);
    }

    QPointF SceneComponent::SignalGenericAddPositionUseBegin()
    {
        m_allowReset = false;
        return GetViewCenterScenePoint() + m_genericAddOffset;
    }

    void SceneComponent::SignalGenericAddPositionUseEnd()
    {
        AZ::Vector2 minorPitch;
        GridRequestBus::EventResult(minorPitch, m_grid, &GridRequests::GetMinorPitch);

        // Don't want to shift it diagonally, because we also shift things diagonally when we drag/drop in stuff
        // So we'll just move it straight down.
        m_genericAddOffset += QPointF(0, minorPitch.GetY() * 2);
        m_allowReset = true;
    }

    bool SceneComponent::AllowContextMenu() const
    {
        return !IsDragSelecting() && !IsDraggingConnection();
    }

    bool SceneComponent::OnMousePress(const AZ::EntityId& sourceId, const QGraphicsSceneMouseEvent* event)
    {
        if (event->button() == Qt::LeftButton && sourceId != m_grid)
        {
            m_enableSpliceTracking = false;
            m_enableNodeDragConnectionSpliceTracking = false;
            m_enableNodeDragCouplingTracking = false;
            m_enableNodeChainDragConnectionSpliceTracking = false;
            m_spliceTarget.SetInvalid();

            m_pressedEntity = sourceId;
            m_gestureSceneHelper.TrackElement(m_pressedEntity);

            GeometryRequestBus::EventResult(m_originalPosition, m_pressedEntity, &GeometryRequests::GetPosition);
        }

        return false;
    }

    bool SceneComponent::OnMouseRelease(const AZ::EntityId& /*sourceId*/, const QGraphicsSceneMouseEvent* /*event*/)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        if (m_isDraggingEntity)
        {
            {
                ScopedGraphUndoBlocker undoBlocker(GetEntityId());

                for (AZ::EntityId groupableElement : m_draggedGroupableElements)
                {
                    GroupableSceneMemberRequestBus::Event(groupableElement, &GroupableSceneMemberRequests::RemoveFromGroup);
                }

                if (m_dragTargetGroup.IsValid())
                {
                    NodeGroupRequestBus::Event(m_dragTargetGroup, &NodeGroupRequests::AddElementsToGroup, m_draggedGroupableElements);

                    const bool growGroupOnly = true;
                    NodeGroupRequestBus::Event(m_dragTargetGroup, &NodeGroupRequests::ResizeGroupToElements, growGroupOnly);
                }

                m_dragTargetGroup.SetInvalid();

                m_forcedLayerStateSetter.ResetStateSetter();
                m_forcedGroupDisplayStateStateSetter.ResetStateSetter();
                m_draggedGroupableElements.clear();
                m_ignoredDragTargets.clear();
            }

            // Set the dragging element after the group resize. Otherwise the group will send out a position change, and remove the thing
            // it just attempted to position.
            m_isDraggingEntity = false;

            AZ::Vector2 finalPosition;
            GeometryRequestBus::EventResult(finalPosition, m_pressedEntity, &GeometryRequests::GetPosition);
        
            SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnSceneMemberDragComplete);

            if (m_forceDragReleaseUndo || !finalPosition.IsClose(m_originalPosition))
            {
                m_forceDragReleaseUndo = false;
                GraphModelRequestBus::Event(GetEntityId(), &GraphModelRequests::RequestUndoPoint);
            }
        }

        m_isDraggingEntity = false;
        m_dragTargetGroup.SetInvalid();

        m_enableSpliceTracking = false;
        m_spliceTimer.stop();
        m_spliceTarget.SetInvalid();
        m_spliceTargetDisplayStateStateSetter.ResetStateSetter();
        m_pressedEntityDisplayStateStateSetter.ResetStateSetter();
        m_couplingEntityDisplayStateStateSetter.ResetStateSetter();

        m_gestureSceneHelper.StopTrack();

        m_pressedEntity.SetInvalid();
        
        return false;
    }

    void SceneComponent::OnPositionChanged(const AZ::EntityId& itemId, const AZ::Vector2& position)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        if (m_pressedEntity.IsValid()
            && itemId == m_pressedEntity)
        {
            if (!m_isDraggingEntity)
            {
                m_isDraggingEntity = true;
                SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnSceneMemberDragBegin);

                AssetEditorSettingsRequestBus::EventResult(m_enableNodeDragConnectionSpliceTracking, GetEditorId(), &AssetEditorSettingsRequests::IsDragConnectionSpliceEnabled);
                AssetEditorSettingsRequestBus::EventResult(m_enableNodeDragCouplingTracking, GetEditorId(), &AssetEditorSettingsRequests::IsDragNodeCouplingEnabled);

                AZStd::vector< AZ::EntityId > selectedEntities = GetSelectedNodes();

                // Precache all of the groups so we can filter out elements that belong to them in our final set.
                AZStd::unordered_set< AZ::EntityId > selectedGroups;

                for (AZ::EntityId selectedId : selectedEntities)
                {
                    if (GraphUtils::IsNodeGroup(selectedId))
                    {
                        m_ignoredDragTargets.insert(selectedId);
                        selectedGroups.insert(selectedId);
                    }
                }

                for (AZ::EntityId selectedId : selectedEntities)
                {
                    if (GraphUtils::IsGroupableElement(selectedId) && !GraphUtils::IsNodeWrapped(selectedId))
                    {
                        // If you are already grouped. Sanity check if your parent group is being moved as well. If it is, don't do anything.
                        // Otherwise, remove yourself from that group and insert yourself into the overall list.
                        AZ::EntityId owningGroup;
                        GroupableSceneMemberRequestBus::EventResult(owningGroup, selectedId, &GroupableSceneMemberRequests::GetGroupId);

                        if (owningGroup.IsValid())
                        {
                            AZ::EntityId previousGroup;
                            while (owningGroup.IsValid())
                            {
                                if (selectedGroups.find(owningGroup) != selectedGroups.end())
                                {
                                    break;
                                }

                                GroupableSceneMemberRequestBus::EventResult(owningGroup, owningGroup, &GroupableSceneMemberRequests::GetGroupId);
                            }

                            if (!owningGroup.IsValid())
                            {
                                m_draggedGroupableElements.insert(selectedId);
                            }
                        }
                        else
                        {
                            m_draggedGroupableElements.insert(selectedId);
                        }
                    }
                }

                if (GraphUtils::IsConnectableNode(m_pressedEntity) && (m_enableNodeDragConnectionSpliceTracking || m_enableNodeDragCouplingTracking)
                    || (GraphUtils::IsNodeGroup(m_pressedEntity) && m_enableNodeDragConnectionSpliceTracking))
                {
                    if (GraphUtils::IsNodeGroup(m_pressedEntity))
                    {
                        NodeGroupRequestBus::Event(m_pressedEntity, &NodeGroupRequests::FindGroupedElements, selectedEntities);
                    }

                    m_inputCouplingTarget.SetInvalid();
                    m_outputCouplingTarget.SetInvalid();
                    m_couplingTarget.SetInvalid();
                    
                    m_selectedSubGraph.Clear();

                    SubGraphParsingConfig config;

                    SubGraphParsingResult subGraphResult = GraphUtils::ParseSceneMembersIntoSubGraphs(selectedEntities, config);

                    if (subGraphResult.m_subGraphs.size() == 1)
                    {
                        m_enableNodeChainDragConnectionSpliceTracking = true;
                        m_selectedSubGraph = subGraphResult.m_subGraphs.front();
                    }
                    else
                    {
                        m_enableNodeChainDragConnectionSpliceTracking = false;
                    }
                    
                    if (m_enableNodeDragCouplingTracking)
                    {
                        if (GraphUtils::IsNodeGroup(m_pressedEntity))
                        {
                            m_enableNodeDragCouplingTracking = false;
                        }
                        else if (selectedEntities.size() > 1)
                        {
                            m_selectedSubGraph.Clear();

                            SubGraphParsingConfig config2;
                            config2.m_createNonConnectableSubGraph = true;

                            SubGraphParsingResult subGraphResult2 = GraphUtils::ParseSceneMembersIntoSubGraphs(selectedEntities, config2);

                            if (subGraphResult2.m_subGraphs.size() == 1)
                            {
                                m_selectedSubGraph = subGraphResult2.m_subGraphs.front();

                                if (m_selectedSubGraph.m_entryNodes.size() == 1
                                    && m_selectedSubGraph.m_exitNodes.size() == 1)
                                {
                                    m_enableNodeDragCouplingTracking = true;
                                    m_inputCouplingTarget = (*m_selectedSubGraph.m_entryNodes.begin());
                                    m_outputCouplingTarget = (*m_selectedSubGraph.m_exitNodes.begin());
                                }
                                else
                                {
                                    for (auto entryNode : m_selectedSubGraph.m_entryNodes)
                                    {
                                        if (entryNode == m_pressedEntity)
                                        {
                                            m_enableNodeDragCouplingTracking = true;
                                            m_inputCouplingTarget = entryNode;
                                        }
                                    }

                                    for (auto entryNode : m_selectedSubGraph.m_exitNodes)
                                    {
                                        if (entryNode == m_pressedEntity)
                                        {
                                            m_enableNodeDragCouplingTracking = true;
                                            m_outputCouplingTarget = entryNode;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                m_enableNodeDragCouplingTracking = false;
                            }
                        }
                        else if (selectedEntities.size() == 1)
                        {
                            m_enableSpliceTracking = true;
                            m_inputCouplingTarget = m_pressedEntity;
                            m_outputCouplingTarget = m_pressedEntity;
                        }
                        else
                        {
                            m_enableSpliceTracking = false;
                            m_inputCouplingTarget.SetInvalid();
                            m_outputCouplingTarget.SetInvalid();
                        }
                    }

                    m_enableSpliceTracking = m_enableNodeChainDragConnectionSpliceTracking || m_enableNodeDragCouplingTracking;

                    if (m_enableSpliceTracking)
                    {
                        m_pressedEntityDisplayStateStateSetter.ResetStateSetter();

                        StateController<RootGraphicsItemDisplayState>* stateController;
                        RootGraphicsItemRequestBus::EventResult(stateController, m_pressedEntity, &RootGraphicsItemRequests::GetDisplayStateStateController);

                        m_pressedEntityDisplayStateStateSetter.AddStateController(stateController);
                    }
                }

                GraphCanvasGraphicsView* graphicsView = nullptr;
                ViewRequestBus::EventResult(graphicsView, m_viewId, &ViewRequests::AsGraphicsView);

                QPointF scenePoint;

                if (graphicsView)
                {
                    QPointF cursorPoint = QCursor::pos();
                    QPointF viewPoint = graphicsView->mapFromGlobal(cursorPoint.toPoint());
                    scenePoint = graphicsView->mapToScene(viewPoint.toPoint());

                    DetermineDragGroupTarget(scenePoint);
                }
            }
        }

        if (!GraphUtils::IsConnection(itemId))
        {
            SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnNodePositionChanged, itemId, position);

            if (m_allowReset)
            {
                m_genericAddOffset.setX(0);
                m_genericAddOffset.setY(0);
            }
        }

        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnSceneMemberPositionChanged, itemId, position);
        if (m_graphicsSceneUi)
        {
            m_graphicsSceneUi->update();
        }
    }

    void SceneComponent::OnEscape()
    {
        ClearSelection();
    }

    void SceneComponent::OnViewParamsChanged(const ViewParams& viewParams)
    {
        m_genericAddOffset.setX(0);
        m_genericAddOffset.setY(0);
        
        m_viewParams = viewParams;
    }

    void SceneComponent::AddDelegate(const AZ::EntityId& delegateId)
    {
        m_delegates.insert(delegateId);
    }

    void SceneComponent::RemoveDelegate(const AZ::EntityId& delegateId)
    {
        m_delegates.erase(delegateId);
    }

    AZ::u32 SceneComponent::GetNewBookmarkCounter()
    {
        return ++m_bookmarkCounter;
    }

    void SceneComponent::OnStylesLoaded()
    {
        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnStylesChanged);
    }

    void SceneComponent::OnSettingsChanged()
    {
        m_gestureSceneHelper.OnSettingsChanged();

        AssetEditorSettingsRequestBus::EventResult(m_enableNudging, GetEditorId(), &AssetEditorSettingsRequests::IsNodeNudgingEnabled);

        if (!m_enableNudging)
        {
            m_nudgingController.CancelNudging();
        }
    }

    void SceneComponent::ConfigureAndAddGraphicsEffect(GraphicsEffectInterface* graphicsEffect)
    {
        graphicsEffect->SetGraphId(GetEntityId());
        graphicsEffect->SetEditorId(GetEditorId());

        QGraphicsItem* graphicsItem = graphicsEffect->AsQGraphicsItem();
        m_graphicsSceneUi->addItem(graphicsItem);
    }

    void SceneComponent::OnSceneDragEnter(const QMimeData* mimeData)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        m_activeDelegates.clear();

        for (const AZ::EntityId& delegateId : m_delegates)
        {
            bool isInterested = false;
            SceneMimeDelegateHandlerRequestBus::EventResult(isInterested, delegateId, &SceneMimeDelegateHandlerRequests::IsInterestedInMimeData, GetEntityId(), mimeData);

            if (isInterested)
            {
                m_activeDelegates.insert(delegateId);
            }
        }
    }

    void SceneComponent::OnSceneDragMoveEvent(const QPointF& scenePoint, const QMimeData* mimeData)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        for (const AZ::EntityId& delegateId : m_activeDelegates)
        {
            SceneMimeDelegateHandlerRequestBus::Event(delegateId, &SceneMimeDelegateHandlerRequests::HandleMove, GetEntityId(), scenePoint, mimeData);
        }
    }

    void SceneComponent::OnSceneDropEvent(const QPointF& scenePoint, const QMimeData* mimeData)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        for (const AZ::EntityId& dropHandler : m_activeDelegates)
        {
            SceneMimeDelegateHandlerRequestBus::Event(dropHandler, &SceneMimeDelegateHandlerRequests::HandleDrop, GetEntityId(), scenePoint, mimeData);
        }

        AZ::EntityId viewId = GetViewId();

        // Force the focus onto the GraphicsView after a drop.
        QTimer::singleShot(0, [viewId]()
        {
            GraphCanvasGraphicsView* graphicsView = nullptr;
            ViewRequestBus::EventResult(graphicsView, viewId, &ViewRequests::AsGraphicsView);

            if (graphicsView)
            {
                graphicsView->setFocus(Qt::FocusReason::MouseFocusReason);
            }
        });
    }

    void SceneComponent::OnSceneDragExit(const QMimeData* mimeData)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        for (const AZ::EntityId& dropHandler : m_activeDelegates)
        {
            SceneMimeDelegateHandlerRequestBus::Event(dropHandler, &SceneMimeDelegateHandlerRequests::HandleLeave, GetEntityId(), mimeData);
        }

        m_activeDelegates.clear();
    }

    bool SceneComponent::HasActiveMimeDelegates() const
    {
        return !m_activeDelegates.empty();
    }

    template<typename Container>
    void SceneComponent::InitItems(const Container& entities) const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        for (const auto& entityRef : entities)
        {
            AZ::Entity* entity = entityRef;
            if (entity)
            {
                if (entity->GetState() == AZ::Entity::State::Constructed)
                {
                    entity->Init();
                }
            }
        }
    }

    template<typename Container>
    void SceneComponent::ActivateItems(const Container& entities)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        for (const auto& entityRef : entities)
        {
            AZ::Entity* entity = entityRef;
            if (entity)
            {
                if (entity->GetState() == AZ::Entity::State::Init)
                {
                    entity->Activate();
                }

                AddSceneMember(entity->GetId());
            }
        }
    }

    template<typename Container>
    void SceneComponent::DeactivateItems(const Container& entities)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        for (const auto& entityRef : entities)
        {
            AZ::Entity* entity = entityRef;
            if (entity)
            {
                if (entity->GetState() == AZ::Entity::State::Active)
                {
                    GeometryNotificationBus::MultiHandler::BusDisconnect(entity->GetId());
                    QGraphicsItem* item = nullptr;
                    SceneMemberUIRequestBus::EventResult(item, entity->GetId(), &SceneMemberUIRequests::GetRootGraphicsItem);
                    SceneMemberRequestBus::Event(entity->GetId(), &SceneMemberRequests::ClearScene, GetEntityId());
                    RemoveItemFromScene(item);
                    entity->Deactivate();
                }
            }
        }
    }

    template<typename Container>
    void SceneComponent::DestroyItems(const Container& entities) const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        for (auto& entityRef : entities)
        {
            delete entityRef;
        }
    }


    void SceneComponent::DestroyGraphicsItem(const GraphicsEffectId& effectId, QGraphicsItem* graphicsItem)
    {
        if (graphicsItem)
        {
            GraphicsEffectRequestBus::Event(effectId, &GraphicsEffectRequests::OnGraphicsEffectCancelled);
            RemoveItemFromScene(graphicsItem);
            // https://stackoverflow.com/questions/38458830/crash-after-qgraphicssceneremoveitem-with-custom-item-class
            // Scene Index does not correctly update causing a crash when the index tree is queried. 
            GraphicsEffectRequestBus::Event(effectId, &GraphicsEffectRequests::PrepareGeometryChange);
            delete graphicsItem;
        }

        // Remove the effect id from our active particle list so we can limit their numbers properly
        AZStd::remove(m_activeParticles.begin(), m_activeParticles.end(), effectId);
    }

    void SceneComponent::InitConnections()
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        BuildEndpointMap(m_graphData);
        InitItems(m_graphData.m_connections);
    }

    void SceneComponent::NotifyConnectedSlots()
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        for (auto& connection : m_graphData.m_connections)
        {
            AZ::Entity* entity = connection;
            auto* connectionEntity = entity ? AZ::EntityUtils::FindFirstDerivedComponent<ConnectionComponent>(entity) : nullptr;
            if (connectionEntity)
            {
                SlotRequestBus::Event(connectionEntity->GetSourceEndpoint().GetSlotId(), &SlotRequests::AddConnectionId, connectionEntity->GetEntityId(), connectionEntity->GetTargetEndpoint());
                SlotRequestBus::Event(connectionEntity->GetTargetEndpoint().GetSlotId(), &SlotRequests::AddConnectionId, connectionEntity->GetEntityId(), connectionEntity->GetSourceEndpoint());
            }
        }
    }

    void SceneComponent::OnSelectionChanged()
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        if((m_isDragSelecting && (m_dragSelectionType != DragSelectionType::Realtime)))
        {
            // Nothing to do.
            return;
        }

        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnSelectionChanged);
    }

    void SceneComponent::RegisterSelectionItem(const AZ::EntityId& itemId)
    {
        QGraphicsItem* selectionItem = nullptr;
        SceneMemberUIRequestBus::EventResult(selectionItem, itemId, &SceneMemberUIRequests::GetSelectionItem);

        m_itemLookup[selectionItem] = itemId;
    }

    void SceneComponent::UnregisterSelectionItem(const AZ::EntityId& itemId)
    {
        QGraphicsItem* selectionItem = nullptr;
        SceneMemberUIRequestBus::EventResult(selectionItem, itemId, &SceneMemberUIRequests::GetSelectionItem);

        m_itemLookup.erase(selectionItem);
        m_hiddenElements.erase(selectionItem);
    }

    void SceneComponent::AddSceneMember(const AZ::EntityId& sceneMemberId, bool positionItem, const AZ::Vector2& position)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();

        QGraphicsItem* graphicsItem = nullptr;
        SceneMemberUIRequestBus::EventResult(graphicsItem, sceneMemberId, &SceneMemberUIRequests::GetRootGraphicsItem);

        if (graphicsItem)
        {
            if (m_graphicsSceneUi)
            {
                m_graphicsSceneUi->addItem(graphicsItem);
            }

            RegisterSelectionItem(sceneMemberId);

            if (positionItem)
            {
                GeometryRequestBus::Event(sceneMemberId, &GeometryRequests::SetPosition, position);
            }
            
            SceneMemberRequestBus::Event(sceneMemberId, &SceneMemberRequests::SetScene, GetEntityId());

            SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnSceneMemberAdded, sceneMemberId);
            GeometryNotificationBus::MultiHandler::BusConnect(sceneMemberId);
            VisualNotificationBus::MultiHandler::BusConnect(sceneMemberId);

            SceneMemberRequestBus::Event(sceneMemberId, &SceneMemberRequests::SignalMemberSetupComplete);
        }
    }

    void SceneComponent::RemoveItemFromScene(QGraphicsItem* graphicsItem)
    {
        if (graphicsItem)
        {
            if (m_graphicsSceneUi && graphicsItem->scene() == m_graphicsSceneUi.get())
            {
                m_graphicsSceneUi->removeItem(graphicsItem);
            }

            m_hiddenElements.erase(graphicsItem);
        }
    }

    void SceneComponent::SieveSceneMembers(const AZStd::unordered_set<AZ::EntityId>& itemIds, SceneMemberBuckets& sceneMembers) const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();

        AZStd::unordered_set< AZ::EntityId > wrapperNodes;

        for (const auto& node : m_graphData.m_nodes)
        {
            if (itemIds.find(node->GetId()) != itemIds.end())
            {
                sceneMembers.m_nodes.insert(node->GetId());

                if (GraphUtils::IsWrapperNode(node->GetId()))
                {
                    wrapperNodes.insert(node->GetId());
                }
            }
        }

        // Wrapper nodes handle copying/deleting everything internal to themselves. 
        // So we need to sanitize our filtering to avoid things that are wrapped when the wrapper
        // is also copied.
        for (const auto& wrapperNode : wrapperNodes)
        {
            AZStd::vector< AZ::EntityId > wrappedNodes;
            WrapperNodeRequestBus::EventResult(wrappedNodes, wrapperNode, &WrapperNodeRequests::GetWrappedNodeIds);

            for (const auto& wrappedNode : wrappedNodes)
            {
                sceneMembers.m_nodes.erase(wrappedNode);
            }
        }
        
        for (const auto& connection : m_graphData.m_connections)
        {
            if (itemIds.find(connection->GetId()) != itemIds.end())
            {
                sceneMembers.m_connections.insert(connection->GetId());
            }
        }

        for (const auto& bookmarkAnchors : m_graphData.m_bookmarkAnchors)
        {
            if (itemIds.find(bookmarkAnchors->GetId()) != itemIds.end())
            {
                sceneMembers.m_bookmarkAnchors.insert(bookmarkAnchors->GetId());
            }
        }
    }

    QPointF SceneComponent::GetViewCenterScenePoint() const
    {
        AZ::Vector2 viewCenter(0,0);

        ViewId viewId = GetViewId();

        ViewRequestBus::EventResult(viewCenter, viewId, &ViewRequests::GetViewSceneCenter);

        return QPointF(viewCenter.GetX(), viewCenter.GetY());
    }

    void SceneComponent::OnDragCursorMove(const QPointF& cursorPoint)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();

        if (m_enableSpliceTracking)
        {
            AZStd::unordered_set< AZ::EntityId > intersectedEntities;
            AZStd::unordered_map< AZ::EntityId, AZ::EntityId > displayMapping;

            for (auto spliceSource : { m_pressedEntity, m_inputCouplingTarget, m_outputCouplingTarget })
            {
                QGraphicsItem* graphicsItem = nullptr;
                SceneMemberUIRequestBus::EventResult(graphicsItem, spliceSource, &SceneMemberUIRequests::GetRootGraphicsItem);

                if (graphicsItem)
                {
                    // We'll use the bounding rect to determine visibility.
                    // But we'll use the cursor position to determine snapping
                    QRectF boundingRect = graphicsItem->sceneBoundingRect();

                    AZStd::vector< AZ::EntityId > sceneEntities = GetEntitiesInRect(boundingRect, Qt::ItemSelectionMode::IntersectsItemShape);

                    for (const AZ::EntityId& entityId : sceneEntities)
                    {
                        if (entityId == spliceSource
                            || m_selectedSubGraph.m_containedNodes.find(entityId) != m_selectedSubGraph.m_containedNodes.end()
                            || m_selectedSubGraph.m_containedConnections.find(entityId) != m_selectedSubGraph.m_containedConnections.end())
                        {
                            continue;
                        }

                        auto insertResult = intersectedEntities.insert(entityId);

                        if (insertResult.second)
                        {
                            displayMapping[entityId] = spliceSource;
                        }
                    }
                }
            }

            if (!intersectedEntities.empty())
            {
                bool ambiguousNode = false;
                AZ::EntityId hoveredNode;

                AZStd::vector< AZ::EntityId > ambiguousConnections;

                for (const AZ::EntityId& currentEntity : intersectedEntities)
                {
                    if (GraphUtils::IsSpliceableConnection(currentEntity)
                        && m_selectedSubGraph.m_containedConnections.find(currentEntity) == m_selectedSubGraph.m_containedConnections.end())
                    {
                        ambiguousConnections.push_back(currentEntity);
                    }
                    else if (GraphUtils::IsConnectableNode(currentEntity))
                    {
                        bool isWrapped = false;
                        NodeRequestBus::EventResult(isWrapped, currentEntity, &NodeRequests::IsWrapped);

                        if (isWrapped)
                        {
                            AZ::EntityId parentId;
                            NodeRequestBus::EventResult(parentId, currentEntity, &NodeRequests::GetWrappingNode);

                            while (parentId.IsValid())
                            {
                                if (parentId == m_inputCouplingTarget
                                    || parentId == m_outputCouplingTarget)
                                {
                                    break;
                                }

                                NodeRequestBus::EventResult(isWrapped, currentEntity, &NodeRequests::IsWrapped);

                                if (isWrapped)
                                {
                                    NodeRequestBus::EventResult(parentId, parentId, &NodeRequests::GetWrappingNode);
                                }
                                else
                                {
                                    break;
                                }
                            }

                            if (parentId == m_inputCouplingTarget
                                || parentId == m_outputCouplingTarget)
                            {
                                continue;
                            }
                        }

                        if (hoveredNode.IsValid())
                        {
                            ambiguousNode = true;
                        }

                        hoveredNode = currentEntity;
                    }
                }

                AZStd::chrono::milliseconds spliceTime(500);

                if (m_enableNodeDragCouplingTracking && !ambiguousNode && hoveredNode.IsValid())
                {
                    AZ::EntityId entityTarget = displayMapping[hoveredNode];

                    if (entityTarget != m_couplingTarget)
                    {
                        m_couplingTarget = entityTarget;

                        StateController<RootGraphicsItemDisplayState>* stateController = nullptr;
                        RootGraphicsItemRequestBus::EventResult(stateController, entityTarget, &RootGraphicsItemRequests::GetDisplayStateStateController);

                        m_couplingEntityDisplayStateStateSetter.ResetStateSetter();
                        m_couplingEntityDisplayStateStateSetter.AddStateController(stateController);
                    }

                    InitiateSpliceToNode(hoveredNode);
                    AssetEditorSettingsRequestBus::EventResult(spliceTime, GetEditorId(), &AssetEditorSettingsRequests::GetDragCouplingTime);
                }
                else if (m_enableNodeDragConnectionSpliceTracking)
                {
                    m_couplingTarget.SetInvalid();
                    m_couplingEntityDisplayStateStateSetter.ResetStateSetter();

                    InitiateSpliceToConnection(ambiguousConnections);
                    AssetEditorSettingsRequestBus::EventResult(spliceTime, GetEditorId(), &AssetEditorSettingsRequests::GetDragConnectionSpliceTime);
                }
                else
                {
                    m_spliceTarget.SetInvalid();

                    m_couplingTarget.SetInvalid();
                    m_couplingEntityDisplayStateStateSetter.ResetStateSetter();
                }

                // If we move, no matter what. Restart the timer, so long as we have a valid target.
                m_spliceTimer.stop();

                if (m_spliceTarget.IsValid())
                {
                    m_spliceTimer.setInterval(aznumeric_cast<int>(spliceTime.count()));
                    m_spliceTimer.start();
                }
            }
            else
            {
                m_spliceTarget.SetInvalid();
                m_couplingTarget.SetInvalid();
                m_couplingEntityDisplayStateStateSetter.ResetStateSetter();

                m_spliceTargetDisplayStateStateSetter.ResetStateSetter();
                m_pressedEntityDisplayStateStateSetter.ReleaseState();

                m_spliceTimer.stop();
            }
        }

        if (!m_draggedGroupableElements.empty())
        {
            DetermineDragGroupTarget(cursorPoint);
        }
    }

    void SceneComponent::DetermineDragGroupTarget(const QPointF& cursorPoint)
    {
        AZ::EntityId bestGroup = FindGroupTarget(cursorPoint, m_ignoredDragTargets);

        if (bestGroup != m_dragTargetGroup)
        {
            m_dragTargetGroup = bestGroup;

            m_forcedGroupDisplayStateStateSetter.ResetStateSetter();

            if (m_dragTargetGroup.IsValid())
            {
                StateController<RootGraphicsItemDisplayState>* displayStateController = nullptr;
                RootGraphicsItemRequestBus::EventResult(displayStateController, m_dragTargetGroup, &RootGraphicsItemRequests::GetDisplayStateStateController);

                m_forcedGroupDisplayStateStateSetter.AddStateController(displayStateController);
                m_forcedGroupDisplayStateStateSetter.SetState(RootGraphicsItemDisplayState::Inspection);

                StateController<AZStd::string>* layerStateController = nullptr;
                LayerControllerRequestBus::EventResult(layerStateController, m_dragTargetGroup, &LayerControllerRequests::GetLayerModifierController);

                m_forcedLayerStateSetter.AddStateController(layerStateController);
                m_forcedLayerStateSetter.SetState("dropTarget");
            }
        }
    }

    AZ::EntityId SceneComponent::FindGroupTarget(const QPointF& scenePoint, const AZStd::unordered_set<AZ::EntityId>& ignoreElements)
    {
        auto entitiesAtPoint = GetEntitiesAt(ConversionUtils::QPointToVector(scenePoint));

        AZStd::unordered_set< AZ::EntityId > groupParentChain;
        AZ::EntityId bestGroup;

        for (auto testEntity : entitiesAtPoint)
        {
            if (ignoreElements.find(testEntity) != ignoreElements.end())
            {
                continue;
            }

            // Only care about groups here. Can ignore anything else for this.
            if (GraphUtils::IsNodeGroup(testEntity))
            {
                bool allowGroup = true;

                // Safe guard against trying to drag a parent group into a child and creating an infinite loop.
                AZStd::unordered_set<AZ::EntityId> testParentChain;
                AZ::EntityId groupedId = testEntity;

                while (groupedId.IsValid())
                {
                    GroupableSceneMemberRequests* groupableRequests = GroupableSceneMemberRequestBus::FindFirstHandler(groupedId);

                    if (groupableRequests)
                    {
                        if (ignoreElements.find(groupedId) != ignoreElements.end())
                        {
                            allowGroup = false;
                            break;
                        }

                        if (GraphUtils::IsNodeGroup(groupedId))
                        {
                            testParentChain.insert(groupedId);
                        }

                        groupedId = groupableRequests->GetGroupId();
                    }
                    else
                    {
                        break;
                    }
                }

                if (!allowGroup)
                {
                    continue;
                }
                ////

                if (bestGroup.IsValid())
                {
                    // If this group is apart of the previous chain, we can ignore it as we have the more specific group.
                    if (groupParentChain.find(testEntity) == groupParentChain.end())
                    {
                        AZ::EntityId groupedId2 = testEntity;

                        for (AZ::EntityId testParent : testParentChain)
                        {
                            if (GraphUtils::IsNodeGroup(testParent))
                            {
                                testParentChain.insert(testParent);
                            }

                            // If we discover a more specific version. We can update to that.
                            if (groupedId2 == bestGroup)
                            {
                                bestGroup = testEntity;
                                groupParentChain = testParentChain;
                                break;
                            }
                        }

                        // If we have two equally 'valid' groups then we want to just ignore them both as the drop is ambiguous.
                        if (bestGroup != testEntity)
                        {
                            bestGroup.SetInvalid();
                            break;
                        }
                    }
                }
                else
                {
                    bestGroup = testEntity;
                    groupParentChain = testParentChain;
                }
            }
        }

        return bestGroup;
    }

    void SceneComponent::OnTrySplice()
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();

        GraphModelRequestBus::Event(GetEntityId(), &GraphModelRequests::RequestPushPreventUndoStateUpdate);

        m_spliceTargetDisplayStateStateSetter.ResetStateSetter();
        m_pressedEntityDisplayStateStateSetter.ReleaseState();
        m_couplingEntityDisplayStateStateSetter.ReleaseState();
        
        // Make sure we have a valid target for whatever we are trying to 'splice' against.
        // Then check the preconditions for the various other tracking elements.
        if (m_enableSpliceTracking && m_spliceTarget.IsValid() 
            && (   (m_enableNodeDragCouplingTracking && (m_inputCouplingTarget.IsValid() || m_outputCouplingTarget.IsValid()))
                || (m_enableNodeDragConnectionSpliceTracking && m_pressedEntity.IsValid())
                || (m_enableNodeChainDragConnectionSpliceTracking && !m_selectedSubGraph.m_containedNodes.empty())
               )
           ) 
        {
            AnimatedPulseConfiguration pulseConfiguration;

            pulseConfiguration.m_durationSec = 0.35f;
            pulseConfiguration.m_enableGradient = true;

            AZ::EntityId pulseTarget = m_pressedEntity;

            if (GraphUtils::IsConnection(m_spliceTarget))
            {
                if (m_enableNodeChainDragConnectionSpliceTracking)
                {
                    if (GraphUtils::SpliceSubGraphOntoConnection(m_selectedSubGraph, m_spliceTarget))
                    {
                        m_forceDragReleaseUndo = true;
                        pulseConfiguration.m_drawColor = QColor(255, 255, 255);                        
                        StartNudging(m_selectedSubGraph.m_containedNodes);
                    }
                    else
                    {
                        pulseConfiguration.m_drawColor = QColor(255, 0, 0);
                    }
                }
                else
                {
                    ConnectionSpliceConfig spliceConfig;
                    spliceConfig.m_allowOpportunisticConnections = false;

                    if (GraphUtils::SpliceNodeOntoConnection(m_pressedEntity, m_spliceTarget, spliceConfig))
                    {
                        m_forceDragReleaseUndo = true;
                        pulseConfiguration.m_drawColor = QColor(255, 255, 255);

                        StartNudging(m_selectedSubGraph.m_containedNodes);
                    }
                    else
                    {
                        pulseConfiguration.m_drawColor = QColor(255, 0, 0);
                    }
                }
            }
            else if (GraphUtils::IsNode(m_spliceTarget))
            {
                pulseTarget = m_couplingTarget;

                QRectF targetRect;

                QGraphicsItem* targetItem = nullptr;
                SceneMemberUIRequestBus::EventResult(targetItem, m_spliceTarget, &SceneMemberUIRequests::GetRootGraphicsItem);

                if (targetItem)
                {
                    targetRect = targetItem->sceneBoundingRect();
                }

                QGraphicsItem* draggingEntity = nullptr;

                AZStd::unordered_set< GraphCanvas::ConnectionType > allowableTypes;

                if (m_inputCouplingTarget == m_outputCouplingTarget)
                {
                    allowableTypes.insert(GraphCanvas::ConnectionType::CT_Input);
                    allowableTypes.insert(GraphCanvas::ConnectionType::CT_Output);

                    SceneMemberUIRequestBus::EventResult(draggingEntity, m_inputCouplingTarget, &SceneMemberUIRequests::GetRootGraphicsItem);
                }
                else if (m_couplingTarget == m_inputCouplingTarget)
                {
                    allowableTypes.insert(GraphCanvas::ConnectionType::CT_Input);
                }
                else
                {
                    allowableTypes.insert(GraphCanvas::ConnectionType::CT_Output);
                }

                SceneMemberUIRequestBus::EventResult(draggingEntity, m_couplingTarget, &SceneMemberUIRequests::GetRootGraphicsItem);

                if (draggingEntity && targetItem)
                {
                    QRectF draggingRect = draggingEntity->sceneBoundingRect();

                    GraphCanvas::ConnectionType allowedType = GraphCanvas::ConnectionType::CT_None;

                    // Reference point is we are gathering slots from the pressed node.
                    // So we want to determine which side is offset from, and grabs the nodes from the other side.
                    if (draggingRect.x() > targetRect.x())
                    {
                        allowedType = GraphCanvas::ConnectionType::CT_Input;
                    }
                    else
                    {
                        allowedType = GraphCanvas::ConnectionType::CT_Output;
                    }

                    AZStd::vector< Endpoint > connectableEndpoints;

                    if (allowableTypes.count(allowedType) > 0)
                    {
                        AZStd::vector< AZ::EntityId > slotIds;
                        NodeRequestBus::EventResult(slotIds, m_couplingTarget, &NodeRequests::GetSlotIds);

                        for (const AZ::EntityId& testSlotId : slotIds)
                        {
                            if (GraphUtils::IsSlotVisible(testSlotId))
                            {
                                GraphCanvas::ConnectionType connectionType = GraphCanvas::ConnectionType::CT_Invalid;
                                SlotRequestBus::EventResult(connectionType, testSlotId, &SlotRequests::GetConnectionType);

                                if (connectionType == allowedType)
                                {
                                    connectableEndpoints.emplace_back(m_couplingTarget, testSlotId);
                                }
                            }
                        }
                    }

                    CreateConnectionsBetweenConfig config;
                    config.m_connectionType = CreateConnectionsBetweenConfig::CreationType::SinglePass;

                    if (!connectableEndpoints.empty() && GraphUtils::CreateConnectionsBetween(connectableEndpoints, m_spliceTarget, config))
                    {
                        m_forceDragReleaseUndo = true;
                        pulseConfiguration.m_drawColor = QColor(255, 255, 255);
                    }
                    else
                    {
                        pulseConfiguration.m_drawColor = QColor(255, 0, 0);
                    }
                }
            }

            QGraphicsItem* item = nullptr;
            SceneMemberUIRequestBus::EventResult(item, pulseTarget, &SceneMemberUIRequests::GetRootGraphicsItem);

            if (item)
            {
                pulseConfiguration.m_zValue = item->zValue() - 1;
            }

            CreatePulseAroundSceneMember(pulseTarget, 3, pulseConfiguration);
            m_spliceTarget.SetInvalid();
        }

        GraphModelRequestBus::Event(GetEntityId(), &GraphModelRequests::RequestPopPreventUndoStateUpdate);
    }

    void SceneComponent::InitiateSpliceToNode(const NodeId& nodeId)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();

        if (m_spliceTarget != nodeId)
        {
            m_spliceTargetDisplayStateStateSetter.ResetStateSetter();

            m_spliceTarget = nodeId;

            if (m_spliceTarget.IsValid())
            {
                m_couplingEntityDisplayStateStateSetter.SetState(RootGraphicsItemDisplayState::InspectionTransparent);

                StateController<RootGraphicsItemDisplayState>* stateController;
                RootGraphicsItemRequestBus::EventResult(stateController, m_spliceTarget, &RootGraphicsItemRequests::GetDisplayStateStateController);

                m_spliceTargetDisplayStateStateSetter.AddStateController(stateController);
                m_spliceTargetDisplayStateStateSetter.SetState(RootGraphicsItemDisplayState::Preview);
            }
            else
            {
                m_couplingEntityDisplayStateStateSetter.ReleaseState();
            }
        }
    }

    void SceneComponent::InitiateSpliceToConnection(const AZStd::vector<ConnectionId>& connectionIds)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();

        m_spliceTarget.SetInvalid();
        m_spliceTargetDisplayStateStateSetter.ResetStateSetter();

        if (!connectionIds.empty())
        {
            m_pressedEntityDisplayStateStateSetter.SetState(RootGraphicsItemDisplayState::InspectionTransparent);
        }
        else if (m_pressedEntityDisplayStateStateSetter.HasState())
        {
            m_pressedEntityDisplayStateStateSetter.ReleaseState();
        }

        GraphCanvasGraphicsView* graphicsView = nullptr;
        ViewRequestBus::EventResult(graphicsView, m_viewId, &ViewRequests::AsGraphicsView);

        QPointF scenePoint;

        if (graphicsView)
        {
            QPointF cursorPoint = QCursor::pos();
            QPointF viewPoint = graphicsView->mapFromGlobal(cursorPoint.toPoint());
            scenePoint = graphicsView->mapToScene(viewPoint.toPoint());
        }

        for (const ConnectionId& connectionId : connectionIds)
        {
            bool containsCursor = false;

            QGraphicsItem* spliceTargetItem = nullptr;
            SceneMemberUIRequestBus::EventResult(spliceTargetItem, connectionId, &SceneMemberUIRequests::GetRootGraphicsItem);

            if (spliceTargetItem)
            {
                containsCursor = spliceTargetItem->contains(scenePoint);
            }

            if (containsCursor)
            {
                m_spliceTarget = connectionId;

                StateController<RootGraphicsItemDisplayState>* stateController;
                RootGraphicsItemRequestBus::EventResult(stateController, m_spliceTarget, &RootGraphicsItemRequests::GetDisplayStateStateController);

                m_spliceTargetDisplayStateStateSetter.AddStateController(stateController);
                m_spliceTargetDisplayStateStateSetter.SetState(RootGraphicsItemDisplayState::Preview);
            }
        }
    }

    //////////////////////////////
    // GraphCanvasGraphicsScenes
    //////////////////////////////

    GraphCanvasGraphicsScene::GraphCanvasGraphicsScene(SceneComponent& scene)
        : m_scene(scene)
        , m_suppressContextMenu(false)
    {
        setMinimumRenderSize(2.0f);
        connect(this, &QGraphicsScene::selectionChanged, this, [this]() { m_scene.OnSelectionChanged(); });
        setSceneRect(-20000, -20000, 40000, 40000);
    }

    AZ::EntityId GraphCanvasGraphicsScene::GetEntityId() const
    {
        return m_scene.GetEntityId();
    }

    void GraphCanvasGraphicsScene::SuppressNextContextMenu()
    {
        m_suppressContextMenu = true;
    }

    void GraphCanvasGraphicsScene::keyPressEvent(QKeyEvent* event)
    {
        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnKeyPressed, event);

        QGraphicsScene::keyPressEvent(event);
    }

    void GraphCanvasGraphicsScene::keyReleaseEvent(QKeyEvent* event)
    {
        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnKeyReleased, event);

        QGraphicsScene::keyPressEvent(event);
    }

    void GraphCanvasGraphicsScene::contextMenuEvent(QGraphicsSceneContextMenuEvent* contextMenuEvent)
    {
        if (!m_suppressContextMenu && m_scene.AllowContextMenu())
        {
            const QPoint& screenPos = contextMenuEvent->screenPos();
            const QPointF& scenePos = contextMenuEvent->scenePos();
            contextMenuEvent->ignore();

            ContextMenuAction::SceneReaction reaction = ContextMenuAction::SceneReaction::Unknown;

            // Send the event to all items at this position until one item accepts the event.
            for (QGraphicsItem* item : itemsAtPosition(contextMenuEvent->screenPos(), contextMenuEvent->scenePos(), contextMenuEvent->widget()))
            {
                AZ::EntityId memberId;

                auto mapIter = m_scene.m_itemLookup.find(item);

                if (mapIter != m_scene.m_itemLookup.end())
                {
                    memberId = mapIter->second;
                }

                if (!memberId.IsValid())
                {
                    continue;
                }
                else if (memberId == m_scene.GetGrid())
                {
                    // Scene context menu might add elements to the scene. So we'll want to highlight the group to ensure we communicate
                    // that the group will be affected by these adds.                    
                    m_contextMenuGroupTarget = m_scene.FindGroupTarget(scenePos);

                    SignalGroupHighlight();

                    AssetEditorRequestBus::EventResult(reaction, m_scene.GetEditorId(), &AssetEditorRequests::ShowSceneContextMenuWithGroup, screenPos, scenePos, m_contextMenuGroupTarget);
                }
                else
                {
                    // Want to early out before I do the selection manipulation for the node groups.
                    // unless it's in the title. Then I treat it like normal.
                    if (GraphUtils::IsNodeGroup(memberId))
                    {
                        bool isInTitle = false;
                        NodeGroupRequestBus::EventResult(isInTitle, memberId, &NodeGroupRequests::IsInTitle, scenePos);

                        if (!isInTitle)
                        {
                            continue;
                        }
                    }

                    bool isMemberSelected = false;
                    SceneMemberUIRequestBus::EventResult(isMemberSelected, memberId, &SceneMemberUIRequests::IsSelected);

                    bool shouldAppendSelection = contextMenuEvent->modifiers().testFlag(Qt::ControlModifier);

                    // clear the current selection if you are not multi selecting and clicking on an unselected node
                    if (!isMemberSelected && !shouldAppendSelection)
                    {
                        m_scene.ClearSelection();
                    }

                    if (!isMemberSelected)
                    {
                        SceneMemberUIRequestBus::Event(memberId, &SceneMemberUIRequests::SetSelected, true);
                    }

                    contextMenuEvent->accept();

                    GraphModelRequestBus::Event(GetEntityId(), &GraphModelRequests::RequestPushPreventUndoStateUpdate);

                    if (GraphUtils::IsNodeGroup(memberId))
                    {
                        AssetEditorRequestBus::EventResult(reaction, m_scene.GetEditorId(), &AssetEditorRequests::ShowNodeGroupContextMenu, memberId, screenPos, scenePos);
                    }
                    else if (GraphUtils::IsConnection(memberId))
                    {
                        // Connection Context Menu might add elements to the scene. So we'll want to highlight the group to ensure we communicate
                        // that the group will be affected by these adds.
                        m_contextMenuGroupTarget = m_scene.FindGroupTarget(scenePos);

                        SignalGroupHighlight();

                        AssetEditorRequestBus::EventResult(reaction, m_scene.GetEditorId(), &AssetEditorRequests::ShowConnectionContextMenuWithGroup, memberId, screenPos, scenePos, m_contextMenuGroupTarget);
                    }
                    else if (GraphUtils::IsBookmarkAnchor(memberId))
                    {
                        AssetEditorRequestBus::EventResult(reaction, m_scene.GetEditorId(), &AssetEditorRequests::ShowBookmarkContextMenu, memberId, screenPos, scenePos);
                    }
                    else if (GraphUtils::IsComment(memberId))
                    {
                        AssetEditorRequestBus::EventResult(reaction, m_scene.GetEditorId(), &AssetEditorRequests::ShowCommentContextMenu, memberId, screenPos, scenePos);
                    }
                    else
                    {
                        bool isNode = GraphUtils::IsNode(memberId);
                        bool isCollapsedGroup = GraphUtils::IsCollapsedNodeGroup(memberId);

                        if (GraphUtils::IsNode(memberId))
                        {
                            AZStd::vector< SlotId > slotIds;
                            NodeRequestBus::EventResult(slotIds, memberId, &NodeRequests::GetSlotIds);

                            AZ::Vector2 azScenePoint = ConversionUtils::QPointToVector(scenePos);

                            SlotId targetSlotId;

                            for (const SlotId& slotId : slotIds)
                            {
                                bool isSlotContextMenu = false;
                                VisualRequestBus::EventResult(isSlotContextMenu, slotId, &VisualRequests::Contains, azScenePoint);

                                if (isSlotContextMenu)
                                {
                                    if (GraphUtils::IsSlotVisible(slotId))
                                    {
                                        targetSlotId = slotId;
                                        break;
                                    }
                                }
                            }

                            if (targetSlotId.IsValid())
                            {
                                AssetEditorRequestBus::EventResult(reaction, m_scene.GetEditorId(), &AssetEditorRequests::ShowSlotContextMenu, targetSlotId, screenPos, scenePos);
                            }
                        }

                        if (reaction == ContextMenuAction::SceneReaction::Unknown)
                        {
                            if (GraphUtils::IsComment(memberId)
                                || isNode)
                            {
                                AssetEditorRequestBus::EventResult(reaction, m_scene.GetEditorId(), &AssetEditorRequests::ShowNodeContextMenu, memberId, screenPos, scenePos);
                            }
                            else if (isCollapsedGroup)
                            {
                                AssetEditorRequestBus::EventResult(reaction, m_scene.GetEditorId(), &AssetEditorRequests::ShowCollapsedNodeGroupContextMenu, memberId, screenPos, scenePos);
                            }
                        }
                    }

                    GraphModelRequestBus::Event(GetEntityId(), &GraphModelRequests::RequestPopPreventUndoStateUpdate);
                }

                break;
            }

            if (reaction == ContextMenuAction::SceneReaction::PostUndo)
            {
                GraphModelRequestBus::Event(GetEntityId(), &GraphModelRequests::RequestUndoPoint);
            }
        }
        else
        {
            m_suppressContextMenu = false;
        }

        CleanupHighlight();
    }

    QList<QGraphicsItem*> GraphCanvasGraphicsScene::itemsAtPosition(const QPoint& screenPos, const QPointF& scenePos, QWidget* widget) const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();

        QGraphicsView* view = widget ? qobject_cast<QGraphicsView*>(widget->parentWidget()) : nullptr;
        if (!view)
        {
            return items(scenePos, Qt::IntersectsItemShape, Qt::DescendingOrder, QTransform());
        }

        const QRectF pointRect(QPointF(widget->mapFromGlobal(screenPos)), QSizeF(1, 1));
        if (!view->isTransformed())
        {
            return items(pointRect, Qt::IntersectsItemShape, Qt::DescendingOrder);
        }

        const QTransform viewTransform = view->viewportTransform();
        if (viewTransform.type() <= QTransform::TxScale)
        {
            return items(viewTransform.inverted().mapRect(pointRect), Qt::IntersectsItemShape,
                Qt::DescendingOrder, viewTransform);
        }
        return items(viewTransform.inverted().map(pointRect), Qt::IntersectsItemShape,
            Qt::DescendingOrder, viewTransform);
    }

    void GraphCanvasGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
    {
        if (event->button() == Qt::RightButton)
        {
            // IMPORTANT: When the user right-clicks on the scene,
            // and there are NO items at the click position, the
            // current selection is lost. See documentation:
            //
            // "If there is no item at the given position on the scene,
            // the selection area is reset, any focus item loses its
            // input focus, and the event is then ignored."
            // http://doc.qt.io/qt-5/qgraphicsscene.html#mousePressEvent
            //
            // This ISN'T the behavior we want. We want to preserve
            // the current selection to allow scene interactions. To get around
            // this behavior, we'll accept the event and by-pass its
            // default implementation.

            event->accept();
            return;
        }

        QGraphicsScene::mousePressEvent(event);
    }

    void GraphCanvasGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
    {
        m_scene.OnSelectionChanged();

        QGraphicsScene::mouseReleaseEvent(event);

        m_scene.FinalizeNudging();
    }

    void GraphCanvasGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
    {
        QPointF scenePos = event->scenePos();
        QPointF lastScenePos = event->lastScenePos();

        // These events seem to fire off regardless of mouse input(so long as mouse is down) which causes weird behavior(broken ctrl+left selection).
        // Only process these if there was actual movement.
        if (scenePos == lastScenePos)
        {
            return;
        }

        QGraphicsScene::mouseMoveEvent(event);

        if ((m_scene.m_enableSpliceTracking || m_scene.m_isDraggingEntity) && event->lastPos() != event->pos())
        {
            m_scene.OnDragCursorMove(scenePos);
        }
    }

    void GraphCanvasGraphicsScene::dragEnterEvent(QGraphicsSceneDragDropEvent * event)
    {
        QGraphicsScene::dragEnterEvent(event);

        m_scene.OnSceneDragEnter(event->mimeData());

        if (m_scene.HasActiveMimeDelegates())
        {
            event->accept();
            event->acceptProposedAction();
        }
    }

    void GraphCanvasGraphicsScene::dragLeaveEvent(QGraphicsSceneDragDropEvent* event)
    {
        QGraphicsScene::dragLeaveEvent(event);

        m_scene.OnSceneDragExit(event->mimeData());
    }

    void GraphCanvasGraphicsScene::dragMoveEvent(QGraphicsSceneDragDropEvent* event)
    {
        QGraphicsScene::dragMoveEvent(event);

        m_scene.OnSceneDragMoveEvent(event->scenePos(), event->mimeData());

        if (m_scene.HasActiveMimeDelegates())
        {
            event->accept();
            event->acceptProposedAction();
        }
    }

    void GraphCanvasGraphicsScene::dropEvent(QGraphicsSceneDragDropEvent* event)
    {
        bool accepted = event->isAccepted();
        event->setAccepted(false);

        QGraphicsScene::dropEvent(event);

        if (!event->isAccepted() && m_scene.HasActiveMimeDelegates())
        {
            event->accept();
            m_scene.OnSceneDropEvent(event->scenePos(), event->mimeData());
        }
        else
        {
            event->setAccepted(accepted);
        }
    }

    void GraphCanvasGraphicsScene::SignalGroupHighlight()
    {
        if (m_contextMenuGroupTarget.IsValid())
        {
            StateController<RootGraphicsItemDisplayState>* displayStateController = nullptr;
            RootGraphicsItemRequestBus::EventResult(displayStateController, m_contextMenuGroupTarget, &RootGraphicsItemRequests::GetDisplayStateStateController);

            m_forcedGroupDisplayStateStateSetter.AddStateController(displayStateController);
            m_forcedGroupDisplayStateStateSetter.SetState(RootGraphicsItemDisplayState::Inspection);

            StateController<AZStd::string>* layerStateController = nullptr;
            LayerControllerRequestBus::EventResult(layerStateController, m_contextMenuGroupTarget, &LayerControllerRequests::GetLayerModifierController);

            m_forcedLayerStateSetter.AddStateController(layerStateController);
            m_forcedLayerStateSetter.SetState("dropTarget");
        }
    }

    void GraphCanvasGraphicsScene::CleanupHighlight()
    {
        m_contextMenuGroupTarget.SetInvalid();

        m_forcedGroupDisplayStateStateSetter.ResetStateSetter();
        m_forcedLayerStateSetter.ResetStateSetter();
    }
}
