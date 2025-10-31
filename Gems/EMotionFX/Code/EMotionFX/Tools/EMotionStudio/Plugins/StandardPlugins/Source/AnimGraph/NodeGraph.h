/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

QT_FORWARD_DECLARE_CLASS(QLineEdit);
QT_FORWARD_DECLARE_CLASS(QPainter);

namespace EMotionFX
{
    class AnimGraphNodeGroup;
}
namespace EMStudio
{
    // forward declarations
    class GraphNode;
    class NodeConnection;
    class NodeGraphWidget;
    class NodePort;
    class StateConnection;
    class ZoomableLineEdit;

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

        const QTransform& GetTransform() const         { return m_transform; }
        float GetScale() const                         { return m_scale; }
        void SetScale(float scale)                     { m_scale = scale; }
        const QPoint& GetScrollOffset() const          { return m_scrollOffset; }
        void SetScrollOffset(const QPoint& offset)     { m_scrollOffset = offset; }
        void SetScalePivot(const QPoint& pivot)        { m_scalePivot = pivot; }
        float GetLowestScale() const                   { return sLowestScale; }
        bool GetIsCreatingConnection() const;
        bool GetIsRelinkingConnection() const;
        void SetCreateConnectionIsValid(bool isValid)  { m_conIsValid = isValid; }
        bool GetIsCreateConnectionValid() const        { return m_conIsValid; }
        void SetTargetPort(NodePort* port)             { m_targetPort = port; }
        NodePort* GetTargetPort()                      { return m_targetPort; }
        float GetDashOffset() const                    { return m_dashOffset; }
        QColor GetErrorBlinkColor() const              { int32 red = aznumeric_cast<int32>(160 + ((0.5f + 0.5f * MCore::Math::Cos(m_errorBlinkOffset)) * 96)); red = MCore::Clamp<int32>(red, 0, 255); return QColor(red, 0, 0); }

        bool GetIsRepositioningTransitionHead() const               { return (m_replaceTransitionHead); }
        bool GetIsRepositioningTransitionTail() const               { return (m_replaceTransitionTail); }
        NodeConnection* GetRepositionedTransitionHead() const       { return m_replaceTransitionHead; }
        NodeConnection* GetRepositionedTransitionTail() const       { return m_replaceTransitionTail; }

        void StartReplaceTransitionHead(NodeConnection* connection, QPoint startOffset, QPoint endOffset, GraphNode* sourceNode, GraphNode* targetNode);
        void StartReplaceTransitionTail(NodeConnection* connection, QPoint startOffset, QPoint endOffset, GraphNode* sourceNode, GraphNode* targetNode);
        void GetReplaceTransitionInfo(NodeConnection** outConnection, QPoint* outOldStartOffset, QPoint* outOldEndOffset, GraphNode** outOldSourceNode, GraphNode** outOldTargetNode);
        void StopReplaceTransitionHead();
        void StopReplaceTransitionTail();
        void SetReplaceTransitionValid(bool isValid)                        { m_replaceTransitionValid = isValid; }
        bool GetReplaceTransitionValid() const                              { return m_replaceTransitionValid; }
        void RenderReplaceTransition(QPainter& painter);

        GraphNode* GetCreateConnectionNode() const;
        NodeConnection* GetRelinkConnection()                  { return m_relinkConnection; }
        AZ::u16 GetCreateConnectionPortNr() const              { return m_conPortNr; }
        bool GetCreateConnectionIsInputPort() const            { return m_conIsInputPort; }
        const QPoint& GetCreateConnectionStartOffset() const   { return m_conStartOffset; }
        const QPoint& GetCreateConnectionEndOffset() const     { return m_conEndOffset; }
        void SetCreateConnectionEndOffset(const QPoint& offset){ m_conEndOffset = offset; }

        bool CheckIfHasConnection(GraphNode* sourceNode, AZ::u16 outputPortNr, GraphNode* targetNode, AZ::u16 inputPortNr) const;
        NodeConnection* FindInputConnection(GraphNode* targetNode, AZ::u16 targetPortNr) const;
        NodeConnection* FindConnection(const QPoint& mousePos);

        void SelectAllNodes();
        void UnselectAllNodes();

        size_t CalcNumSelectedNodes() const;

        GraphNode* FindNode(const QPoint& globalPoint);
        void StartCreateConnection(AZ::u16 portNr, bool isInputPort, GraphNode* portNode, NodePort* port, const QPoint& startOffset);
        void StartRelinkConnection(NodeConnection* connection, AZ::u16 portNr, GraphNode* node);
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
        NodePort* FindPort(int32 x, int32 y, GraphNode** outNode, AZ::u16* outPortNr, bool* outIsInputPort, bool includeInputPorts = true);

        EMotionFX::AnimGraphNodeGroup* FindNodeGroup(const QPoint& globalPoint) const;
        bool CheckInsideNodeGroupTitleRect(const EMotionFX::AnimGraphNodeGroup* nodeGroup, const QPoint& globalPoint) const;
        void EnableNameEditForNodeGroup(EMotionFX::AnimGraphNodeGroup* nodeGroup);
        void DisableNameEditForNodeGroup();
        void RemoveNodeGroup(EMotionFX::AnimGraphNodeGroup* nodeGroup);

        // entry state helper functions
        void SetEntryNode(GraphNode* entryNode)                                        { m_entryNode = entryNode; }
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

        bool GetUseAnimation() const                       { return m_useAnimation; }
        void SetUseAnimation(bool useAnim)                 { m_useAnimation = useAnim; }

        // These methods are not slots, they are  being called from BlendGraphWidget
        void OnRowsAboutToBeRemoved(const QModelIndexList& modelIndexes);
        void OnRowsInserted(const QModelIndexList& modelIndexes);
        void OnDataChanged(const QModelIndex& modelIndex, const QVector<int>& roles);

        void Reinit();

        GraphNode* FindGraphNode(const QModelIndex& modelIndex);
        GraphNode* FindGraphNode(const EMotionFX::AnimGraphNode* node);
        const GraphNode* FindGraphNode(const EMotionFX::AnimGraphNode* node) const;
        StateConnection* FindStateConnection(const QModelIndex& modelIndex);
        NodeConnection* FindNodeConnection(const QModelIndex& modelIndex);

        void UpdateVisualGraphFlags();

        static bool CheckIfIsRelinkConnectionValid(NodeConnection* connection, GraphNode* newTargetNode, AZ::u16 newTargetPortNr, bool isTargetInput);

        void RecursiveSetOpacity(EMotionFX::AnimGraphNode* startNode, float opacity);

        AnimGraphModel& GetAnimGraphModel() const;

    protected:
        void RenderNodeGroups(QPainter& painter);
        void RenderTitlebar(QPainter& painter, const QString& text, int32 width);
        void RenderTitlebar(QPainter& painter, int32 width);
        void SyncTransition(StateConnection* visualStateConnection, const EMotionFX::AnimGraphStateTransition* transition, GraphNode* targetGraphNode);
        QRect ComputeNodeGroupRect(const EMotionFX::AnimGraphNodeGroup* nodeGroup) const;

        NodeGraphWidget*            m_graphWidget;
        QPersistentModelIndex       m_currentModelIndex;
        QPersistentModelIndex       m_parentReferenceNode; // if this graph is in a referenced graph, if not the model index will be invalid
        using GraphNodeByModelIndex = AZStd::unordered_map<QPersistentModelIndex, AZStd::unique_ptr<GraphNode>, QPersistentModelIndexHash>;
        GraphNodeByModelIndex m_graphNodeByModelIndex;

        GraphNode*                  m_entryNode;
        QTransform                  m_transform;
        float                       m_scale;
        static float                sLowestScale;
        int32                       m_minStepSize;
        int32                       m_maxStepSize;
        QPoint                      m_scrollOffset;
        QPoint                      m_scalePivot;

        QPointF                     m_targetScrollOffset;
        QPointF                     m_startScrollOffset;
        QTimer                      m_scrollTimer;
        AZ::Debug::Timer            m_scrollPreciseTimer;

        float                       m_targetScale;
        float                       m_startScale;
        QTimer                      m_scaleTimer;
        AZ::Debug::Timer            m_scalePreciseTimer;

        // connection info
        QPoint                      m_conStartOffset;
        QPoint                      m_conEndOffset;
        AZ::u16                     m_conPortNr;
        bool                        m_conIsInputPort;
        QModelIndex                 m_conNodeIndex;
        NodeConnection*             m_relinkConnection; // nullptr when not relinking a connection
        NodePort*                   m_conPort;
        NodePort*                   m_targetPort;
        bool                        m_conIsValid;
        float                       m_dashOffset;
        float                       m_errorBlinkOffset;
        bool                        m_useAnimation;

        NodeConnection*             m_replaceTransitionHead; // nullptr when not replacing a transition head
        NodeConnection*             m_replaceTransitionTail; // nullptr when not replacing a transition tail
        QPoint                      m_replaceTransitionStartOffset;
        QPoint                      m_replaceTransitionEndOffset;
        GraphNode*                  m_replaceTransitionSourceNode;
        GraphNode*                  m_replaceTransitionTargetNode;
        bool                        m_replaceTransitionValid;

        QPen                        m_subgridPen;
        QPen                        m_gridPen;

        // Overlay drawing
        QFont                       m_font;
        QString                     m_qtTempString;
        QTextOption                 m_textOptions;
        QFontMetrics*               m_fontMetrics;
        AZStd::string               m_tempStringA;
        AZStd::string               m_tempStringB;
        AZStd::string               m_tempStringC;
        AZStd::string               m_mcoreTempString;

        // Group drawing
        QFont                       m_groupFont;
        QScopedPointer<QFontMetrics> m_groupFontMetrics;
        QScopedPointer<ZoomableLineEdit> m_nodeGroupNameLineEdit;
        EMotionFX::AnimGraphNodeGroup* m_currentNameEditNodeGroup = nullptr;

    protected slots:
        void UpdateAnimatedScrollOffset();
        void UpdateAnimatedScale();
    };
}   // namespace EMStudio
