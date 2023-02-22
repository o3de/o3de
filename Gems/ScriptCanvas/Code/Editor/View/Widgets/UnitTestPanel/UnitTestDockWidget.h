/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/containers/vector.h>

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
        , private AZ::Data::AssetBus::MultiHandler
        , private AZ::SystemTickBus::Handler
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(UnitTestDockWidget, AZ::SystemAllocator);

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
        bool IsModeEnabled(ScriptCanvas::ExecutionMode mode);
        QLabel* GetStatusLabel(ScriptCanvas::ExecutionMode mode);
        QCheckBox* GetEnabledCheckBox(ScriptCanvas::ExecutionMode mode);
        void ClearSearchFilter();
        void UpdateSearchFilter();

        void OnReturnPressed();
        void OnQuickFilterChanged(const QString &text);

        void OnStartTestsButton();
        void OnCloseResultsButton();

        void OpenScriptInEditor(AZ::Uuid sourceUuid);
        void OpenTestResults(AZ::Uuid sourceUuid, AZStd::string_view sourceDisplayName);
        void RunTests(const AZStd::vector<AZ::Uuid>& scriptUuids);

        void RunTestGraph(SourceHandle sourceHandle, ScriptCanvas::ExecutionMode);

        void OnTestsComplete();

        void OnSystemTick() override;

        AZ::EntityId m_scriptCanvasGraphId;
        AZ::EntityId m_graphCanvasGraphId;

        bool widgetActive;

        AZStd::unique_ptr<Ui::UnitTestDockWidget> m_ui;
        UnitTestBrowserFilterModel* m_filter;

        ItemButtonsDelegate* m_itemButtonsDelegate;

        QTimer m_filterTimer;

        class PendingTests
        {
        public:

            void Add(SourceHandle assetId, ExecutionMode mode);

            void Complete(SourceHandle assetId, ExecutionMode mode);

            bool IsFinished() const;

        private:

            AZStd::vector<AZStd::pair<SourceHandle, ExecutionMode>> m_pendingTests;
        };

        PendingTests m_pendingTests;

        struct TestMetrics
        {
            int m_graphsTested = 0;
            int m_success = 0;
            int m_failures = 0;
            int m_compilationFailures = 0;

            void Clear()
            {
                m_graphsTested = 0;
                m_success = 0;
                m_failures = 0;
                m_compilationFailures = 0;
            }
        };
        TestMetrics m_testMetrics[static_cast<int>(ExecutionMode::COUNT)];
    };
}
