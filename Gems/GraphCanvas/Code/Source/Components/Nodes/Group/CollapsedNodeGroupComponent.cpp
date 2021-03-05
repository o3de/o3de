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
#include "precompiled.h"

#include <QGraphicsWidget>
#include <QTimer>
#include <QSequentialAnimationGroup>

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Components/Nodes/Group/CollapsedNodeGroupComponent.h>

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

    ////////////////////////////////
    // CollapsedNodeGroupComponent
    ////////////////////////////////

    constexpr int k_collapsingAnimationTimeMS = 175;
    constexpr int k_fadeInTimeMS = 50;

    // 0.9f found through the scientific process of it looking right.
    constexpr float k_endpointanimationTimeSec = (k_collapsingAnimationTimeMS/1000.0f) * 0.9f;
    
    void CollapsedNodeGroupComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        
        if (serializeContext)
        {
            serializeContext->Class<SlotRedirectionConfiguration>()
                ->Version(1)
                ->Field("Name", &SlotRedirectionConfiguration::m_name)
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
    }
    
    void CollapsedNodeGroupComponent::Activate()
    {
        GraphCanvasPropertyComponent::Activate();
        
        CollapsedNodeGroupRequestBus::Handler::BusConnect(GetEntityId());
        VisualNotificationBus::Handler::BusConnect(GetEntityId());
        NodeNotificationBus::Handler::BusConnect(GetEntityId());
        SceneMemberNotificationBus::Handler::BusConnect(GetEntityId());
        GeometryNotificationBus::Handler::BusConnect(GetEntityId());
    }
    
    void CollapsedNodeGroupComponent::Deactivate()
    {
        GraphCanvasPropertyComponent::Deactivate();

        CommentNotificationBus::Handler::BusDisconnect();
        GeometryNotificationBus::Handler::BusDisconnect();
        SceneMemberNotificationBus::Handler::BusDisconnect();
        NodeNotificationBus::Handler::BusDisconnect();
        VisualNotificationBus::Handler::BusDisconnect();
        CollapsedNodeGroupRequestBus::Handler::BusDisconnect();
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

        SubGraphParsingConfig config;
        config.m_ignoredGraphMembers.insert(GetEntityId());
        config.m_createNonConnectableSubGraph = true;

        m_containedSubGraphs = GraphUtils::ParseSceneMembersIntoSubGraphs(groupedElements, config);
        
        ConstructGrouping(graphId);

        // Want to adjust the position of the node to make it a little more centered.
        // But the scene component will re-position it to the passed in location
        // before it attempts this part(and the node needs a frame to adjust it's sizing to be correct)
        //
        // So, fire off a single shot timer and hope we never create/delete this thing.
        QGraphicsItem* graphicsItem = nullptr;
        SceneMemberUIRequestBus::EventResult(graphicsItem, GetEntityId(), &SceneMemberUIRequests::GetRootGraphicsItem);

        NodeUIRequestBus::Event(GetEntityId(), &NodeUIRequests::AdjustSize);
        
        QRectF boundingRect = graphicsItem->sceneBoundingRect();

        qreal width = boundingRect.width();
        qreal height = boundingRect.height();

        // Want the Collapsed Node Group to appear centered over top of the Node Group.
        AZ::Vector2 offset(aznumeric_cast<float>(width * 0.5f), aznumeric_cast<float>(height * 0.5f));

        m_previousPosition = ConversionUtils::QPointToVector(boundingRect.topLeft());;
        m_previousPosition -= offset;
        graphicsItem->setPos(ConversionUtils::AZToQPoint(m_previousPosition));

        GeometryRequestBus::Event(GetEntityId(), &GeometryRequests::SetPosition, m_previousPosition);

        // Reget the position we set since we might have snapped to a grid.
        GeometryRequestBus::EventResult(m_previousPosition, GetEntityId(), &GeometryRequests::GetPosition);

        m_ignorePositionChanges = false;
        m_positionDirty = false;

        CommentNotificationBus::Handler::BusConnect(m_nodeGroupId);        

        bool isLoading = false;
        SceneRequestBus::EventResult(isLoading, graphId, &SceneRequests::IsLoading);

        bool isPasting = false;
        SceneRequestBus::EventResult(isPasting, graphId, &SceneRequests::IsPasting);

        if (!isLoading && !isPasting)
        {
            const bool isExpanding = false;
            CreateOccluder(graphId, isExpanding);
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
    }

    void CollapsedNodeGroupComponent::OnPositionChanged([[maybe_unused]] const AZ::EntityId& targetEntity, const AZ::Vector2& position)
    {
        if (!m_ignorePositionChanges)
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
        m_ignorePositionChanges = true;
    }

    void CollapsedNodeGroupComponent::OnSceneMemberDragComplete()
    {
        m_ignorePositionChanges = false;

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

    bool CollapsedNodeGroupComponent::OnMouseDoubleClick([[maybe_unused]] const QGraphicsSceneMouseEvent* mouseEvent)
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

    void CollapsedNodeGroupComponent::CreateOccluder(const GraphId& graphId, bool isExpanding)
    {
        m_unhideOnAnimationComplete = isExpanding;

        QGraphicsItem* blockItem = nullptr;
        VisualRequestBus::EventResult(blockItem, m_nodeGroupId, &VisualRequests::AsGraphicsItem);

        if (blockItem == nullptr)
        {
            OnAnimationFinished();
            return;
        }

        QGraphicsItem* graphicsItem = nullptr;
        NodeUIRequestBus::Event(GetEntityId(), &NodeUIRequests::AdjustSize);
        SceneMemberUIRequestBus::EventResult(graphicsItem, GetEntityId(), &SceneMemberUIRequests::GetRootGraphicsItem);

        if (graphicsItem == nullptr)
        {
            OnAnimationFinished();
            return;
        }

        AZ::Color groupColor;
        NodeGroupRequestBus::EventResult(groupColor, m_nodeGroupId, &NodeGroupRequests::GetGroupColor);

        OccluderConfiguration configuration;

        configuration.m_renderColor = ConversionUtils::AZToQColor(groupColor);        

        if (isExpanding)
        {
            configuration.m_bounds = graphicsItem->sceneBoundingRect();
        }
        else
        {
            configuration.m_bounds = blockItem->sceneBoundingRect();
        }

        configuration.m_zValue = aznumeric_cast<int>(graphicsItem->zValue() + 10);

        SceneRequestBus::EventResult(m_effectId, graphId, &SceneRequests::CreateOccluder, configuration);

        QGraphicsItem* occluderItem = nullptr;
        GraphicsEffectRequestBus::EventResult(occluderItem, m_effectId, &GraphicsEffectRequests::AsQGraphicsItem);

        if (occluderItem)
        {
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
            m_sizeAnimation->setStartValue(configuration.m_bounds.size());
            m_sizeAnimation->setEndValue(targetRect.size());

            m_positionAnimation->setTargetObject(occluderObject);
            m_positionAnimation->setStartValue(configuration.m_bounds.topLeft());
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

        const bool isExpanding = true;
        CreateOccluder(graphId, isExpanding);

        SceneRequestBus::Event(graphId, &SceneRequests::Hide, GetEntityId());
    }

    void CollapsedNodeGroupComponent::MoveGroupedElementsBy(const AZ::Vector2& offset)
    {
        GraphId graphId;
        SceneMemberRequestBus::EventResult(graphId, GetEntityId(), &SceneMemberRequests::GetScene);

        GraphModelRequestBus::Event(graphId, &GraphModelRequests::RequestPushPreventUndoStateUpdate);

        MoveSubGraphBy(m_containedSubGraphs.m_nonConnectableGraph, offset);

        for (const GraphSubGraph& subGraph : m_containedSubGraphs.m_subGraphs)
        {
            MoveSubGraphBy(subGraph, offset);
        }

        // Update the NodeGroup
        {
            AZ::Vector2 position;
            GeometryRequestBus::EventResult(position, m_nodeGroupId, &GeometryRequests::GetPosition);

            position += offset;

            StateSetter<bool> externallyControlledStateSetter;

            StateController<bool>* stateController = nullptr;
            NodeGroupRequestBus::EventResult(stateController, m_nodeGroupId, &NodeGroupRequests::GetExternallyControlledStateController);

            externallyControlledStateSetter.AddStateController(stateController);
            externallyControlledStateSetter.SetState(true);

            GeometryRequestBus::Event(m_nodeGroupId, &GeometryRequests::SetPosition, position);
        }

        GraphModelRequestBus::Event(graphId, &GraphModelRequests::RequestPopPreventUndoStateUpdate);
    }

    void CollapsedNodeGroupComponent::MoveSubGraphBy(const GraphSubGraph& subGraph, const AZ::Vector2& offset)
    {
        for (const NodeId& nodeId : subGraph.m_containedNodes)
        {
            bool lockedForExternalMovement = false;
            SceneMemberRequestBus::EventResult(lockedForExternalMovement, nodeId, &SceneMemberRequests::LockForExternalMovement, GetEntityId());

            if (lockedForExternalMovement)
            {
                AZ::Vector2 position;
                GeometryRequestBus::EventResult(position, nodeId, &GeometryRequests::GetPosition);

                position += offset;

                GeometryRequestBus::Event(nodeId, &GeometryRequests::SetPosition, position);
                SceneMemberRequestBus::Event(nodeId, &SceneMemberRequests::UnlockForExternalMovement, GetEntityId());
            }
        }
    }

    void CollapsedNodeGroupComponent::OnAnimationFinished()
    {
        GraphId graphId;
        SceneMemberRequestBus::EventResult(graphId, GetEntityId(), &SceneMemberRequests::GetScene);

        if (m_effectId.IsValid())
        {
            SceneRequestBus::Event(graphId, &SceneRequests::CancelGraphicsEffect, m_effectId);
            m_effectId.SetInvalid();
        }

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

                m_deleteObjects = false;

                AZStd::unordered_set<NodeId> deleteIds = { GetEntityId() };
                SceneRequestBus::Event(graphId, &SceneRequests::Delete, deleteIds);
            }

            GraphModelRequestBus::Event(graphId, &GraphModelRequests::RequestUndoPoint);
        }
    }

    SlotId CollapsedNodeGroupComponent::CreateSlotRedirection([[maybe_unused]] const GraphId& graphId, const Endpoint& endpoint)
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

        return retVal;
    }
}