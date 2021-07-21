/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <QGraphicsWidget>
#include <QTimer>
#include <QSequentialAnimationGroup>

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Components/Nodes/Group/CollapsedNodeGroupComponent.h>

#include <Components/LayerControllerComponent.h>
#include <Components/Nodes/General/GeneralNodeLayoutComponent.h>
#include <GraphCanvas/Components/Connections/ConnectionBus.h>
#include <GraphCanvas/Components/Nodes/Comment/CommentBus.h>
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/Editor/GraphModelBus.h>

#include <GraphCanvas/Utils/ConversionUtils.h>

namespace GraphCanvas
{
    //////////////////////////
    // RedirectedSlotWatcher
    //////////////////////////

    RedirectedSlotWatcher::~RedirectedSlotWatcher()
    {
        NodeNotificationBus::MultiHandler::BusDisconnect();
    }

    void RedirectedSlotWatcher::ConfigureWatcher(const AZ::EntityId& collapsedGroupId)
    {
        m_collapsedGroupId = collapsedGroupId;

        NodeNotificationBus::MultiHandler::BusDisconnect();
    }

    void RedirectedSlotWatcher::RegisterEndpoint(const Endpoint& sourceEndpoint, const Endpoint& remappedEndpoint)
    {
        m_endpointMapping[sourceEndpoint] = remappedEndpoint;
        NodeNotificationBus::MultiHandler::BusConnect(sourceEndpoint.GetNodeId());
    }

    void RedirectedSlotWatcher::OnNodeAboutToBeDeleted()
    {
        const AZ::EntityId* nodeRemoved = NodeNotificationBus::GetCurrentBusId();

        if (nodeRemoved)
        {
            auto mapIter = m_endpointMapping.begin();

            while (mapIter != m_endpointMapping.end())
            {
                if (mapIter->first.GetNodeId() == (*nodeRemoved))
                {
                    NodeRequestBus::Event(m_collapsedGroupId, &NodeRequests::RemoveSlot, mapIter->second.GetSlotId());
                    mapIter = m_endpointMapping.erase(mapIter);
                }
                else
                {
                    ++mapIter;
                }
            }

            NodeNotificationBus::MultiHandler::BusDisconnect((*nodeRemoved));
        }
    }

    void RedirectedSlotWatcher::OnSlotRemovedFromNode(const AZ::EntityId& slotId)
    {
        const AZ::EntityId* nodeSource = NodeNotificationBus::GetCurrentBusId();

        if (nodeSource)
        {
            Endpoint sourceEndpoint((*nodeSource), slotId);

            auto endpointIter = m_endpointMapping.find(sourceEndpoint);

            if (endpointIter != m_endpointMapping.end())
            {
                NodeRequestBus::Event(m_collapsedGroupId, &NodeRequests::RemoveSlot, endpointIter->second.GetSlotId());
                m_endpointMapping.erase(endpointIter);

                bool maintainConnection = false;

                for (const auto& mapPair : m_endpointMapping)
                {
                    if (mapPair.first.GetNodeId() == (*nodeSource))
                    {
                        maintainConnection = true;
                        break;
                    }
                }

                if (!maintainConnection)
                {
                    NodeNotificationBus::MultiHandler::BusDisconnect((*nodeSource));
                }
            }
        }
    }

    ////////////////////////////////
    // CollapsedNodeGroupComponent
    ////////////////////////////////

    constexpr int k_collapsingAnimationTimeMS = 175;
    constexpr int k_fadeInTimeMS = 50;    

    // 0.9f found through the scientific process of it looking right.
    constexpr float k_endpointanimationTimeSec = (k_collapsingAnimationTimeMS/1000.0f) * 0.9f;

    // General frame delay to ensure that qt has updated and refreshed its display so that everything looks right.
    // 3 is just a magic number found through visual testing.
    constexpr int k_qtFrameDelay = 3;
    
    void CollapsedNodeGroupComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        
        if (serializeContext)
        {
            serializeContext->Class<SlotRedirectionConfiguration>()
                ->Version(1)
                ->Field("Name", &SlotRedirectionConfiguration::m_name)
                ->Field("TargetId", &SlotRedirectionConfiguration::m_targetEndpoint)
            ;

            serializeContext->Class<CollapsedNodeGroupComponent, GraphCanvasPropertyComponent>()
                ->Version(1)
            ;
        }
    }
    
    AZ::Entity* CollapsedNodeGroupComponent::CreateCollapsedNodeGroupEntity(const CollapsedNodeGroupConfiguration& config)
    {
        AZ::Entity* nodeEntity = GeneralNodeLayoutComponent::CreateGeneralNodeEntity(".collapsedGroup", config);

        nodeEntity->CreateComponent<CollapsedNodeGroupComponent>(config);

        return nodeEntity;
    }
    
    CollapsedNodeGroupComponent::CollapsedNodeGroupComponent()
        : GraphCanvasPropertyComponent()
        , m_animationDelayCounter(0)
        , m_isExpandingOccluderAnimation(false)
        , m_occluderDestructionCounter(0)        
        , m_unhideOnAnimationComplete(false)
        , m_deleteObjects(true)
        , m_positionDirty(false)
        , m_ignorePositionChanges(true)
    {
        // Two part animation.
        QSequentialAnimationGroup* opacityGroup = new QSequentialAnimationGroup();

        QPropertyAnimation* delayAnimation = new QPropertyAnimation();
        delayAnimation->setDuration((k_collapsingAnimationTimeMS - k_fadeInTimeMS));
        opacityGroup->addAnimation(delayAnimation);

        m_opacityAnimation = new QPropertyAnimation();
        m_opacityAnimation->setDuration(k_fadeInTimeMS);
        m_opacityAnimation->setPropertyName("opacity");
        m_opacityAnimation->setStartValue(1.0f);
        m_opacityAnimation->setEndValue(0.25f);

        opacityGroup->addAnimation(m_opacityAnimation);
        opacityGroup->setLoopCount(1);

        m_occluderAnimation.addAnimation(opacityGroup);

        m_sizeAnimation = new QPropertyAnimation();
        m_sizeAnimation->setDuration(k_collapsingAnimationTimeMS);
        m_sizeAnimation->setPropertyName("size");

        m_occluderAnimation.addAnimation(m_sizeAnimation);

        m_positionAnimation = new QPropertyAnimation();
        m_positionAnimation->setDuration(k_collapsingAnimationTimeMS);
        m_positionAnimation->setPropertyName("pos");

        m_occluderAnimation.addAnimation(m_positionAnimation);

        QObject::connect(&m_occluderAnimation, &QAnimationGroup::finished, [this]()
        {
            OnAnimationFinished();
        });
    }
    
    CollapsedNodeGroupComponent::CollapsedNodeGroupComponent(const CollapsedNodeGroupConfiguration& config)
        : CollapsedNodeGroupComponent()
    {
        m_nodeGroupId = config.m_nodeGroupId;
    }
    
    CollapsedNodeGroupComponent::~CollapsedNodeGroupComponent()
    {
    }
    
    void CollapsedNodeGroupComponent::Init()
    {
        GraphCanvasPropertyComponent::Init();

        m_memberHiddenStateSetter.AddStateController(&m_ignorePositionChanges);
        m_memberDraggedStateSetter.AddStateController(&m_ignorePositionChanges);
    }
    
    void CollapsedNodeGroupComponent::Activate()
    {
        GraphCanvasPropertyComponent::Activate();

        AZ::EntityId entityId = GetEntityId();

        m_redirectedSlotWatcher.ConfigureWatcher(entityId);
        
        CollapsedNodeGroupRequestBus::Handler::BusConnect(entityId);
        VisualNotificationBus::Handler::BusConnect(entityId);
        NodeNotificationBus::Handler::BusConnect(entityId);
        SceneMemberNotificationBus::Handler::BusConnect(entityId);
        GeometryNotificationBus::Handler::BusConnect(entityId);
    }
    
    void CollapsedNodeGroupComponent::Deactivate()
    {
        GraphCanvasPropertyComponent::Deactivate();

        GroupableSceneMemberNotificationBus::Handler::BusDisconnect();
        CommentNotificationBus::Handler::BusDisconnect();
        GeometryNotificationBus::Handler::BusDisconnect();
        SceneMemberNotificationBus::Handler::BusDisconnect();
        NodeNotificationBus::Handler::BusDisconnect();
        VisualNotificationBus::Handler::BusDisconnect();
        CollapsedNodeGroupRequestBus::Handler::BusDisconnect();
        AZ::SystemTickBus::Handler::BusDisconnect();
    }

    void CollapsedNodeGroupComponent::OnSystemTick()
    {
        // Delay count for Qt to catch up with the visuals so I can animate in a visually pleasing way.
        if (m_animationDelayCounter > 0)
        {
            m_animationDelayCounter--;
            
            if (m_animationDelayCounter <= 0)
            {
                AnimateOccluder(m_isExpandingOccluderAnimation);

                m_isExpandingOccluderAnimation = false;
                m_animationDelayCounter = 0;

                UpdateSystemTickBus();
            }
        }

        if (m_occluderDestructionCounter > 0)
        {
            m_occluderDestructionCounter--;

            if (m_occluderDestructionCounter <= 0)
            {
                AZ::EntityId graphId;
                SceneMemberRequestBus::EventResult(graphId, GetEntityId(), &SceneMemberRequests::GetScene);

                if (m_effectId.IsValid())
                {
                    SceneRequestBus::Event(graphId, &SceneRequests::CancelGraphicsEffect, m_effectId);
                    m_effectId.SetInvalid();
                }

                m_occluderDestructionCounter = 0;
                UpdateSystemTickBus();

                CollapsedNodeGroupNotificationBus::Event(GetEntityId(), &CollapsedNodeGroupNotifications::OnExpansionComplete);

                AZStd::unordered_set<NodeId> deleteIds = { GetEntityId() };
                SceneRequestBus::Event(graphId, &SceneRequests::Delete, deleteIds);
            }
        }
    }
    
    void CollapsedNodeGroupComponent::OnAddedToScene(const GraphId& graphId)
    {
        SceneNotificationBus::Handler::BusConnect(graphId);

        m_containedSubGraphs.Clear();

        AZStd::string comment;
        CommentRequestBus::EventResult(comment, m_nodeGroupId, &CommentRequests::GetComment);

        NodeTitleRequestBus::Event(GetEntityId(), &NodeTitleRequests::SetTitle, comment);
        NodeTitleRequestBus::Event(GetEntityId(), &NodeTitleRequests::SetSubTitle, "Collapsed Node Group");

        AZ::Color color;
        NodeGroupRequestBus::EventResult(color, m_nodeGroupId, &NodeGroupRequests::GetGroupColor);

        OnBackgroundColorChanged(color);

        AZStd::vector< NodeId > groupedElements;
        NodeGroupRequestBus::Event(m_nodeGroupId, &NodeGroupRequests::FindGroupedElements, groupedElements);

        AZStd::vector< NodeId > elementsToManage;
        elementsToManage.reserve(groupedElements.size());

        AZStd::vector< NodeId > elementsToSearch = groupedElements;

        while (!elementsToSearch.empty())
        {
            AZ::EntityId searchedElement = elementsToSearch.front();
            elementsToSearch.erase(elementsToSearch.begin());

            if (GraphUtils::IsNodeGroup(searchedElement))
            {
                QGraphicsItem* graphicsItem = nullptr;
                SceneMemberUIRequestBus::EventResult(graphicsItem, searchedElement, &SceneMemberUIRequests::GetRootGraphicsItem);

                if (graphicsItem->isVisible())
                {
                    elementsToManage.emplace_back(searchedElement);

                    AZStd::vector<NodeId> subGroupedElements;
                    NodeGroupRequestBus::Event(searchedElement, &NodeGroupRequests::FindGroupedElements, subGroupedElements);

                    if (!subGroupedElements.empty())
                    {
                        elementsToSearch.insert(elementsToSearch.end(), subGroupedElements.begin(), subGroupedElements.end());
                        elementsToManage.reserve(elementsToManage.size() + subGroupedElements.size());
                    }
                }
            }
            else
            {
                elementsToManage.emplace_back(searchedElement);
            }
        }

        SubGraphParsingConfig config;
        config.m_ignoredGraphMembers.insert(GetEntityId());
        config.m_createNonConnectableSubGraph = true;

        m_containedSubGraphs = GraphUtils::ParseSceneMembersIntoSubGraphs(elementsToManage, config);
        
        ConstructGrouping(graphId);

        SetupGroupPosition(graphId);

        CommentNotificationBus::Handler::BusConnect(m_nodeGroupId);

        bool isLoading = false;
        SceneRequestBus::EventResult(isLoading, graphId, &SceneRequests::IsLoading);

        bool isPasting = false;
        SceneRequestBus::EventResult(isPasting, graphId, &SceneRequests::IsPasting);

        GroupableSceneMemberNotificationBus::Handler::BusConnect(GetEntityId());

        if (!isLoading && !isPasting)
        {
            CreateOccluder(graphId, m_nodeGroupId);

            // Node won't be the correct size right away, need to wait for Qt to tick an update.
            TriggerCollapseAnimation();
        }
    }
    
    void CollapsedNodeGroupComponent::OnRemovedFromScene(const GraphId& graphId)
    {        
        if (m_effectId.IsValid())
        {
            SceneRequestBus::Event(graphId, &SceneRequests::CancelGraphicsEffect, m_effectId);
            m_effectId.SetInvalid();
        }

        if (m_deleteObjects)
        {
            GraphModelRequestBus::Event(graphId, &GraphModelRequests::RequestPushPreventUndoStateUpdate);

            SceneRequestBus::Event(graphId, &SceneRequests::Delete, m_containedSubGraphs.m_nonConnectableGraph.m_containedNodes);

            for (const GraphSubGraph& subGraph : m_containedSubGraphs.m_subGraphs)
            {
                SceneRequestBus::Event(graphId, &SceneRequests::Delete, subGraph.m_containedNodes);
            }

            AZStd::unordered_set< AZ::EntityId > deletionIds = { m_nodeGroupId };
            SceneRequestBus::Event(graphId, &SceneRequests::Delete, deletionIds);

            GraphModelRequestBus::Event(graphId, &GraphModelRequests::RequestPopPreventUndoStateUpdate);
        }

        SceneNotificationBus::Handler::BusDisconnect();
        AZ::SystemTickBus::Handler::BusDisconnect();
    }

    void CollapsedNodeGroupComponent::OnBoundsChanged()
    {
        if (AZ::SystemTickBus::Handler::BusIsConnected())
        {
            AZ::EntityId graphId;
            SceneMemberRequestBus::EventResult(graphId, GetEntityId(), &SceneMemberRequests::GetScene);

            SetupGroupPosition(graphId);

            if (m_animationDelayCounter != 0)
            {
                m_animationDelayCounter = k_qtFrameDelay;
            }
        }
    }

    void CollapsedNodeGroupComponent::OnPositionChanged(const AZ::EntityId& /*targetEntity*/, const AZ::Vector2& position)
    {
        if (!m_ignorePositionChanges.GetState())
        {
            MoveGroupedElementsBy(position - m_previousPosition);
            m_previousPosition = position;
        }
        else
        {
            m_positionDirty = true;
        }
    }

    void CollapsedNodeGroupComponent::OnSceneMemberDragBegin()
    {
        m_memberDraggedStateSetter.SetState(true);
    }

    void CollapsedNodeGroupComponent::OnSceneMemberDragComplete()
    {
        m_memberDraggedStateSetter.ReleaseState();

        // This is a quick implementation of this. Really this shouldn't be necessary as I can just
        // calculate the offset when the group is broken and just apply the changes then.
        // 
        // But for simplicity now, I'll do this the quick way and just update everything after each move.
        if (m_positionDirty)
        {
            m_positionDirty = false;

            AZ::Vector2 position;
            GeometryRequestBus::EventResult(position, GetEntityId(), &GeometryRequests::GetPosition);

            MoveGroupedElementsBy(position - m_previousPosition);
            m_previousPosition = position;
        }
    }

    void CollapsedNodeGroupComponent::OnCommentChanged(const AZStd::string& comment)
    {
        NodeTitleRequestBus::Event(GetEntityId(), &NodeTitleRequests::SetTitle, comment);
        NodeUIRequestBus::Event(GetEntityId(), &NodeUIRequests::AdjustSize);
    }

    void CollapsedNodeGroupComponent::OnBackgroundColorChanged(const AZ::Color& color)
    {
        QColor titleColor = ConversionUtils::AZToQColor(color);

        NodeTitleRequestBus::Event(GetEntityId(), &NodeTitleRequests::SetColorPaletteOverride, titleColor);
    }

    bool CollapsedNodeGroupComponent::OnMouseDoubleClick(const QGraphicsSceneMouseEvent* /*mouseEvent*/)
    {
        ExpandGroup();
        return true;
    }

    void CollapsedNodeGroupComponent::ExpandGroup()
    {
        GraphId graphId;
        SceneMemberRequestBus::EventResult(graphId, GetEntityId(), &SceneMemberRequests::GetScene);

        ReverseGrouping(graphId);
    }

    AZ::EntityId CollapsedNodeGroupComponent::GetSourceGroup() const
    {
        return m_nodeGroupId;
    }

    AZStd::vector< Endpoint > CollapsedNodeGroupComponent::GetRedirectedEndpoints() const
    {
        AZStd::vector< Endpoint > redirectedEndpoints;

        for (const auto& redirectionConfigurations : m_redirections)
        {
            AZStd::unordered_set< Endpoint > remappedEndpoints = GraphUtils::RemapEndpointForModel(redirectionConfigurations.m_targetEndpoint);

            redirectedEndpoints.insert(redirectedEndpoints.begin(), remappedEndpoints.begin(), remappedEndpoints.end());
        }

        return redirectedEndpoints;
    }

    void CollapsedNodeGroupComponent::ForceEndpointRedirection(const AZStd::vector<Endpoint>& endpoints)
    {
        m_forcedRedirections.insert(endpoints.begin(), endpoints.end());
    }

    void CollapsedNodeGroupComponent::OnGroupChanged()
    {
        AZ::EntityId groupId;
        GroupableSceneMemberRequestBus::EventResult(groupId, GetEntityId(), &GroupableSceneMemberRequests::GetGroupId);

        if (groupId.IsValid())
        {
            NodeGroupRequestBus::Event(groupId, &NodeGroupRequests::AddElementToGroup, m_nodeGroupId);
        }
        else
        {
            GroupableSceneMemberRequestBus::Event(m_nodeGroupId, &GroupableSceneMemberRequests::RemoveFromGroup);
        }
    }

    void CollapsedNodeGroupComponent::OnSceneMemberHidden()
    {
        m_memberHiddenStateSetter.SetState(true);
    }

    void CollapsedNodeGroupComponent::OnSceneMemberShown()
    {
        m_memberHiddenStateSetter.ReleaseState();
    }

    void CollapsedNodeGroupComponent::SetupGroupPosition(const GraphId& /*graphId*/)
    {
        StateSetter<bool> ignorePositionSetter;
        ignorePositionSetter.AddStateController(&m_ignorePositionChanges);
        ignorePositionSetter.SetState(true);

        QPointF centerPoint;

        QGraphicsItem* blockItem = nullptr;
        SceneMemberUIRequestBus::EventResult(blockItem, m_nodeGroupId, &SceneMemberUIRequests::GetRootGraphicsItem);

        if (blockItem)
        {
            centerPoint = blockItem->sceneBoundingRect().center();
        }

        // Want to adjust the position of the node to make it a little more centered.
        // But the scene component will re-position it to the passed in location
        // before it attempts this part(and the node needs a frame to adjust it's sizing to be correct)
        //
        // So, fire off a single shot timer and hope we never create/delete this thing.
        QGraphicsItem* graphicsItem = nullptr;
        SceneMemberUIRequestBus::EventResult(graphicsItem, GetEntityId(), &SceneMemberUIRequests::GetRootGraphicsItem);

        QRectF boundingRect = graphicsItem->sceneBoundingRect();

        qreal width = boundingRect.width();
        qreal height = boundingRect.height();

        // Want the Collapsed Node Group to appear centered over top of the Node Group.
        AZ::Vector2 offset(aznumeric_cast<float>(width * 0.5f), aznumeric_cast<float>(height * 0.5f));

        m_previousPosition = ConversionUtils::QPointToVector(centerPoint);
        m_previousPosition -= offset;
        graphicsItem->setPos(ConversionUtils::AZToQPoint(m_previousPosition));

        GeometryRequestBus::Event(GetEntityId(), &GeometryRequests::SetPosition, m_previousPosition);

        // Reget the position we set since we might have snapped to a grid.
        GeometryRequestBus::EventResult(m_previousPosition, GetEntityId(), &GeometryRequests::GetPosition);
        
        m_positionDirty = false;
    }

    void CollapsedNodeGroupComponent::CreateOccluder(const GraphId& graphId, const AZ::EntityId& initialElement)
    {
        if (m_effectId.IsValid())
        {
            GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::CancelGraphicsEffect, m_effectId);
        }

        QGraphicsItem* graphicsItem = nullptr;
        VisualRequestBus::EventResult(graphicsItem, initialElement, &VisualRequests::AsGraphicsItem);

        if (graphicsItem == nullptr)
        {
            return;
        }

        AZ::Color groupColor;
        NodeGroupRequestBus::EventResult(groupColor, m_nodeGroupId, &NodeGroupRequests::GetGroupColor);

        OccluderConfiguration configuration;

        configuration.m_renderColor = ConversionUtils::AZToQColor(groupColor);
        configuration.m_bounds = graphicsItem->sceneBoundingRect();

        configuration.m_zValue = LayerUtils::AlwaysOnTopZValue();

        SceneRequestBus::EventResult(m_effectId, graphId, &SceneRequests::CreateOccluder, configuration);
    }

    void CollapsedNodeGroupComponent::AnimateOccluder(bool isExpanding)
    {
        m_unhideOnAnimationComplete = isExpanding;

        AZ::EntityId graphId;
        SceneMemberRequestBus::EventResult(graphId, GetEntityId(), &SceneMemberRequests::GetScene);

        if (!m_effectId.IsValid())
        {
            if (isExpanding)
            {
                CreateOccluder(graphId, GetEntityId());
            }
            else
            {
                CreateOccluder(graphId, m_nodeGroupId);
            }
        }

        if (!m_effectId.IsValid())
        {
            OnAnimationFinished();
        }

        QGraphicsItem* blockItem = nullptr;
        VisualRequestBus::EventResult(blockItem, m_nodeGroupId, &VisualRequests::AsGraphicsItem);

        if (blockItem == nullptr)
        {
            OnAnimationFinished();
            return;
        }

        QGraphicsItem* graphicsItem = nullptr;
        SceneMemberUIRequestBus::EventResult(graphicsItem, GetEntityId(), &SceneMemberUIRequests::GetRootGraphicsItem);

        if (graphicsItem == nullptr)
        {
            OnAnimationFinished();
            return;
        }

        QGraphicsItem* occluderItem = nullptr;
        GraphicsEffectRequestBus::EventResult(occluderItem, m_effectId, &GraphicsEffectRequests::AsQGraphicsItem);

        if (occluderItem)
        {
            QRectF startRect = occluderItem->sceneBoundingRect();
            QRectF targetRect;

            if (isExpanding)
            {
                targetRect = blockItem->sceneBoundingRect();
            }
            else
            {
                targetRect = graphicsItem->sceneBoundingRect();
            }

            QGraphicsObject* occluderObject = static_cast<QGraphicsObject*>(occluderItem);

            m_sizeAnimation->setTargetObject(occluderObject);
            m_sizeAnimation->setStartValue(startRect.size());
            m_sizeAnimation->setEndValue(targetRect.size());

            m_positionAnimation->setTargetObject(occluderObject);
            m_positionAnimation->setStartValue(startRect.topLeft());
            m_positionAnimation->setEndValue(targetRect.topLeft());

            m_opacityAnimation->setTargetObject(occluderObject);

            m_occluderAnimation.start();
        }
    }

    void CollapsedNodeGroupComponent::ConstructGrouping(const GraphId& graphId)
    {
        bool isLoading = false;
        SceneRequestBus::EventResult(isLoading, graphId, &SceneRequests::IsLoading);

        bool isPasting = false;
        SceneRequestBus::EventResult(isPasting, graphId, &SceneRequests::IsPasting);

        // Keeps track of a mapping from the raw slot Id to the corresponding slot endpoint that we created.
        AZStd::unordered_map< SlotId, SlotId > internalSlotMappings;

        OrderedEndpointSet sourceEndpointOrdering;
        AZStd::unordered_multimap< Endpoint, ConnectionId > sourceEndpointRemapping;

        OrderedEndpointSet targetEndpointOrdering;
        AZStd::unordered_multimap< Endpoint, ConnectionId > targetEndpointRemapping;

        for (const NodeId& nodeId : m_containedSubGraphs.m_nonConnectableGraph.m_containedNodes)
        {
            SceneRequestBus::Event(graphId, &SceneRequests::Hide, nodeId);
        }

        for (const Endpoint& forcedEndpoint : m_forcedRedirections)
        {
            ConnectionType connectionType = ConnectionType::CT_Invalid;
            SlotRequestBus::EventResult(connectionType, forcedEndpoint.GetSlotId(), &SlotRequests::GetConnectionType);

            if (connectionType == ConnectionType::CT_Input)
            {
                targetEndpointOrdering.insert(EndpointOrderingStruct::ConstructOrderingInformation(forcedEndpoint));
            }
            else if (connectionType == ConnectionType::CT_Output)
            {
                sourceEndpointOrdering.insert(EndpointOrderingStruct::ConstructOrderingInformation(forcedEndpoint));
            }
        }

        for (const GraphSubGraph& subGraph : m_containedSubGraphs.m_subGraphs)
        {
            for (const ConnectionId& connectionId : subGraph.m_entryConnections)
            {
                Endpoint targetEndpoint;
                ConnectionRequestBus::EventResult(targetEndpoint, connectionId, &ConnectionRequests::GetTargetEndpoint);

                targetEndpointOrdering.insert(EndpointOrderingStruct::ConstructOrderingInformation(targetEndpoint));
                targetEndpointRemapping.insert(AZStd::make_pair(targetEndpoint, connectionId));
            }

            for (const ConnectionId& connectionId : subGraph.m_exitConnections)
            {
                Endpoint sourceEndpoint;
                ConnectionRequestBus::EventResult(sourceEndpoint, connectionId, &ConnectionRequests::GetSourceEndpoint);

                sourceEndpointOrdering.insert(EndpointOrderingStruct::ConstructOrderingInformation(sourceEndpoint));
                sourceEndpointRemapping.insert(AZStd::make_pair(sourceEndpoint, connectionId));
            }

            for (const NodeId& nodeId : subGraph.m_containedNodes)
            {
                SceneRequestBus::Event(graphId, &SceneRequests::Hide, nodeId);
            }

            for (const ConnectionId& connectionId : subGraph.m_innerConnections)
            {
                SceneRequestBus::Event(graphId, &SceneRequests::Hide, connectionId);
            }
        }

        for (const EndpointOrderingStruct& targetEndpointStruct : targetEndpointOrdering)
        {
            auto slotIter = internalSlotMappings.find(targetEndpointStruct.m_endpoint.GetSlotId());

            if (slotIter == internalSlotMappings.end())
            {
                SlotId redirectionSlotId = CreateSlotRedirection(graphId, targetEndpointStruct.m_endpoint);

                auto insertIter = internalSlotMappings.insert(AZStd::make_pair(targetEndpointStruct.m_endpoint.GetSlotId(), redirectionSlotId));

                if (insertIter.second)
                {
                    SlotRequestBus::Event(redirectionSlotId, &SlotRequests::RemapSlotForModel, targetEndpointStruct.m_endpoint);
                    slotIter = insertIter.first;
                }
            }

            if (slotIter == internalSlotMappings.end())
            {
                continue;
            }

            Endpoint mappedTargetEndpoint = Endpoint(GetEntityId(), slotIter->second);
            auto iterPair = targetEndpointRemapping.equal_range(targetEndpointStruct.m_endpoint);

            for (auto mapIter = iterPair.first; mapIter != iterPair.second; ++mapIter)
            {
                if (isLoading || isPasting)
                {
                    ConnectionRequestBus::Event(mapIter->second, &ConnectionRequests::SnapTargetDisplayTo, mappedTargetEndpoint);
                }
                else
                {
                    ConnectionRequestBus::Event(mapIter->second, &ConnectionRequests::AnimateTargetDisplayTo, mappedTargetEndpoint, k_endpointanimationTimeSec);
                }
            }
        }

        for (const EndpointOrderingStruct& sourceEndpointStruct : sourceEndpointOrdering)
        {
            auto slotIter = internalSlotMappings.find(sourceEndpointStruct.m_endpoint.GetSlotId());

            if (slotIter == internalSlotMappings.end())
            {
                SlotId redirectionSlotId = CreateSlotRedirection(graphId, sourceEndpointStruct.m_endpoint);

                auto insertIter = internalSlotMappings.insert(AZStd::make_pair(sourceEndpointStruct.m_endpoint.GetSlotId(), redirectionSlotId));

                if (insertIter.second)
                {
                    SlotRequestBus::Event(redirectionSlotId, &SlotRequests::RemapSlotForModel, sourceEndpointStruct.m_endpoint);
                    slotIter = insertIter.first;
                }
            }

            if (slotIter == internalSlotMappings.end())
            {
                continue;
            }

            Endpoint mappedSourceEndpoint = Endpoint(GetEntityId(), slotIter->second);
            auto iterPair = sourceEndpointRemapping.equal_range(sourceEndpointStruct.m_endpoint);

            for (auto mapIter = iterPair.first; mapIter != iterPair.second; ++mapIter)
            {
                if (isLoading || isPasting)
                {
                    ConnectionRequestBus::Event(mapIter->second, &ConnectionRequests::SnapSourceDisplayTo, mappedSourceEndpoint);
                }
                else
                {
                    ConnectionRequestBus::Event(mapIter->second, &ConnectionRequests::AnimateSourceDisplayTo, mappedSourceEndpoint, k_endpointanimationTimeSec);
                }
            }
        }

        SceneRequestBus::Event(graphId, &SceneRequests::Hide, m_nodeGroupId);
    }

    void CollapsedNodeGroupComponent::ReverseGrouping(const GraphId& graphId)
    {
        AZStd::vector< SlotId > slotIds;
        NodeRequestBus::EventResult(slotIds, GetEntityId(), &NodeRequests::GetSlotIds);

        for (const SlotId& slotId : slotIds)
        {
            ConnectionType connectionType = CT_Invalid;
            SlotRequestBus::EventResult(connectionType, slotId, &SlotRequests::GetConnectionType);

            if (connectionType != CT_Invalid && connectionType != CT_None)
            {
                AZStd::vector< Endpoint > redirectedEndpoints;
                SlotRequestBus::EventResult(redirectedEndpoints, slotId, &SlotRequests::GetRemappedModelEndpoints);

                AZ_Assert(redirectedEndpoints.size() == 1, "A single slot being redirected to multiple slots is not currently supported.");

                if (redirectedEndpoints.size() == 0)
                {
                    continue;
                }

                Endpoint redirectedEndpoint = redirectedEndpoints.front();

                AZStd::vector< ConnectionId > connectionIds;
                SlotRequestBus::EventResult(connectionIds, slotId, &SlotRequests::GetConnections);

                for (const ConnectionId& connectionId : connectionIds)
                {
                    if (connectionType == CT_Input)
                    {
                        ConnectionRequestBus::Event(connectionId, &ConnectionRequests::AnimateTargetDisplayTo, redirectedEndpoint, k_endpointanimationTimeSec);
                    }
                    else if (connectionType == CT_Output)
                    {
                        ConnectionRequestBus::Event(connectionId, &ConnectionRequests::AnimateSourceDisplayTo, redirectedEndpoint, k_endpointanimationTimeSec);
                    }
                }
            }
        }

        CreateOccluder(graphId, GetEntityId());
        TriggerExpandAnimation();

        SceneRequestBus::Event(graphId, &SceneRequests::Hide, GetEntityId());
    }

    void CollapsedNodeGroupComponent::TriggerExpandAnimation()
    {
        m_animationDelayCounter = k_qtFrameDelay;
        m_isExpandingOccluderAnimation = true;
        VisualRequestBus::Event(GetEntityId(), &VisualRequests::SetVisible, false);
        UpdateSystemTickBus();
    }

    void CollapsedNodeGroupComponent::TriggerCollapseAnimation()
    {
        m_animationDelayCounter = k_qtFrameDelay;
        m_isExpandingOccluderAnimation = false;
        VisualRequestBus::Event(GetEntityId(), &VisualRequests::SetVisible, false);
        UpdateSystemTickBus();
    }

    void CollapsedNodeGroupComponent::MoveGroupedElementsBy(const AZ::Vector2& offset)
    {
        GraphId graphId;
        SceneMemberRequestBus::EventResult(graphId, GetEntityId(), &SceneMemberRequests::GetScene);

        GraphModelRequestBus::Event(graphId, &GraphModelRequests::RequestPushPreventUndoStateUpdate);

        // Update the NodeGroup
        {
            AZ::Vector2 position;
            GeometryRequestBus::EventResult(position, m_nodeGroupId, &GeometryRequests::GetPosition);

            position += offset;

            // TODO: Potentially fix the collapsed node groups
            GeometryRequestBus::Event(m_nodeGroupId, &GeometryRequests::SetPosition, position);
        }

        GraphModelRequestBus::Event(graphId, &GraphModelRequests::RequestPopPreventUndoStateUpdate);
    }

    void CollapsedNodeGroupComponent::MoveSubGraphBy(const GraphSubGraph& subGraph, const AZ::Vector2& offset)
    {
        for (const NodeId& nodeId : subGraph.m_containedNodes)
        {
            AZ::Vector2 position;
            GeometryRequestBus::EventResult(position, nodeId, &GeometryRequests::GetPosition);

            position += offset;

            GeometryRequestBus::Event(nodeId, &GeometryRequests::SetPosition, position);
        }
    }

    void CollapsedNodeGroupComponent::OnAnimationFinished()
    {
        GraphId graphId;
        SceneMemberRequestBus::EventResult(graphId, GetEntityId(), &SceneMemberRequests::GetScene);

        if (m_unhideOnAnimationComplete)
        {
            {
                ScopedGraphUndoBlocker undoBlocker(graphId);
                SceneRequestBus::Event(graphId, &SceneRequests::Show, m_nodeGroupId);

                for (const NodeId& nodeId : m_containedSubGraphs.m_nonConnectableGraph.m_containedNodes)
                {
                    SceneRequestBus::Event(graphId, &SceneRequests::Show, nodeId);
                }

                for (const GraphSubGraph& subGraph : m_containedSubGraphs.m_subGraphs)
                {
                    for (const NodeId& nodeId : subGraph.m_containedNodes)
                    {
                        SceneRequestBus::Event(graphId, &SceneRequests::Show, nodeId);
                    }

                    for (const ConnectionId& connectionId : subGraph.m_innerConnections)
                    {
                        SceneRequestBus::Event(graphId, &SceneRequests::Show, connectionId);
                    }
                }

                AZ::EntityId groupId;
                GroupableSceneMemberRequestBus::EventResult(groupId, m_nodeGroupId, &GroupableSceneMemberRequests::GetGroupId);

                if (groupId.IsValid())
                {
                    const bool growOnly = true;
                    NodeGroupRequestBus::Event(groupId, &NodeGroupRequests::ResizeGroupToElements, growOnly);
                }

                m_deleteObjects = false;

                GroupableSceneMemberNotificationBus::Handler::BusDisconnect();

                // Want to delay removing the occluder, because the wrapper nodes sometime deform slightly and need a tick to visually
                // update.
                m_occluderDestructionCounter = k_qtFrameDelay;
                UpdateSystemTickBus();
            }

            GraphModelRequestBus::Event(graphId, &GraphModelRequests::RequestUndoPoint);
        }
        else
        {
            if (m_effectId.IsValid())
            {
                SceneRequestBus::Event(graphId, &SceneRequests::CancelGraphicsEffect, m_effectId);
                m_effectId.SetInvalid();
            }

            VisualRequestBus::Event(GetEntityId(), &VisualRequests::SetVisible, true);
            SceneMemberUIRequestBus::Event(GetEntityId(), &SceneMemberUIRequests::SetSelected, true);

            GraphUtils::SanityCheckEnabledState(GetEntityId());
        }
    }

    SlotId CollapsedNodeGroupComponent::CreateSlotRedirection(const GraphId& /*graphId*/, const Endpoint& endpoint)
    {
        m_redirections.emplace_back();
        SlotRedirectionConfiguration& configuration = m_redirections.back();

        configuration.m_targetEndpoint = endpoint;

        return InitializeRedirectionSlot(configuration);
    }

    SlotId CollapsedNodeGroupComponent::InitializeRedirectionSlot(const SlotRedirectionConfiguration& configuration)
    {
        SlotId retVal;

        SlotConfiguration* cloneConfiguration = nullptr;
        SlotRequestBus::EventResult(cloneConfiguration, configuration.m_targetEndpoint.GetSlotId(), &SlotRequests::CloneSlotConfiguration);

        if (cloneConfiguration)
        {
            if (!configuration.m_name.empty())
            {
                cloneConfiguration->m_name.Clear();
                cloneConfiguration->m_name.SetFallback(configuration.m_name);
            }
            else
            {
                AZStd::string nodeTitle;
                NodeTitleRequestBus::EventResult(nodeTitle, configuration.m_targetEndpoint.GetNodeId(), &NodeTitleRequests::GetTitle);

                AZStd::string displayName = AZStd::string::format("%s:%s", nodeTitle.c_str(), cloneConfiguration->m_name.GetDisplayString().c_str());

                // Gain some context. Lost the ability to refresh the strings.
                // Should be fixable once we get an actual use case for this setup.
                cloneConfiguration->m_name.Clear();
                cloneConfiguration->m_name.SetFallback(displayName);
            }

            AZ::Entity* slotEntity = nullptr;
            GraphCanvasRequestBus::BroadcastResult(slotEntity, &GraphCanvasRequests::CreateSlot, GetEntityId(), (*cloneConfiguration));

            if (slotEntity)
            {
                slotEntity->Init();
                slotEntity->Activate();

                GraphCanvas::NodeRequestBus::Event(GetEntityId(), &GraphCanvas::NodeRequests::AddSlot, slotEntity->GetId());

                retVal = slotEntity->GetId();
            }

            delete cloneConfiguration;
        }

        Endpoint redirectedSlot(GetEntityId(), retVal);

        if (redirectedSlot.IsValid())
        {
            m_redirectedSlotWatcher.RegisterEndpoint(configuration.m_targetEndpoint, redirectedSlot);
        }

        return retVal;
    }

    void CollapsedNodeGroupComponent::UpdateSystemTickBus()
    {
        if (m_animationDelayCounter > 0 || m_occluderDestructionCounter > 0)
        {
            AZ::SystemTickBus::Handler::BusConnect();
        }
        else
        {
            AZ::SystemTickBus::Handler::BusDisconnect();
        }
    }
}
