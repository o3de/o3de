/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <qcursor.h>
#include <qgraphicsscene.h>
#include <qgraphicsview.h>
#include <QToolTip>

#include <AzCore/Serialization/EditContext.h>

#include <AzQtComponents/Components/ToastNotification.h>

#include <Components/Connections/ConnectionComponent.h>

#include <Components/Connections/ConnectionLayerControllerComponent.h>
#include <Components/Connections/ConnectionVisualComponent.h>
#include <Components/StylingComponent.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/Slots/Extender/ExtenderSlotBus.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Editor/GraphModelBus.h>
#include <GraphCanvas/Utils/GraphUtils.h>
#include <GraphCanvas/Utils/ConversionUtils.h>
#include <GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>

namespace GraphCanvas
{
    ///////////////////////////////
    // ConnectionEndpointAnimator
    ///////////////////////////////

    ConnectionComponent::ConnectionEndpointAnimator::ConnectionEndpointAnimator()
        : m_isAnimating(false)
        , m_timer(0.0f)
        , m_maxTime(0.25f)
    {

    }

    bool ConnectionComponent::ConnectionEndpointAnimator::IsAnimating() const
    {
        return m_isAnimating;
    }

    void ConnectionComponent::ConnectionEndpointAnimator::AnimateToEndpoint(const QPointF& startPoint, const Endpoint& endPoint, float maxTime)
    {
        if (m_isAnimating)
        {
            m_startPosition = m_currentPosition;
        }
        else
        {
            m_isAnimating = true;
            m_startPosition = startPoint;
        }

        m_targetEndpoint = endPoint;

        m_timer = 0.0f;
        m_maxTime = AZ::GetMax(maxTime, 0.001f);

        m_currentPosition = m_startPosition;
    }

    QPointF ConnectionComponent::ConnectionEndpointAnimator::GetAnimatedPosition() const
    {
        return m_currentPosition;
    }

    bool ConnectionComponent::ConnectionEndpointAnimator::Tick(float deltaTime)
    {
        m_timer += deltaTime;

        QPointF targetPosition;
        SlotUIRequestBus::EventResult(targetPosition, m_targetEndpoint.m_slotId, &SlotUIRequests::GetConnectionPoint);

        if (m_timer >= m_maxTime)
        {
            m_isAnimating = false;
            m_currentPosition = targetPosition;
        }
        else
        {
            float lerpPercent = (m_timer / m_maxTime);
            m_currentPosition.setX(AZ::Lerp(static_cast<float>(m_startPosition.x()), static_cast<float>(targetPosition.x()), lerpPercent));
            m_currentPosition.setY(AZ::Lerp(static_cast<float>(m_startPosition.y()), static_cast<float>(targetPosition.y()), lerpPercent));
        }

        return m_isAnimating;
    }

    ////////////////////////
    // ConnectionComponent
    ////////////////////////

    void ConnectionComponent::Reflect(AZ::ReflectContext* context)
    {
        Endpoint::Reflect(context);
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<ConnectionComponent, AZ::Component>()
            ->Version(3)
            ->Field("Source", &ConnectionComponent::m_sourceEndpoint)
            ->Field("Target", &ConnectionComponent::m_targetEndpoint)
            ->Field("Tooltip", &ConnectionComponent::m_tooltip)
            ->Field("UserData", &ConnectionComponent::m_userData)
            ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<ConnectionComponent>("Position", "The connection's position in the scene")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "Connection's class attributes")
            ->DataElement(AZ::Edit::UIHandlers::Default, &ConnectionComponent::m_tooltip, "Tooltip", "The connection's tooltip")
            ;
    }

    AZ::Entity* ConnectionComponent::CreateBaseConnectionEntity(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint, bool createModelConnection, const AZStd::string& selectorClass)
    {
        // Create this Connection's entity.
        AZ::Entity* entity = aznew AZ::Entity("Connection");

        entity->CreateComponent<ConnectionComponent>(sourceEndpoint, targetEndpoint, createModelConnection);
        entity->CreateComponent<StylingComponent>(Styling::Elements::Connection, AZ::EntityId(), selectorClass);
        entity->CreateComponent<ConnectionLayerControllerComponent>();

        return entity;
    }

    AZ::Entity* ConnectionComponent::CreateGeneralConnection(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint, bool createModelConnection, const AZStd::string& substyle)
    {
        AZ::Entity* entity = CreateBaseConnectionEntity(sourceEndpoint, targetEndpoint, createModelConnection, substyle);
        entity->CreateComponent<ConnectionVisualComponent>();

        return entity;
    }    

    ConnectionComponent::ConnectionComponent()
        : m_dragContext(DragContext::Unknown)
    {
    }

    ConnectionComponent::ConnectionComponent(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint, bool createModelConnection)
        : AZ::Component()
    {
        m_sourceEndpoint = sourceEndpoint;
        m_targetEndpoint = targetEndpoint;

        AZ_Warning("GraphCanvas", m_targetEndpoint.IsValid() || m_sourceEndpoint.IsValid(), "Either source or target endpoint must be valid when creating a connection.");

        if (createModelConnection && m_sourceEndpoint.IsValid() && m_targetEndpoint.IsValid())
        {
            m_dragContext = DragContext::TryConnection;
        }
        else
        {
            m_dragContext = DragContext::Unknown;
        }
    }

    void ConnectionComponent::Activate()
    {
        ConnectionRequestBus::Handler::BusConnect(GetEntityId());
        SceneMemberRequestBus::Handler::BusConnect(GetEntityId());
        
        if (m_sourceEndpoint.IsValid() && m_targetEndpoint.IsValid() && m_dragContext != DragContext::TryConnection)
        {
            m_dragContext = DragContext::Connected;
        }

        StateController<RootGraphicsItemDisplayState>* displayStateController = nullptr;
        RootGraphicsItemRequestBus::EventResult(displayStateController, GetEntityId(), &RootGraphicsItemRequests::GetDisplayStateStateController);

        m_connectionStateStateSetter.AddStateController(displayStateController);
    }

    void ConnectionComponent::Deactivate()
    {
        StopMove();

        SceneMemberRequestBus::Handler::BusDisconnect();
        ConnectionRequestBus::Handler::BusDisconnect();

        CleanupToast();
    }

    void ConnectionComponent::OnSlotRemovedFromNode(const AZ::EntityId& slotId)
    {
        if (m_dragContext != DragContext::Connected)
        {
            if (slotId == m_sourceEndpoint.GetSlotId()
                || slotId == m_targetEndpoint.GetSlotId())
            {
                OnEscape();
            }
        }
    }

    AZ::EntityId ConnectionComponent::GetSourceSlotId() const
    {
        return m_sourceEndpoint.GetSlotId();
    }

    AZ::EntityId ConnectionComponent::GetSourceNodeId() const
    {
        return m_sourceEndpoint.GetNodeId();
    }

    Endpoint ConnectionComponent::GetSourceEndpoint() const
    {
        return m_sourceEndpoint;
    }

    QPointF ConnectionComponent::GetSourcePosition() const
    {
        if (m_sourceAnimator.IsAnimating())
        {
            return m_sourceAnimator.GetAnimatedPosition();
        }
        else if (m_sourceEndpoint.IsValid())
        {
            QPointF connectionPoint;
            SlotUIRequestBus::EventResult(connectionPoint, m_sourceEndpoint.m_slotId, &SlotUIRequests::GetConnectionPoint);

            return connectionPoint;
        }
        else
        {
            return m_mousePoint;
        }
    }

    void ConnectionComponent::StartSourceMove()
    {
        SlotRequestBus::Event(GetSourceEndpoint().GetSlotId(), &SlotRequests::RemoveConnectionId, GetEntityId(), GetTargetEndpoint());
        m_previousEndPoint = m_sourceEndpoint;
        m_sourceEndpoint = Endpoint();

        m_dragContext = DragContext::MoveSource;

        StartMove();
    }

    void ConnectionComponent::SnapSourceDisplayTo(const Endpoint& sourceEndpoint)
    {
        if (!sourceEndpoint.IsValid())
        {
            AZ_Error("GraphCanvas", false, "Trying to display a connection to an unknown source Endpoint");
            return;
        }

        bool canDisplaySource = false;
        SlotRequestBus::EventResult(canDisplaySource, m_targetEndpoint.GetSlotId(), &SlotRequests::CanDisplayConnectionTo, sourceEndpoint);

        if (!canDisplaySource)
        {
            return;
        }

        if (m_sourceEndpoint.IsValid())
        {
            SlotRequestBus::Event(m_sourceEndpoint.GetSlotId(), &SlotRequests::RemoveConnectionId, GetEntityId(), m_targetEndpoint);
        }

        Endpoint oldEndpoint = m_sourceEndpoint;
        m_sourceEndpoint = sourceEndpoint;

        ConnectionNotificationBus::Event(GetEntityId(), &ConnectionNotifications::OnSourceSlotIdChanged, oldEndpoint.GetSlotId(), m_sourceEndpoint.GetSlotId());
        SlotRequestBus::Event(m_sourceEndpoint.GetSlotId(), &SlotRequests::AddConnectionId, GetEntityId(), m_targetEndpoint);
    }

    void ConnectionComponent::AnimateSourceDisplayTo(const Endpoint& sourceEndpoint, float connectionTime)
    {
        QPointF startPosition = GetSourcePosition();
        SnapSourceDisplayTo(sourceEndpoint);        
        m_sourceAnimator.AnimateToEndpoint(startPosition, sourceEndpoint, connectionTime);
            
        if (!AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusConnect();
        }        
    }    

    AZ::EntityId ConnectionComponent::GetTargetSlotId() const
    {
        return m_targetEndpoint.GetSlotId();
    }

    AZ::EntityId ConnectionComponent::GetTargetNodeId() const
    {
        return m_targetEndpoint.GetNodeId();
    }

    Endpoint ConnectionComponent::GetTargetEndpoint() const
    {
        return m_targetEndpoint;
    }

    QPointF ConnectionComponent::GetTargetPosition() const
    {
        if (m_targetAnimator.IsAnimating())
        {
            return m_targetAnimator.GetAnimatedPosition();
        }
        else if (m_targetEndpoint.IsValid())
        {
            QPointF connectionPoint;
            SlotUIRequestBus::EventResult(connectionPoint, m_targetEndpoint.m_slotId, &SlotUIRequests::GetConnectionPoint);

            return connectionPoint;
        }
        else
        {
            return m_mousePoint;
        }
    }

    void ConnectionComponent::StartTargetMove()
    {
        SlotRequestBus::Event(GetTargetEndpoint().GetSlotId(), &SlotRequests::RemoveConnectionId, GetEntityId(), GetSourceEndpoint());

        m_previousEndPoint = m_targetEndpoint;
        m_targetEndpoint = Endpoint();
        m_dragContext = DragContext::MoveTarget;

        StartMove();
    }

    void ConnectionComponent::SnapTargetDisplayTo(const Endpoint& targetEndpoint)
    {
        if (!targetEndpoint.IsValid())
        {
            AZ_Error("GraphCanvas", false, "Trying to display a connection to an unknown source Endpoint");
            return;
        }

        bool canDisplayTarget = false;
        SlotRequestBus::EventResult(canDisplayTarget, m_sourceEndpoint.GetSlotId(), &SlotRequests::CanDisplayConnectionTo, targetEndpoint);

        if (!canDisplayTarget)
        {
            return;
        }        

        if (m_targetEndpoint.IsValid())
        {
            SlotRequestBus::Event(m_targetEndpoint.GetSlotId(), &SlotRequests::RemoveConnectionId, GetEntityId(), m_sourceEndpoint);
        }

        Endpoint oldTarget = m_targetEndpoint;

        m_targetEndpoint = targetEndpoint;

        ConnectionNotificationBus::Event(GetEntityId(), &ConnectionNotifications::OnTargetSlotIdChanged, oldTarget.GetSlotId(), m_targetEndpoint.GetSlotId());
        SlotRequestBus::Event(m_targetEndpoint.GetSlotId(), &SlotRequests::AddConnectionId, GetEntityId(), m_sourceEndpoint);        
    }

    void ConnectionComponent::AnimateTargetDisplayTo(const Endpoint& targetEndpoint, float connectionTime)
    {
        QPointF startPosition = GetTargetPosition();

        SnapTargetDisplayTo(targetEndpoint);

        m_targetAnimator.AnimateToEndpoint(startPosition, targetEndpoint, connectionTime);

        if (!AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusConnect();
        }
    }

    bool ConnectionComponent::ContainsEndpoint(const Endpoint& endpoint) const
    {
        bool containsEndpoint = false;

        if (m_sourceEndpoint == endpoint)
        {
            containsEndpoint = (m_dragContext != DragContext::MoveSource);
        }
        else if (m_targetEndpoint == endpoint)
        {
            containsEndpoint = (m_dragContext != DragContext::MoveTarget);
        }

        return containsEndpoint;
    }

    void ConnectionComponent::ChainProposalCreation(const QPointF& scenePos, const QPoint& screenPos, AZ::EntityId groupTarget)
    {
        UpdateMovePosition(scenePos);

        SetGroupTarget(groupTarget);

        const bool chainAddition = true;
        FinalizeMove(scenePos, screenPos, chainAddition);
    }

    void ConnectionComponent::OnEscape()
    {
        StopMove();

        bool keepConnection = OnConnectionMoveCancelled();

        if (!keepConnection)
        {
            AZStd::unordered_set<AZ::EntityId> deletion;
            deletion.insert(GetEntityId());

            SceneRequestBus::Event(m_graphId, &SceneRequests::Delete, deletion);
        }
    }

    void ConnectionComponent::SetScene(const GraphId& graphId)
    {
        CleanupToast();

        m_graphId = graphId;

        if (!m_sourceEndpoint.IsValid())
        {
            StartSourceMove();
        }
        else if (!m_targetEndpoint.IsValid())
        {
            StartTargetMove();
        }
        else if (m_dragContext == DragContext::TryConnection)
        {
            OnConnectionMoveComplete(QPointF(), QPoint(), AZ::EntityId());
        }

        SceneMemberNotificationBus::Event(GetEntityId(), &SceneMemberNotifications::OnSceneSet, m_graphId);
    }

    void ConnectionComponent::ClearScene(const AZ::EntityId& /*oldSceneId*/)
    {
        AZ_Warning("Graph Canvas", m_graphId.IsValid(), "This connection (ID: %s) is not in a scene (ID: %s)!", GetEntityId().ToString().data(), m_graphId.ToString().data());
        AZ_Warning("Graph Canvas", GetEntityId().IsValid(), "This connection (ID: %s) doesn't have an Entity!", GetEntityId().ToString().data());

        if (!m_graphId.IsValid() || !GetEntityId().IsValid())
        {
            return;
        }

        SceneMemberNotificationBus::Event(GetEntityId(), &SceneMemberNotifications::OnRemovedFromScene, m_graphId);
        m_graphId.SetInvalid();
    }

    void ConnectionComponent::SignalMemberSetupComplete()
    {
        SceneMemberNotificationBus::Event(GetEntityId(), &SceneMemberNotifications::OnMemberSetupComplete);
    }

    AZ::EntityId ConnectionComponent::GetScene() const
    {
        return m_graphId;
    }

    AZStd::string ConnectionComponent::GetTooltip() const
    {
        return m_tooltip;
    }

    void ConnectionComponent::SetTooltip(const AZStd::string& tooltip)
    {
        m_tooltip = tooltip;
    }

    AZStd::any* ConnectionComponent::GetUserData()
    {
        return &m_userData;
    }

    void ConnectionComponent::OnTick(float deltaTime, AZ::ScriptTimePoint)
    {
        bool sourceAnimating = m_sourceAnimator.IsAnimating() && m_sourceAnimator.Tick(deltaTime);
        bool targetAnimating = m_targetAnimator.IsAnimating() && m_targetAnimator.Tick(deltaTime);

        ConnectionUIRequestBus::Event(GetEntityId(), &ConnectionUIRequests::UpdateConnectionPath);

        if (!sourceAnimating && !targetAnimating)
        {
            AZ::TickBus::Handler::BusDisconnect();
        }
    }

    void ConnectionComponent::OnFocusLost()
    {
        OnEscape();
    }

    void ConnectionComponent::OnNodeIsBeingEdited(bool isBeingEdited)
    {
        if (isBeingEdited)
        {
            OnEscape();
        }
    }

    void ConnectionComponent::SetGroupTarget(AZ::EntityId groupTarget)
    {
        if (groupTarget != m_groupTarget)
        {
            m_groupTarget = groupTarget;

            if (m_groupTarget.IsValid())
            {
                StateController<RootGraphicsItemDisplayState>* displayStateController = nullptr;
                RootGraphicsItemRequestBus::EventResult(displayStateController, m_groupTarget, &RootGraphicsItemRequests::GetDisplayStateStateController);

                m_forcedGroupDisplayStateStateSetter.AddStateController(displayStateController);
                m_forcedGroupDisplayStateStateSetter.SetState(RootGraphicsItemDisplayState::Inspection);

                StateController<AZStd::string>* layerStateController = nullptr;
                LayerControllerRequestBus::EventResult(layerStateController, m_groupTarget, &LayerControllerRequests::GetLayerModifierController);

                m_forcedLayerStateSetter.AddStateController(layerStateController);
                m_forcedLayerStateSetter.SetState("dropTarget");
            }
            else
            {
                m_forcedGroupDisplayStateStateSetter.ResetStateSetter();
                m_forcedLayerStateSetter.ResetStateSetter();
            }
        }
    }

    void ConnectionComponent::FinalizeMove()
    {
        DragContext dragContext = m_dragContext;
        m_dragContext = DragContext::Connected;

        if (dragContext == DragContext::MoveSource)
        {
            ConnectionNotificationBus::Event(GetEntityId(), &ConnectionNotifications::OnSourceSlotIdChanged, m_previousEndPoint.GetSlotId(), m_sourceEndpoint.GetSlotId());
            SlotRequestBus::Event(GetSourceEndpoint().GetSlotId(), &SlotRequests::AddConnectionId, GetEntityId(), GetTargetEndpoint());
        }
        else
        {
            ConnectionNotificationBus::Event(GetEntityId(), &ConnectionNotifications::OnTargetSlotIdChanged, m_previousEndPoint.GetSlotId(), m_targetEndpoint.GetSlotId());
            SlotRequestBus::Event(GetTargetEndpoint().GetSlotId(), &SlotRequests::AddConnectionId, GetEntityId(), GetSourceEndpoint());
        }

        const bool isValidConnection = true;
        ConnectionNotificationBus::Event(GetEntityId(), &ConnectionNotifications::OnMoveFinalized, isValidConnection);
    }

    void ConnectionComponent::OnConnectionMoveStart()
    {
        SceneRequestBus::Event(m_graphId, &SceneRequests::SignalConnectionDragBegin);
        ConnectionNotificationBus::Event(GetEntityId(), &ConnectionNotifications::OnMoveBegin);
        GraphModelRequestBus::Event(m_graphId, &GraphModelRequests::DisconnectConnection, GetEntityId());

        if (m_dragContext == DragContext::MoveSource)
        {
            NodeRequestBus::Event(GetTargetNodeId(), &NodeRequests::SignalConnectionMoveBegin, GetEntityId());
        }
        else if (m_dragContext == DragContext::MoveTarget)
        {
            NodeRequestBus::Event(GetSourceNodeId(), &NodeRequests::SignalConnectionMoveBegin, GetEntityId());
        }
    }

    bool ConnectionComponent::OnConnectionMoveCancelled()
    {
        bool keepConnection = false;

        if (m_previousEndPoint.IsValid())
        {
            if (m_dragContext == DragContext::MoveSource)
            {
                m_sourceEndpoint = m_previousEndPoint;
            }
            else
            {
                m_targetEndpoint = m_previousEndPoint;
            }

            bool acceptConnection = GraphUtils::CreateModelConnection(m_graphId, GetEntityId(), m_sourceEndpoint, m_targetEndpoint);

            AZ_Error("GraphCanvas", acceptConnection, "Cancelled a move, and was unable to reconnect to the previous connection.");
            if (acceptConnection)
            {
                keepConnection = true;
                FinalizeMove();
            }
        }

        if (!keepConnection)
        {
            const bool isValidConnection = false;
            ConnectionNotificationBus::Event(GetEntityId(), &ConnectionNotifications::OnMoveFinalized, isValidConnection);
        }

        return keepConnection;
    }

    ConnectionComponent::ConnectionMoveResult ConnectionComponent::OnConnectionMoveComplete(const QPointF& scenePos, const QPoint& screenPos, AZ::EntityId groupTarget)
    {
        ConnectionMoveResult connectionResult = ConnectionMoveResult::DeleteConnection;

        bool acceptConnection = GraphUtils::CreateModelConnection(m_graphId, GetEntityId(), m_sourceEndpoint, m_targetEndpoint);

        if (acceptConnection)
        {
            connectionResult = ConnectionMoveResult::ConnectionMove;
        }
        else if (!acceptConnection && !m_previousEndPoint.IsValid() && m_dragContext != DragContext::TryConnection && AllowNodeCreation())
        {
            Endpoint knownEndpoint = m_sourceEndpoint;

            if (!knownEndpoint.IsValid())
            {
                knownEndpoint = m_targetEndpoint;
            }

            GraphCanvas::Endpoint otherEndpoint;

            EditorId editorId;
            SceneRequestBus::EventResult(editorId, m_graphId, &SceneRequests::GetEditorId);
            AssetEditorRequestBus::EventResult(otherEndpoint, editorId, &AssetEditorRequests::CreateNodeForProposalWithGroup, GetEntityId(), knownEndpoint, scenePos, screenPos, groupTarget);

            if (otherEndpoint.IsValid())
            {
                if (!m_sourceEndpoint.IsValid())
                {
                    m_sourceEndpoint = otherEndpoint;
                }
                else if (!m_targetEndpoint.IsValid())
                {
                    m_targetEndpoint = otherEndpoint;
                }

                acceptConnection = GraphUtils::CreateModelConnection(m_graphId, GetEntityId(), m_sourceEndpoint, m_targetEndpoint);

                if (acceptConnection)
                {
                    connectionResult = ConnectionMoveResult::NodeCreation;
                }
            }
        }

        return connectionResult;
    }

    bool ConnectionComponent::AllowNodeCreation() const
    {
        return true;
    }

    void ConnectionComponent::CleanupToast()
    {
        if (m_toastId.IsValid())
        {
            ViewId viewId;
            SceneRequestBus::EventResult(viewId, m_graphId, &SceneRequests::GetViewId);

            auto viewHandler = ViewRequestBus::FindFirstHandler(viewId);

            if (viewHandler == nullptr)
            {
                return;
            }
            
            viewHandler->HideToastNotification(m_toastId);
            m_toastId.SetInvalid();
        }
    }

    void ConnectionComponent::StartMove()
    {
        QGraphicsItem* connectionGraphicsItem = nullptr;
        SceneMemberUIRequestBus::EventResult(connectionGraphicsItem, GetEntityId(), &SceneMemberUIRequests::GetRootGraphicsItem);

        if (connectionGraphicsItem)
        {
            connectionGraphicsItem->setSelected(false);
            connectionGraphicsItem->setOpacity(0.5f);

            m_eventFilter = aznew ConnectionEventFilter((*this));

            ViewId viewId;
            SceneRequestBus::EventResult(viewId, m_graphId, &SceneRequests::GetViewId);

            ViewNotificationBus::Handler::BusConnect(viewId);

            QGraphicsScene* graphicsScene = nullptr;
            SceneRequestBus::EventResult(graphicsScene, m_graphId, &SceneRequests::AsQGraphicsScene);

            if (graphicsScene)
            {
                graphicsScene->addItem(m_eventFilter);

                if (!graphicsScene->views().empty())
                {
                    QGraphicsView* view = graphicsScene->views().front();
                    m_mousePoint = view->mapToScene(view->mapFromGlobal(QCursor::pos()));
                }
            }

            connectionGraphicsItem->installSceneEventFilter(m_eventFilter);
            connectionGraphicsItem->grabMouse();

            SceneNotificationBus::Handler::BusConnect(m_graphId);
            StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::AddSelectorState, Styling::States::Dragging);

            AZ::EntityId nodeId;
            StateController<RootGraphicsItemDisplayState>* stateController = nullptr;

            if (m_dragContext == DragContext::MoveSource)
            {
                nodeId = GetTargetEndpoint().GetNodeId();
            }
            else
            {
                nodeId = GetSourceEndpoint().GetNodeId();
            }

            RootGraphicsItemRequestBus::EventResult(stateController, nodeId, &RootGraphicsItemRequests::GetDisplayStateStateController);

            m_nodeDisplayStateStateSetter.AddStateController(stateController);
            m_nodeDisplayStateStateSetter.SetState(RootGraphicsItemDisplayState::Inspection);

            ConnectionUIRequestBus::Event(GetEntityId(), &ConnectionUIRequests::SetAltDeletionEnabled, false);
            NodeNotificationBus::Handler::BusConnect(nodeId);

            OnConnectionMoveStart();
        }
    }

    void ConnectionComponent::StopMove()
    {
        QGraphicsItem* connectionGraphicsItem = nullptr;
        SceneMemberUIRequestBus::EventResult(connectionGraphicsItem, GetEntityId(), &SceneMemberUIRequests::GetRootGraphicsItem);

        if (connectionGraphicsItem)
        {
            connectionGraphicsItem->removeSceneEventFilter(m_eventFilter);
            delete m_eventFilter;
            m_eventFilter = nullptr;
            
            connectionGraphicsItem->setOpacity(1.0f);
            connectionGraphicsItem->ungrabMouse();

            StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::RemoveSelectorState, Styling::States::Dragging);
        }

        SceneNotificationBus::Handler::BusDisconnect();;
        NodeNotificationBus::Handler::BusDisconnect();
        ViewNotificationBus::Handler::BusDisconnect();

        if (m_dragContext == DragContext::MoveSource)
        {
            SlotRequestBus::Event(GetSourceEndpoint().GetSlotId(), &SlotRequests::RemoveProposedConnection, GetEntityId(), GetTargetEndpoint());
            StyledEntityRequestBus::Event(GetSourceEndpoint().GetSlotId(), &StyledEntityRequests::RemoveSelectorState, Styling::States::ValidDrop);
        }
        else
        {
            SlotRequestBus::Event(GetTargetEndpoint().GetSlotId(), &SlotRequests::RemoveProposedConnection, GetEntityId(), GetSourceEndpoint());
            StyledEntityRequestBus::Event(GetTargetEndpoint().GetSlotId(), &StyledEntityRequests::RemoveSelectorState, Styling::States::ValidDrop);
        }

        m_nodeDisplayStateStateSetter.ResetStateSetter();
        SceneRequestBus::Event(m_graphId, &SceneRequests::SignalConnectionDragEnd);
        ConnectionUIRequestBus::Event(GetEntityId(), &ConnectionUIRequests::SetAltDeletionEnabled, true);

        SetGroupTarget(AZ::EntityId());
    }

    bool ConnectionComponent::UpdateProposal(Endpoint& activePoint, const Endpoint& proposalPoint, AZStd::function< void(const AZ::EntityId&, const AZ::EntityId&)> endpointChangedFunctor)
    {
        bool retVal = false;

        if (activePoint != proposalPoint)
        {
            retVal = true;

            QGraphicsItem* connectionGraphicsItem = nullptr;
            SceneMemberUIRequestBus::EventResult(connectionGraphicsItem, GetEntityId(), &SceneMemberUIRequests::GetRootGraphicsItem);

            if (activePoint.IsValid())
            {
                StateController<RootGraphicsItemDisplayState>* stateController = nullptr;
                RootGraphicsItemRequestBus::EventResult(stateController, activePoint.GetNodeId(), &RootGraphicsItemRequests::GetDisplayStateStateController);

                m_nodeDisplayStateStateSetter.RemoveStateController(stateController);

                StyledEntityRequestBus::Event(activePoint.m_slotId, &StyledEntityRequests::RemoveSelectorState, Styling::States::ValidDrop);
                if (connectionGraphicsItem)
                {
                    connectionGraphicsItem->setOpacity(0.5f);
                }
            }

            AZ::EntityId oldId = activePoint.GetSlotId();

            activePoint = proposalPoint;
            endpointChangedFunctor(oldId, activePoint.GetSlotId());

            if (activePoint.IsValid())
            {
                StateController<RootGraphicsItemDisplayState>* stateController = nullptr;
                RootGraphicsItemRequestBus::EventResult(stateController, activePoint.GetNodeId(), &RootGraphicsItemRequests::GetDisplayStateStateController);

                m_nodeDisplayStateStateSetter.AddStateController(stateController);

                StyledEntityRequestBus::Event(activePoint.m_slotId, &StyledEntityRequests::AddSelectorState, Styling::States::ValidDrop);
                if (connectionGraphicsItem)
                {
                    connectionGraphicsItem->setOpacity(1.0f);
                }
            }
        }

        return retVal || !activePoint.IsValid();
    }

    ConnectionComponent::ConnectionCandidate ConnectionComponent::FindConnectionCandidateAt(const QPointF& scenePos) const
    {
        Endpoint knownEndpoint = m_targetEndpoint;

        if (m_dragContext == DragContext::MoveTarget)
        {
            knownEndpoint = m_sourceEndpoint;
        }

        EditorId editorId;
        SceneRequestBus::EventResult(editorId, m_graphId, &SceneRequests::GetEditorId);

        double snapDist = 10.0;
        AssetEditorSettingsRequestBus::EventResult(snapDist, editorId, &AssetEditorSettingsRequests::GetSnapDistance);

        QPointF dist(snapDist, snapDist);
        QRectF rect(scenePos - dist, scenePos + dist);

        // These are returnned sorted. So we just need to return the first match we find.
        AZStd::vector<Endpoint> endpoints;
        SceneRequestBus::EventResult(endpoints, m_graphId, &SceneRequests::GetEndpointsInRect, rect);

        ConnectionCandidate candidate;

        for (Endpoint endpoint : endpoints)
        {
            // Skip over ourselves.
            if (endpoint == knownEndpoint)
            {
                continue;
            }

            if (!GraphUtils::IsSlotVisible(endpoint.GetSlotId()))
            {
                continue;
            }

            // For our tool tips we really only want to focus in on the first element
            if (!candidate.m_testedTarget.IsValid())
            {
                candidate.m_testedTarget = endpoint;
            }

            bool canCreateConnection = false;

            if (m_dragContext == DragContext::MoveSource && endpoint == m_sourceEndpoint)
            {
                canCreateConnection = true;
            }
            else if (m_dragContext == DragContext::MoveTarget && endpoint == m_targetEndpoint)
            {
                canCreateConnection = true;
            }
            else if (m_dragContext == DragContext::MoveTarget && endpoint == m_sourceEndpoint
                     || m_dragContext == DragContext::MoveSource && endpoint == m_targetEndpoint)
            {
                continue;
            }
            else
            {
                // If we are checking against an extender slot. We need to go through special flow.
                // Since the extender will create a new slot for us to connect to.
                if (auto extenderHandler = ExtenderSlotRequestBus::FindFirstHandler(endpoint.GetSlotId()))
                {
                    Endpoint newConnectionEndpoint = extenderHandler->ExtendForConnectionProposal(GetEntityId(), knownEndpoint);

                    if (newConnectionEndpoint.IsValid())
                    {
                        canCreateConnection = true;
                        endpoint = newConnectionEndpoint;                        
                    }
                }
                else
                {
                    SlotRequestBus::EventResult(canCreateConnection, endpoint.GetSlotId(), &SlotRequests::CanCreateConnectionTo, knownEndpoint);
                }
            }

            if (canCreateConnection)
            {
                candidate.m_connectableTarget = endpoint;                
                break;
            }
        }

        return candidate;
    }

    void ConnectionComponent::UpdateMovePosition(const QPointF& position)
    {
        if (m_dragContext == DragContext::MoveSource
            || m_dragContext == DragContext::MoveTarget)
        {
            m_mousePoint = position;

            ConnectionCandidate connectionCandidate = FindConnectionCandidateAt(m_mousePoint);

            bool updateConnection = false;
            if (m_dragContext == DragContext::MoveSource)
            {
                AZStd::function<void(const AZ::EntityId&, const AZ::EntityId)> updateFunction = [this](const AZ::EntityId& oldId, const AZ::EntityId& newId)
                {
                    SlotRequestBus::Event(oldId, &SlotRequests::RemoveProposedConnection, this->GetEntityId(), this->GetTargetEndpoint());
                    SlotRequestBus::Event(newId, &SlotRequests::DisplayProposedConnection, this->GetEntityId(), this->GetTargetEndpoint());
                    ConnectionNotificationBus::Event(this->GetEntityId(), &ConnectionNotifications::OnSourceSlotIdChanged, oldId, newId);
                };

                updateConnection = UpdateProposal(m_sourceEndpoint, connectionCandidate.m_connectableTarget, updateFunction);
            }
            else
            {
                AZStd::function<void(const AZ::EntityId&, const AZ::EntityId)> updateFunction = [this](const AZ::EntityId& oldId, const AZ::EntityId& newId)
                {
                    SlotRequestBus::Event(oldId, &SlotRequests::RemoveProposedConnection, this->GetEntityId(), this->GetSourceEndpoint());
                    SlotRequestBus::Event(newId, &SlotRequests::DisplayProposedConnection, this->GetEntityId(), this->GetSourceEndpoint());
                    ConnectionNotificationBus::Event(this->GetEntityId(), &ConnectionNotifications::OnTargetSlotIdChanged, oldId, newId);
                };

                updateConnection = UpdateProposal(m_targetEndpoint, connectionCandidate.m_connectableTarget, updateFunction);
            }

            if (connectionCandidate.m_connectableTarget.IsValid())
            {
                Endpoint invalidEndpoint;
                DisplayConnectionToolTip(position, invalidEndpoint);

                // If we have a valid target. We want to not manage our group target.
                SetGroupTarget(AZ::EntityId());
            }
            else
            {
                DisplayConnectionToolTip(position, connectionCandidate.m_testedTarget);
                
                AZ::EntityId groupTarget;
                SceneRequestBus::EventResult(groupTarget, GetScene(), &SceneRequests::FindTopmostGroupAtPoint, m_mousePoint);
                SetGroupTarget(groupTarget);                
            }

            if (updateConnection)
            {
                ConnectionUIRequestBus::Event(GetEntityId(), &ConnectionUIRequests::UpdateConnectionPath);
            }
        }
    }

    void ConnectionComponent::FinalizeMove(const QPointF& scenePos, const QPoint& screenPos, bool chainAddition)
    {
        if (m_dragContext == DragContext::Connected)
        {
            AZ_Error("Graph Canvas", false, "Receiving MouseRelease event in invalid drag state.");
            return;
        }

        DisplayConnectionToolTip(scenePos, Endpoint());

        DragContext chainContext = m_dragContext;

        AZ::EntityId groupTarget = m_groupTarget;

        StopMove();

        // Have to copy the GraphId here because deletion of the Entity this component is attached to deletes this component.
        GraphId graphId = m_graphId;

        ConnectionMoveResult connectionResult = OnConnectionMoveComplete(scenePos, screenPos, groupTarget);

        if (connectionResult == ConnectionMoveResult::DeleteConnection)
        {
            // If the previous endpoint is not valid, this implies a new connection is being created
            bool preventUndoState = !m_previousEndPoint.IsValid();
            if (preventUndoState)
            {
                GraphModelRequestBus::Event(graphId, &GraphModelRequests::RequestPushPreventUndoStateUpdate);
            }

            AZ::EntityId connectionId = GetEntityId();

            // OnMoveFinalized might end up deleting the connection if it was from an ExtenderSlot, so no member methods should be called after either of these methods.
            //
            // The SceneRequests::Delete will delete the Entity this component is attached.
            // Therefore it is invalid to access the members of this component after the call.
            const bool isValidConnection = false;
            ConnectionNotificationBus::Event(connectionId, &ConnectionNotifications::OnMoveFinalized, isValidConnection);

            AZStd::unordered_set<AZ::EntityId> deletion;
            deletion.insert(connectionId);
            
            SceneRequestBus::Event(graphId, &SceneRequests::Delete, deletion);
            if (preventUndoState)
            {
                GraphModelRequestBus::Event(graphId, &GraphModelRequests::RequestPopPreventUndoStateUpdate);
            }
        }
        else
        {
            FinalizeMove();
            GraphModelRequestBus::Event(graphId, &GraphModelRequests::RequestUndoPoint);

            if (chainAddition && connectionResult == ConnectionMoveResult::NodeCreation)
            {
                AZ::EntityId graphId2;
                SceneMemberRequestBus::EventResult(graphId2, GetEntityId(), &SceneMemberRequests::GetScene);

                AZ::EntityId nodeId;
                SlotType slotType = SlotTypes::Invalid;
                ConnectionType connectionType = CT_Invalid;

                if (chainContext == DragContext::MoveSource)
                {
                    nodeId = GetSourceNodeId();
                    slotType = SlotTypes::ExecutionSlot;
                    connectionType = CT_Input;
                }
                else if (chainContext == DragContext::MoveTarget)
                {
                    nodeId = GetTargetNodeId();
                    slotType = SlotTypes::ExecutionSlot;
                    connectionType = CT_Output;
                }

                SceneRequestBus::Event(graphId2, &SceneRequests::HandleProposalDaisyChainWithGroup, nodeId, slotType, connectionType, screenPos, scenePos, groupTarget);
            }
        }
    }

    void ConnectionComponent::DisplayConnectionToolTip(const QPointF& /*scenePoint*/, const Endpoint& connectionTarget)
    {
        if (m_endpointTooltip != connectionTarget)
        {
            GraphId graphId;
            SceneMemberRequestBus::EventResult(graphId, GetEntityId(), &SceneMemberRequests::GetScene);

            ViewId viewId;
            SceneRequestBus::EventResult(viewId, graphId, &SceneRequests::GetViewId);

            auto viewHandler = ViewRequestBus::FindFirstHandler(viewId);

            if (viewHandler == nullptr)
            {
                return;
            }

            CleanupToast();

            m_endpointTooltip = connectionTarget;

            // No endpoint I'm going to treat like a valid connection
            m_validationResult = ConnectionValidationTooltip();
            m_validationResult.m_isValid = true;

            // If our tooltip target is the same as both our target and source, this means we are trying to connect to the point we started from.
            // This just looks weird, so we won't do it.
            if (m_endpointTooltip.IsValid())
            {
                // If we are pointing at an extender slot. Don't investigate the reason for a failure.                
                if (ExtenderSlotRequestBus::FindFirstHandler(m_endpointTooltip.GetSlotId()) != nullptr)
                {
                    return;
                }

                if (m_dragContext == DragContext::MoveTarget)
                {
                    if (m_sourceEndpoint != m_endpointTooltip)
                    {
                        m_validationResult = GraphUtils::GetModelConnnectionValidityToolTip(graphId, m_sourceEndpoint, m_endpointTooltip);
                    }
                }
                else
                {
                    if (m_targetEndpoint != m_endpointTooltip)
                    {
                        m_validationResult = GraphUtils::GetModelConnnectionValidityToolTip(graphId, m_endpointTooltip, m_targetEndpoint);
                    }
                }
            }

            if (!m_validationResult.m_isValid)
            {
                EditorId editorId;
                SceneRequestBus::EventResult(editorId, graphId, &SceneRequests::GetEditorId);

                QPointF connectionPoint;
                SlotUIRequestBus::EventResult(connectionPoint, m_endpointTooltip.GetSlotId(), &SlotUIRequests::GetConnectionPoint);
                
                AZ::Vector2 globalConnectionVector = ConversionUtils::QPointToVector(connectionPoint);
                globalConnectionVector = viewHandler->MapToGlobal(globalConnectionVector);

                QPointF globalConnectionPoint = ConversionUtils::AZToQPoint(globalConnectionVector);

                QPointF anchorPoint(0.0f, 0.0f);
                AzQtComponents::ToastConfiguration toastConfiguration(AzQtComponents::ToastType::Error, "Unable to connect to slot", m_validationResult.m_failureReason.c_str());
                toastConfiguration.m_closeOnClick = false;

                m_toastId = viewHandler->ShowToastAtPoint(globalConnectionPoint.toPoint(), anchorPoint, toastConfiguration);
            }
        }
    }
}
