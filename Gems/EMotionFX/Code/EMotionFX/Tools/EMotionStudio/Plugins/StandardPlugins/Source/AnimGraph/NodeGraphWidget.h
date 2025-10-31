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
#include <AzCore/std/string/string.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/StandardPluginsConfig.h>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QPoint>
#endif

QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QPainter)

#define NODEGRAPHWIDGET_USE_OPENGL

namespace EMotionFX
{
    class AnimGraphNodeGroup;
}

namespace EMStudio
{
    // forward declarations
    class NodeGraph;
    class GraphNode;
    class NodePort;
    class NodeConnection;
    class AnimGraphPlugin;


    /**
     *
     *
     */
    class NodeGraphWidget
        : public QOpenGLWidget
        , public QOpenGLFunctions
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(NodeGraphWidget, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

    public:
        NodeGraphWidget(AnimGraphPlugin* plugin, NodeGraph* activeGraph = nullptr, QWidget* parent = nullptr);
        virtual ~NodeGraphWidget();

        AnimGraphPlugin* GetPlugin() { return m_plugin; }

        void SetActiveGraph(NodeGraph* graph);
        NodeGraph* GetActiveGraph() const;

        MCORE_INLINE const QPoint& GetMousePos() const                  { return m_mousePos; }
        MCORE_INLINE void SetMousePos(const QPoint& pos)                { m_mousePos = pos; }
        MCORE_INLINE void SetShowFPS(bool showFPS)                      { m_showFps = showFPS; }

        size_t CalcNumSelectedNodes() const;

        QPoint LocalToGlobal(const QPoint& inPoint) const;
        QPoint GlobalToLocal(const QPoint& inPoint) const;

        static constexpr uint32 s_snapCellSize = 10;
        QPoint SnapLocalToGrid(const QPoint& inPoint) const;

        void CalcSelectRect(QRect& outRect);

        virtual bool PreparePainting() { return true; }

        virtual bool CheckIfIsCreateConnectionValid(AZ::u16 portNr, GraphNode* portNode, NodePort* port, bool isInputPort);
        virtual bool CheckIfIsValidTransition(GraphNode* sourceState, GraphNode* targetState);
        virtual bool CheckIfIsValidTransitionSource(GraphNode* sourceState);
        virtual bool CreateConnectionMustBeCurved() { return true; }
        virtual bool CreateConnectionShowsHelpers() { return true; }

        // callbacks
        virtual void OnDrawOverlay(QPainter& painter)                                   { MCORE_UNUSED(painter); }
        virtual void OnMoveStart()                                                      {}
        virtual void OnMoveNode(GraphNode* node, int32 x, int32 y)                      { MCORE_UNUSED(node); MCORE_UNUSED(x); MCORE_UNUSED(y); }
        virtual void OnMoveEnd()                                                        {}
        virtual void OnCreateConnection(AZ::u16 sourcePortNr, GraphNode* sourceNode, bool sourceIsInputPort, AZ::u16 targetPortNr, GraphNode* targetNode, bool targetIsInputPort, const QPoint& startOffset, const QPoint& endOffset);
        virtual void OnNodeCollapsed(GraphNode* node, bool isCollapsed)                 { MCORE_UNUSED(node); MCORE_UNUSED(isCollapsed); }
        virtual void OnShiftClickedNode(GraphNode* node)                                { MCORE_UNUSED(node); }
        virtual void OnVisualizeToggle(GraphNode* node, bool visualizeEnabled)          { MCORE_UNUSED(node); MCORE_UNUSED(visualizeEnabled); }
        virtual void OnEnabledToggle(GraphNode* node, bool enabled)                     { MCORE_UNUSED(node); MCORE_UNUSED(enabled); }
        virtual void OnSetupVisualizeOptions(GraphNode* node)                           { MCORE_UNUSED(node); }

        virtual void ReplaceTransition(NodeConnection* connection, QPoint oldStartOffset, QPoint oldEndOffset,
            GraphNode* oldSourceNode, GraphNode* oldTargetNode, GraphNode* newSourceNode, GraphNode* newTargetNode);

        void EnableBorderOverwrite(const QColor& borderColor, float borderWidth)
        {
            m_borderOverwrite = true;
            m_borderOverwriteColor = borderColor;
            m_borderOverwriteWidth = borderWidth;
        }

        void DisableBorderOverwrite() { m_borderOverwrite = false; }

        const QString& GetTitleBarText() const { return m_titleBarText; }
        void SetTitleBarText(const QString& text) { m_titleBarText = text; }

    signals:
        void ActiveGraphChanged();

    protected:
        //virtual void paintEvent(QPaintEvent* event);
        void mouseMoveEvent(QMouseEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void wheelEvent(QWheelEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override; // TODO: check if it is really needed to have them virutal as the events get propagated
        void keyReleaseEvent(QKeyEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;

        void initializeGL() override;
        void paintGL() override;
        void resizeGL(int w, int h) override;

        GraphNode* UpdateMouseCursor(const QPoint& localMousePos, const QPoint& globalMousePos);

    protected:
        AnimGraphPlugin*            m_plugin;
        bool                        m_showFps;
        QPoint                      m_mousePos;
        QPoint                      m_mouseLastPos;
        QPoint                      m_mouseLastPressPos;
        QPoint                      m_selectStart;
        QPoint                      m_selectEnd;
        int                         m_prevWidth;
        int                         m_prevHeight;
        int                         m_curWidth;
        int                         m_curHeight;
        GraphNode*                  m_moveNode;  // the node we're moving
        NodeGraph*                  m_activeGraph = nullptr;
        QFont                       m_font;
        QFontMetrics*               m_fontMetrics;
        AZ::Debug::Timer            m_renderTimer;
        AZStd::string               m_tempString;
        AZStd::string               m_fullActorName;
        AZStd::string               m_actorName;
        bool                        m_allowContextMenu;
        bool                        m_leftMousePressed;
        bool                        m_middleMousePressed;
        bool                        m_rightMousePressed;
        bool                        m_panning;
        bool                        m_rectSelecting;
        bool                        m_shiftPressed;
        bool                        m_controlPressed;
        bool                        m_altPressed;
        bool                        m_borderOverwrite = false;
        QColor                      m_borderOverwriteColor;
        float                       m_borderOverwriteWidth;
        QString                     m_titleBarText;

        void SelectNodesInGroup(EMotionFX::AnimGraphNodeGroup* nodeGroup);
    };
} // namespace EMStudio
