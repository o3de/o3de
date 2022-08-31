/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/containers/unordered_map.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphActionManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/StandardPluginsConfig.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphModel.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NodeGraphWidget.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/IconDecoratedNodePaletteTreeItem.h>
#include <GraphCanvas/Widgets/GraphCanvasMimeEvent.h>
#include <MCore/Source/CommandGroup.h>
#endif

QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QItemSelection);

namespace EMotionFX
{
    class AnimGraphNode;
    class AnimGraphStateTransition;
    class BlendTreeConnection;
}

namespace EMStudio
{
    // forward declarations
    class AnimGraphPlugin;

    class BlendGraphMimeEvent : public GraphCanvas::GraphCanvasMimeEvent
    {
    public:
        explicit BlendGraphMimeEvent(QString typeString, QString namePrefix);
        bool ExecuteEvent(const AZ::Vector2& sceneMousePosition, AZ::Vector2& sceneDropPosition, const AZ::EntityId& sceneId) override;

        QString GetTypeString() const;
        QString GetNamePrefix() const;
    private:
        QString m_typeString;
        QString m_namePrefix;
    };

    class BlendGraphNodePaletteTreeItem : public GraphCanvas::IconDecoratedNodePaletteTreeItem
    {
    public:
        BlendGraphNodePaletteTreeItem(AZStd::string_view name, const QString& typeString, GraphCanvas::EditorId editorId);
        BlendGraphMimeEvent* CreateMimeEvent() const override;

        void SetTypeString(const QString& typeString);
        QString GetTypeString() const;
    private:
        QString m_typeString;
    };

    class BlendGraphWidget
        : public NodeGraphWidget
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendGraphWidget, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
        Q_OBJECT

    public:
        BlendGraphWidget(AnimGraphPlugin* plugin, QWidget* parent);

        // overloaded
        bool CheckIfIsCreateConnectionValid(AZ::u16 portNr, GraphNode* portNode, NodePort* port, bool isInputPort) override;
        bool CheckIfIsValidTransition(GraphNode* sourceState, GraphNode* targetState) override;
        bool CheckIfIsValidTransitionSource(GraphNode* sourceState) override;
        bool CreateConnectionMustBeCurved() override;
        bool CreateConnectionShowsHelpers() override;

        // callbacks
        void OnMoveStart() override;
        void OnMoveNode(GraphNode* node, int32 x, int32 y) override;
        void OnMoveEnd() override;
        void OnNodeCollapsed(GraphNode* node, bool isCollapsed) override;
        void OnShiftClickedNode(GraphNode* node) override;
        void OnVisualizeToggle(GraphNode* node, bool visualizeEnabled) override;
        void OnEnabledToggle(GraphNode* node, bool enabled) override;
        void OnSetupVisualizeOptions(GraphNode* node) override;
        void ReplaceTransition(NodeConnection* connection, QPoint oldStartOffset, QPoint oldEndOffset, GraphNode* oldSourceNode, GraphNode* oldTargetNode, GraphNode* newSourceNode, GraphNode* newTargetNode) override;

        void OnCreateConnection(AZ::u16 sourcePortNr, GraphNode* sourceNode, bool sourceIsInputPort, AZ::u16 targetPortNr, GraphNode* targetNode, bool targetIsInputPort, const QPoint& startOffset, const QPoint& endOffset) override;

        void DeleteSelectedItems(NodeGraph* nodeGraph);

        static bool OnEnterDropEvent(QDragEnterEvent* event, EMotionFX::AnimGraphNode* currentNode, NodeGraph* activeGraph);

        // checks if the currently shown graph is a state machine
        bool CheckIfIsStateMachine();

        // context menu shared function (definitions in ContextMenu.cpp)
        void AddNodeGroupSubmenu(QMenu* menu, EMotionFX::AnimGraph* animGraph, const AZStd::vector<EMotionFX::AnimGraphNode*>& selectedNodes);
        void AddPreviewMotionSubmenu(QMenu* menu, AnimGraphActionManager* actionManager, const EMotionFX::AnimGraphNode* selectedNode);
        void AddAnimGraphObjectCategoryMenu(AnimGraphPlugin* plugin, QMenu* parentMenu,
            EMotionFX::AnimGraphObject::ECategory category, EMotionFX::AnimGraphObject* focusedGraphObject);
        void AddCategoryToNodePalette(EMotionFX::AnimGraphNode::ECategory category, GraphCanvas::NodePaletteTreeItem* rootNode,
            EMotionFX::AnimGraphObject* focusedGraphObject);

        void OnContextMenuEvent(QWidget* parentWidget, QPoint localMousePos, QPoint globalMousePos, AnimGraphPlugin* plugin,
            const AZStd::vector<EMotionFX::AnimGraphNode*>& selectedNodes, bool graphWidgetOnlyMenusEnabled, bool selectingAnyReferenceNodeFromNavigation,
            const AnimGraphActionFilter& actionFilter);

        void SetSelectedTransitionsEnabled(bool isEnabled);

        bool PreparePainting() override;

        void ProcessFrame(bool redraw);

        void SetVirtualFinalNode(const QModelIndex& nodeModelIndex);

    protected:
        void dropEvent(QDropEvent* event) override;
        void dragEnterEvent(QDragEnterEvent* event) override;
        void dragLeaveEvent(QDragLeaveEvent* event) override;
        void dragMoveEvent(QDragMoveEvent* event) override;

        void mouseDoubleClickEvent(QMouseEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;

        void OnContextMenuEvent(QPoint mousePos, QPoint globalMousePos, const AnimGraphActionFilter& actionFilter);

        bool event(QEvent* event) override;

    public slots:
        void DeleteSelectedItems();
        void OnContextMenuCreateNode(const BlendGraphMimeEvent* event);
        void OnNodeGroupSelected();
        void EnableSelectedTransitions()                    { SetSelectedTransitionsEnabled(true); }
        void DisableSelectedTransitions()                   { SetSelectedTransitionsEnabled(false); }

        void OnRowsInserted(const QModelIndex& parent, int first, int last);
        void OnRowsAboutToBeRemoved(const QModelIndex& parent, int first, int last);
        void OnDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);
        void OnFocusChanged(const QModelIndex& newFocusIndex, const QModelIndex& newFocusParent, const QModelIndex& oldFocusIndex, const QModelIndex& oldFocusParent);
        void OnSelectionModelChanged(const QItemSelection& selected, const QItemSelection& deselected);

    private:

        EMotionFX::AnimGraphStateTransition* FindTransitionForConnection(NodeConnection* connection) const;
        EMotionFX::BlendTreeConnection* FindBlendTreeConnection(NodeConnection* connection) const;

        // We are going to cache the NodeGraph that we have been focusing on
        // so we can swap them quickly.
        // TODO: investigate if we can avoid the caching
        // TODO: defer updates from graphs we are not showing
        using NodeGraphByModelIndex = AZStd::unordered_map<QPersistentModelIndex, AZStd::unique_ptr<NodeGraph>, QPersistentModelIndexHash>;
        NodeGraphByModelIndex m_nodeGraphByModelIndex;

        QPoint                      m_contextMenuEventMousePos;
        bool                        m_doubleClickHappened;
        MCore::CommandGroup         m_moveGroup;
    };
}   // namespace EMStudio
