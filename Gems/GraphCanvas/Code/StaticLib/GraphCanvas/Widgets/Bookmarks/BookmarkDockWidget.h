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

#if !defined(Q_MOC_RUN)
#include <QAction>
#include <QTimer>
#include <qlabel.h>
#include <qitemselectionmodel.h>

#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Widgets/Bookmarks/BookmarkTableModel.h>
#include <GraphCanvas/Widgets/StyledItemDelegates/IconDecoratedNameDelegate.h>
#endif

class QSortFilterProxyModel;

namespace Ui
{
    class BookmarkDockWidget;
}

namespace GraphCanvas
{
    class BookmarkDockWidget
        : public AzQtComponents::StyledDockWidget
        , public BookmarkTableRequestBus::Handler
        , public GraphCanvas::SceneNotificationBus::Handler
        , public AssetEditorNotificationBus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(BookmarkDockWidget, AZ::SystemAllocator, 0);

        BookmarkDockWidget(EditorId editorId, QWidget* parent = nullptr);
        ~BookmarkDockWidget();

        // AssetEditorNotificationBus
        void OnActiveGraphChanged(const GraphId& graphId) override;
        ////

        // GraphCanvas::GraphCanvasTreeModelRequestBus::Handler
        void ClearSelection() override;
        ////

        // GraphCanvas::SceneNotifications
        void OnSelectionChanged();
        ////

    public Q_SLOTS:

        void OnCreateBookmark();
        void OnDeleteBookmark();
        void OnContextMenuRequested(const QPoint &pos);
        void SelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

    private:

        void OnQuickFilterChanged(const QString &text);
        void UpdateFilter();
        void ClearFilter();

        AZStd::unique_ptr<Ui::BookmarkDockWidget> m_ui;

        QTimer m_filterTimer;

        EditorId m_editorId;

        AZ::EntityId m_activeGraphCanvasGraphId;
        AZ::EntityId m_remapTarget;

        GraphCanvas::BookmarkTableSourceModel*      m_model;
        GraphCanvas::BookmarkTableSortProxyModel*   m_proxyModel;
    };
}
