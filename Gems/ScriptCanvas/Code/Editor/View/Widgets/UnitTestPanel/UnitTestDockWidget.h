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
#include <QAbstractListModel>
#include <QAbstractItemView>
#include <QListView>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QTimer>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QFocusEvent>
#include <QMenu>
#include <QStyledItemDelegate>
#include <QPainter>

#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Casting/numeric_cast.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <AzQtComponents/Components/StyledDockWidget.h>

#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Components/GraphCanvasPropertyBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>
#include <GraphCanvas/Widgets/GraphCanvasTreeItem.h>
#include <GraphCanvas/Widgets/GraphCanvasTreeModel.h>

#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Bus/UnitTestVerificationBus.h>
#endif

class QAction;
class QLineEdit;
class QPushButton;

namespace Ui
{
    class UnitTestDockWidget;
}

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class SourceAssetBrowserEntry;
    }
}

namespace ScriptCanvasEditor
{

    class ItemButtonsDelegate
        : public QStyledItemDelegate 
    {
        Q_OBJECT
    public:
        explicit ItemButtonsDelegate(QObject* parent = nullptr);
        ItemButtonsDelegate(const ItemButtonsDelegate&) = delete;
        ItemButtonsDelegate& operator= (const ItemButtonsDelegate&) = delete;

        void paint(QPainter* painter, const QStyleOptionViewItem &option, const QModelIndex& index) const override;
        bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;

    Q_SIGNALS:
        void EditButtonClicked(QModelIndex);
        void ResultsButtonClicked(QModelIndex);

    private:
        QPoint GetEditPosition(const QStyleOptionViewItem& option) const;
        QPoint GetResultsPosition(const QStyleOptionViewItem& option) const;

        QPixmap m_editIcon;

        static const int m_leftIconPadding = 9;
    };

    class UnitTestComponent
        : public GraphCanvas::GraphCanvasPropertyComponent
    {
    public:
        AZ_COMPONENT(UnitTestComponent, "{D4C073E6-DBFA-48A0-8B43-0A699A6CE293}", GraphCanvasPropertyComponent);

        static void Reflect(AZ::ReflectContext*);
        static AZ::Entity* CreateUnitTestEntity();

        UnitTestComponent();
        ~UnitTestComponent() override = default;

        AZStd::string_view GetTitle();

    private:
        AZStd::string m_componentTitle;
    };

    class UnitTestDockWidget;
    class UnitTestBrowserFilterModel;

    class UnitTestContextMenu
        : public QMenu
    {
    public:
        UnitTestContextMenu(UnitTestDockWidget* dockWidget, AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry* sourceEntry);
    };

    class UnitTestDockWidget
        : public AzQtComponents::StyledDockWidget
        , public GraphCanvas::AssetEditorNotificationBus::Handler
        , public AzToolsFramework::EditorEvents::Bus::Handler
        , public UnitTestWidgetNotificationBus::Handler
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(UnitTestDockWidget, AZ::SystemAllocator, 0);

        UnitTestDockWidget(QWidget* parent = nullptr);
        ~UnitTestDockWidget();

        // ScriptCanvasEditor::UnitTestWidgetNotificationBus
        void OnCheckStateCountChange(const int count) override;
        ////

        friend class UnitTestContextMenu;

    public Q_SLOTS:
        void OnContextMenuRequested(const QPoint &pos);
        void OnRowDoubleClicked(QModelIndex index);
        void OnEditButtonClicked(QModelIndex index);
        void OnResultsButtonClicked(QModelIndex index);

    private:
        void ClearSearchFilter();
        void UpdateSearchFilter();

        void OnReturnPressed();
        void OnQuickFilterChanged(const QString &text);

        void OnStartTestsButton();
        void OnCloseResultsButton();

        void OpenScriptInEditor(AZ::Uuid sourceUuid);
        void OpenTestResults(AZ::Uuid sourceUuid, AZStd::string_view sourceDisplayName);
        void RunTests(const AZStd::vector<AZ::Uuid>& scriptUuids);

        AZ::EntityId m_scriptCanvasGraphId;
        AZ::EntityId m_graphCanvasGraphId;

        bool widgetActive;

        AZStd::unique_ptr<Ui::UnitTestDockWidget> m_ui;
        UnitTestBrowserFilterModel* m_filter;

        ItemButtonsDelegate* m_itemButtonsDelegate;

        QTimer m_filterTimer;
    };
}