/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/sort.h>

#include <GraphCanvas/Utils/GraphUtils.h>

#include <GraphCanvas/Components/Bookmarks/BookmarkBus.h>
#include <GraphCanvas/Components/Nodes/Comment/CommentBus.h>
#include <GraphCanvas/Components/Connections/ConnectionBus.h>
#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/Nodes/Group/NodeGroupBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Nodes/NodeUIBus.h>
#include <GraphCanvas/Components/Nodes/Wrapper/WrapperNodeBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Components/VisualBus.h>

#include <GraphCanvas/Editor/GraphCanvasProfiler.h>
#include <GraphCanvas/Editor/GraphModelBus.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/Utils/ConversionUtils.h>
#include <GraphCanvas/Utils/QtVectorMath.h>

namespace GraphCanvas
{
    AZ::Vector2 CalculateAlignmentAnchorPoint(const AlignConfig& alignConfig)
    {
        AZ::Vector2 anchorPoint(0.0f, 0.0f);

        switch (alignConfig.m_horAlign)
        {
        case GraphUtils::HorizontalAlignment::Left:
            anchorPoint.SetX(0.0f);
            break;
        case GraphUtils::HorizontalAlignment::Center:
            anchorPoint.SetX(0.5f);
            break;
        case GraphUtils::HorizontalAlignment::Right:
            anchorPoint.SetX(1.0f);
            break;
        default:
            break;
        }

        switch (alignConfig.m_verAlign)
        {
        case GraphUtils::VerticalAlignment::Top:
            anchorPoint.SetY(0.0f);
            break;
        case GraphUtils::VerticalAlignment::Middle:
            anchorPoint.SetY(0.5f);
            break;
        case GraphUtils::VerticalAlignment::Bottom:
            anchorPoint.SetY(1.0f);
            break;
        default:
            break;
        }

        return anchorPoint;
    }

    void SanitizeMovementDirection(QPointF& movement, const AlignConfig& alignConfig)
    {
        if (alignConfig.m_horAlign == GraphUtils::HorizontalAlignment::None)
        {
            movement.setX(0.0f);
        }

        if (alignConfig.m_verAlign == GraphUtils::VerticalAlignment::None)
        {
            movement.setY(0.0f);
        }
    }

    // Some helper structs that just make some bookkeeping saner
    // Hiding it in here to keep the header somewhat clean

    //////////////////////////////////
    // OrganizationHelper Structures
    //////////////////////////////////
    struct FloatingElementAnchor
    {
        AZ::EntityId m_elementId;
        QPointF  m_offset;
    };

    class OrganizationHelper
    {
    public:
        struct ConnectionStruct
        {
            ConnectionStruct(SlotId slotId, ConnectionId connectionId)
                : m_slotId(slotId)
                , m_connectionId(connectionId)
            {
            }

            SlotId m_slotId;
            ConnectionId m_connectionId;
        };

        AZ_CLASS_ALLOCATOR(OrganizationHelper, AZ::SystemAllocator);
        OrganizationHelper() = default;

        OrganizationHelper(const NodeId& nodeId, const AlignConfig& alignConfig, const QRectF& overallBoundingRect)
            : m_nodeId(nodeId)
            , m_triggeredNodes(NodeOrderingStruct::Comparator(overallBoundingRect, AlignConfig(GraphUtils::VerticalAlignment::Top, GraphUtils::HorizontalAlignment::Left)))
            , m_alignTime(alignConfig.m_alignTime)
        {
            m_alignmentPoint = CalculateAlignmentAnchorPoint(alignConfig);

            QGraphicsItem* graphicsItem = nullptr;
            SceneMemberUIRequestBus::EventResult(graphicsItem, nodeId, &SceneMemberUIRequests::GetRootGraphicsItem);

            if (graphicsItem)
            {
                m_boundingArea = graphicsItem->sceneBoundingRect();

                m_anchorPoint.setX(m_boundingArea.left() + m_boundingArea.width() * m_alignmentPoint.GetX());
                m_anchorPoint.setY(m_boundingArea.top() + m_boundingArea.height() * m_alignmentPoint.GetY());
            }
            else
            {
                m_anchorPoint = QPointF(0, 0);
                m_boundingArea = QRectF(0, 0, 100, 100);
            }

            m_finalPosition = m_boundingArea.topLeft();
        }

        NodeOrderingStruct GetOrderingStruct() const
        {
            return NodeOrderingStruct(m_nodeId, m_alignmentPoint);
        }

        void MoveHelperBy(const QPointF& offset)
        {
            m_finalPosition += offset;
            m_boundingArea.moveTopLeft(m_boundingArea.topLeft() + offset);

            for (OrganizationHelper* helper : m_finalizedElements)
            {
                helper->MoveHelperBy(offset);
            }
        }

        void MoveToFinalPosition(bool animate = true)
        {
            if (animate)
            {
                RootGraphicsItemRequestBus::Event(m_nodeId, &RootGraphicsItemRequests::AnimatePositionTo, m_finalPosition, m_alignTime);
            }
            else
            {
                GeometryRequestBus::Event(m_nodeId, &GeometryRequests::SetPosition, AZ::Vector2(aznumeric_cast<float>(m_finalPosition.x()), aznumeric_cast<float>(m_finalPosition.y())));
            }
        }

        void TriggeredElement(SlotId slotId, ConnectionId connectionId, OrganizationHelper* helper)
        {
            bool inserted = m_incitedElements.insert(helper).second;

            if (inserted)
            {
                [[maybe_unused]] size_t removedCount = m_finalizedElements.erase(helper);
                AZ_Error("GraphCanvas", removedCount == 0, "Inciting an element after it has already been finalized.");

                m_triggeredNodes.insert(helper->GetOrderingStruct());
                helper->m_incitingElements.insert(this);

                m_slotConnections.insert(AZStd::make_pair(helper->m_nodeId, ConnectionStruct(slotId, connectionId)));
            }
        }

        void OnElementFinalized(OrganizationHelper* helper)
        {
            size_t removedCount = m_incitedElements.erase(helper);

            if (removedCount > 0)
            {
                m_finalizedElements.insert(helper);
            }
        }

        bool IsReadyToFinalize() const
        {
            return m_incitedElements.empty();
        }

        NodeId m_nodeId;

        AZStd::unordered_set< OrganizationHelper* > m_incitingElements;

        AZStd::unordered_map< NodeId, ConnectionStruct > m_slotConnections;

        AZStd::unordered_set< OrganizationHelper* > m_incitedElements;
        AZStd::unordered_set< OrganizationHelper* > m_finalizedElements;

        AZStd::chrono::milliseconds m_alignTime;

        AZ::Vector2 m_alignmentPoint;
        QPointF m_anchorPoint;

        QPointF m_finalPosition;

        QRectF m_boundingArea;
        OrderedNodeStruct m_triggeredNodes;
    };

    struct OrganizationSpaceAllocationHelper
    {
        void AllocateSpace(OrganizationHelper* helper, int spaceAllocation, int seperator)
        {
            auto iterator = AZStd::find_if(m_subSections.begin(), m_subSections.end(), [helper](OrganizationHelper* testHelper)
            {
                return testHelper == helper;
            });

            if (iterator == m_subSections.end())
            {
                if (!m_subSections.empty())
                {
                    m_space += seperator;
                }

                m_space += spaceAllocation;
                m_subSections.push_back(helper);
            }
        }

        int m_space = 0;
        AZStd::vector< OrganizationHelper* > m_subSections;
    };

    ///////////////////////
    // NodeOrderingStruct
    ///////////////////////

    NodeOrderingStruct::NodeOrderingStruct(const NodeId& nodeId, const AZ::Vector2& anchorPoint)
        : m_nodeId(nodeId)
    {
        QGraphicsItem* graphicsItem = nullptr;
        SceneMemberUIRequestBus::EventResult(graphicsItem, nodeId, &SceneMemberUIRequests::GetRootGraphicsItem);

        if (graphicsItem)
        {
            m_boundingBox = graphicsItem->sceneBoundingRect();

            CalculateAnchorPoint(anchorPoint);
        }
    }

    NodeOrderingStruct::NodeOrderingStruct(const NodeId& nodeId, const AZ::Vector2& anchorPoint, const QRectF& boundingRect)
        : m_nodeId(nodeId)
        , m_boundingBox(boundingRect)
    {
        CalculateAnchorPoint(anchorPoint);
    }

    void NodeOrderingStruct::CalculateAnchorPoint(const AZ::Vector2& anchorPoint)
    {
        float widthPercent = AZ::GetClamp(anchorPoint.GetX(), 0.0f, 1.0f);
        float heightPercent = AZ::GetClamp(anchorPoint.GetY(), 0.0f, 1.0f);

        m_anchorPoint.setX(m_boundingBox.left() + m_boundingBox.width() * widthPercent);
        m_anchorPoint.setY(m_boundingBox.top() + m_boundingBox.height() * heightPercent);
    }

    ///////////////////////////////////
    // NodeOrderingStruct::Comparator
    ///////////////////////////////////

    NodeOrderingStruct::Comparator::Comparator(const QRectF& boundingRect, const AlignConfig& alignConfig)
        : m_alignConfig(alignConfig)
    {
        AZ::Vector2 anchorPoint = CalculateAlignmentAnchorPoint(alignConfig);

        m_targetPoint.setX(boundingRect.left() + boundingRect.width() * anchorPoint.GetX());
        m_targetPoint.setY(boundingRect.top() + boundingRect.height() * anchorPoint.GetY());
    }

    bool NodeOrderingStruct::Comparator::operator()(const NodeOrderingStruct& lhs, const NodeOrderingStruct& rhs) const
    {
        QPointF leftDifference = m_targetPoint - lhs.m_anchorPoint;
        QPointF sanitizedLeftDifference = leftDifference;

        QPointF rightDifference = m_targetPoint - rhs.m_anchorPoint;
        QPointF sanitizedRightDifference = rightDifference;

        SanitizeMovementDirection(sanitizedLeftDifference, m_alignConfig);
        SanitizeMovementDirection(sanitizedRightDifference, m_alignConfig);

        float leftLength = QtVectorMath::GetLength(sanitizedLeftDifference);
        float rightLength = QtVectorMath::GetLength(sanitizedRightDifference);

        if (AZ::IsClose(leftLength, rightLength, 0.1f))
        {
            leftLength = QtVectorMath::GetLength(leftDifference);
            rightLength = QtVectorMath::GetLength(rightDifference);

            if (AZ::IsClose(leftLength, rightLength, 0.1f))
            {
                // If they are directly on top of each other. 
                // Pick one based on node id, since at this point the difference is arbitrary.
                return lhs.m_nodeId < rhs.m_nodeId;
            }
        }

        return leftLength < rightLength;
    }

    ///////////////////////////
    // SubGraphOrderingStruct
    ///////////////////////////

    SubGraphOrderingStruct::SubGraphOrderingStruct()
        : m_subGraph(nullptr)
        , m_orderedNodes(NodeOrderingStruct::Comparator(QRectF(), AlignConfig()))
    {
    }

    SubGraphOrderingStruct::SubGraphOrderingStruct(const QRectF& overallBoundingRect, const GraphSubGraph& subGraph, const AlignConfig& alignConfig)
        : m_subGraph(&subGraph)
        , m_orderedNodes(NodeOrderingStruct::Comparator(overallBoundingRect, alignConfig))
    {
        AZ::Vector2 anchorPoint = CalculateAlignmentAnchorPoint(alignConfig);

        int counter = 0;

        for (const NodeId& nodeId : m_subGraph->m_containedNodes)
        {
            auto insertResult = m_orderedNodes.emplace(nodeId, anchorPoint);

            if (insertResult.second)
            {
                const NodeOrderingStruct& nodeStruct = (*insertResult.first);

                if (counter == 0)
                {
                    m_averagePoint = nodeStruct.m_anchorPoint;
                }
                else
                {
                    m_averagePoint += nodeStruct.m_anchorPoint;
                }

                ++counter;

                if (m_graphBoundingRect.isEmpty())
                {
                    m_graphBoundingRect = nodeStruct.m_boundingBox;
                }
                else
                {
                    m_graphBoundingRect &= nodeStruct.m_boundingBox;
                }
            }
        }

        if (counter != 0)
        {
            m_averagePoint /= counter;
        }

        m_anchorPoint.setX(m_graphBoundingRect.left() + m_graphBoundingRect.width() * anchorPoint.GetX());
        m_anchorPoint.setY(m_graphBoundingRect.top() + m_graphBoundingRect.height() * anchorPoint.GetY());
    }

    ///////////////////////////////////////
    // SubGraphOrderingStruct::Comparator
    ///////////////////////////////////////

    SubGraphOrderingStruct::Comparator::Comparator(const QRectF& overallBoundingRect, const AlignConfig& alignConfig)
        : m_alignConfig(alignConfig)
    {
        AZ::Vector2 anchorPoint = CalculateAlignmentAnchorPoint(alignConfig);

        m_targetPoint.setX(overallBoundingRect.left() + overallBoundingRect.width() * anchorPoint.GetX());
        m_targetPoint.setY(overallBoundingRect.top() + overallBoundingRect.height() * anchorPoint.GetY());
    }

    bool SubGraphOrderingStruct::Comparator::operator()(const SubGraphOrderingStruct& lhs, const SubGraphOrderingStruct& rhs) const
    {
        QPointF leftDifference = m_targetPoint - lhs.m_anchorPoint;
        QPointF rightDifference = m_targetPoint - rhs.m_anchorPoint;

        SanitizeMovementDirection(leftDifference, m_alignConfig);
        SanitizeMovementDirection(rightDifference, m_alignConfig);

        float leftDifferenceLength = QtVectorMath::GetLength(leftDifference);
        float rightDifferenceLength = QtVectorMath::GetLength(rightDifference);

        if (AZ::IsClose(leftDifferenceLength, rightDifferenceLength, 0.01f))
        {
            AZStd::hash<const GraphSubGraph*> hasher;
            return hasher(lhs.m_subGraph) < hasher(rhs.m_subGraph);
        }
        else
        {
            return leftDifferenceLength < rightDifferenceLength;
        }
    }

    ///////////////////////////
    // EndpointOrderingStruct
    ///////////////////////////
    EndpointOrderingStruct EndpointOrderingStruct::ConstructOrderingInformation(const Endpoint& endpoint)
    {
        EndpointOrderingStruct orderingStruct;

        orderingStruct.m_endpoint = endpoint;
        SlotRequestBus::EventResult(orderingStruct.m_slotDisplayOrdering, endpoint.GetSlotId(), &SlotRequests::GetDisplayOrdering);
        SlotRequestBus::EventResult(orderingStruct.m_connectionType, endpoint.GetSlotId(), &SlotRequests::GetConnectionType);

        SlotGroup slotGroup;
        SlotRequestBus::EventResult(slotGroup, endpoint.GetSlotId(), &SlotRequests::GetSlotGroup);
        SlotLayoutRequestBus::EventResult(orderingStruct.m_displayGroupOrdering, endpoint.GetNodeId(), &SlotLayoutRequests::GetSlotGroupDisplayOrder, slotGroup);

        SlotUIRequestBus::EventResult(orderingStruct.m_position, endpoint.GetSlotId(), &SlotUIRequests::GetConnectionPoint);

        return orderingStruct;
    }

    EndpointOrderingStruct EndpointOrderingStruct::ConstructOrderingInformation(const SlotId& slotId)
    {
        NodeId nodeId;
        SlotRequestBus::EventResult(nodeId, slotId, &SlotRequests::GetNode);

        return ConstructOrderingInformation(Endpoint(nodeId, slotId));
    }

    bool EndpointOrderingStruct::Comparator::operator()(const EndpointOrderingStruct& lhs, const EndpointOrderingStruct& rhs)
    {
        if (lhs.m_endpoint.GetNodeId() != rhs.m_endpoint.GetNodeId())
        {
            // Imparting a top to bottom, left to right bias on this for now.
            //
            // If this ever changes we will need to find a better way to drive this.
            if (AZ::IsClose(lhs.m_position.y(), rhs.m_position.y(), 0.001))
            {
                return lhs.m_position.x() < rhs.m_position.x();
            }
            else
            {
                return lhs.m_position.y() < rhs.m_position.y();
            }
        }
        else
        {
            if (lhs.m_displayGroupOrdering == rhs.m_displayGroupOrdering)
            {
                if (lhs.m_slotDisplayOrdering == rhs.m_slotDisplayOrdering)
                {
                    return lhs.m_connectionType < rhs.m_connectionType;
                }
                else
                {
                    return lhs.m_slotDisplayOrdering < rhs.m_slotDisplayOrdering;
                }
            }
            else
            {
                return lhs.m_displayGroupOrdering < rhs.m_displayGroupOrdering;
            }
        }
    }

    ///////////////////////////
    // ScopedGraphUndoBlocker
    ///////////////////////////

    ScopedGraphUndoBlocker::ScopedGraphUndoBlocker(const GraphId& graphId)
        : m_graphId(graphId)
    {
        GraphModelRequestBus::Event(m_graphId, &GraphModelRequests::RequestPushPreventUndoStateUpdate);
    }

    ScopedGraphUndoBlocker::~ScopedGraphUndoBlocker()
    {
        GraphModelRequestBus::Event(m_graphId, &GraphModelRequests::RequestPopPreventUndoStateUpdate);
    }

    ///////////////////////////
    // ScopedGraphUndoBatch
    ///////////////////////////

    ScopedGraphUndoBatch::ScopedGraphUndoBatch(const GraphId& graphId)
        : m_graphId(graphId)
    {
        GraphModelRequestBus::Event(m_graphId, &GraphModelRequests::RequestPushPreventUndoStateUpdate);
    }

    ScopedGraphUndoBatch::~ScopedGraphUndoBatch()
    {
        GraphModelRequestBus::Event(m_graphId, &GraphModelRequests::RequestPopPreventUndoStateUpdate);
        GraphModelRequestBus::Event(m_graphId, &GraphModelRequests::RequestUndoPoint);
    }

    ///////////////////////////
    // NodeFocusCyclingHelper
    ///////////////////////////

    NodeFocusCyclingHelper::NodeFocusCyclingHelper()
        : m_comparator(QRect(-20000, -20000, 40000, 40000), AlignConfig(GraphUtils::VerticalAlignment::Top, GraphUtils::HorizontalAlignment::Left))
    {
    }

    bool NodeFocusCyclingHelper::IsConfigured() const
    {
        return !m_sourceNodes.empty();
    }

    void NodeFocusCyclingHelper::Clear()
    {
        m_cycleOffset = 0;

        m_sourceNodes.clear();
        m_sortedNodes.clear();
    }

    void NodeFocusCyclingHelper::SetActiveGraph(const GraphId& graphId)
    {
        m_graphId = graphId;

        m_viewId.SetInvalid();
        SceneRequestBus::EventResult(m_viewId, m_graphId, &SceneRequests::GetViewId);
    }

    void NodeFocusCyclingHelper::SetNodes(const AZStd::vector<NodeId>& nodes)
    {
        m_cycleOffset = -1;
        m_sortedNodes.clear();

        m_sourceNodes = nodes;
    }

    void NodeFocusCyclingHelper::CycleToNextNode()
    {
        ParseNodes();

        if (m_sortedNodes.empty())
        {
            return;
        }

        m_cycleOffset++;

        if (m_cycleOffset >= m_sortedNodes.size())
        {
            m_cycleOffset = 0;
        }

        FocusOnNode(m_sortedNodes[m_cycleOffset].m_nodeId);
    }

    void NodeFocusCyclingHelper::CycleToPreviousNode()
    {
        ParseNodes();

        if (m_sortedNodes.empty())
        {
            return;
        }

        m_cycleOffset--;

        if (m_cycleOffset < 0)
        {
            m_cycleOffset = static_cast<int>(m_sortedNodes.size()) - 1;
        }

        FocusOnNode(m_sortedNodes[m_cycleOffset].m_nodeId);
    }

    void NodeFocusCyclingHelper::ParseNodes()
    {
        if (m_sortedNodes.empty() && !m_sourceNodes.empty())
        {
            m_sortedNodes.reserve(m_sourceNodes.size());

            for (const NodeId& nodeId : m_sourceNodes)
            {
                m_sortedNodes.emplace_back(nodeId, AZ::Vector2(0, 0));
            }

            AZStd::sort(m_sortedNodes.begin(), m_sortedNodes.end(), m_comparator);
        }
    }

    void NodeFocusCyclingHelper::FocusOnNode(const NodeId& nodeId)
    {
        QGraphicsItem* graphicsItem = nullptr;
        SceneMemberUIRequestBus::EventResult(graphicsItem, nodeId, &SceneMemberUIRequests::GetRootGraphicsItem);

        if (graphicsItem)
        {
            QRectF boundingBox = graphicsItem->sceneBoundingRect();

            QPointF centerPoint = boundingBox.center();

            ViewRequestBus::Event(m_viewId, &ViewRequests::PanSceneTo, centerPoint, AZStd::chrono::milliseconds(250));
        }
    }

    //////////////////
    // GraphSubGraph
    //////////////////

    GraphSubGraph::GraphSubGraph(bool isNonConnectable)
        : m_isNonConnectable(isNonConnectable)
    {

    }

    GraphSubGraph::GraphSubGraph(const NodeId& sourceNode, AZStd::unordered_set< NodeId >& internalSceneMembers)
        : GraphSubGraph(false)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::unordered_set< NodeId > searchableEntities;
        searchableEntities.insert(sourceNode);

        while (!searchableEntities.empty())
        {
            NodeId currentEntity = (*searchableEntities.begin());

            searchableEntities.erase(currentEntity);
            internalSceneMembers.erase(currentEntity);

            if (!GraphUtils::IsConnectableNode(currentEntity))
            {
                continue;
            }

            QGraphicsItem* item = nullptr;
            SceneMemberUIRequestBus::EventResult(item, currentEntity, &SceneMemberUIRequests::GetRootGraphicsItem);

            if (item)
            {
                m_sceneBoundingRect |= item->sceneBoundingRect();
            }

            m_containedNodes.insert(currentEntity);

            AZStd::vector<SlotId> slotIds;
            NodeRequestBus::EventResult(slotIds, currentEntity, &NodeRequests::GetSlotIds);

            bool hasInternalOutput = false;
            bool isExit = false;
            bool hasOutputSlot = false;

            bool hasInternalInput = false;
            bool isEntrance = false;
            bool hasInputSlot = false;

            for (const SlotId& testSlot : slotIds)
            {
                bool hasConnection = false;
                SlotRequestBus::EventResult(hasConnection, testSlot, &SlotRequests::HasConnections);

                ConnectionType connectionType = ConnectionType::CT_Invalid;
                SlotRequestBus::EventResult(connectionType, testSlot, &SlotRequests::GetConnectionType);

                switch (connectionType)
                {
                case ConnectionType::CT_Input:
                    isEntrance = true;
                    hasInputSlot = true;
                    break;
                case ConnectionType::CT_Output:
                    isExit = true;
                    hasOutputSlot = true;
                    break;
                default:
                    break;
                }

                if (hasConnection)
                {
                    AZStd::vector< ConnectionId > connectionIds;
                    SlotRequestBus::EventResult(connectionIds, testSlot, &SlotRequests::GetConnections);

                    for (const ConnectionId& connectionId : connectionIds)
                    {
                        m_containedConnections.insert(connectionId);

                        NodeId expansionNode;

                        if (connectionType == ConnectionType::CT_Input)
                        {
                            ConnectionRequestBus::EventResult(expansionNode, connectionId, &ConnectionRequests::GetSourceNodeId);
                        }
                        else if (connectionType == ConnectionType::CT_Output)
                        {
                            ConnectionRequestBus::EventResult(expansionNode, connectionId, &ConnectionRequests::GetTargetNodeId);
                        }

                        if (expansionNode.IsValid())
                        {
                            if (IsInternalNode(expansionNode, internalSceneMembers))
                            {
                                m_innerConnections.insert(connectionId);

                                switch (connectionType)
                                {
                                case ConnectionType::CT_Input:
                                    hasInternalInput = true;
                                    break;
                                case ConnectionType::CT_Output:
                                    hasInternalOutput = true;
                                    break;
                                default:
                                    break;
                                }
                            }
                            else
                            {
                                if (connectionType == ConnectionType::CT_Input)
                                {
                                    m_entryConnections.insert(connectionId);
                                }
                                else if (connectionType == ConnectionType::CT_Output)
                                {
                                    m_exitConnections.insert(connectionId);
                                }
                            }

                            AZ::EntityId outermostNode = GraphUtils::FindOutermostNode(expansionNode);

                            if (m_containedNodes.find(outermostNode) == m_containedNodes.end()
                                && internalSceneMembers.find(outermostNode) != internalSceneMembers.end())
                            {
                                searchableEntities.insert(outermostNode);
                            }
                        }
                    }
                }
            }
            // </Slots>

            // Need to process all of our wrapped elements
            // to avoid trying to snap to connections coming from things we may wrap.
            if (GraphUtils::IsWrapperNode(currentEntity))
            {
                AZStd::vector< NodeId > wrappedNodes;
                WrapperNodeRequestBus::EventResult(wrappedNodes, currentEntity, &WrapperNodeRequests::GetWrappedNodeIds);

                while (!wrappedNodes.empty())
                {
                    NodeId wrappedNodeId = wrappedNodes.back();
                    wrappedNodes.pop_back();

                    internalSceneMembers.erase(wrappedNodeId);

                    if (m_containedNodes.count(wrappedNodeId) == 0)
                    {
                        searchableEntities.insert(wrappedNodeId);
                    }

                    if (GraphUtils::IsWrapperNode(wrappedNodeId))
                    {
                        AZStd::vector< NodeId > internalNodes;
                        WrapperNodeRequestBus::EventResult(internalNodes, wrappedNodeId, &WrapperNodeRequests::GetWrappedNodeIds);

                        for (NodeId nodeId : internalNodes)
                        {
                            wrappedNodes.push_back(nodeId);
                        }
                    }
                }
            }
            else if (GraphUtils::IsNode(currentEntity))
            {
                NodeId wrapperNode;
                NodeRequestBus::EventResult(wrapperNode, currentEntity, &NodeRequests::GetWrappingNode);

                if (wrapperNode.IsValid())
                {
                    if (m_containedNodes.count(wrapperNode) == 0)
                    {
                        searchableEntities.insert(wrapperNode);
                    }
                }
            }

            bool isLeafNode = false;

            // If we do not have an input slot, treat that as an entry point to the graph
            if (!hasInputSlot || (isEntrance && !hasInternalInput))
            {
                isLeafNode = true;
                m_entryNodes.insert(currentEntity);
            }

            // If we do not have an output slot, treat that as an exit point to the sub-graph
            if (!hasOutputSlot || (isExit && !hasInternalOutput))
            {
                isLeafNode = true;
                m_exitNodes.insert(currentEntity);
            }

            if (!isLeafNode)
            {
                m_innerNodes.insert(currentEntity);
            }
        } // </Breadth First Search>
    }

    void GraphSubGraph::Clear()
    {
        m_containedNodes.clear();
        m_containedConnections.clear();

        m_innerNodes.clear();
        m_innerConnections.clear();

        m_entryNodes.clear();
        m_entryConnections.clear();

        m_exitNodes.clear();
        m_exitConnections.clear();
    }

    bool GraphSubGraph::IsNonConnectableSubGraph() const
    {
        return m_isNonConnectable;
    }

    bool GraphSubGraph::IsInternalNode(NodeId currentEntity, const AZStd::unordered_set< NodeId >& searchableSceneMembers) const
    {
        bool isInternal = false;

        while (!isInternal && currentEntity.IsValid())
        {
            if (m_containedNodes.count(currentEntity) > 0
                || searchableSceneMembers.count(currentEntity) > 0)
            {
                isInternal = true;
            }

            NodeRequestBus::EventResult(currentEntity, currentEntity, &NodeRequests::GetWrappingNode);
        }

        return isInternal;
    }

    ///////////////
    // GraphUtils
    ///////////////

    bool GraphUtils::IsConnectableNode(const NodeId& entityId)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        return NodeRequestBus::FindFirstHandler(entityId) != nullptr && CommentRequestBus::FindFirstHandler(entityId) == nullptr;
    }

    bool GraphUtils::IsNodeOrWrapperSelected(const NodeId& nodeId)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        bool isSelected = false;

        NodeId currentNode = nodeId;

        do
        {
            SceneMemberUIRequestBus::EventResult(isSelected, currentNode, &SceneMemberUIRequests::IsSelected);

            bool isWrapped = false;
            NodeRequestBus::EventResult(isWrapped, currentNode, &NodeRequests::IsWrapped);

            if (isWrapped)
            {
                NodeRequestBus::EventResult(currentNode, currentNode, &NodeRequests::GetWrappingNode);
            }
            else
            {
                break;
            }

        } while (!isSelected);

        return isSelected;
    }

    bool GraphUtils::IsSpliceableConnection(const ConnectionId& connectionId)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        if (IsConnection(connectionId))
        {
            AZ::EntityId testId;
            ConnectionRequestBus::EventResult(testId, connectionId, &ConnectionRequests::GetSourceNodeId);

            bool isSelected = false;
            SceneMemberUIRequestBus::EventResult(isSelected, testId, &SceneMemberUIRequests::IsSelected);

            if (isSelected)
            {
                return false;
            }

            isSelected = false;

            testId.SetInvalid();
            ConnectionRequestBus::EventResult(testId, connectionId, &ConnectionRequests::GetTargetNodeId);
            SceneMemberUIRequestBus::EventResult(isSelected, testId, &SceneMemberUIRequests::IsSelected);

            if (isSelected)
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        return true;
    }

    bool GraphUtils::IsConnection(const AZ::EntityId& graphMemberId)
    {
        return (ConnectionRequestBus::FindFirstHandler(graphMemberId) != nullptr);
    }

    bool GraphUtils::IsNode(const AZ::EntityId& graphMemberId)
    {
        return (NodeRequestBus::FindFirstHandler(graphMemberId) != nullptr)
            && !GraphUtils::IsComment(graphMemberId)
            && !GraphUtils::IsNodeGroup(graphMemberId);
    }

    bool GraphUtils::IsNodeWrapped(const NodeId& nodeId)
    {
        bool isWrapped = false;
        NodeRequestBus::EventResult(isWrapped, nodeId, &NodeRequests::IsWrapped);

        return isWrapped;
    }

    bool GraphUtils::IsWrapperNode(const AZ::EntityId& graphMemberId)
    {
        return (WrapperNodeRequestBus::FindFirstHandler(graphMemberId) != nullptr);
    }

    bool GraphUtils::IsSlot(const AZ::EntityId& graphMemberId)
    {
        return (SlotRequestBus::FindFirstHandler(graphMemberId) != nullptr);
    }

    bool GraphUtils::IsGroupableElement(const AZ::EntityId& graphMemberId)
    {
        return (GroupableSceneMemberRequestBus::FindFirstHandler(graphMemberId) != nullptr);
    }

    bool GraphUtils::IsSlotVisible(const SlotId& slotId)
    {
        SlotGroup slotGroup = SlotGroups::Invalid;
        SlotRequestBus::EventResult(slotGroup, slotId, &SlotRequests::GetSlotGroup);

        NodeId nodeId;
        SlotRequestBus::EventResult(nodeId, slotId, &SlotRequests::GetNode);

        bool isGroupVisible = false;
        SlotLayoutRequestBus::EventResult(isGroupVisible, nodeId, &SlotLayoutRequests::IsSlotGroupVisible, slotGroup);

        bool isSlotVisible = false;
        VisualRequestBus::EventResult(isSlotVisible, slotId, &VisualRequests::IsVisible);

        return isSlotVisible && (slotGroup != SlotGroups::Invalid && isGroupVisible);
    }

    bool GraphUtils::IsSlotHideable(const SlotId& slotId)
    {
        bool canHideSlot = false;

        bool isExecutionSlot = IsSlotType(slotId, SlotTypes::ExecutionSlot);
        bool isDataSlot = IsSlotType(slotId, SlotTypes::DataSlot);

        if (IsSlotVisible(slotId)
            && (isExecutionSlot || isDataSlot))
        {
            if (isDataSlot)
            {
                DataSlotType dataSlotType = DataSlotType::Unknown;

                DataSlotRequestBus::EventResult(dataSlotType, slotId, &DataSlotRequests::GetDataSlotType);

                if (dataSlotType == DataSlotType::Reference)
                {
                    return false;
                }
            }

            bool isConnected = false;

            SlotRequestBus::EventResult(isConnected, slotId, &SlotRequests::HasConnections);

            if (!isConnected)
            {
                canHideSlot = true;
            }
        }

        return canHideSlot;
    }

    bool GraphUtils::IsSlotConnectionType(const SlotId& slotId, ConnectionType connectionType)
    {
        ConnectionType testType = ConnectionType::CT_Invalid;
        SlotRequestBus::EventResult(testType, slotId, &SlotRequests::GetConnectionType);

        return testType == connectionType;
    }

    bool GraphUtils::IsSlotType(const SlotId& slotId, SlotType slotType)
    {
        SlotType testType = SlotTypes::Invalid;
        SlotRequestBus::EventResult(testType, slotId, &SlotRequests::GetSlotType);

        return testType == slotType;
    }

    bool GraphUtils::IsSlot(const SlotId& slotId, SlotType slotType, ConnectionType connectionType)
    {
        return IsSlotType(slotId, slotType) && IsSlotConnectionType(slotId, connectionType);
    }

    bool GraphUtils::IsNodeGroup(const AZ::EntityId& graphMemberId)
    {
        return (NodeGroupRequestBus::FindFirstHandler(graphMemberId) != nullptr);
    }

    bool GraphUtils::IsCollapsedNodeGroup(const AZ::EntityId& graphMemberId)
    {
        return (CollapsedNodeGroupRequestBus::FindFirstHandler(graphMemberId) != nullptr);
    }

    bool GraphUtils::IsComment(const AZ::EntityId& graphMemberId)
    {
        return (CommentRequestBus::FindFirstHandler(graphMemberId) != nullptr) && !IsNodeGroup(graphMemberId);
    }

    bool GraphUtils::IsBookmarkAnchor(const AZ::EntityId& graphMemberId)
    {
        return BookmarkRequestBus::FindFirstHandler(graphMemberId) != nullptr
            && !GraphUtils::IsNodeGroup(graphMemberId);
    }

    AZ::EntityId GraphUtils::CreateGroupForElements(const AZ::EntityId& graphId, const AZStd::vector<AZ::EntityId>& memberIds, AZ::Vector2 scenePoint)
    {
        AZ::Entity* nodeGroupEntity = nullptr;
        GraphCanvasRequestBus::BroadcastResult(nodeGroupEntity, &GraphCanvasRequests::CreateNodeGroupAndActivate);

        if (nodeGroupEntity)
        {
            SceneRequestBus::Event(graphId, &SceneRequests::AddNode, nodeGroupEntity->GetId(), scenePoint, false);

            ResizeGroupToElements(nodeGroupEntity->GetId(), memberIds);

            return nodeGroupEntity->GetId();
        }

        return AZ::EntityId();
    }

    void GraphUtils::ResizeGroupToElements(const AZ::EntityId& groupId, const AZStd::vector<AZ::EntityId>& memberIds)
    {
        if (memberIds.empty())
        {
            return;
        }

        AZ::EntityId graphId;
        SceneMemberRequestBus::EventResult(graphId, groupId, &SceneMemberRequests::GetScene);

        QGraphicsItem* rootItem = nullptr;
        QRectF boundingArea;

        for (const AZ::EntityId& selectedNode : memberIds)
        {
            SceneMemberUIRequestBus::EventResult(rootItem, selectedNode, &SceneMemberUIRequests::GetRootGraphicsItem);

            if (rootItem)
            {
                if (boundingArea.isEmpty())
                {
                    boundingArea = rootItem->sceneBoundingRect();
                }
                else
                {
                    boundingArea = boundingArea.united(rootItem->sceneBoundingRect());
                }
            }
        }

        AZ::Vector2 gridStep = FindMinorStep(graphId);
        boundingArea.adjust(-gridStep.GetX(), -gridStep.GetY(), gridStep.GetX(), gridStep.GetY());

        NodeGroupRequestBus::Event(groupId, &NodeGroupRequests::SetGroupSize, boundingArea);
    }

    AZ::Vector2 GraphUtils::FindMinorStep(const AZ::EntityId& graphId)
    {
        AZ::EntityId gridId;
        SceneRequestBus::EventResult(gridId, graphId, &SceneRequests::GetGrid);

        AZ::Vector2 minorStep(0, 0);
        GridRequestBus::EventResult(minorStep, gridId, &GridRequests::GetMinorPitch);

        return minorStep;
    }

    AZ::Vector2 GraphUtils::FindMajorStep(const AZ::EntityId& graphId)
    {
        AZ::EntityId gridId;
        SceneRequestBus::EventResult(gridId, graphId, &SceneRequests::GetGrid);

        AZ::Vector2 majorStep(0, 0);
        GridRequestBus::EventResult(majorStep, gridId, &GridRequests::GetMajorPitch);

        return majorStep;
    }

    NodeId GraphUtils::FindOutermostNode(const AZ::EntityId& graphMemberId)
    {
        GraphCanvas::NodeId outermostNode = graphMemberId;
        bool isWrapped = IsNodeWrapped(outermostNode);

        while (isWrapped)
        {
            NodeRequestBus::EventResult(outermostNode, outermostNode, &GraphCanvas::NodeRequests::GetWrappingNode);
            isWrapped = IsNodeWrapped(outermostNode);
        }

        return outermostNode;
    }

    void GraphUtils::DeleteOutermostNode(const GraphId& graphId, const AZ::EntityId& graphMemberId)
    {
        GraphCanvas::NodeId outermostNode = FindOutermostNode(graphMemberId);

        SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::ClearSelection);
        SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::Delete, AZStd::unordered_set<AZ::EntityId>{ outermostNode });
    }

    void GraphUtils::ParseMembersForSerialization(GraphSerialization& graphSerialization, const AZStd::unordered_set< AZ::EntityId >& memberIds)
    {
        GraphData& graphData = graphSerialization.GetGraphData();

        for (const AZ::EntityId& memberId : memberIds)
        {
            AZ::Entity* memberEntity = AzToolsFramework::GetEntity(memberId);

            bool insertedMember = false;

            if (IsBookmarkAnchor(memberId))
            {
                auto insertResult = graphData.m_bookmarkAnchors.insert(memberEntity);
                insertedMember = insertResult.second;
            }
            else if (IsNode(memberId)
                || IsNodeGroup(memberId)
                || IsComment(memberId))
            {
                auto insertResult = graphData.m_nodes.insert(memberEntity);
                insertedMember = insertResult.second;
            }

            if (insertedMember)
            {
                SceneMemberNotificationBus::Event(memberId, &SceneMemberNotifications::OnSceneMemberAboutToSerialize, graphSerialization);
            }
        }

        AZStd::unordered_set< NodeId > nodeIds;

        for (AZ::Entity* nodeEntity : graphData.m_nodes)
        {
            nodeIds.insert(nodeEntity->GetId());
        }

        // This copies only connections among nodes in the copied node set
        bool internalConnectionsOnly = true;
        AZStd::unordered_set< AZ::EntityId > connections = FindConnectionsForNodes(nodeIds, internalConnectionsOnly);
        GraphUtils::ParseConnectionsForSerialization(graphSerialization, connections);
    }

    SubGraphParsingResult GraphUtils::ParseSceneMembersIntoSubGraphs(const AZStd::vector< NodeId >& sourceSceneMembers, const SubGraphParsingConfig& config)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        SubGraphParsingResult result;
        AZStd::unordered_set< NodeId > searchableSceneMembers(sourceSceneMembers.begin(), sourceSceneMembers.end());

        for (const NodeId& ignoreGraphMember : config.m_ignoredGraphMembers)
        {
            searchableSceneMembers.erase(ignoreGraphMember);
        }

        while (!searchableSceneMembers.empty())
        {
            NodeId currentSceneMember = (*searchableSceneMembers.begin());
            searchableSceneMembers.erase(currentSceneMember);

            if (!GraphUtils::IsConnectableNode(currentSceneMember))
            {
                if (config.m_createNonConnectableSubGraph)
                {
                    result.m_nonConnectableGraph.m_innerNodes.insert(currentSceneMember);
                    result.m_nonConnectableGraph.m_containedNodes.insert(currentSceneMember);
                }

                continue;
            }

            // Mostly here for sanity. If something is wrapped, they act as a 'single' node despite being multiple nodes.
            // As such, we want to walk up to our wrapped parrent and just use that.
            //
            // All duplicate entiries will be removed from the searchable scene member list.
            // to avoid double exploring a single wrapped node.
            bool isWrapped = false;

            do
            {
                NodeRequestBus::EventResult(isWrapped, currentSceneMember, &NodeRequests::IsWrapped);

                if (isWrapped)
                {
                    NodeRequestBus::EventResult(currentSceneMember, currentSceneMember, &NodeRequests::GetWrappingNode);
                    searchableSceneMembers.erase(currentSceneMember);
                }
            } while (isWrapped);

            // Constructor for the sub graphs will go through the searchable members
            // and remove out all of the elements that are contained within the specified subgraph.
            result.m_subGraphs.emplace_back(currentSceneMember, searchableSceneMembers);
        }

        return result;
    }

    bool GraphUtils::IsValidModelConnection(const GraphId& graphId, const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint)
    {
        bool validConnection = false;

        AZStd::unordered_set< Endpoint > finalSourceEndpoints = RemapEndpointForModel(sourceEndpoint);
        AZStd::unordered_set< Endpoint > finalTargetEndpoints = RemapEndpointForModel(targetEndpoint);

        for (const Endpoint& modelSourceEndpoint : finalSourceEndpoints)
        {
            for (const Endpoint& modelTargetEndpoint : finalTargetEndpoints)
            {
                GraphModelRequestBus::EventResult(validConnection, graphId, &GraphModelRequests::IsValidConnection, modelSourceEndpoint, modelTargetEndpoint);

                if (!validConnection)
                {
                    break;
                }
            }
        }

        return validConnection;
    }

    ConnectionValidationTooltip GraphUtils::GetModelConnnectionValidityToolTip(const GraphId& graphId, const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint)
    {
        ConnectionValidationTooltip validationTooltip;

        AZStd::unordered_set< Endpoint > finalSourceEndpoints = RemapEndpointForModel(sourceEndpoint);
        AZStd::unordered_set< Endpoint > finalTargetEndpoints = RemapEndpointForModel(targetEndpoint);

        for (const Endpoint& modelSourceEndpoint : finalSourceEndpoints)
        {
            for (const Endpoint& modelTargetEndpoint : finalTargetEndpoints)
            {
                GraphModelRequestBus::EventResult(validationTooltip, graphId, &GraphModelRequests::GetConnectionValidityTooltip, modelSourceEndpoint, modelTargetEndpoint);

                if (!validationTooltip.m_isValid)
                {
                    break;
                }
            }
        }

        return validationTooltip;
    }

    bool GraphUtils::CreateModelConnection(const GraphId& graphId, const ConnectionId& connectionId, const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint)
    {
        bool created = false;

        AZStd::unordered_set< Endpoint > finalSourceEndpoints = RemapEndpointForModel(sourceEndpoint);
        AZStd::unordered_set< Endpoint > finalTargetEndpoints = RemapEndpointForModel(targetEndpoint);

        for (const Endpoint& modelSourceEndpoint : finalSourceEndpoints)
        {
            for (const Endpoint& modelTargetEndpoint : finalTargetEndpoints)
            {
                GraphModelRequestBus::EventResult(created, graphId, &GraphModelRequests::CreateConnection, connectionId, modelSourceEndpoint, modelTargetEndpoint);

                if (!created)
                {
                    break;
                }
            }
        }

        return created;
    }

    AZStd::unordered_set< Endpoint > GraphUtils::RemapEndpointForModel(const Endpoint& endpoint)
    {
        AZStd::unordered_set< Endpoint > retVal;
        AZStd::unordered_set< Endpoint > exploreSet;
        exploreSet.insert(endpoint);

        while (!exploreSet.empty())
        {
            Endpoint currentEndpoint = (*exploreSet.begin());
            exploreSet.erase(exploreSet.begin());

            bool hasRemapping = false;
            SlotRequestBus::EventResult(hasRemapping, currentEndpoint.GetSlotId(), &SlotRequests::HasModelRemapping);

            if (hasRemapping)
            {
                AZStd::vector< Endpoint > endpoints;
                SlotRequestBus::EventResult(endpoints, currentEndpoint.GetSlotId(), &SlotRequests::GetRemappedModelEndpoints);

                for (const Endpoint& e : endpoints)
                {
                    // If we haven't already processed the node, add it to our explore set so we can recurse.
                    if (retVal.count(e) == 0)
                    {
                        exploreSet.insert(e);
                    }
                }
            }
            else
            {
                retVal.insert(currentEndpoint);
            }
        }

        return retVal;
    }

    AZStd::unordered_set<AZ::EntityId> GraphUtils::FindConnectionsForNodes(const AZStd::unordered_set<NodeId>& nodeIds, bool internalConnectionsOnly)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();

        AZStd::unordered_set<AZ::EntityId> foundConnections;

        for (NodeId nodeId : nodeIds)
        {
            // This should really only happen if it's in a copy.
            // This is kinda messy, and I want to improve the node grouping stuff down the road.
            // But for a first pass this will suffice.
            if (GraphUtils::IsNodeGroup(nodeId))
            {
                bool isCollapsed = false;

                NodeGroupRequestBus::EventResult(isCollapsed, nodeId, &NodeGroupRequests::IsCollapsed);

                if (isCollapsed)
                {
                    NodeGroupRequestBus::EventResult(nodeId, nodeId, &NodeGroupRequests::GetCollapsedNodeId);
                }
            }

            AZStd::vector< SlotId > slotIds;
            NodeRequestBus::EventResult(slotIds, nodeId, &NodeRequests::GetSlotIds);

            for (const SlotId& slotId : slotIds)
            {
                AZStd::vector< ConnectionId > connectionIds;
                SlotRequestBus::EventResult(connectionIds, slotId, &SlotRequests::GetConnections);

                for (const ConnectionId& connectionId : connectionIds)
                {
                    if (internalConnectionsOnly)
                    {
                        // Connections might be remapped. So we want to figure
                        // out what connections we are actually doing. And confirm if they are internal
                        // Then allow that connectionId.
                        Endpoint sourceEndpoint;
                        ConnectionRequestBus::EventResult(sourceEndpoint, connectionId, &ConnectionRequests::GetSourceEndpoint);

                        AZStd::unordered_set< Endpoint > sourceEndpoints = GraphUtils::RemapEndpointForModel(sourceEndpoint);

                        Endpoint targetEndpoint;
                        ConnectionRequestBus::EventResult(targetEndpoint, connectionId, &ConnectionRequests::GetTargetEndpoint);

                        AZStd::unordered_set< Endpoint > targetEndpoints = GraphUtils::RemapEndpointForModel(targetEndpoint);

                        bool acceptConnection = false;

                        // Only need to accept one. Since in theory this should all be internalize to a collapsed group.
                        // So a single success should mean everything else is already included.
                        //
                        // And if it wasn't, I have no way of isolating out the connections correctly in this function.
                        for (const Endpoint& modelSourceEndpoint : sourceEndpoints)
                        {
                            for (const Endpoint& modelTargetEndpoint : targetEndpoints)
                            {
                                if (nodeIds.count(modelSourceEndpoint.GetNodeId()) > 0 && nodeIds.count(modelTargetEndpoint.GetNodeId()) > 0)
                                {
                                    acceptConnection = true;
                                    break;
                                }
                            }

                            if (acceptConnection)
                            {
                                break;
                            }
                        }

                        if (acceptConnection)
                        {
                            foundConnections.insert(connectionId);
                        }
                    }
                    else
                    {
                        foundConnections.insert(connectionId);
                    }
                }
            }
        }

        return foundConnections;
    }    

    bool GraphUtils::SpliceNodeOntoConnection(const NodeId& nodeId, const ConnectionId& connectionId, ConnectionSpliceConfig& spliceConfiguration)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        bool allowNode = false;
        if (nodeId.IsValid() && connectionId.IsValid())
        {
            GraphId graphId;
            SceneMemberRequestBus::EventResult(graphId, nodeId, &SceneMemberRequests::GetScene);

            if (!graphId.IsValid())
            {
                return false;
            }

            ScopedGraphUndoBlocker undoBlocker(graphId);

            Endpoint connectionSourceEndpoint;
            ConnectionRequestBus::EventResult(connectionSourceEndpoint, connectionId, &ConnectionRequests::GetSourceEndpoint);

            Endpoint connectionTargetEndpoint;
            ConnectionRequestBus::EventResult(connectionTargetEndpoint, connectionId, &ConnectionRequests::GetTargetEndpoint);

            if (connectionSourceEndpoint.IsValid()
                && connectionTargetEndpoint.IsValid())
            {
                NodeRequestBus::Event(connectionSourceEndpoint.GetNodeId(), &NodeRequests::SignalBatchedConnectionManipulationBegin);
                NodeRequestBus::Event(connectionTargetEndpoint.GetNodeId(), &NodeRequests::SignalBatchedConnectionManipulationBegin);

                // Delete the old connection just in case something prevents new connections while it has one.
                AZStd::unordered_set< AZ::EntityId > deletionIds{ connectionId };
                SceneRequestBus::Event(graphId, &SceneRequests::Delete, deletionIds);

                CreateConnectionsBetweenConfig config;
                config.m_connectionType = CreateConnectionsBetweenConfig::CreationType::SinglePass;

                allowNode = CreateConnectionsBetween({ connectionSourceEndpoint, connectionTargetEndpoint }, nodeId, config);

                if (allowNode)
                {
                    for (ConnectionId createdConnectionId : config.m_createdConnections)
                    {
                        ConnectionEndpoints connectionEndpoints;
                        ConnectionRequestBus::EventResult(connectionEndpoints, createdConnectionId, &ConnectionRequests::GetEndpoints);

                        if (connectionEndpoints.m_sourceEndpoint == connectionSourceEndpoint)
                        {
                            spliceConfiguration.m_splicedTargetEndpoint = connectionEndpoints.m_targetEndpoint;
                        }
                        else if (connectionEndpoints.m_targetEndpoint == connectionTargetEndpoint)
                        {
                            spliceConfiguration.m_splicedSourceEndpoint = connectionEndpoints.m_sourceEndpoint;
                        }
                    }
                }
                // If we failed to make it, restore the previous connection
                else if (!allowNode)
                {
                    SceneRequestBus::Event(graphId, &SceneRequests::CreateConnectionBetween, connectionSourceEndpoint, connectionTargetEndpoint);
                }
            }

            if (allowNode)
            {
                GraphModelRequestBus::Event(graphId, &GraphModelRequests::RequestUndoPoint);

                if (spliceConfiguration.m_allowOpportunisticConnections)
                {
                    AZStd::vector<ConnectionEndpoints> connectedEndpoints;
                    connectedEndpoints.reserve(2);

                    connectedEndpoints.emplace_back(connectionSourceEndpoint, spliceConfiguration.m_splicedTargetEndpoint);
                    connectedEndpoints.emplace_back(spliceConfiguration.m_splicedSourceEndpoint, connectionTargetEndpoint);

                    spliceConfiguration.m_opportunisticSpliceResult = CreateOpportunisticsConnectionsForSplice(graphId, connectedEndpoints, nodeId);
                }
            }

            NodeRequestBus::Event(connectionSourceEndpoint.GetNodeId(), &NodeRequests::SignalBatchedConnectionManipulationEnd);
            NodeRequestBus::Event(connectionTargetEndpoint.GetNodeId(), &NodeRequests::SignalBatchedConnectionManipulationEnd);
        }

        return allowNode;
    }

    bool GraphUtils::SpliceSubGraphOntoConnection(const GraphSubGraph& subGraph, const ConnectionId& connectionId)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        bool handledSplice = false;

        GraphId graphId;
        SceneMemberRequestBus::EventResult(graphId, connectionId, &SceneMemberRequests::GetScene);

        {
            ScopedGraphUndoBlocker undoBlocker(graphId);

            Endpoint sourceEndpoint;
            ConnectionRequestBus::EventResult(sourceEndpoint, connectionId, &ConnectionRequests::GetSourceEndpoint);

            Endpoint targetEndpoint;
            ConnectionRequestBus::EventResult(targetEndpoint, connectionId, &ConnectionRequests::GetTargetEndpoint);

            NodeRequestBus::Event(sourceEndpoint.GetNodeId(), &NodeRequests::SignalBatchedConnectionManipulationBegin);
            NodeRequestBus::Event(targetEndpoint.GetNodeId(), &NodeRequests::SignalBatchedConnectionManipulationBegin);

            AZStd::unordered_set< AZ::EntityId > deletionIds{ connectionId };
            SceneRequestBus::Event(graphId, &SceneRequests::Delete, deletionIds);

            bool createdEntry = false;
            AZStd::unordered_set< AZ::EntityId > createdConnections;

            for (const AZ::EntityId& entryNode : subGraph.m_entryNodes)
            {
                CreateConnectionsBetweenConfig config;
                config.m_connectionType = CreateConnectionsBetweenConfig::CreationType::SingleConnection;
                
                if (CreateConnectionsBetween({ sourceEndpoint }, entryNode, config))
                {
                    createdConnections.insert(config.m_createdConnections.begin(), config.m_createdConnections.end());
                    createdEntry = true;
                }
            }

            bool createdExit = false;

            for (const AZ::EntityId& exitNode : subGraph.m_exitNodes)
            {
                CreateConnectionsBetweenConfig config;
                config.m_connectionType = CreateConnectionsBetweenConfig::CreationType::SingleConnection;

                if (CreateConnectionsBetween({ targetEndpoint }, exitNode, config))
                {
                    createdConnections.insert(config.m_createdConnections.begin(), config.m_createdConnections.end());
                    createdExit = true;
                }
            }

            if (!createdEntry || !createdExit)
            {
                handledSplice = false;
                SceneRequestBus::Event(graphId, &SceneRequests::Delete, createdConnections);

                SceneRequestBus::Event(graphId, &SceneRequests::CreateConnectionBetween, sourceEndpoint, targetEndpoint);               
            }
            else
            {
                handledSplice = true;                
            }

            NodeRequestBus::Event(sourceEndpoint.GetNodeId(), &NodeRequests::SignalBatchedConnectionManipulationEnd);
            NodeRequestBus::Event(targetEndpoint.GetNodeId(), &NodeRequests::SignalBatchedConnectionManipulationEnd);
        }

        if (handledSplice)
        {
            GraphModelRequestBus::Event(graphId, &GraphModelRequests::RequestUndoPoint);
        }

        return handledSplice;
    }

    void GraphUtils::DetachNodeAndStitchConnections(const NodeDetachConfig& detachConfig)
    {
        GraphId graphId;
        SceneMemberRequestBus::EventResult(graphId, detachConfig.m_nodeId, &SceneMemberRequests::GetScene);

        {
            ScopedGraphUndoBlocker undoBlocker(graphId);

            AZStd::vector< AZ::EntityId > slotIds;
            NodeRequestBus::EventResult(slotIds, detachConfig.m_nodeId, &NodeRequests::GetSlotIds);

            AZStd::vector< Endpoint > sourceEndpoints;
            AZStd::vector< Endpoint > targetEndpoints;

            AZStd::unordered_set< NodeId > nodesInvolvedInSplice;

            AZStd::unordered_set< AZ::EntityId > deletedConnections;

            for (const AZ::EntityId& slotId : slotIds)
            {
                AZStd::vector< AZ::EntityId > connectionIds;
                SlotRequestBus::EventResult(connectionIds, slotId, &SlotRequests::GetConnections);

                SlotType slotType = SlotTypes::Invalid;
                SlotRequestBus::EventResult(slotType, slotId, &SlotRequests::GetSlotType);

                if (detachConfig.m_listingType == ListingType::InclusiveList)
                {
                    if (detachConfig.m_typeListing.count(slotType) == 0)
                    {
                        continue;
                    }
                }
                else if (detachConfig.m_listingType == ListingType::ExclusiveList)
                {
                    if (detachConfig.m_typeListing.count(slotType) != 0)
                    {
                        continue;
                    }
                }

                for (const AZ::EntityId& connectionId : connectionIds)
                {
                    Endpoint sourceEndpoint;
                    ConnectionRequestBus::EventResult(sourceEndpoint, connectionId, &ConnectionRequests::GetSourceEndpoint);

                    if (sourceEndpoint.m_nodeId != detachConfig.m_nodeId)
                    {
                        sourceEndpoints.push_back(sourceEndpoint);

                        auto insertResult = nodesInvolvedInSplice.insert(sourceEndpoint.GetNodeId());

                        if (insertResult.second)
                        {
                            NodeRequestBus::Event(sourceEndpoint.GetNodeId(), &NodeRequests::SignalBatchedConnectionManipulationBegin);
                        }
                    }

                    Endpoint targetEndpoint;
                    ConnectionRequestBus::EventResult(targetEndpoint, connectionId, &ConnectionRequests::GetTargetEndpoint);

                    if (targetEndpoint.m_nodeId != detachConfig.m_nodeId)
                    {
                        targetEndpoints.push_back(targetEndpoint);
                        
                        auto insertResult = nodesInvolvedInSplice.insert(targetEndpoint.GetNodeId());

                        if (insertResult.second)
                        {
                            NodeRequestBus::Event(targetEndpoint.GetNodeId(), &NodeRequests::SignalBatchedConnectionManipulationBegin);
                        }
                    }

                    deletedConnections.insert(connectionId);
                }
            }

            SceneRequestBus::Event(graphId, &SceneRequests::Delete, deletedConnections);

            // TODO: Figure out how to deal with nodes that are wrapped when trying to stitch connections.
            for (const Endpoint& sourceEndpoint : sourceEndpoints)
            {
                for (const Endpoint& targetEndpoint : targetEndpoints)
                {
                    SceneRequestBus::Event(graphId, &SceneRequests::CreateConnectionBetween, sourceEndpoint, targetEndpoint);
                }
            }

            for (const NodeId& nodeId : nodesInvolvedInSplice)
            {
                NodeRequestBus::Event(nodeId, &NodeRequests::SignalBatchedConnectionManipulationEnd);
            }

            if (GraphUtils::IsWrapperNode(detachConfig.m_nodeId))
            {
                AZStd::vector< NodeId > wrappedNodeIds;
                WrapperNodeRequestBus::EventResult(wrappedNodeIds, detachConfig.m_nodeId, &WrapperNodeRequests::GetWrappedNodeIds);

                for (const NodeId& wrappedNodeId : wrappedNodeIds)
                {
                    DetachNodeAndStitchConnections(wrappedNodeId);
                }
            }
        }

        GraphModelRequestBus::Event(graphId, &GraphModelRequests::RequestUndoPoint);
    }

    void GraphUtils::DetachSubGraphAndStitchConnections(const GraphSubGraph& subGraph)
    {
        if (subGraph.m_containedNodes.empty())
        {
            return;
        }

        AZ::EntityId nodeId = (*subGraph.m_containedNodes.begin());

        GraphId graphId;
        SceneMemberRequestBus::EventResult(graphId, nodeId, &SceneMemberRequests::GetScene);

        {
            ScopedGraphUndoBlocker undoBlocker(graphId);

            AZStd::unordered_set< Endpoint > sourceEndpoints;
            AZStd::unordered_set< Endpoint > targetEndpoints;

            AZStd::unordered_set< NodeId > nodesInvolvedInSplice;

            AZStd::unordered_set< ConnectionId > deletionIds;

            for (const ConnectionId& connectionId : subGraph.m_entryConnections)
            {
                Endpoint sourceEndpoint;
                ConnectionRequestBus::EventResult(sourceEndpoint, connectionId, &ConnectionRequests::GetSourceEndpoint);                

                sourceEndpoints.insert(sourceEndpoint);

                auto insertIter = nodesInvolvedInSplice.insert(sourceEndpoint.GetNodeId());

                if (insertIter.second)
                {
                    NodeRequestBus::Event(sourceEndpoint.GetNodeId(), &NodeRequests::SignalBatchedConnectionManipulationBegin);
                }

                deletionIds.insert(connectionId);
            }

            for (const ConnectionId& connectionId : subGraph.m_exitConnections)
            {
                Endpoint targetEndpoint;
                ConnectionRequestBus::EventResult(targetEndpoint, connectionId, &ConnectionRequests::GetTargetEndpoint);

                targetEndpoints.insert(targetEndpoint);

                auto insertIter = nodesInvolvedInSplice.insert(targetEndpoint.GetNodeId());

                if (insertIter.second)
                {
                    NodeRequestBus::Event(targetEndpoint.GetNodeId(), &NodeRequests::SignalBatchedConnectionManipulationBegin);
                }

                deletionIds.insert(connectionId);
            }
            
            SceneRequestBus::Event(graphId, &SceneRequests::Delete, deletionIds);

            for (const Endpoint& sourceEndpoint : sourceEndpoints)
            {
                for (const Endpoint& targetEndpoint : targetEndpoints)
                {
                    SceneRequestBus::Event(graphId, &SceneRequests::CreateConnectionBetween, sourceEndpoint, targetEndpoint);
                }
            }

            for (const NodeId& nodeId2 : nodesInvolvedInSplice)
            {
                NodeRequestBus::Event(nodeId2, &NodeRequests::SignalBatchedConnectionManipulationEnd);
            }
        }

        GraphModelRequestBus::Event(graphId, &GraphModelRequests::RequestUndoPoint);
    }

    bool GraphUtils::CreateConnectionsBetween(const AZStd::vector< Endpoint >& endpoints, const AZ::EntityId& targetNode, CreateConnectionsBetweenConfig& config)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        bool allowNode = false;
        AZStd::vector< AZ::EntityId > slotIds;
        NodeRequestBus::EventResult(slotIds, targetNode, &NodeRequests::GetSlotIds);

        GraphId graphId;
        SceneMemberRequestBus::EventResult(graphId, targetNode, &SceneMemberRequests::GetScene);

        OrderedEndpointSet slotOrderings;

        for (const SlotId& slotId : slotIds)
        {
            if (!IsSlotVisible(slotId))
            {
                continue;
            }

            EndpointOrderingStruct orderingStruct = EndpointOrderingStruct::ConstructOrderingInformation(Endpoint(targetNode, slotId));
            slotOrderings.insert(orderingStruct);
        }
        
        for (const Endpoint& testEndpoint : endpoints)
        {
            DataSlotType testType = DataSlotType::Unknown;
            DataSlotRequestBus::EventResult(testType, testEndpoint.GetSlotId(), &DataSlotRequests::GetDataSlotType);

            auto slotIter = slotOrderings.begin();
            while (slotIter != slotOrderings.end())
            {
                // Check whether or not the slots are already connected. If they are Skip over those slots.
                bool isConnected = false;
                SlotRequestBus::EventResult(isConnected, testEndpoint.GetSlotId(), &SlotRequests::IsConnectedTo, (*slotIter).m_endpoint);

                if (!isConnected)
                {
                    if (testType == DataSlotType::Reference)
                    {
                        GraphModelRequestBus::EventResult(isConnected, graphId, &GraphModelRequests::SynchronizeReferences, testEndpoint, (*slotIter).m_endpoint);
                    }
                    else if (testType == DataSlotType::Value)
                    {
                        DataSlotRequests* dataSlotRequests = DataSlotRequestBus::FindFirstHandler((*slotIter).m_endpoint.GetSlotId());

                        if (dataSlotRequests)
                        {
                            DataSlotType sourceSlotType = dataSlotRequests->GetDataSlotType();

                            if (sourceSlotType == DataSlotType::Reference)
                            {
                                bool isValidConnection = false;

                                GraphModelRequestBus::EventResult(isValidConnection, graphId, &GraphModelRequests::IsValidConnection, testEndpoint, (*slotIter).m_endpoint);

                                if (isValidConnection)
                                {
                                    dataSlotRequests->ConvertToValue();
                                }
                            }
                        }
                    }

                    if (!isConnected)
                    {
                        ConnectionId connectionId = CreateUnknownConnection(graphId, testEndpoint, (*slotIter).m_endpoint);

                        if (connectionId.IsValid())
                        {
                            isConnected = true;
                            config.m_createdConnections.insert(connectionId);
                        }
                    }
                }

                if (isConnected)
                {
                    allowNode = true;
                    if (config.m_connectionType == CreateConnectionsBetweenConfig::CreationType::SingleConnection)
                    {
                        return allowNode;
                    }
                    else if (config.m_connectionType == CreateConnectionsBetweenConfig::CreationType::SinglePass)
                    {
                        slotIter = slotOrderings.erase(slotIter);
                        break;
                    }
                }

                ++slotIter;
            }
        }
        
        return allowNode;
    }

    AZStd::unordered_set< GraphCanvas::ConnectionId > GraphUtils::CreateOpportunisticConnectionsBetween(const Endpoint& initializingEndpoint, const Endpoint& opportunisticEndpoint)
    {
        SlotType targetSlotType = SlotTypes::Invalid;
        SlotRequestBus::EventResult(targetSlotType, opportunisticEndpoint.GetSlotId(), &SlotRequests::GetSlotType);

        if (targetSlotType == SlotTypes::DataSlot)
        {
            targetSlotType = SlotTypes::ExecutionSlot;
        }
        else if (targetSlotType == SlotTypes::ExecutionSlot)
        {
            targetSlotType = SlotTypes::DataSlot;
        }
        else
        {
            targetSlotType = SlotTypes::Invalid;
        }

        ConnectionType targetConnectionType = ConnectionType::CT_Invalid;
        SlotRequestBus::EventResult(targetConnectionType, opportunisticEndpoint.GetSlotId(), &SlotRequests::GetConnectionType);

        if (targetConnectionType == ConnectionType::CT_Input)
        {
            targetConnectionType = ConnectionType::CT_Output;
        }
        else if (targetConnectionType == ConnectionType::CT_Output)
        {
            targetConnectionType = ConnectionType::CT_Input;
        }
        else
        {
            targetConnectionType = ConnectionType::CT_Invalid;
        }

        AZStd::vector< SlotId > slotIds;
        NodeRequestBus::EventResult(slotIds, initializingEndpoint.GetNodeId(), &NodeRequests::GetSlotIds);        

        OrderedEndpointSet targetSlots;

        for (const SlotId& slotId : slotIds)
        {
            SlotType testSlotType = SlotTypes::Invalid;
            SlotRequestBus::EventResult(testSlotType, slotId, &SlotRequests::GetSlotType);

            ConnectionType testConnectionType = ConnectionType::CT_Invalid;
            SlotRequestBus::EventResult(testConnectionType, slotId, &SlotRequests::GetConnectionType);

            if (testSlotType == targetSlotType && testConnectionType == targetConnectionType)
            {
                if (IsSlotVisible(slotId))
                {
                    EndpointOrderingStruct orderingStruct = EndpointOrderingStruct::ConstructOrderingInformation(slotId);
                    targetSlots.insert(orderingStruct);
                }
            }
        }

        CreateConnectionsBetweenConfig config;
        config.m_connectionType = CreateConnectionsBetweenConfig::CreationType::SingleConnection;

        if (!targetSlots.empty())
        {
            AZStd::vector< Endpoint > orderedEndpoints;
            orderedEndpoints.reserve(targetSlots.size());

            for (const EndpointOrderingStruct& orderingStruct : targetSlots)
            {
                orderedEndpoints.emplace_back(orderingStruct.m_endpoint);
            }

            GraphCanvas::GraphUtils::CreateConnectionsBetween(orderedEndpoints, opportunisticEndpoint.GetNodeId(), config);
        }

        return config.m_createdConnections;
    }

    OpportunisticSpliceResult GraphUtils::CreateOpportunisticsConnectionsForSplice(const GraphId& graphId, const AZStd::vector< ConnectionEndpoints >& connectedEndpoints, const NodeId& splicedNode)
    {
        OpportunisticSpliceResult spliceResult;

        {
            ScopedGraphUndoBlocker undoBlocker(graphId);

            AZ_Warning("GraphCanvas", connectedEndpoints.size() == 2, "Not really sure what to do if you pass in 3 connections that you spliced.")

            Endpoint sourceEndpoint;
            Endpoint splicedTargetEndpoint;
            Endpoint splicedSourceEndpoint;
            Endpoint targetEndpoint;

            for (const ConnectionEndpoints& endpoints : connectedEndpoints)
            {
                if (endpoints.m_sourceEndpoint.GetNodeId() == splicedNode)
                {
                    splicedSourceEndpoint = endpoints.m_sourceEndpoint;
                    targetEndpoint = endpoints.m_targetEndpoint;
                }
                else if (endpoints.m_targetEndpoint.GetNodeId() == splicedNode)
                {
                    sourceEndpoint = endpoints.m_sourceEndpoint;
                    splicedTargetEndpoint = endpoints.m_targetEndpoint;
                }
            }

            if (splicedTargetEndpoint.IsValid() && splicedSourceEndpoint.IsValid() && sourceEndpoint.IsValid() && targetEndpoint.IsValid())
            {
                AZStd::unordered_set< ConnectionId > opportunisticSourceConnections = GraphUtils::CreateOpportunisticConnectionsBetween(sourceEndpoint, splicedTargetEndpoint);
                AZStd::unordered_set< ConnectionId > opportunisticTargetConnections = GraphUtils::CreateOpportunisticConnectionsBetween(splicedSourceEndpoint, targetEndpoint);

                spliceResult.m_createdConnections.insert(opportunisticSourceConnections.begin(), opportunisticSourceConnections.end());
                spliceResult.m_createdConnections.insert(opportunisticTargetConnections.begin(), opportunisticTargetConnections.end());

                AZStd::unordered_set< Endpoint > opportunisticTargetEndpoints;

                opportunisticTargetEndpoints.reserve(opportunisticTargetConnections.size());

                for (GraphCanvas::ConnectionId opportunisticTargetConnection : opportunisticTargetConnections)
                {
                    GraphCanvas::Endpoint opportunisticEndpoint;
                    GraphCanvas::ConnectionRequestBus::EventResult(opportunisticEndpoint, opportunisticTargetConnection, &GraphCanvas::ConnectionRequests::GetTargetEndpoint);

                    opportunisticTargetEndpoints.emplace(opportunisticEndpoint);
                }

                AZStd::unordered_set< GraphCanvas::ConnectionId > removedConnections;

                for (GraphCanvas::ConnectionId opportunisticSourceConnection : opportunisticSourceConnections)
                {
                    GraphCanvas::Endpoint opportunisticEndpoint;
                    GraphCanvas::ConnectionRequestBus::EventResult(opportunisticEndpoint, opportunisticSourceConnection, &GraphCanvas::ConnectionRequests::GetSourceEndpoint);

                    GraphCanvas::SlotRequestBus::Event(opportunisticEndpoint.GetSlotId(), &GraphCanvas::SlotRequests::FindConnectionsForEndpoints, opportunisticTargetEndpoints, removedConnections);
                }

                spliceResult.m_removedConnections.reserve(removedConnections.size());

                for (const GraphCanvas::ConnectionId& toRemoveConnectionId : removedConnections)
                {
                    Endpoint removeSourceEndpoint;
                    ConnectionRequestBus::EventResult(removeSourceEndpoint, toRemoveConnectionId, &ConnectionRequests::GetSourceEndpoint);

                    Endpoint removedTargetEndpoint;
                    ConnectionRequestBus::EventResult(removedTargetEndpoint, toRemoveConnectionId, &ConnectionRequests::GetTargetEndpoint);

                    spliceResult.m_removedConnections.emplace_back(removeSourceEndpoint, removedTargetEndpoint);
                }

                // Remove all of the opportunistic splices we encountered.
                GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::Delete, removedConnections);
            }
        }


        if (!spliceResult.m_createdConnections.empty())
        {
            GraphModelRequestBus::Event(graphId, &GraphModelRequests::RequestUndoPoint);
        }

        return spliceResult;
    }

    QRectF GraphUtils::AlignNodes(const AZStd::vector< AZ::EntityId >& memberIds, const AlignConfig& alignConfig, QRectF overallBoundingRect)
    {
        bool calculateBoundingRect = overallBoundingRect.isEmpty();

        AZ::EntityId graphId;

        AZStd::unordered_set< AZ::EntityId > validElements;
        AZStd::unordered_set< AZ::EntityId > ignoredElements;

        AZ::Vector2 minorStep = AZ::Vector2(1, 1);

        for (const AZ::EntityId& memberId : memberIds)
        {
            // Don't need to align the wrapped nodes.
            // So we don't want to include them in our bounding calculations
            if (GraphUtils::IsNodeWrapped(memberId))
            {
                continue;
            }

            if (GraphUtils::IsNodeGroup(memberId))
            {
                AZStd::vector< AZ::EntityId > groupedElements;
                NodeGroupRequestBus::Event(memberId, &NodeGroupRequests::FindGroupedElements, groupedElements);

                for (const AZ::EntityId& groupedElement : groupedElements)
                {
                    ignoredElements.insert(groupedElement);
                    validElements.erase(groupedElement);
                }
            }

            if (ignoredElements.count(memberId) > 0)
            {
                continue;
            }

            validElements.insert(memberId);

            if (!graphId.IsValid())
            {
                SceneMemberRequestBus::EventResult(graphId, memberId, &SceneMemberRequests::GetScene);

                minorStep = GraphUtils::FindMinorStep(graphId);

                if (minorStep.GetX() < 1.0f)
                {
                    minorStep.SetX(1.0f);
                }

                if (minorStep.GetY() < 1.0f)
                {
                    minorStep.SetY(1.0f);
                }
            }

            if (calculateBoundingRect)
            {
                QGraphicsItem* graphicsItem = nullptr;
                SceneMemberUIRequestBus::EventResult(graphicsItem, memberId, &SceneMemberUIRequests::GetRootGraphicsItem);

                if (graphicsItem)
                {
                    overallBoundingRect |= GraphUtils::AlignBoundingBoxToGrid(graphicsItem->sceneBoundingRect(), minorStep);
                }
            }
        }

        if (validElements.empty())
        {
            return QRectF();
        }

        AZStd::set< NodeOrderingStruct, NodeOrderingStruct::Comparator > nodeOrdering(NodeOrderingStruct::Comparator(overallBoundingRect, alignConfig));

        AZ::Vector2 anchorPoint = CalculateAlignmentAnchorPoint(alignConfig);

        for (const AZ::EntityId& memberId : validElements)
        {
            if (GraphUtils::IsComment(memberId)
                || GraphUtils::IsNodeGroup(memberId))
            {
                QGraphicsItem* graphicsItem = nullptr;
                SceneMemberUIRequestBus::EventResult(graphicsItem, memberId, &SceneMemberUIRequests::GetRootGraphicsItem);

                if (graphicsItem)
                {
                    QRectF boundingBox = graphicsItem->sceneBoundingRect();
                    boundingBox = GraphUtils::AlignBoundingBoxToGrid(boundingBox, minorStep);

                    nodeOrdering.emplace(memberId, anchorPoint, boundingBox);
                }
                else
                {
                    nodeOrdering.emplace(memberId, anchorPoint);
                }
            }
            else
            {
                nodeOrdering.emplace(memberId, anchorPoint);
            }
        }

        AZ::EntityId gridId;
        SceneRequestBus::EventResult(gridId, graphId, &SceneRequests::GetGrid);

        AZ::Vector2 gridStep(0, 0);
        GridRequestBus::EventResult(gridStep, gridId, &GridRequests::GetMinorPitch);

        AZStd::vector< QRectF > boundingRects;
        boundingRects.reserve(nodeOrdering.size());

        QPointF alignmentPoint(0, 0);

        alignmentPoint.setX(overallBoundingRect.left() + overallBoundingRect.width() * anchorPoint.GetX());
        alignmentPoint.setY(overallBoundingRect.top() + overallBoundingRect.height() * anchorPoint.GetY());

        QRectF finalBoundingRect = QRectF();

        {
            ScopedGraphUndoBlocker undoBlocker(graphId);

            // We want to align everything to the average position to try to minimize the amount of motion required to do the alignment
            for (const NodeOrderingStruct& nodeStruct : nodeOrdering)
            {
                // Calculate the movement we need to take to get it to the center line.
                QPointF movementVector = alignmentPoint - nodeStruct.m_anchorPoint;

                QRectF moveableBoundingRect = CalculateAlignedPosition(alignConfig, nodeStruct.m_boundingBox, movementVector, boundingRects, gridStep, gridStep * 0.5f);

                RootGraphicsItemRequestBus::Event(nodeStruct.m_nodeId, &RootGraphicsItemRequests::AnimatePositionTo, moveableBoundingRect.topLeft(), alignConfig.m_alignTime);

                if (GraphUtils::IsNodeGroup(nodeStruct.m_nodeId))
                {
                    AlignConfig groupAlignConfig = alignConfig;
                    groupAlignConfig.m_ignoreNodes.insert(nodeStruct.m_nodeId);

                    AZStd::vector< AZ::EntityId > groupedElements;
                    NodeGroupRequestBus::Event(nodeStruct.m_nodeId, &NodeGroupRequests::FindGroupedElements, groupedElements);

                    QRectF internalGroupBoundingBox;
                    NodeGroupRequestBus::EventResult(internalGroupBoundingBox, nodeStruct.m_nodeId, &NodeGroupRequests::GetGroupBoundingBox);

                    // Figure out our relative movement distance
                    QPointF movementVector2 = moveableBoundingRect.topLeft() - nodeStruct.m_boundingBox.topLeft();

                    internalGroupBoundingBox.moveTopLeft(internalGroupBoundingBox.topLeft() + movementVector2);
                    internalGroupBoundingBox = GraphUtils::AlignBoundingBoxToGrid(internalGroupBoundingBox, minorStep);
                    internalGroupBoundingBox.adjust(gridStep.GetX(), gridStep.GetY(), -gridStep.GetX(), -gridStep.GetY());

                    QRectF groupedBoundingRect = AlignNodes(groupedElements, alignConfig, internalGroupBoundingBox);

                    internalGroupBoundingBox.adjust(-gridStep.GetX(), 0, gridStep.GetX(), gridStep.GetY());

                    if (groupedBoundingRect.width() > internalGroupBoundingBox.width())
                    {
                        float difference = aznumeric_cast<float>(groupedBoundingRect.width() - internalGroupBoundingBox.width());

                        switch (alignConfig.m_horAlign)
                        {
                        case HorizontalAlignment::Left:
                        {
                            moveableBoundingRect.adjust(0.0f, 0.0f, difference, 0.0f);
                            break;
                        }
                        case HorizontalAlignment::Center:
                        {
                            float halfDiff = difference * 0.5f;
                            moveableBoundingRect.adjust(halfDiff, 0.0f, -halfDiff, 0.0f);
                            break;
                        }
                        case HorizontalAlignment::Right:
                        {
                            moveableBoundingRect.adjust(-difference, 0.0f, 0.0f, 0.0f);
                            break;
                        }
                        default:
                            break;
                        }
                    }

                    if (groupedBoundingRect.height() > internalGroupBoundingBox.height())
                    {
                        float difference = aznumeric_cast<float>(groupedBoundingRect.height() - internalGroupBoundingBox.height());

                        switch (alignConfig.m_verAlign)
                        {
                        case VerticalAlignment::Top:
                        {
                            moveableBoundingRect.adjust(0.0f, 0.0f, 0.0f, difference);
                            break;
                        }
                        case VerticalAlignment::Middle:
                        {
                            float halfDiff = difference * 0.5f;
                            moveableBoundingRect.adjust(halfDiff, 0.0f, -halfDiff, 0.0f);
                            break;
                        }
                        case VerticalAlignment::Bottom:
                        {
                            moveableBoundingRect.adjust(0.0f, difference, 0.0f, 0.0f);
                            break;
                        }
                        default:
                            break;
                        }
                    }

                    moveableBoundingRect = GraphUtils::AlignBoundingBoxToGrid(moveableBoundingRect, gridStep);
                }

                boundingRects.push_back(moveableBoundingRect);

                finalBoundingRect |= moveableBoundingRect;
            }
        }

        GraphModelRequestBus::Event(graphId, &GraphModelRequests::RequestUndoPoint);

        return finalBoundingRect;
    }

    void GraphUtils::OrganizeNodes(const AZStd::vector< AZ::EntityId>& memberIds, const AlignConfig& alignConfig)
    {        
        if (memberIds.empty())
        {
            return;
        }

        AZStd::unordered_set< NodeId > ignoredElements;

        SubGraphParsingConfig config;
        config.m_createNonConnectableSubGraph = true;

        SubGraphParsingResult subGraphResult = ParseSceneMembersIntoSubGraphs(memberIds, config);

        // Maps from Group EntityId to the set of EntityId's contained within the group.
        AZStd::unordered_multimap< AZ::EntityId, AZ::EntityId >  groupElementMapping;

        // Maps from the node that was closest to the floating comment, to the configuration for the comment.
        AZStd::unordered_multimap< AZ::EntityId, FloatingElementAnchor > floatingElementAnchoring;

        GraphId graphId;
        SceneMemberRequestBus::EventResult(graphId, memberIds.front(), &SceneMemberRequests::GetScene);

        AZ::EntityId gridId;
        SceneRequestBus::EventResult(gridId, graphId, &SceneRequests::GetGrid);

        AZ::Vector2 gridStep(0, 0);
        GridRequestBus::EventResult(gridStep, gridId, &GridRequests::GetMinorPitch);

        AZ::Vector2 majorGridStep(0, 0);
        GridRequestBus::EventResult(majorGridStep, gridId, &GridRequests::GetMajorPitch);
        
        for (const AZ::EntityId& entityId : subGraphResult.m_nonConnectableGraph.m_containedNodes)
        {
            if (GraphUtils::IsNodeGroup(entityId))
            {
                // TODO: Add in support for organizing groups indepdendentally of the rest of the selections.
            }
            else if (GraphUtils::IsComment(entityId)
                    || GraphUtils::IsBookmarkAnchor(entityId))
            {
                    QGraphicsItem* item = nullptr;
                SceneMemberUIRequestBus::EventResult(item, entityId, &SceneMemberUIRequests::GetRootGraphicsItem);

                QRectF originalBoundingRect(0, 0, 100, 100);
                QRectF sceneBoundingRect = originalBoundingRect;

                if (item)
                {
                    originalBoundingRect = item->sceneBoundingRect();
                    sceneBoundingRect = originalBoundingRect;
                }

                // Only want to do this 5 times in case there's only a single comment.
                int breakoutCounter = 5;

                AZ::EntityId anchorEntity;
                float minDistance = -1;
                AZStd::vector<AZ::EntityId> nearbyEntities;

                FloatingElementAnchor floatingAnchor;
                floatingAnchor.m_elementId = entityId;
                
                while (breakoutCounter >= 0)
                {
                    breakoutCounter -= 1;

                    // Triple the size of the node to try to minimize the number of repetitions we need to do here.
                    // Going to try to use Qt's underlying tree system rather then iterating over everything on the graph.
                    sceneBoundingRect.adjust(-sceneBoundingRect.width(), -sceneBoundingRect.height(), sceneBoundingRect.width(), sceneBoundingRect.height());

                    SceneRequestBus::EventResult(nearbyEntities, graphId, &SceneRequests::GetEntitiesInRect, sceneBoundingRect, Qt::ItemSelectionMode::IntersectsItemBoundingRect);

                    for (const AZ::EntityId& nearbyEntityId : nearbyEntities)
                    {
                        // Keep things anchor to nodes only
                        if (GraphUtils::IsNode(nearbyEntityId) && nearbyEntityId != entityId)
                        {
                            QGraphicsItem* testItem = nullptr;
                            SceneMemberUIRequestBus::EventResult(testItem, nearbyEntityId, &SceneMemberUIRequests::GetRootGraphicsItem);

                            if (testItem)
                            {
                                QRectF testBoundingRect = testItem->sceneBoundingRect();
                                float testDistance = aznumeric_cast<float>(QtVectorMath::GetMinimumDistanceBetween(originalBoundingRect, testBoundingRect));

                                if (!anchorEntity.IsValid() || minDistance > testDistance)
                                {
                                    anchorEntity = nearbyEntityId;
                                    minDistance = testDistance;

                                    floatingAnchor.m_offset = originalBoundingRect.topLeft() - testBoundingRect.topLeft();
                                }
                            }
                        }
                    }

                    if (anchorEntity.IsValid())
                    {
                        floatingElementAnchoring.insert(AZStd::make_pair(anchorEntity, floatingAnchor));
                        break;
                    }
                }
            }
        }

        // Need to calculate out the overall bounding rect to try to minimize the distance things need to move
        QRectF overallBoundingRect;

        for (const GraphSubGraph& subGraph : subGraphResult.m_subGraphs)
        {
            overallBoundingRect |= subGraph.m_sceneBoundingRect;
        }

        AZStd::set< SubGraphOrderingStruct, SubGraphOrderingStruct::Comparator > subGraphOrderingStructs(SubGraphOrderingStruct::Comparator(overallBoundingRect, alignConfig));

        for (const GraphSubGraph& subGraph : subGraphResult.m_subGraphs)
        {
            subGraphOrderingStructs.emplace(overallBoundingRect, subGraph, alignConfig);
        }

        AZStd::vector< QRectF > finalizedRectangles;
        QPointF originalAnchorCenter;

        // We can handle each sub graph individually, then just organize adjacent to each other like they were nodes.
        //
        // Main trick is to only grab the location of the nodes once, then operate on a bounding box which we move around.
        for (const SubGraphOrderingStruct& orderingStruct : subGraphOrderingStructs)
        {            
            const GraphSubGraph* subGraph = orderingStruct.m_subGraph;            

            AZStd::unordered_set< OrganizationHelper* > minimalSpanningSet;
            AZStd::unordered_set< OrganizationHelper* > currentSearchableElements;
            AZStd::unordered_set< OrganizationHelper* > nextLayer;

            AZStd::unordered_map< NodeId, OrganizationHelper* > organizationHelperMap;

            if (subGraph->m_entryNodes.empty())
            {
                // If we have no searchable elements, that means we have a closed cycle. Pick an arbitrary starting point.
                AZ::EntityId entityId = (*subGraph->m_containedNodes.begin());

                OrganizationHelper* helper = aznew OrganizationHelper(entityId, alignConfig, subGraph->m_sceneBoundingRect);
                organizationHelperMap[entityId] = helper;
                
                nextLayer.insert(helper);
                minimalSpanningSet.insert(helper);
            }
            else
            {
                for (auto entityId : subGraph->m_entryNodes)
                {
                    OrganizationHelper* helper = aznew OrganizationHelper(entityId, alignConfig, subGraph->m_sceneBoundingRect);
                    organizationHelperMap[entityId] = helper;

                    nextLayer.insert(helper);
                    minimalSpanningSet.insert(helper);
                }
            }

            // Main concept of this is starting at a node, we do a depth first-y search and keep track of the chain that
            // lead us into the current situation.
            //
            // Next we grab all of the terminal organizers and walking backwards up that chain we can build up a reasonable bounding
            // box to use when laying out the elements from each section. Which is done once all of the nodes that were triggered
            // from a particular node were laid out.
            //
            // Known Quirks: 
            // - If a starting point of a node is in the middle of the graph(for whatever reason), this can lead to overlap.
            // - Always ranks the nodes in a Top/Left align-y fashion, just for consistency since that is how it tries to lay out the nodes
            //   in the resulting layout. So that just makes it consistent, and we don't have to worry about inverting elements
            //   or the doing something crazy with the center alignment
            // - Overall Alignment is a bit...non-deterministic right now, and can change for some reason.
            AZStd::queue< OrganizationHelper* > termimnalOrganizationHelpers;

            // Tail recursed loop.
            while (!nextLayer.empty())
            {
                currentSearchableElements = AZStd::move(nextLayer);
                nextLayer.clear();

                // Go through each of our searchable elements. And begin the process.
                for (OrganizationHelper* helper : currentSearchableElements)
                {
                    NodeId entryId = helper->m_nodeId;                    

                    AZStd::vector< SlotId > slotIds;
                    NodeRequestBus::EventResult(slotIds, entryId, &NodeRequests::GetSlotIds);

                    // Find each of the connections that will be triggered by the current node
                    for (SlotId slotId : slotIds)
                    {
                        if (IsSlotVisible(slotId) && IsSlotType(slotId, ConnectionType::CT_Output))
                        {
                            AZStd::vector< ConnectionId > connectionIds;
                            SlotRequestBus::EventResult(connectionIds, slotId, &SlotRequests::GetConnections);                            

                            for (ConnectionId connectionId : connectionIds)
                            {
                                NodeId targetNode;
                                ConnectionRequestBus::EventResult(targetNode, connectionId, &ConnectionRequests::GetTargetNodeId);

                                if (subGraph->m_containedNodes.find(targetNode) != subGraph->m_containedNodes.end())
                                {
                                    auto helperIter = organizationHelperMap.find(targetNode);

                                    // For now we want to do a one an done system with the incited elements.
                                    if (helperIter == organizationHelperMap.end())
                                    {
                                        OrganizationHelper* newHelper = aznew OrganizationHelper(targetNode, alignConfig, subGraph->m_sceneBoundingRect);
                                        organizationHelperMap[targetNode] = newHelper;

                                        helper->TriggeredElement(slotId, connectionId, newHelper);

                                        nextLayer.insert(newHelper);
                                    }
                                }
                            }
                        }
                    }

                    if (helper->m_triggeredNodes.empty())
                    {
                        termimnalOrganizationHelpers.push(helper);
                    }
                }
            }
            
            while (!termimnalOrganizationHelpers.empty())
            {
                OrganizationHelper* helper = termimnalOrganizationHelpers.front();
                termimnalOrganizationHelpers.pop();

                OrganizationSpaceAllocationHelper leftAllocation;
                OrganizationSpaceAllocationHelper rightAllocation;

                OrganizationSpaceAllocationHelper topAllocation;
                OrganizationSpaceAllocationHelper bottomAllocation;

                for (NodeOrderingStruct triggeredNode : helper->m_triggeredNodes)
                {
                    auto triggeredHelperIter = organizationHelperMap.find(triggeredNode.m_nodeId);

                    if (triggeredHelperIter != organizationHelperMap.end())
                    {
                        OrganizationHelper* triggeredHelper = triggeredHelperIter->second;
                        auto connectionIter = helper->m_slotConnections.find(triggeredHelper->m_nodeId);

                        if (connectionIter != helper->m_slotConnections.end())
                        {
                            QPointF connectionDirection;

                            SlotUIRequestBus::EventResult(connectionDirection, connectionIter->second.m_slotId, &SlotUIRequests::GetJutDirection);

                            int space = 0;
                            int seperator = 0;

                            OrganizationSpaceAllocationHelper* allocationHelper = nullptr;

                            // Determine if we want to treat something as more vertical or horizontal for our space allocation
                            if (abs(connectionDirection.x()) > abs(connectionDirection.y()))
                            {
                                space = aznumeric_cast<int>(triggeredHelper->m_boundingArea.height());
                                seperator = aznumeric_cast<int>(gridStep.GetY());

                                if (connectionDirection.x() < 0)
                                {
                                    allocationHelper = &leftAllocation;
                                }
                                else
                                {
                                    allocationHelper = &rightAllocation;
                                }
                            }
                            else
                            {
                                if (connectionDirection.y() < 0)
                                {
                                    allocationHelper = &topAllocation;
                                }
                                else
                                {
                                    allocationHelper = &bottomAllocation;
                                }
                            }

                            allocationHelper->AllocateSpace(triggeredHelper, space, seperator);
                        }
                    }
                }

                // Going to bias this so we have some extra vertical space, to minimize the horizontal space.
                // Could bias it the other way, but for now "simplicity".
                int maxHeight = AZStd::GetMax(leftAllocation.m_space, rightAllocation.m_space);

                int topOffset = 0;
                int bottomOffset = 0;

                switch (alignConfig.m_verAlign)
                {
                case VerticalAlignment::Bottom:
                    topOffset = AZStd::GetMax(0, static_cast<int>(maxHeight - helper->m_boundingArea.height()));
                    bottomOffset = 0;
                    break;
                case VerticalAlignment::Middle:
                {
                    float overshoot = 0.5f * AZStd::GetMax(0, static_cast<int>(maxHeight - helper->m_boundingArea.height()));
                    topOffset = aznumeric_cast<int>(ceil(overshoot));
                    bottomOffset = aznumeric_cast<int>(floor(overshoot));
                    break;
                }
                case VerticalAlignment::Top:
                default:
                    topOffset = 0;
                    bottomOffset = AZStd::GetMax(0, static_cast<int>(maxHeight - helper->m_boundingArea.height()));
                    break;
                }

                QRectF originalBoundingBox = helper->m_boundingArea;

                // Layout Left
                {
                    QPointF position = QPointF(originalBoundingBox.left() - gridStep.GetX(), 0.0f);

                    switch (alignConfig.m_verAlign)
                    {
                    case VerticalAlignment::Bottom:
                        position.setY(originalBoundingBox.bottom() - leftAllocation.m_space);
                        break;
                    case VerticalAlignment::Middle:
                        position.setY(originalBoundingBox.center().y() - (leftAllocation.m_space * 0.5f));
                        break;
                    case VerticalAlignment::Top:
                    default:
                        position.setY(originalBoundingBox.top());
                        break;
                    }

                    for (OrganizationHelper* triggeredHelper : leftAllocation.m_subSections)
                    {
                        QPointF relativeMove = position - triggeredHelper->m_boundingArea.topRight();
                        triggeredHelper->MoveHelperBy(relativeMove);

                        position.setY(position.y() + triggeredHelper->m_boundingArea.height() + gridStep.GetY());

                        helper->m_boundingArea |= triggeredHelper->m_boundingArea;
                    }
                }

                // Layout Right
                {
                    QPointF position = QPointF(originalBoundingBox.right() + gridStep.GetX(), 0.0f);

                    switch (alignConfig.m_verAlign)
                    {
                    case VerticalAlignment::Top:
                        position.setY(originalBoundingBox.top());
                        break;
                    case VerticalAlignment::Bottom:
                        position.setY(originalBoundingBox.bottom() - rightAllocation.m_space);
                        break;
                    case VerticalAlignment::Middle:
                        position.setY(originalBoundingBox.center().y() - (rightAllocation.m_space * 0.5f));
                        break;
                    default:
                        position.setY(originalBoundingBox.top());
                        break;
                    }

                    for (OrganizationHelper* triggeredHelper : rightAllocation.m_subSections)
                    {
                        QPointF relativeMove = position - triggeredHelper->m_boundingArea.topLeft();
                        triggeredHelper->MoveHelperBy(relativeMove);

                        position.setY(position.y() + triggeredHelper->m_boundingArea.height() + gridStep.GetY());

                        helper->m_boundingArea |= triggeredHelper->m_boundingArea;
                    }
                }

                // Layout Top
                {
                    QPointF position = QPointF(0.0f, originalBoundingBox.top() - gridStep.GetY() - topOffset);

                    switch (alignConfig.m_horAlign)
                    {
                    case HorizontalAlignment::Left:
                        position.setX(originalBoundingBox.left());
                        break;
                    case HorizontalAlignment::Right:
                        position.setX(originalBoundingBox.right() - topAllocation.m_space);
                        break;
                    case HorizontalAlignment::Center:
                        position.setX(originalBoundingBox.center().x() - (topAllocation.m_space * 0.5f));
                        break;
                    default:
                        position.setX(originalBoundingBox.left());
                        break;
                    }

                    for (OrganizationHelper* triggeredHelper : topAllocation.m_subSections)
                    {
                        QPointF relativeMove = position - triggeredHelper->m_boundingArea.bottomLeft();
                        triggeredHelper->MoveHelperBy(relativeMove);

                        position.setX(position.x() + triggeredHelper->m_boundingArea.width() + gridStep.GetX());

                        helper->m_boundingArea |= triggeredHelper->m_boundingArea;
                    }
                }

                // Layout Top
                {
                    QPointF position = QPointF(0.0f, originalBoundingBox.bottom() + gridStep.GetY() + bottomOffset);

                    switch (alignConfig.m_horAlign)
                    {
                    case HorizontalAlignment::Left:
                        position.setX(originalBoundingBox.left());
                        break;
                    case HorizontalAlignment::Right:
                        position.setX(originalBoundingBox.right() - bottomAllocation.m_space);
                        break;
                    case HorizontalAlignment::Center:
                        position.setX(originalBoundingBox.center().x() - (bottomAllocation.m_space * 0.5f));
                        break;
                    default:
                        position.setX(originalBoundingBox.left());
                        break;
                    }

                    for (OrganizationHelper* triggeredHelper : bottomAllocation.m_subSections)
                    {
                        QPointF relativeMove = position - triggeredHelper->m_boundingArea.topLeft();
                        triggeredHelper->MoveHelperBy(relativeMove);

                        position.setX(position.x() + triggeredHelper->m_boundingArea.width() + gridStep.GetX());

                        helper->m_boundingArea |= triggeredHelper->m_boundingArea;
                    }
                }

                for (OrganizationHelper* incitingHelper : helper->m_incitingElements)
                {
                    incitingHelper->OnElementFinalized(helper);

                    if (incitingHelper->IsReadyToFinalize())
                    {
                        termimnalOrganizationHelpers.push(incitingHelper);
                    }
                }
            }

            QRectF organizedSubGraphRect;

            for (auto mapIter : organizationHelperMap)
            {                
                organizedSubGraphRect |= mapIter.second->m_boundingArea;
            }            

            if (finalizedRectangles.empty())
            {
                originalAnchorCenter = organizedSubGraphRect.center();
                finalizedRectangles.push_back(organizedSubGraphRect);
            }
            else
            {
                QPointF movementDirection = originalAnchorCenter - organizedSubGraphRect.center();

                // Code that does the aligning does not do a great job with multiple alignments at once.
                // Going to split this into two aligns for now. Biasing towards the horizontal align first.
                AlignConfig splitConfig;
                splitConfig.m_horAlign = alignConfig.m_horAlign;
                splitConfig.m_verAlign = VerticalAlignment::None;

                QRectF alignedPosition = CalculateAlignedPosition(splitConfig, organizedSubGraphRect, movementDirection, finalizedRectangles, majorGridStep, majorGridStep * 0.5f);

                movementDirection = originalAnchorCenter - alignedPosition.center();

                splitConfig.m_horAlign = HorizontalAlignment::None;
                splitConfig.m_verAlign = alignConfig.m_verAlign;

                alignedPosition = CalculateAlignedPosition(splitConfig, alignedPosition, movementDirection, finalizedRectangles, majorGridStep, majorGridStep * 0.5f);

                QPointF relativeOffset = alignedPosition.center() - organizedSubGraphRect.center();

                for (auto incitingHelper : minimalSpanningSet)
                {
                    incitingHelper->MoveHelperBy(relativeOffset);
                }

                finalizedRectangles.push_back(alignedPosition);
            }            

            {
                ScopedGraphUndoBlocker undoBlock(graphId);
                for (auto mapIter : organizationHelperMap)
                {
                    mapIter.second->MoveToFinalPosition();

                    auto equalRange = floatingElementAnchoring.equal_range(mapIter.second->m_nodeId);

                    for (auto floatingIter = equalRange.first; floatingIter != equalRange.second; ++floatingIter)
                    {
                        FloatingElementAnchor anchor = floatingIter->second;

                        QPointF anchorPosition = mapIter.second->m_finalPosition;
                        anchorPosition += anchor.m_offset;

                        RootGraphicsItemRequestBus::Event(anchor.m_elementId, &RootGraphicsItemRequests::AnimatePositionTo, anchorPosition, alignConfig.m_alignTime);
                    }
                }
            }
        }

        GraphModelRequestBus::Event(graphId, &GraphModelRequests::RequestUndoPoint);
    }

    void GraphUtils::FocusOnElements(const AZStd::vector< AZ::EntityId >& memberIds, const FocusConfig& focusConfig)
    {
        if (memberIds.empty())
        {
            return;
        }

        QRectF focusArea;
        GraphCanvas::GraphId graphId;

        for (AZ::EntityId memberId : memberIds)
        {
            if (!graphId.IsValid())
            {
                GraphCanvas::SceneMemberRequestBus::EventResult(graphId, memberId, &GraphCanvas::SceneMemberRequests::GetScene);
            }

            QGraphicsItem* graphicsItem = nullptr;
            GraphCanvas::SceneMemberUIRequestBus::EventResult(graphicsItem, memberId, &GraphCanvas::SceneMemberUIRequests::GetRootGraphicsItem);

            if (graphicsItem)
            {
                focusArea |= graphicsItem->sceneBoundingRect();
            }
        }

        if (graphId.IsValid() && !focusArea.isEmpty())
        {
            if (focusConfig.m_spacingType == FocusConfig::SpacingType::FixedAmount)
            {
                focusArea.adjust(-focusConfig.m_spacingAmount, -focusConfig.m_spacingAmount, focusConfig.m_spacingAmount, focusConfig.m_spacingAmount);
            }
            else if (focusConfig.m_spacingType == FocusConfig::SpacingType::Scalar)
            {
                focusArea.adjust(-focusConfig.m_spacingAmount * focusArea.width(), -focusConfig.m_spacingAmount * focusArea.height(), focusConfig.m_spacingAmount * focusArea.width(), focusConfig.m_spacingAmount * focusArea.height());
            }
            else if (focusConfig.m_spacingType == FocusConfig::SpacingType::GridStep)
            {
                AZ::Vector2 gridStep;

                AZ::EntityId gridId;
                SceneRequestBus::EventResult(gridId, graphId, &SceneRequests::GetGrid);

                GridRequestBus::EventResult(gridStep, gridId, &GridRequests::GetMinorPitch);

                focusArea.adjust(-focusConfig.m_spacingAmount * gridStep.GetX(), -focusConfig.m_spacingAmount * gridStep.GetY(), focusConfig.m_spacingAmount * gridStep.GetX(), focusConfig.m_spacingAmount * gridStep.GetY());
            }

            GraphCanvas::ViewId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, graphId, &GraphCanvas::SceneRequests::GetViewId);

            GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::CenterOnArea, focusArea);
        }
    }

    void GraphUtils::FindConnectedNodes(const AZStd::vector<AZ::EntityId>& seedNodeIds, AZStd::unordered_set<AZ::EntityId>& connectedNodes, const AZStd::unordered_set<ConnectionType>& connectionTypes)
    {
        AZStd::unordered_set< AZ::EntityId > sourceNodes;
        sourceNodes.insert(seedNodeIds.begin(), seedNodeIds.end());

        AZStd::unordered_set< AZ::EntityId > exploreableNodes;
        exploreableNodes.insert(seedNodeIds.begin(), seedNodeIds.end());

        AZStd::unordered_set< AZ::EntityId > exploredNodes;

        while (!exploreableNodes.empty())
        {
            NodeId nodeId = (*exploreableNodes.begin());

            exploreableNodes.erase(exploreableNodes.begin());

            if (exploredNodes.find(nodeId) != exploredNodes.end())
            {
                continue;
            }

            exploredNodes.insert(nodeId);

            if (IsWrapperNode(nodeId))
            {
                AZStd::vector< NodeId > wrappedNodeIds;
                WrapperNodeRequestBus::EventResult(wrappedNodeIds, nodeId, &WrapperNodeRequests::GetWrappedNodeIds);

                for (const NodeId& wrappedNodeId : wrappedNodeIds)
                {
                    // Purposefully not adding the individually wrapped nodes to the selected list.
                    // just going to have the outermost node selected
                    if (exploredNodes.find(wrappedNodeId) == exploredNodes.end())
                    {
                        exploreableNodes.insert(wrappedNodeId);
                    }
                }
            }
            else if (IsNodeWrapped(nodeId) && sourceNodes.find(nodeId) == sourceNodes.end())
            {
                NodeId outermostNodeId = FindOutermostNode(nodeId);

                connectedNodes.insert(outermostNodeId);

                if (exploredNodes.find(outermostNodeId) == exploredNodes.end())
                {
                    exploreableNodes.insert(outermostNodeId);
                }
            }

            AZStd::vector< SlotId > slotIds;
            NodeRequestBus::EventResult(slotIds, nodeId, &NodeRequests::GetSlotIds);

            for (const SlotId& slotId : slotIds)
            {
                ConnectionType slotConnectionType = CT_Invalid;
                SlotRequestBus::EventResult(slotConnectionType, slotId, &SlotRequests::GetConnectionType);

                if (slotConnectionType == CT_Invalid
                    || connectionTypes.find(slotConnectionType) == connectionTypes.end())
                {
                    continue;
                }

                AZStd::vector< ConnectionId > connectionIds;
                SlotRequestBus::EventResult(connectionIds, slotId, &SlotRequests::GetConnections);

                for (const ConnectionId& connectionId : connectionIds)
                {
                    GraphCanvas::ConnectionEndpoints endpoints;
                    ConnectionRequestBus::EventResult(endpoints, connectionId, &ConnectionRequests::GetEndpoints);

                    if (endpoints.m_sourceEndpoint.GetSlotId() == slotId)
                    {
                        NodeId newNodeId = endpoints.m_targetEndpoint.GetNodeId();

                        connectedNodes.emplace(newNodeId);

                        if (exploredNodes.find(newNodeId) == exploredNodes.end())
                        {
                            exploreableNodes.insert(newNodeId);
                        }
                    }
                    else if (endpoints.m_targetEndpoint.GetSlotId() == slotId)
                    {
                        NodeId newNodeId = endpoints.m_sourceEndpoint.GetNodeId();

                        connectedNodes.emplace(newNodeId);

                        if (exploredNodes.find(newNodeId) == exploredNodes.end())
                        {
                            exploreableNodes.insert(newNodeId);
                        }
                    }
                }
            }
        }
    }

    AZStd::unordered_set<NodeId> GraphUtils::FindTerminalForNodeChain(const AZStd::vector<AZ::EntityId>& nodeIds, ConnectionType searchDirection)
    {
        AZStd::unordered_set<NodeId> terminalNodes;

        AZStd::unordered_set< NodeId> nodesToTraverse;
        nodesToTraverse.insert(nodeIds.begin(), nodeIds.end());

        AZStd::unordered_set< NodeId > exploredNodes;

        while (!nodesToTraverse.empty())
        {
            NodeId testNode = (*nodesToTraverse.begin());
            nodesToTraverse.erase(nodesToTraverse.begin());

            auto insertResult = exploredNodes.insert(testNode);

            // If we already explored the node, we can skip it.
            if (!insertResult.second)
            {
                continue;
            }

            AZStd::unordered_set<AZ::EntityId> connectedNodes;
            FindConnectedNodes({ testNode }, connectedNodes, {searchDirection});

            if (connectedNodes.empty())
            {
                terminalNodes.insert(testNode);
            }
            else
            {
                nodesToTraverse.insert(connectedNodes.begin(), connectedNodes.end());
            }
        }

        return terminalNodes;
    }

    void GraphUtils::SanityCheckEnabledState(AZ::EntityId nodeId)
    {
        // If we are wrapped we want to add the parent.
        if (IsNodeWrapped(nodeId))
        {
            nodeId = FindOutermostNode(nodeId);
        }

        RootGraphicsItemRequests* itemInterface = RootGraphicsItemRequestBus::FindFirstHandler(nodeId);

        RootGraphicsItemEnabledState startingState = RootGraphicsItemEnabledState::ES_Unknown;

        if (itemInterface)
        {
            startingState = itemInterface->GetEnabledState();
        }

        RootGraphicsItemEnabledState updatedState = RootGraphicsItemEnabledState::ES_Unknown;
        NodeRequestBus::EventResult(updatedState, nodeId, &NodeRequests::UpdateEnabledState);

        if (updatedState == RootGraphicsItemEnabledState::ES_Unknown)
        {
            updatedState = startingState;
        }

        if (startingState != updatedState)
        {
            AZStd::unordered_set< NodeId > unexploredNodes = { nodeId };
            PropagateNewEnabledState(unexploredNodes);
        }
    }

    void GraphUtils::SetNodesEnabledState(const AZStd::unordered_set<NodeId>& nodeIds, RootGraphicsItemEnabledState enabledState)
    {
        AZStd::unordered_set< NodeId > unexploredNodes;

        for (AZ::EntityId nodeId : nodeIds)
        {
            // If we are wrapped we want to add the parent.
            if (IsNodeWrapped(nodeId))
            {
                nodeId = FindOutermostNode(nodeId);
            }

            RootGraphicsItemRequests* itemInterface = RootGraphicsItemRequestBus::FindFirstHandler(nodeId);

            if (itemInterface)
            {
                if (enabledState == RootGraphicsItemEnabledState::ES_Enabled)
                {
                    itemInterface->SetEnabledState(enabledState);

                    // The node could be set to 'enabled' but still be partially disabled.
                    // This method will handle that information, and send back the propagation.
                    RootGraphicsItemEnabledState newState = RootGraphicsItemEnabledState::ES_Enabled;
                    NodeRequestBus::EventResult(newState, nodeId, &NodeRequests::UpdateEnabledState);
                }
                else
                {
                    itemInterface->SetEnabledState(enabledState);
                }
                
                unexploredNodes.insert(nodeId);
            }

            if (IsWrapperNode(nodeId) && unexploredNodes.count(nodeId) > 0)
            {
                AZStd::vector< NodeId > wrapperNodes;

                wrapperNodes.push_back(nodeId);

                while (!wrapperNodes.empty())
                {
                    NodeId wrapperNode = wrapperNodes.back();

                    wrapperNodes.pop_back();

                    AZStd::vector< NodeId > wrappedNodes;
                    WrapperNodeRequestBus::EventResult(wrappedNodes, wrapperNode, &WrapperNodeRequests::GetWrappedNodeIds);

                    for (auto wrappedNodeId : wrappedNodes)
                    {
                        if (IsWrapperNode(wrappedNodeId))
                        {
                            wrapperNodes.push_back(wrappedNodeId);
                        }

                        RootGraphicsItemRequestBus::Event(wrappedNodeId, &RootGraphicsItemRequests::SetEnabledState, enabledState);
                        unexploredNodes.insert(wrappedNodeId);
                    }
                }
            }
        }        

        PropagateNewEnabledState(unexploredNodes);
    }

    bool GraphUtils::CanHideEndpoint(const Endpoint& endpoint, const HideSlotConfig& hideConfig)
    {
        SlotRequests* slotRequests = SlotRequestBus::FindFirstHandler(endpoint.GetSlotId());

        if (slotRequests == nullptr)
        {
            return false;
        }

        if (!IsSlotHideable(endpoint.GetSlotId()))
        {
            return false;
        }

        if (slotRequests->HasConnections())
        {
            return false;
        }

        SlotType slotType = slotRequests->GetSlotType();

        if (hideConfig.m_slotTypeListing.AllowsType(slotType))
        {
            return false;
        }        

        SlotGroup slotGroup = slotRequests->GetSlotGroup();

        if (hideConfig.m_slotGroupListing.AllowsType(slotGroup))
        {
            return false;
        }

        ConnectionType connectionType = slotRequests->GetConnectionType();

        if (hideConfig.m_connectionTypeListing.AllowsType(connectionType))
        {
            return false;
        }

        return true;
    }

    void GraphUtils::AlignSlotForConnection(const GraphCanvas::Endpoint& moveableEndpoint, const GraphCanvas::Endpoint& fixedEndpoint)
    {
        NodeUIRequestBus::Event(moveableEndpoint.GetNodeId(), &NodeUIRequests::AdjustSize);

        GraphCanvas::GraphId graphId;
        SceneMemberRequestBus::EventResult(graphId, moveableEndpoint.GetNodeId(), &SceneMemberRequests::GetScene);

        AZ::EntityId gridId;
        SceneRequestBus::EventResult(gridId, graphId, &SceneRequests::GetGrid);

        QPointF jutDirection;
        GraphCanvas::SlotUIRequestBus::EventResult(jutDirection, fixedEndpoint.GetSlotId(), &GraphCanvas::SlotUIRequests::GetJutDirection);

        AZ::Vector2 minorStep(0, 0);
        GraphCanvas::GridRequestBus::EventResult(minorStep, gridId, &GraphCanvas::GridRequests::GetMinorPitch);

        // TODO: Make this customizable
        jutDirection.setX(jutDirection.x() * minorStep.GetX() * 2.0f);
        jutDirection.setY(jutDirection.y() * minorStep.GetY() * 2.0f);

        QPointF finalPosition;
        GraphCanvas::SlotUIRequestBus::EventResult(finalPosition, fixedEndpoint.GetSlotId(), &GraphCanvas::SlotUIRequests::GetConnectionPoint);

        finalPosition += jutDirection;        

        QPointF originalPosition;
        GraphCanvas::SlotUIRequestBus::EventResult(originalPosition, moveableEndpoint.GetSlotId(), &GraphCanvas::SlotUIRequests::GetConnectionPoint);        

        AZ::Vector2 difference = ConversionUtils::QPointToVector(finalPosition - originalPosition);

        AZ::Vector2 originalCorner;
        GraphCanvas::GeometryRequestBus::EventResult(originalCorner, moveableEndpoint.GetNodeId(), &GraphCanvas::GeometryRequests::GetPosition);

        AZ::Vector2 finalCorner = originalCorner + difference;

        GraphCanvas::GeometryRequestBus::Event(moveableEndpoint.GetNodeId(), &GraphCanvas::GeometryRequests::SetPosition, finalCorner);
    }

    QPointF GraphUtils::CalculateAnchorPoint(const QPointF& position, const AZ::Vector2& anchorPoint, const QRectF& boundingBox)
    {
        QSizeF offset;
        offset.setWidth(boundingBox.width() * anchorPoint.GetX());
        offset.setHeight(boundingBox.height() * anchorPoint.GetY());

        int xPoint = aznumeric_cast<int>(position.x() + offset.width());
        int yPoint = aznumeric_cast<int>(position.y() + offset.height());

        return QPointF(xPoint - offset.width(), yPoint - offset.height());
    }

    QPointF GraphUtils::CalculateGridSnapPosition(const QPointF& position, const AZ::Vector2& anchorPoint, const QRectF& boundingBox, const AZ::Vector2& gridStep, CalculationType calculationType)
    {
        float gridOffset = 0.5f;

        if (calculationType == CalculationType::Ceiling)
        {
            gridOffset = 1.0f;
        }
        else if (calculationType == CalculationType::Floor)
        {
            gridOffset = 0.0f;
        }

        QSizeF offset;
        offset.setWidth(boundingBox.width() * anchorPoint.GetX());
        offset.setHeight(boundingBox.height() * anchorPoint.GetY());

        int xPoint = aznumeric_cast<int>(position.x() + offset.width());
        int yPoint = aznumeric_cast<int>(position.y() + offset.height());

        int gridX = aznumeric_cast<int>(gridStep.GetX());
        int gridY = aznumeric_cast<int>(gridStep.GetY());
        
        if (xPoint < 0)
        {
            xPoint = aznumeric_cast<int>(xPoint - (gridX * gridOffset));
            xPoint += aznumeric_cast<int>(abs(xPoint)) % gridX;
        }
        else
        {
            xPoint = aznumeric_cast<int>(xPoint + (gridX * gridOffset));
            xPoint -= aznumeric_cast<int>(xPoint) % gridX;
        }

        if (yPoint < 0)
        {
            yPoint = aznumeric_cast<int>(yPoint - (gridY * gridOffset));
            yPoint += aznumeric_cast<int>(abs(yPoint)) % gridY;
        }
        else
        {
            yPoint = aznumeric_cast<int>(yPoint + (gridY * gridOffset));
            yPoint -= aznumeric_cast<int>(yPoint) % gridY;
        }

        return QPointF(xPoint - offset.width(), yPoint - offset.height());
    }

    void GraphUtils::AddElementToGroup(const AZ::EntityId& memberId, const AZ::EntityId& groupTarget)
    {
        if (IsNodeGroup(groupTarget))
        {
            AZ::EntityId outermostNode = FindOutermostNode(memberId);

            if (IsGroupableElement(outermostNode))
            {
                NodeGroupRequests* nodeGroupRequests = NodeGroupRequestBus::FindFirstHandler(groupTarget);

                nodeGroupRequests->AddElementToGroup(outermostNode);

                const bool growGroupOnly = true;
                nodeGroupRequests->ResizeGroupToElements(growGroupOnly);
            }
        }
    }

    AZ::EntityId GraphUtils::FindVisibleElement(const AZ::EntityId& memberId)
    {
        AZ::EntityId visibleMemberId = memberId;

        GraphCanvas::GraphId graphId;
        GraphCanvas::SceneMemberRequestBus::EventResult(graphId, visibleMemberId, &GraphCanvas::SceneMemberRequests::GetScene);

        // Deal with elements being hidden when they are going to be highlighted
        bool isHidden = false;
        GraphCanvas::SceneRequestBus::EventResult(isHidden, graphId, &GraphCanvas::SceneRequests::IsHidden, visibleMemberId);

        if (isHidden)
        {
            AZ::EntityId groupId;
            GraphCanvas::GroupableSceneMemberRequestBus::EventResult(groupId, visibleMemberId, &GraphCanvas::GroupableSceneMemberRequests::GetGroupId);

            while (groupId.IsValid())
            {
                visibleMemberId = groupId;

                bool isCollapsed = false;
                GraphCanvas::NodeGroupRequestBus::EventResult(isCollapsed, groupId, &GraphCanvas::NodeGroupRequests::IsCollapsed);

                if (isCollapsed)
                {
                    GraphCanvas::NodeGroupRequestBus::EventResult(visibleMemberId, groupId, &GraphCanvas::NodeGroupRequests::GetCollapsedNodeId);
                }

                bool isGroupHidden = false;
                GraphCanvas::SceneRequestBus::EventResult(isHidden, visibleMemberId, &GraphCanvas::SceneRequests::IsHidden, visibleMemberId);

                if (isGroupHidden)
                {
                    GraphCanvas::GroupableSceneMemberRequestBus::EventResult(groupId, visibleMemberId, &GraphCanvas::GroupableSceneMemberRequests::GetGroupId);
                }
                else
                {
                    break;
                }
            }
        }

        return visibleMemberId;
    }

    bool GraphUtils::UngroupGroup(const GraphId& /*graphId*/, AZ::EntityId groupElement)
    {
        bool ungroupedGroup = false;

        if (GraphUtils::IsCollapsedNodeGroup(groupElement))
        {
            CollapsedNodeGroupRequestBus::EventResult(groupElement, groupElement, &CollapsedNodeGroupRequests::GetSourceGroup);
        }
        else if (!GraphUtils::IsNodeGroup(groupElement))
        {
            return false;
        }

        if (groupElement.IsValid())
        {
            AZ::EntityId parentGroup;
            GroupableSceneMemberRequestBus::EventResult(parentGroup, groupElement, &GroupableSceneMemberRequests::GetGroupId);

            // Collect our grouped elements, if we have any to assign to our parent group when we ungroup.
            AZStd::vector< AZ::EntityId > groupedElements;

            if (parentGroup.IsValid())
            {
                NodeGroupRequestBus::Event(groupElement, &NodeGroupRequests::FindGroupedElements, groupedElements);
            }

            NodeGroupRequestBus::Event(groupElement, &NodeGroupRequests::UngroupGroup);

            if (parentGroup.IsValid() && !groupedElements.empty())
            {
                NodeGroupRequestBus::Event(parentGroup, &NodeGroupRequests::AddElementsVectorToGroup, groupedElements);
            }

            ungroupedGroup = true;
        }

        return ungroupedGroup;
    }

    AZ::EntityId GraphUtils::CreateUnknownConnection(const GraphId& graphId, const Endpoint& firstEndpoint, const Endpoint& secondEndpoint)
    {
        // Could be a source or a target endpoint, or possibly an omni-direction one.
        // So try both combinations to see which one fits.
        // Will currently fail with uni-direction connections
        AZ::EntityId testConnectionId;
        SceneRequestBus::EventResult(testConnectionId, graphId, &SceneRequests::CreateConnectionBetween, firstEndpoint, secondEndpoint);

        if (!testConnectionId.IsValid())
        {
            SceneRequestBus::EventResult(testConnectionId, graphId, &SceneRequests::CreateConnectionBetween, secondEndpoint, firstEndpoint);
        }
        
        return testConnectionId;
    }

    void GraphUtils::ParseConnectionsForSerialization(GraphSerialization& graphSerialization, const AZStd::unordered_set< ConnectionId >& connectionIds)
    {
        AZStd::unordered_multimap< Endpoint, Endpoint > connectedEndpoints;

        for (const ConnectionId& connectionId : connectionIds)
        {
            Endpoint sourceEndpoint;
            ConnectionRequestBus::EventResult(sourceEndpoint, connectionId, &ConnectionRequests::GetSourceEndpoint);

            Endpoint targetEndpoint;
            ConnectionRequestBus::EventResult(targetEndpoint, connectionId, &ConnectionRequests::GetTargetEndpoint);

            if (sourceEndpoint.IsValid() && targetEndpoint.IsValid())
            {
                AZStd::unordered_set< Endpoint > sourceEndpoints = RemapEndpointForModel(sourceEndpoint);
                AZStd::unordered_set< Endpoint > targetEndpoints = RemapEndpointForModel(targetEndpoint);

                for (const Endpoint& modelSourceEndpoint : sourceEndpoints)
                {
                    for (const Endpoint& modelTargetEndpoint : targetEndpoints)
                    {
                        connectedEndpoints.insert(AZStd::make_pair(modelSourceEndpoint, modelTargetEndpoint));
                    }
                }
            }
        }

        graphSerialization.SetConnectedEndpoints(connectedEndpoints);
    }

    QRectF GraphUtils::CalculateAlignedPosition(const AlignConfig& alignConfig, QRectF boundingBox, QPointF movementDirection, const AZStd::vector< QRectF >& collidableObjects, const AZ::Vector2& spacing, const AZ::Vector2& overlapSpacing)
    {
        // Remove the directions we don't care about aligning to.
        SanitizeMovementDirection(movementDirection, alignConfig);

        QRectF originalRect = boundingBox;
        QRectF moveableBoundingRect = boundingBox;
        moveableBoundingRect.adjust(movementDirection.x(), movementDirection.y(), movementDirection.x(), movementDirection.y());

        // Check for collisions with the elements until we no longer hit anything
        bool collided = true;

        while (collided)
        {
            collided = false;

            bool calculateBoundedMovement = true;

            // Create the horizontal movement rectangle to make collision detection better
            QRectF horizontalBoundedMovement;

            QRectF verticalBoundedMovement;

            for (const QRectF& testRect : collidableObjects)
            {
                if (calculateBoundedMovement)
                {
                    calculateBoundedMovement = false;

                    horizontalBoundedMovement = moveableBoundingRect;
                    horizontalBoundedMovement.setWidth(abs(moveableBoundingRect.left() - originalRect.left()) + moveableBoundingRect.width());

                    horizontalBoundedMovement.moveLeft(AZ::GetMin(moveableBoundingRect.left(), originalRect.left()));

                    verticalBoundedMovement = moveableBoundingRect;
                    verticalBoundedMovement.setHeight(abs(moveableBoundingRect.top() - originalRect.top()) + moveableBoundingRect.height());

                    verticalBoundedMovement.moveTop(AZ::GetMin(moveableBoundingRect.top(), originalRect.top()));
                }

                QRectF collidableTestRect = testRect;

                if (alignConfig.m_horAlign != GraphUtils::HorizontalAlignment::None)
                {
                    collidableTestRect.adjust(-overlapSpacing.GetX(), 0, overlapSpacing.GetX(), 0);
                }

                if (alignConfig.m_verAlign != GraphUtils::VerticalAlignment::None)
                {
                    collidableTestRect.adjust(0, -overlapSpacing.GetY(), 0, overlapSpacing.GetY());
                }

                bool originalIntersection = collidableTestRect.intersects(originalRect);
                bool intersected = collidableTestRect.intersects(moveableBoundingRect);

                if (!intersected && alignConfig.m_horAlign != GraphUtils::HorizontalAlignment::None && !originalIntersection)
                {
                    intersected = horizontalBoundedMovement.intersects(collidableTestRect);
                }

                if (!intersected && alignConfig.m_verAlign != GraphUtils::VerticalAlignment::None && !originalIntersection)
                {
                    intersected = verticalBoundedMovement.intersects(collidableTestRect);
                }

                if (intersected)
                {
                    collided = true;
                    calculateBoundedMovement = true;

                    switch (alignConfig.m_horAlign)
                    {
                    case GraphUtils::HorizontalAlignment::Left:
                        moveableBoundingRect.moveLeft(testRect.right() + spacing.GetX());
                        break;
                    case GraphUtils::HorizontalAlignment::Right:
                        moveableBoundingRect.moveRight(testRect.left() - spacing.GetX());
                        break;
                        // Treat Center and None as the same.
                    case GraphUtils::HorizontalAlignment::Center:
                        // Treat this like it was aligning based on
                        // which direction it was moving in on.
                        // i.e. if it was moving left, it needs to align left
                        //      if it was moving right, it needs to align right
                        if (movementDirection.x() < 0)
                        {
                            moveableBoundingRect.moveLeft(testRect.right() + spacing.GetX());
                        }
                        else
                        {
                            moveableBoundingRect.moveRight(testRect.left() - spacing.GetX());
                        }
                        break;
                    default:
                        break;
                    }

                    switch (alignConfig.m_verAlign)
                    {
                    case GraphUtils::VerticalAlignment::Top:
                        moveableBoundingRect.moveTop(testRect.bottom() + spacing.GetY());
                        break;
                    case GraphUtils::VerticalAlignment::Bottom:
                        moveableBoundingRect.moveBottom(testRect.top() - spacing.GetY());
                        break;
                    case GraphUtils::VerticalAlignment::Middle:
                        // Treat this like it was aligning based on
                        // which direction it was moving in on.
                        // i.e. if it was moving up, it needs to align top.
                        //      if it was moving down, it needs to align bottom.
                        if (movementDirection.y() < 0)
                        {
                            moveableBoundingRect.moveTop(testRect.bottom() + spacing.GetY());
                        }
                        else
                        {
                            moveableBoundingRect.moveBottom(testRect.top() - spacing.GetY());
                        }
                    default:
                        break;
                    }
                }
            }

            // Update the original rect to it's new spot to keep the sanity checking.
            originalRect = moveableBoundingRect;
        }

        return moveableBoundingRect;
    }

    QRectF GraphUtils::AlignBoundingBoxToGrid(const QRectF& boundingBox, const AZ::Vector2& stepSize)
    {
        int widthStep = aznumeric_cast<int>(stepSize.GetX());
        int heightStep = aznumeric_cast<int>(stepSize.GetY());
        QRectF resultBox = boundingBox;

        int width = aznumeric_cast<int>(boundingBox.width());

        int deltaStep = (width % aznumeric_cast<int>(stepSize.GetX()));

        if (deltaStep != 0)
        {
            width += (widthStep - deltaStep);
        }

        resultBox.setWidth(width);

        int height = aznumeric_cast<int>(boundingBox.height());

        deltaStep = (height % aznumeric_cast<int>(stepSize.GetY()));

        if (deltaStep != 0)
        {
            height += (heightStep - deltaStep);
        }

        resultBox.setHeight(height);

        QPointF newPosition = CalculateGridSnapPosition(boundingBox.topLeft(), AZ::Vector2(0, 0), resultBox, stepSize, CalculationType::Floor);
        resultBox.moveTo(newPosition.x(), newPosition.y());

        return resultBox;
    }

    void GraphUtils::PropagateNewEnabledState(AZStd::unordered_set<AZ::EntityId> unexploredNodes)
    {
        AZStd::unordered_set< NodeId > exploredNodes;

        while (!unexploredNodes.empty())
        {
            NodeId nodeId = (*unexploredNodes.begin());

            auto insertResult = exploredNodes.insert(nodeId);

            unexploredNodes.erase(unexploredNodes.begin());

            if (!insertResult.second)
            {
                continue;
            }

            RootGraphicsItemEnabledState enabledState = RootGraphicsItemEnabledState::ES_Unknown;
            RootGraphicsItemRequestBus::EventResult(enabledState, nodeId, &RootGraphicsItemRequests::GetEnabledState);

            // We'll allow re-entrant attempts for partially disabled nodes as they're beholden on order of operation problems.
            if (enabledState == RootGraphicsItemEnabledState::ES_PartialDisabled)
            {
                exploredNodes.erase(nodeId);
            }

            AZStd::vector< SlotId > nodeSlots;
            NodeRequestBus::EventResult(nodeSlots, nodeId, &NodeRequests::GetSlotIds);

            for (const SlotId& slotId : nodeSlots)
            {
                Endpoint currentEndpoint(nodeId, slotId);

                AZStd::vector< ConnectionId > connectionIds;
                SlotRequestBus::EventResult(connectionIds, slotId, &SlotRequests::GetConnections);

                ConnectionType connectionType = CT_Invalid;
                SlotRequestBus::EventResult(connectionType, slotId, &SlotRequests::GetConnectionType);

                SlotType slotType = SlotTypes::Invalid;
                SlotRequestBus::EventResult(slotType, slotId, &SlotRequests::GetSlotType);

                for (const ConnectionId& connectionId : connectionIds)
                {
                    RootGraphicsItemEnabledState connectionState = RootGraphicsItemEnabledState::ES_Enabled;
                    RootGraphicsItemRequestBus::EventResult(connectionState, connectionId, &RootGraphicsItemRequests::GetEnabledState);

                    if (enabledState != connectionState)
                    {
                        Endpoint otherEndpoint;

                        ConnectionRequestBus::EventResult(otherEndpoint, connectionId, &ConnectionRequests::FindOtherEndpoint, currentEndpoint);

                        RootGraphicsItemEnabledState otherEnabledState = RootGraphicsItemEnabledState::ES_Enabled;
                        RootGraphicsItemRequestBus::EventResult(otherEnabledState, otherEndpoint.GetNodeId(), &RootGraphicsItemRequests::GetEnabledState);

                        // If we are an execution out slot. We'll want to tell the node to update its display state
                        // to maintain the right affect.
                        if (connectionType == CT_Output && slotType == SlotTypes::ExecutionSlot)
                        {
                            RootGraphicsItemEnabledState newState = RootGraphicsItemEnabledState::ES_Enabled;
                            NodeRequestBus::EventResult(newState, otherEndpoint.GetNodeId(), &NodeRequests::UpdateEnabledState);

                            if (newState != otherEnabledState
                                && exploredNodes.count(otherEndpoint.GetNodeId()) == 0)
                            {
                                unexploredNodes.insert(otherEndpoint.GetNodeId());
                            }
                        }

                        RootGraphicsItemEnabledState connectionState2 = enabledState;

                        if (otherEnabledState > enabledState)
                        {
                            connectionState2 = otherEnabledState;
                        }

                        RootGraphicsItemRequestBus::Event(connectionId, &RootGraphicsItemRequests::SetEnabledState, connectionState2);
                    }
                }
            }
        }
    }
}
