/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Debug/Timer.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphModel.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/StandardPluginsConfig.h>
#include <QColor>
#include <QFont>
#include <QPen>
#include <QTextOption>
#include <QTimer>
#include <QTransform>
#endif

QT_FORWARD_DECLARE_CLASS(QPainter);

namespace EMStudio
{
    // forward declarations
    class GraphNode;
    class NodeConnection;
    class NodeGraphWidget;
    class NodePort;
    class StateConnection;

    class NodeGraph
        : public QObject
    {
        MCORE_MEMORYOBJECTCATEGORY(NodeGraph, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
        Q_OBJECT

    public:
        NodeGraph(const QModelIndex& modelIndex, NodeGraphWidget* graphWidget = nullptr);
        virtual ~NodeGraph();

        const QModelIndex& GetModelIndex() const { return m_currentModelIndex; }

        AZStd::vector<GraphNode*> GetSelectedGraphNodes() const;
        AZStd::vector<EMotionFX::AnimGraphNode*> GetSelectedAnimGraphNodes() const;
        AZStd::vector<NodeConnection*> GetSelectedNodeConnections() const;

        bool IsInReferencedGraph() const { return m_parentReferenceNode.isValid(); }

        const QTransform& GetTransform() const         { return mTransform; }
        float GetScale() const                         { return mScale; }
        void SetScale(float scale)                     { mScale = scale; }
        const QPoint& GetScrollOffset() const          { return mScrollOffset; }
        void SetScrollOffset(const QPoint& offset)     { mScrollOffset = offset; }
        void SetScalePivot(const QPoint& pivot)        { mScalePivot = pivot; }
        float GetLowestScale() const                   { return sLowestScale; }
        bool GetIsCreatingConnection() const           { return (mConNode && mRelinkConnection == nullptr); }
        bool GetIsRelinkingConnection() const          { return (mConNode && mRelinkConnection); }
        void SetCreateConnectionIsValid(bool isValid)  { mConIsValid = isValid; }
        bool GetIsCreateConnectionValid() const        { return mConIsValid; }
        void SetTargetPort(NodePort* port)             { mTargetPort = port; }
        NodePort* GetTargetPort()                      { return mTargetPort; }
        float GetDashOffset() const                    { return mDashOffset; }
        QColor GetErrorBlinkColor() const              { int32 red = aznumeric_cast<int32>(160 + ((0.5f + 0.5f * MCore::Math::Cos(mErrorBlinkOffset)) * 96)); red = MCore::Clamp<int32>(red, 0, 255); return QColor(red, 0, 0); }

        bool GetIsRepositioningTransitionHead() const               { return (mReplaceTransitionHead); }
        bool GetIsRepositioningTransitionTail() const               { return (mReplaceTransitionTail); }
        NodeConnection* GetRepositionedTransitionHead() const       { return mReplaceTransitionHead; }
        NodeConnection* GetRepositionedTransitionTail() const       { return mReplaceTransitionTail; }

        void StartReplaceTransitionHead(NodeConnection* connection, QPoint startOffset, QPoint endOffset, GraphNode* sourceNode, GraphNode* targetNode);
        void StartReplaceTransitionTail(NodeConnection* connection, QPoint startOffset, QPoint endOffset, GraphNode* sourceNode, GraphNode* targetNode);
        void GetReplaceTransitionInfo(NodeConnection** outConnection, QPoint* outOldStartOffset, QPoint* outOldEndOffset, GraphNode** outOldSourceNode, GraphNode** outOldTargetNode);
        void StopReplaceTransitionHead();
        void StopReplaceTransitionTail();
        void SetReplaceTransitionValid(bool isValid)                        { mReplaceTransitionValid = isValid; }
        bool GetReplaceTransitionValid() const                              { return mReplaceTransitionValid; }
        void RenderReplaceTransition(QPainter& painter);

        GraphNode* GetCreateConnectionNode()                   { return mConNode; }
        NodeConnection* GetRelinkConnection()                  { return mRelinkConnection; }
        uint32 GetCreateConnectionPortNr() const               { return mConPortNr; }
        bool GetCreateConnectionIsInputPort() const            { return mConIsInputPort; }
        const QPoint& GetCreateConnectionStartOffset() const   { return mConStartOffset; }
        const QPoint& GetCreateConnectionEndOffset() const     { return mConEndOffset; }
        void SetCreateConnectionEndOffset(const QPoint& offset){ mConEndOffset = offset; }

        bool CheckIfHasConnection(GraphNode* sourceNode, uint32 outputPortNr, GraphNode* targetNode, uint32 inputPortNr) const;
        NodeConnection* FindInputConnection(GraphNode* targetNode, uint32 targetPortNr) const;
        NodeConnection* FindConnection(const QPoint& mousePos);

        void SelectAllNodes();
        void UnselectAllNodes();

        uint32 CalcNumSelectedNodes() const;

        GraphNode* FindNode(const QPoint& globalPoint);
        void StartCreateConnection(uint32 portNr, bool isInputPort, GraphNode* portNode, NodePort* port, const QPoint& startOffset);
        void StartRelinkConnection(NodeConnection* connection, uint32 portNr, GraphNode* node);
        void StopCreateConnection();
        void StopRelinkConnection();

        /*
         * Update highlight flags for all connections in the currently visible graph.
         * This is called when the selection or the graph changes and makes sure to highlight the connections that are connected to of from
         * the currently selected nodes, to easily spot them in spagetthi graphs.
         */
        void UpdateHighlightConnectionFlags(const QPoint& mousePos);

        virtual void RenderBackground(QPainter& painter, int32 width, int32 height);
        virtual void Render(const QItemSelectionModel& selectionModel, QPainter& painter, int32 width, int32 height, const QPoint& mousePos, float timePassedInSeconds);
        virtual void RenderCreateConnection(QPainter& painter);
        void UpdateNodesAndConnections(int32 width, int32 height, const QPoint& mousePos);

        void SelectNodesInRect(const QRect& rect, bool overwriteCurSelection = true, bool toggleMode = false);
        void SelectConnectionCloseTo(const QPoint& point, bool overwriteCurSelection = true, bool toggle = false);
        QRect CalcRectFromSelection(bool includeConnections = true) const;
        QRect CalcRectFromGraph() const;
        NodePort* FindPort(int32 x, int32 y, GraphNode** outNode, uint32* outPortNr, bool* outIsInputPort, bool includeInputPorts = true);

        // entry state helper functions
        void SetEntryNode(GraphNode* entryNode)                                        { mEntryNode = entryNode; }
        static void RenderEntryPoint(QPainter& painter, GraphNode* node);

        void FitGraphOnScreen(int32 width, int32 height, const QPoint& mousePos, bool animate = true);
        void ScrollTo(const QPointF& point);
        void ZoomIn();
        void ZoomOut();
        void ZoomTo(float scale);
        void ZoomOnRect(const QRect& rect, int32 width, int32 height, bool animate = true);
        void StopAnimatedZoom();
        void StopAnimatedScroll();

        // helpers
        static void DrawSmoothedLineFast(QPainter& painter, int32 x1, int32 y1, int32 x2, int32 y2, int32 stepSize);
        static float DistanceToLine(float x1, float y1, float x2, float y2, float px, float py);
        static bool LinesIntersect(double Ax, double Ay, double Bx, double By,  double Cx, double Cy, double Dx, double Dy, double* X, double* Y);
        static bool LineIntersectsRect(const QRect& b, float x1, float y1, float x2, float y2, double* outX = nullptr, double* outY = nullptr);
        void DrawOverlay(QPainter& painter);

        bool GetUseAnimation() const                       { return mUseAnimation; }
        void SetUseAnimation(bool useAnim)                 { mUseAnimation = useAnim; }

        // These methods are not slots, they are  being called from BlendGraphWidget
        void OnRowsAboutToBeRemoved(const QModelIndexList& modelIndexes);
        void OnRowsInserted(const QModelIndexList& modelIndexes);
        void OnDataChanged(const QModelIndex& modelIndex, const QVector<int>& roles);

        void Reinit();

        GraphNode* FindGraphNode(const QModelIndex& modelIndex);
        GraphNode* FindGraphNode(const EMotionFX::AnimGraphNode* node);
        StateConnection* FindStateConnection(const QModelIndex& modelIndex);
        NodeConnection* FindNodeConnection(const QModelIndex& modelIndex);

        void UpdateVisualGraphFlags();

        static bool CheckIfIsRelinkConnectionValid(NodeConnection* connection, GraphNode* newTargetNode, uint32 newTargetPortNr, bool isTargetInput);

        void RecursiveSetOpacity(EMotionFX::AnimGraphNode* startNode, float opacity);

        AnimGraphModel& GetAnimGraphModel() const;

    protected:
        void RenderNodeGroups(QPainter& painter);
        void RenderTitlebar(QPainter& painter, const QString& text, int32 width);
        void RenderTitlebar(QPainter& painter, int32 width);
        void SyncTransition(StateConnection* visualStateConnection, const EMotionFX::AnimGraphStateTransition* transition, GraphNode* targetGraphNode);

        NodeGraphWidget*            m_graphWidget;
        QPersistentModelIndex       m_currentModelIndex;
        QPersistentModelIndex       m_parentReferenceNode; // if this graph is in a referenced graph, if not the model index will be invalid
        using GraphNodeByModelIndex = AZStd::unordered_map<QPersistentModelIndex, AZStd::unique_ptr<GraphNode>, QPersistentModelIndexHash>;
        GraphNodeByModelIndex m_graphNodeByModelIndex;

        GraphNode*                  mEntryNode;
        QTransform                  mTransform;
        float                       mScale;
        static float                sLowestScale;
        int32                       mMinStepSize;
        int32                       mMaxStepSize;
        QPoint                      mScrollOffset;
        QPoint                      mScalePivot;

        QPointF                     mTargetScrollOffset;
        QPointF                     mStartScrollOffset;
        QTimer                      mScrollTimer;
        AZ::Debug::Timer            mScrollPreciseTimer;

        float                       mTargetScale;
        float                       mStartScale;
        QTimer                      mScaleTimer;
        AZ::Debug::Timer            mScalePreciseTimer;

        // connection info
        QPoint                      mConStartOffset;
        QPoint                      mConEndOffset;
        uint32                      mConPortNr;
        bool                        mConIsInputPort;
        GraphNode*                  mConNode;       // nullptr when no connection is being created
        NodeConnection*             mRelinkConnection; // nullptr when not relinking a connection
        NodePort*                   mConPort;
        NodePort*                   mTargetPort;
        bool                        mConIsValid;
        float                       mDashOffset;
        float                       mErrorBlinkOffset;
        bool                        mUseAnimation;

        NodeConnection*             mReplaceTransitionHead; // nullptr when not replacing a transition head
        NodeConnection*             mReplaceTransitionTail; // nullptr when not replacing a transition tail
        QPoint                      mReplaceTransitionStartOffset;
        QPoint                      mReplaceTransitionEndOffset;
        GraphNode*                  mReplaceTransitionSourceNode;
        GraphNode*                  mReplaceTransitionTargetNode;
        bool                        mReplaceTransitionValid;

        QPen                        mSubgridPen;
        QPen                        mGridPen;

        // Overlay drawing
        QFont                       mFont;
        QString                     mQtTempString;
        QTextOption                 mTextOptions;
        QFontMetrics*               mFontMetrics;
        AZStd::string               m_tempStringA;
        AZStd::string               m_tempStringB;
        AZStd::string               m_tempStringC;
        AZStd::string               m_mcoreTempString;

        // Group drawing
        QFont                       m_groupFont;
        QFontMetrics*               m_groupFontMetrics;

    protected slots:
        void UpdateAnimatedScrollOffset();
        void UpdateAnimatedScale();
    };
}   // namespace EMStudio
