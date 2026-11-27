/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector2.h>
#include <AzCore/std/containers/vector.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/StandardPluginsConfig.h>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QPainter>
#include <QPainterPath>

namespace EMotionFX
{
    class AnimGraphInstance;
    class AnimGraphNode;
    class BlendTreePinnedWire;
}

namespace EMStudio
{
    // forward declarations
    class GraphNode;
    class NodeGraph;

#define WILDCARDTRANSITION_SIZE 20

    class NodeConnection
    {
        MCORE_MEMORYOBJECTCATEGORY(NodeConnection, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

    public:
        enum
        {
            TYPE_ID = 0x00000001
        };

        NodeConnection(NodeGraph* parentGraph, const QModelIndex& modelIndex, GraphNode* targetNode, AZ::u16 portNr, GraphNode* sourceNode, AZ::u16 sourceOutputPortNr);
        virtual ~NodeConnection();

        const QPersistentModelIndex& GetModelIndex() const { return m_modelIndex; }

        virtual void Render(const QItemSelectionModel& selectionModel, QPainter& painter, QPen* pen, QBrush* brush, int32 stepSize, const QRect& visibleRect, float opacity, bool alwaysColor);
        virtual bool Intersects(const QRect& rect);
        virtual bool CheckIfIsCloseTo(const QPoint& point);

        void UpdatePainterPath();
        const QPainterPath& GetPainterPath() const { return m_painterPath; }

        // False for a wire that stops at a pin: its curve then ends exactly on the last point, like a connection passing through it.
        virtual bool GetEndsAtPort() const { return true; }

        virtual void Update(const QRect& visibleRect, const QPoint& mousePos);
        virtual uint32 GetType() const                      { return TYPE_ID; }

        QRect CalcRect() const;
        QRect CalcFinalRect() const;
        QRect GetSourceRect() const;
        QRect GetTargetRect() const;
        QRect CalcCollapsedSourceRect() const;
        QRect CalcCollapsedTargetRect() const;

        bool GetIsSelected() const;

        MCORE_INLINE bool GetIsVisible()                            { return m_isVisible; }

        MCORE_INLINE AZ::u16 GetInputPortNr() const                 { return m_portNr; }
        MCORE_INLINE AZ::u16 GetOutputPortNr() const                { return m_sourcePortNr; }
        MCORE_INLINE GraphNode* GetSourceNode()                     { return m_sourceNode; }
        MCORE_INLINE GraphNode* GetTargetNode()                     { return m_targetNode; }

        MCORE_INLINE bool GetIsSynced() const                       { return m_isSynced; }
        MCORE_INLINE void SetIsSynced(bool synced)                  { m_isSynced = synced; }

        MCORE_INLINE bool GetIsProcessed() const                    { return m_isProcessed; }
        MCORE_INLINE void SetIsProcessed(bool processed)            { m_isProcessed = processed; }

        MCORE_INLINE bool GetIsDashed() const                       { return m_isDashed; }
        MCORE_INLINE void SetIsDashed(bool flag)                    { m_isDashed = flag; }

        MCORE_INLINE bool GetIsDisabled() const                     { return m_isDisabled; }
        MCORE_INLINE void SetIsDisabled(bool flag)                  { m_isDisabled = flag; }

        MCORE_INLINE bool GetIsHighlighted() const                  { return m_isHighlighted; }
        MCORE_INLINE void SetIsHighlighted(bool flag)               { m_isHighlighted = flag; }


        // when a node is selected, we highlight all incoming/outgoing connections from/to that, this is the flag to indicate that
        MCORE_INLINE bool GetIsConnectedHighlighted() const         { return m_isConnectedHighlighted; }
        MCORE_INLINE void SetIsConnectedHighlighted(bool flag)      { m_isConnectedHighlighted = flag; }

        MCORE_INLINE void SetIsTailHighlighted(bool flag)                       { m_isTailHighlighted = flag; }
        MCORE_INLINE void SetIsHeadHighlighted(bool flag)                       { m_isHeadHighlighted = flag; }
        MCORE_INLINE bool GetIsTailHighlighted() const                          { return m_isTailHighlighted; }
        MCORE_INLINE bool GetIsHeadHighlighted() const                          { return m_isHeadHighlighted; }
        virtual bool CheckIfIsCloseToHead(const QPoint& point) const            { MCORE_UNUSED(point); return false; }
        virtual bool CheckIfIsCloseToTail(const QPoint& point) const            { MCORE_UNUSED(point); return false; }
        virtual void CalcStartAndEndPoints(QPoint& start, QPoint& end) const    { MCORE_UNUSED(start); MCORE_UNUSED(end); }

        virtual bool GetIsWildcardTransition() const                            { return false; }

        MCORE_INLINE void SetColor(const QColor& color)             { m_color = color; }
        MCORE_INLINE const QColor& GetColor() const                 { return m_color; }

        void SetSourceNode(GraphNode* node)                         { m_sourceNode = node; }
        void SetTargetNode(GraphNode* node)                         { m_targetNode = node; }

        void SetTargetPort(AZ::u16 portIndex)                       { m_portNr = portIndex; }

        // Visual-only bend points in graph space, ordered from the source to the target, or nullptr when the connection
        // has no data behind it. A blend tree connection keeps them on its BlendTreeConnection, a transition on the transition.
        virtual const AZStd::vector<AZ::Vector2>* GetWaypoints() const;
        virtual void SetWaypoints(const AZStd::vector<AZ::Vector2>& waypoints);
        void SetWaypoint(size_t index, const AZ::Vector2& position);
        size_t GetNumWaypoints() const;

        // Builds the routed path in graph space: the start point, every waypoint, then the end point.
        virtual void CalcPolyline(AZStd::vector<QPoint>& outPoints) const;

        // Returns the point halfway along the routed path, which is where a new waypoint goes by default.
        QPoint CalcPathMidpoint() const;

        // Returns the index of the waypoint handle under the given point, or InvalidIndex when none is hit.
        size_t FindWaypoint(const QPoint& point) const;

        // Returns the index of the waypoint nearest the given point regardless of distance, or InvalidIndex when there are none.
        size_t FindClosestWaypoint(const QPoint& point) const;

        // Returns the index at which a new waypoint has to be inserted so it splits the path segment closest to the given point.
        size_t CalcWaypointInsertIndex(const QPoint& point) const;

        void RenderWaypoints(QPainter& painter, QBrush& brush, QPen& pen, const QColor& color) const;

        // Finds the point halfway along a polyline, plus the direction and length of the segment it falls on.
        static bool FindPathMidpoint(const QPoint* points, size_t pointCount, AZ::Vector2& outMid, AZ::Vector2& outDir, float& outSegmentLength);

        static constexpr int s_waypointRadius = 4;


    protected:
        NodeGraph*          m_parentGraph = nullptr;
        QPersistentModelIndex m_modelIndex;
        QRect               m_rect;
        QRect               m_finalRect;
        QColor              m_color;
        GraphNode*          m_sourceNode;        // source node from which the connection comes
        GraphNode*          m_targetNode;        // the target node
        QPainterPath        m_painterPath;
        AZ::u16             m_portNr;            // input port where this is connected to
        AZ::u16             m_sourcePortNr;      // source output port number
        bool                m_isVisible;         // is this connection visible?
        bool                m_isProcessed;       // is this connection processed?
        bool                m_isDisabled;
        bool                m_isDashed;
        bool                m_isHighlighted;
        bool                m_isHeadHighlighted;
        bool                m_isTailHighlighted;
        bool                m_isConnectedHighlighted;
        bool                m_isSynced;

        // Reused across frames so the render and hit-test paths stay free of per-frame allocations.
        mutable AZStd::vector<QPoint> m_pathScratch;
    };

    // Draws and edits a BlendTreePinnedWire: a wire from an output whose last waypoint is its pin.
    // It is not a model item, so it is never selected or deleted like a connection.
    class PinnedWire
        : public NodeConnection
    {
        MCORE_MEMORYOBJECTCATEGORY(PinnedWire, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

    public:
        enum
        {
            TYPE_ID = 0x00000003
        };

        PinnedWire(NodeGraph* parentGraph, GraphNode* sourceNode, size_t wireIndex);

        uint32 GetType() const override { return TYPE_ID; }

        // Which of the source node's pinned wires this is.
        size_t GetWireIndex() const { return m_wireIndex; }
        EMotionFX::AnimGraphNode* GetAnimGraphNode() const;
        const EMotionFX::BlendTreePinnedWire* GetPinnedWire() const;

        const AZStd::vector<AZ::Vector2>* GetWaypoints() const override;
        void SetWaypoints(const AZStd::vector<AZ::Vector2>& waypoints) override;
        void CalcPolyline(AZStd::vector<QPoint>& outPoints) const override;

        bool GetEndsAtPort() const override { return false; }

        void Update(const QRect& visibleRect, const QPoint& mousePos) override;
        // Draws the wire and its bend points. It goes under the connections, so a wire branched off it is never doubled.
        void Render(const QItemSelectionModel& selectionModel, QPainter& painter, QPen* pen, QBrush* brush, int32 stepSize,
            const QRect& visibleRect, float opacity, bool alwaysColor) override;
        // Draws the pin, over the connections so it stays visible where wires branch off it.
        void RenderPin(QPainter& painter, QPen* pen, QBrush* brush);

        static constexpr int s_pinRadius = 5;

    private:
        size_t m_wireIndex = 0;
    };
}   // namespace EMStudio
