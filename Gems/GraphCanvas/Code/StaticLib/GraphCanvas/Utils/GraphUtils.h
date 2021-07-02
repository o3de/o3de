/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QPoint>
#include <QRect>

#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/list.h>

#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/Connections/ConnectionBus.h>
#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/Types/Endpoint.h>
#include <GraphCanvas/Types/GraphCanvasGraphSerialization.h>

namespace GraphCanvas
{
    struct EndpointOrderingStruct
    {
        // Helper Functions
        struct Comparator
        {
            bool operator()(const EndpointOrderingStruct& lhs, const EndpointOrderingStruct& rhs);
        };

        static EndpointOrderingStruct ConstructOrderingInformation(const Endpoint& endpoint);
        static EndpointOrderingStruct ConstructOrderingInformation(const SlotId& slotId);
        ////

        Endpoint m_endpoint;

        int m_displayGroupOrdering = 0;
        int m_slotDisplayOrdering = 0;
        ConnectionType m_connectionType = CT_Invalid;

        QPointF m_position;
    };    

    using OrderedEndpointSet = AZStd::set< EndpointOrderingStruct, EndpointOrderingStruct::Comparator >;

    class ScopedGraphUndoBlocker
    {
    public:
        ScopedGraphUndoBlocker(const GraphId& graphId);
        ~ScopedGraphUndoBlocker();

    private:

        GraphId m_graphId;
    };

    // This is just a way of parsing out information into various sections.
    //
    // Mainly used to find a 'chunk' representation of various nodes, including the input/output connections
    class GraphSubGraph
    {
    public:
        GraphSubGraph() = default;
        GraphSubGraph(bool isNonConnectable);
        GraphSubGraph(const NodeId& sourceNode, AZStd::unordered_set< NodeId >& searchableSceneMembers);

        void Clear();

        bool IsNonConnectableSubGraph() const;

        AZStd::unordered_set< NodeId > m_containedNodes;
        AZStd::unordered_set< ConnectionId > m_containedConnections;

        AZStd::unordered_set< NodeId > m_innerNodes;
        AZStd::unordered_set< ConnectionId > m_innerConnections;

        AZStd::unordered_set< NodeId > m_entryNodes;
        AZStd::unordered_set< ConnectionId > m_entryConnections;

        AZStd::unordered_set< NodeId > m_exitNodes;
        AZStd::unordered_set< ConnectionId > m_exitConnections;

        // Only valid in the case of a connectable sub graph.
        QRectF m_sceneBoundingRect;

    private:

        bool IsInternalNode(NodeId currentEntity, const AZStd::unordered_set< NodeId >& searchableSceneMembers) const;

        bool m_isNonConnectable;
    };

    struct SubGraphParsingConfig
    {
        // List of Entity Id's to ignore.
        AZStd::unordered_set< AZ::EntityId > m_ignoredGraphMembers;

        // Whether or not to make a SubGraph containing all of the non-connectable elements
        bool m_createNonConnectableSubGraph = false;        
    };

    struct SubGraphParsingResult
    {
        SubGraphParsingResult()
            // True dneotes that this graph contains non-connectable elements
            : m_nonConnectableGraph(true)
        {
        }

        void Clear()
        {
            m_nonConnectableGraph.Clear();
            m_subGraphs.clear();
        }

        GraphSubGraph m_nonConnectableGraph;
        AZStd::list< GraphSubGraph > m_subGraphs;
    };

    struct CreateConnectionsBetweenConfig
    {
        enum class CreationType
        {
            // Will create a single connection, and then stop(1 connection)
            SingleConnection,

            // Tries to connect highest priority elements to each other in a single element fashion.
            // Whenever an element is used, it is removed from the pool of available possibilities.
            // (1:1 connections at best)
            SinglePass,

            // Tries every permutation of the connections and lets whatever succeeds succeed.
            // (1:N connections)
            FullyConnected,
        };

        AZStd::unordered_set< ConnectionId > m_createdConnections;
        CreationType m_connectionType = CreationType::FullyConnected;

        bool m_createModelConnections = false;
    };    

    struct AlignConfig;

    struct FocusConfig
    {
        enum class SpacingType
        {
            Scalar,
            FixedAmount,
            GridStep
        };

        SpacingType m_spacingType = SpacingType::Scalar;
        float m_spacingAmount = 2.0f;
    };

    struct OpportunisticSpliceResult
    {
        AZStd::unordered_set< ConnectionId > m_createdConnections;
        AZStd::vector< ConnectionEndpoints > m_removedConnections;
    };

    struct ConnectionSpliceConfig
    {
        Endpoint m_splicedSourceEndpoint;
        Endpoint m_splicedTargetEndpoint;

        bool m_allowOpportunisticConnections = false;
        OpportunisticSpliceResult m_opportunisticSpliceResult;
    };    

    struct NodeDetachConfig
    {
        NodeDetachConfig() = default;

        NodeDetachConfig(const NodeId& nodeId)
            : m_nodeId(nodeId)
        {
        }

        NodeId                           m_nodeId;

        ListingType                      m_listingType = ListingType::ExclusiveList;
        AZStd::unordered_set< SlotType > m_typeListing;
    };

    struct HideSlotConfig
    {
        TypeListingConfiguration<SlotGroup>         m_slotGroupListing;
        TypeListingConfiguration<ConnectionType>    m_connectionTypeListing;
        TypeListingConfiguration<SlotType>          m_slotTypeListing;
    };

    class GraphUtils
    {
    public:
        enum class VerticalAlignment
        {
            Top,
            Middle,
            Bottom,
            None
        };

        enum class HorizontalAlignment
        {
            Left,
            Center,
            Right,
            None
        };

        enum class CalculationType
        {
            Floor,
            Round,
            Ceiling
        };

    public:
        static bool IsConnectableNode(const NodeId& entityId);
        static bool IsNodeOrWrapperSelected(const NodeId& nodeId);
        static bool IsSpliceableConnection(const ConnectionId& connectionId);

        static bool IsConnection(const AZ::EntityId& graphMemberId);
        static bool IsNode(const AZ::EntityId& graphMemberId);
        static bool IsNodeWrapped(const NodeId& nodeId);

        static bool IsWrapperNode(const AZ::EntityId& graphMemberId);
        static bool IsSlot(const AZ::EntityId& graphMemberId);
        static bool IsSlotVisible(const SlotId& slotId);
        static bool IsSlotHideable(const SlotId& slotId);
        static bool IsSlotConnectionType(const SlotId& slotId, ConnectionType connectionType);
        static bool IsSlotType(const SlotId& slotId, SlotType slotType);
        static bool IsSlot(const SlotId& slotId, SlotType slotType, ConnectionType connectionType);
        static bool IsGroupableElement(const AZ::EntityId& graphMemberId);

        static bool IsNodeGroup(const AZ::EntityId& graphMemberId);
        static bool IsCollapsedNodeGroup(const AZ::EntityId& graphMemberId);

        static bool IsComment(const AZ::EntityId& graphMemberId);
        static bool IsBookmarkAnchor(const AZ::EntityId& graphMemberId);

        static AZ::EntityId CreateGroupForElements(const AZ::EntityId& graphId, const AZStd::vector<AZ::EntityId>& memberIds, AZ::Vector2 startingPoint = AZ::Vector2(0,0));
        static void ResizeGroupToElements(const AZ::EntityId& groupId, const AZStd::vector<AZ::EntityId>& memberIds);

        static AZ::Vector2 FindMinorStep(const AZ::EntityId& graphId);
        static AZ::Vector2 FindMajorStep(const AZ::EntityId& graphId);

        static NodeId FindOutermostNode(const AZ::EntityId& graphMemberId);
        static void DeleteOutermostNode(const GraphId& graphId, const AZ::EntityId& graphMemberId);

        static AZStd::unordered_set< Endpoint > RemapEndpointForModel(const Endpoint& endpoint);
        
        static void ParseMembersForSerialization(GraphSerialization& graphData, const AZStd::unordered_set< AZ::EntityId >& memberIds);
        
        static SubGraphParsingResult ParseSceneMembersIntoSubGraphs(const AZStd::vector< NodeId >& sourceSceneMembers, const SubGraphParsingConfig& subGraphParsingConfig);

        static bool IsValidModelConnection(const GraphId& graphId, const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint);
        static ConnectionValidationTooltip GetModelConnnectionValidityToolTip(const GraphId& graphId, const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint);

        static bool CreateModelConnection(const GraphId& graphId, const ConnectionId& connectionId, const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint);

        static AZStd::unordered_set< ConnectionId > FindConnectionsForNodes(const AZStd::unordered_set<NodeId>& nodeIds, bool internalConnectionsOnly);

        static bool SpliceNodeOntoConnection(const NodeId& node, const ConnectionId& connectionId, ConnectionSpliceConfig& spliceConfiguration);
        static bool SpliceSubGraphOntoConnection(const GraphSubGraph& subGraph, const ConnectionId& connectionId);

        static void DetachNodeAndStitchConnections(const NodeDetachConfig& nodeDetatchConfig);
        static void DetachSubGraphAndStitchConnections(const GraphSubGraph& subGraph);

        static bool CreateConnectionsBetween(const AZStd::vector< Endpoint >& endpoints, const AZ::EntityId& targetNode, CreateConnectionsBetweenConfig& creationConfig);

        static AZStd::unordered_set< GraphCanvas::ConnectionId > CreateOpportunisticConnectionsBetween(const Endpoint& initializingEndpoint, const Endpoint& opportunisticEndpoint);
        static OpportunisticSpliceResult CreateOpportunisticsConnectionsForSplice(const GraphId& graphId, const AZStd::vector< ConnectionEndpoints >& connectedEndpoints , const NodeId& splicedNode);

        static QRectF AlignNodes(const AZStd::vector< AZ::EntityId >& memberIds, const AlignConfig& alignConfig, QRectF overallBoundingRect = QRectF());
        static void OrganizeNodes(const AZStd::vector< AZ::EntityId >& memberIds, const AlignConfig& alignConfig);

        static void FocusOnElements(const AZStd::vector< AZ::EntityId >& memberIds, const FocusConfig& focusConfig);

        static void FindConnectedNodes(const AZStd::vector<AZ::EntityId>& seedNodeIds, AZStd::unordered_set<AZ::EntityId>& connectedNodes, const AZStd::unordered_set<ConnectionType>& connectionTypes);

        static AZStd::unordered_set<NodeId> FindTerminalForNodeChain(const AZStd::vector<AZ::EntityId>& nodeIds, ConnectionType searchDirection);

        static void SanityCheckEnabledState(AZ::EntityId nodeId);
        static void SetNodesEnabledState(const AZStd::unordered_set<NodeId>& nodeIds, RootGraphicsItemEnabledState enabledState);

        static bool CanHideEndpoint(const Endpoint& endpoint, const HideSlotConfig& slotConfig);

        static void AlignSlotForConnection(const GraphCanvas::Endpoint& moveableEndpoint, const GraphCanvas::Endpoint& fixedEndpoint);

        static QPointF CalculateAnchorPoint(const QPointF& position, const AZ::Vector2& anchorPoint, const QRectF& boundingBox);
        static QPointF CalculateGridSnapPosition(const QPointF& position, const AZ::Vector2& anchorPoint, const QRectF& boundingBox, const AZ::Vector2& gridStep, CalculationType calculationType = CalculationType::Round);

        static void AddElementToGroup(const AZ::EntityId& memberId, const AZ::EntityId& groupTarget);

        static AZ::EntityId FindVisibleElement(const AZ::EntityId& memberId);

        static bool UngroupGroup(const GraphId& graphId, AZ::EntityId groupElement);

    private:
        static ConnectionId CreateUnknownConnection(const GraphId& graphId, const Endpoint& firstEndpoint, const Endpoint& secondEndpoint);
        static void ParseConnectionsForSerialization(GraphSerialization& graphData, const AZStd::unordered_set< ConnectionId >& connectionIds);

        static QRectF CalculateAlignedPosition(const AlignConfig& alignConfig, QRectF boundingBox, QPointF movementDirection, const AZStd::vector< QRectF >& collidableObjects, const AZ::Vector2& spacing, const AZ::Vector2& overlapSpacing);

        static QRectF AlignBoundingBoxToGrid(const QRectF& boundingBox, const AZ::Vector2& stepSize);

        static void PropagateNewEnabledState(AZStd::unordered_set<AZ::EntityId> unexploredNodes);
    };

    struct AlignConfig
    {
        AlignConfig() = default;

        AlignConfig(GraphUtils::VerticalAlignment verAlign, GraphUtils::HorizontalAlignment horAlign)
            : m_verAlign(verAlign)
            , m_horAlign(horAlign)
        {
        }

        GraphUtils::VerticalAlignment m_verAlign = GraphUtils::VerticalAlignment::None;
        GraphUtils::HorizontalAlignment m_horAlign = GraphUtils::HorizontalAlignment::None;

        AZStd::unordered_set< AZ::EntityId > m_ignoreNodes;

        AZStd::chrono::milliseconds m_alignTime = AZStd::chrono::milliseconds(250);
    };

    struct NodeOrderingStruct
    {
        struct Comparator
        {
            Comparator() = default;
            Comparator(const QRectF& boundingRect, const AlignConfig& alignConfig);

            bool operator()(const NodeOrderingStruct& lhs, const NodeOrderingStruct& rhs) const;

            QPointF m_targetPoint;
            AlignConfig m_alignConfig;
        };

        NodeOrderingStruct(const NodeId& nodeId, const AZ::Vector2& anchorPoint);
        NodeOrderingStruct(const NodeId& nodeId, const AZ::Vector2& anchorPoint, const QRectF& boundingBox);

        NodeId  m_nodeId;
        QRectF  m_boundingBox;
        QPointF m_anchorPoint;

    private:

        void CalculateAnchorPoint(const AZ::Vector2& anchorPoint);
    };

    typedef AZStd::set< NodeOrderingStruct, NodeOrderingStruct::Comparator > OrderedNodeStruct;

    struct SubGraphOrderingStruct
    {
        struct Comparator
        {
            Comparator() = default;
            Comparator(const QRectF& overallBoundingRect, const AlignConfig& alignConfig);

            bool operator()(const SubGraphOrderingStruct& lhs, const SubGraphOrderingStruct& rhs) const;

            QPointF m_targetPoint;
            AlignConfig m_alignConfig;
        };        

        SubGraphOrderingStruct();
        SubGraphOrderingStruct(const QRectF& overallBoundingRect, const GraphSubGraph& subGraph, const AlignConfig& alignConfig);
        
        const GraphSubGraph* m_subGraph;
        QRectF m_graphBoundingRect;
        QPointF m_anchorPoint;

        QPointF m_averagePoint;

        OrderedNodeStruct m_orderedNodes;
    };

    class NodeFocusCyclingHelper
    {
    public:

        NodeFocusCyclingHelper();

        bool IsConfigured() const;
        void Clear();

        void SetActiveGraph(const GraphId& graphId);
        void SetNodes(const AZStd::vector<NodeId>& nodes);

        void CycleToNextNode();
        void CycleToPreviousNode();

    private:

        void ParseNodes();
        void FocusOnNode(const NodeId& nodeId);

        ViewId m_viewId;
        GraphId m_graphId;

        AZStd::vector<NodeId> m_sourceNodes;
        
        int                            m_cycleOffset;
        NodeOrderingStruct::Comparator m_comparator;
        AZStd::vector<NodeOrderingStruct> m_sortedNodes;
    };
}
