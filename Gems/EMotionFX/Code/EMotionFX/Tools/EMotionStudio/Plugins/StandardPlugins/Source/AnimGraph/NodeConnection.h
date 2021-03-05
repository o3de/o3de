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

        NodeConnection(NodeGraph* parentGraph, const QModelIndex& modelIndex, GraphNode* targetNode, uint32 portNr, GraphNode* sourceNode, uint32 sourceOutputPortNr);
        virtual ~NodeConnection();

        const QModelIndex& GetModelIndex() const { return m_modelIndex; }

        virtual void Render(const QItemSelectionModel& selectionModel, QPainter& painter, QPen* pen, QBrush* brush, int32 stepSize, const QRect& visibleRect, float opacity, bool alwaysColor);
        virtual bool Intersects(const QRect& rect);
        virtual bool CheckIfIsCloseTo(const QPoint& point);

        void UpdatePainterPath();

        virtual void Update(const QRect& visibleRect, const QPoint& mousePos);
        virtual uint32 GetType()                            { return TYPE_ID; }

        QRect CalcRect() const;
        QRect CalcFinalRect() const;
        QRect GetSourceRect() const;
        QRect GetTargetRect() const;
        QRect CalcCollapsedSourceRect() const;
        QRect CalcCollapsedTargetRect() const;

        bool GetIsSelected() const;

        MCORE_INLINE bool GetIsVisible()                            { return mIsVisible; }

        MCORE_INLINE uint32 GetInputPortNr() const                  { return mPortNr; }
        MCORE_INLINE uint32 GetOutputPortNr() const                 { return mSourcePortNr; }
        MCORE_INLINE GraphNode* GetSourceNode()                     { return mSourceNode; }
        MCORE_INLINE GraphNode* GetTargetNode()                     { return mTargetNode; }

        MCORE_INLINE bool GetIsSynced() const                       { return mIsSynced; }
        MCORE_INLINE void SetIsSynced(bool synced)                  { mIsSynced = synced; }

        MCORE_INLINE bool GetIsProcessed() const                    { return mIsProcessed; }
        MCORE_INLINE void SetIsProcessed(bool processed)            { mIsProcessed = processed; }

        MCORE_INLINE bool GetIsDashed() const                       { return mIsDashed; }
        MCORE_INLINE void SetIsDashed(bool flag)                    { mIsDashed = flag; }

        MCORE_INLINE bool GetIsDisabled() const                     { return mIsDisabled; }
        MCORE_INLINE void SetIsDisabled(bool flag)                  { mIsDisabled = flag; }

        MCORE_INLINE bool GetIsHighlighted() const                  { return mIsHighlighted; }
        MCORE_INLINE void SetIsHighlighted(bool flag)               { mIsHighlighted = flag; }


        // when a node is selected, we highlight all incoming/outgoing connections from/to that, this is the flag to indicate that
        MCORE_INLINE bool GetIsConnectedHighlighted() const         { return mIsConnectedHighlighted; }
        MCORE_INLINE void SetIsConnectedHighlighted(bool flag)      { mIsConnectedHighlighted = flag; }

        MCORE_INLINE void SetIsTailHighlighted(bool flag)                       { mIsTailHighlighted = flag; }
        MCORE_INLINE void SetIsHeadHighlighted(bool flag)                       { mIsHeadHighlighted = flag; }
        MCORE_INLINE bool GetIsTailHighlighted() const                          { return mIsTailHighlighted; }
        MCORE_INLINE bool GetIsHeadHighlighted() const                          { return mIsHeadHighlighted; }
        virtual bool CheckIfIsCloseToHead(const QPoint& point) const            { MCORE_UNUSED(point); return false; }
        virtual bool CheckIfIsCloseToTail(const QPoint& point) const            { MCORE_UNUSED(point); return false; }
        virtual void CalcStartAndEndPoints(QPoint& start, QPoint& end) const    { MCORE_UNUSED(start); MCORE_UNUSED(end); }

        virtual bool GetIsWildcardTransition() const                            { return false; }

        MCORE_INLINE void SetColor(const QColor& color)             { mColor = color; }
        MCORE_INLINE const QColor& GetColor() const                 { return mColor; }

        void SetSourceNode(GraphNode* node)                         { mSourceNode = node; }
        void SetTargetNode(GraphNode* node)                         { mTargetNode = node; }

        void SetTargetPort(uint32 portIndex)                        { mPortNr = portIndex; }


    protected:
        NodeGraph*          m_parentGraph = nullptr;
        QPersistentModelIndex m_modelIndex;
        QRect               mRect;
        QRect               mFinalRect;
        QColor              mColor;
        GraphNode*          mSourceNode;        // source node from which the connection comes
        GraphNode*          mTargetNode;        // the target node
        QPainterPath        mPainterPath;
        uint32              mPortNr;            // input port where this is connected to
        uint32              mSourcePortNr;      // source output port number
        bool                mIsVisible;         // is this connection visible?
        bool                mIsProcessed;       // is this connection processed?
        bool                mIsDisabled;
        bool                mIsDashed;
        bool                mIsHighlighted;
        bool                mIsHeadHighlighted;
        bool                mIsTailHighlighted;
        bool                mIsConnectedHighlighted;
        bool                mIsSynced;
    };
}   // namespace EMStudio
