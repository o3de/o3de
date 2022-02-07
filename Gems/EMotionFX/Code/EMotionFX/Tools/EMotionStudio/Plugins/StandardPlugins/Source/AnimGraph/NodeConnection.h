/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <EMotionStudio/Plugins/StandardPlugins/Source/StandardPluginsConfig.h>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QPainter>
#include <QPainterPath>

namespace EMotionFX
{
    class AnimGraphInstance;
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

        const QModelIndex& GetModelIndex() const { return m_modelIndex; }

        virtual void Render(const QItemSelectionModel& selectionModel, QPainter& painter, QPen* pen, QBrush* brush, int32 stepSize, const QRect& visibleRect, float opacity, bool alwaysColor);
        virtual bool Intersects(const QRect& rect);
        virtual bool CheckIfIsCloseTo(const QPoint& point);

        void UpdatePainterPath();

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
    };
}   // namespace EMStudio
