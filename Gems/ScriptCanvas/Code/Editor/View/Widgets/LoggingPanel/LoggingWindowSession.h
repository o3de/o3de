/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/PlatformDef.h>

// qbrush.h(118): warning C4251: 'QBrush::d': class 'QScopedPointer<QBrushData,QBrushDataPointerDeleter>' needs to have dll-interface to be used by clients of class 'QBrush'
// qwidget.h(858): warning C4800: 'uint': forcing value to bool 'true' or 'false' (performance warning)
AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#if !defined(Q_MOC_RUN)
#include <QAbstractItemModel>
#include <QIcon>
#include <QSortFilterProxyModel>
#include <QWidget>
AZ_POP_DISABLE_WARNING

#include <AzCore/Component/NamedEntityId.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Styling/StyleHelper.h>
#include <GraphCanvas/Widgets/GraphCanvasTreeItem.h>
#include <GraphCanvas/Widgets/GraphCanvasTreeModel.h>

#include <Editor/View/Widgets/LoggingPanel/LoggingDataAggregator.h>
#include <Editor/View/Widgets/LoggingPanel/LoggingWindowTreeItems.h>
#include <ScriptCanvas/Core/Endpoint.h>
#include <ScriptCanvas/Core/NodeBus.h>

// Qt Generated
// warning C4251: 'QLayoutItem::align': class 'QFlags<Qt::AlignmentFlag>' needs to have dll-interface to be used by clients of class 'QLayoutItem'
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
#include <Editor/View/Widgets/LoggingPanel/ui_LoggingWindowSession.h>
#endif
AZ_POP_DISABLE_WARNING

namespace ScriptCanvasEditor
{
    class LoggingWindowFilterModel
        : public QSortFilterProxyModel
    {
    public:
        AZ_CLASS_ALLOCATOR(LoggingWindowFilterModel, AZ::SystemAllocator);

        LoggingWindowFilterModel() = default;

        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

        void SetFilter(const QString& filter);
        void ClearFilter();
        bool HasFilter() const;

    private:
        QString m_filter;

        DebugLogFilter m_logFilter;
    };

    class LoggingWindowSession
        : public QWidget
        , public GraphCanvas::AssetEditorNotificationBus::Handler
        , public GraphCanvas::SceneNotificationBus::Handler
    {
        Q_OBJECT
    protected:
        LoggingWindowSession(QWidget* parentWidget = nullptr);
    public:
        ~LoggingWindowSession() override;

        const LoggingDataId& GetDataId() const;

        void ClearFilter();

        // GraphCanvas::AssetEditorNotificationBus
        void OnActiveGraphChanged(const AZ::EntityId& graphId) override;
        ////

        // GraphCanvas::SceneNotificationBus
        void OnSelectionChanged() override;
        ////

    protected:

        void RegisterTreeRoot(DebugLogRootItem* debugRoot);

        void SetDataId(const LoggingDataId& loggingDataId);

        virtual void OnCaptureButtonPressed() = 0;
        virtual void OnPlaybackButtonPressed() = 0;
        virtual void OnOptionsButtonPressed() = 0;

        virtual void OnTargetChanged(int currentIndex) = 0;

        void OnExpandAll();
        void OnCollapseAll();

    protected:

        AZStd::unique_ptr< Ui::LoggingWindowSession > m_ui;

    private:

        void OnSearchFilterChanged(const QString& filterString);

        void OnLogScrolled(int value);
        void OnLogItemExpanded(const QModelIndex& modelIndex);
        void OnLogRangeChanged(int min, int max);

        void OnLogClicked(const QModelIndex& modelIndex);
        void OnLogDoubleClicked(const QModelIndex& modelIndex);

        void OnLogSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
        ExecutionLogTreeItem* ResolveExecutionItem(const QModelIndex& proxyModelIndex);

        void HandleQueuedFocus();
        void FocusOnElement(const AZ::Data::AssetId& assetId, const AZ::EntityId& assetNodeId);
        void HighlightElement(const AZ::Data::AssetId& assetId, const AZ::EntityId& assetNodeId);
        void RemoveHighlight(const AZ::Data::AssetId& assetId, const AZ::EntityId& assetNodeId);

        void ScrollToSelection();
        void ClearLoggingSelection();
        
        bool          m_clearSelectionOnSceneSelectionChange;
        bool          m_scrollToBottom;

        LoggingDataId m_loggingDataId;

        DebugLogRootItem*                   m_debugRoot;

        GraphCanvas::GraphCanvasTreeModel*  m_treeModel;
        LoggingWindowFilterModel*           m_filterModel;
        
        AZStd::unordered_map< AZ::EntityId, GraphCanvas::GraphicsEffectId > m_highlightEffects;

        QTimer            m_focusDelayTimer;

        AZ::Data::AssetId m_assetId;
        AZ::EntityId m_assetNodeId;
    };
}
