/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Utils/Utils.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzQtComponents/Components/Widgets/AssetFolderTableView.h>
#include <AzQtComponents/DragAndDrop/MainWindowDragAndDrop.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/ActionManager/HotKey/HotKeyManagerInterface.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserTableViewProxyModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserTreeToTableProxyModel.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTableView.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTreeView.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserViewUtils.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerUtils.h>
#include <AzToolsFramework/Editor/RichTextHighlighter.h>

#if !defined(Q_MOC_RUN)
#include <QApplication>
#include <QDragMoveEvent>
#include <QHeaderView>
#include <QLineEdit>
#include <QMenu>
#include <QVBoxLayout>
#endif

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        static constexpr const char* const TableViewMainViewName = "AssetBrowserTableView_main";

        AssetBrowserTableView::AssetBrowserTableView(QWidget* parent)
            : QWidget(parent)
            , m_tableViewWidget(new AzQtComponents::AssetFolderTableView(parent))
            , m_tableViewProxyModel(new AssetBrowserTableViewProxyModel(parent))
            , m_treeToTableProxyModel(new AssetBrowserTreeToTableProxyModel(parent))
            , m_assetFilterModel(new AssetBrowserFilterModel(parent, true))
            , m_tableViewDelegate(new TableViewDelegate(m_tableViewWidget))
        {
            // Using our own instance of AssetBrowserFilterModel to be able to show also files when the main model
            // only lists directories, and at the same time get sort and filter entries features from AssetBrowserFilterModel.
            using namespace AzToolsFramework::AssetBrowser;
            AssetBrowserComponentRequestBus::BroadcastResult(m_assetBrowserModel, &AssetBrowserComponentRequests::GetAssetBrowserModel);

            m_treeToTableProxyModel->setSourceModel(m_assetFilterModel);
            m_tableViewProxyModel->setSourceModel(m_assetFilterModel);
            m_tableViewWidget->setSortingEnabled(false);
            m_tableViewWidget->header()->setSectionsClickable(true);
            m_tableViewWidget->header()->setSortIndicatorShown(true);
            m_tableViewWidget->setModel(m_tableViewProxyModel);
            m_tableViewWidget->setItemDelegateForColumn(0, m_tableViewDelegate);

            for (int i = 0; i < m_tableViewWidget->header()->count(); ++i)
            {
                m_tableViewWidget->header()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
            }

            SetSortMode(AssetBrowserEntry::AssetEntrySortMode::Name);

            connect(
                m_tableViewWidget->header(),
                &QHeaderView::sortIndicatorChanged,
                this,
                [this](const int index, Qt::SortOrder order)
                {
                    m_assetFilterModel->sort(index, order);
                });

            connect(
                m_tableViewWidget->header(),
                &QHeaderView::sectionClicked,
                this,
                [this](const int index)
                {
                    SetSortMode(ColumnToSortMode(index));
                });

            connect(
                m_tableViewWidget,
                &AzQtComponents::AssetFolderTableView::selectionChangedSignal,
                this,
                [this](const QItemSelection& selected, const QItemSelection& deselected)
                {
                    Q_EMIT selectionChangedSignal(selected, deselected);
                });

            connect(
                m_tableViewWidget,
                &AzQtComponents::AssetFolderTableView::clicked,
                this,
                [this](const QModelIndex& index)
                {
                    auto indexData = index.data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserEntry*>();
                    emit entryClicked(indexData);
                });

            connect(
                m_tableViewWidget,
                &AzQtComponents::AssetFolderTableView::rowDeselected,
                this,
                []
                {
                    AssetBrowserPreviewRequestBus::Broadcast(&AssetBrowserPreviewRequest::ClearPreview);
                });

            connect(
                m_tableViewWidget,
                &AzQtComponents::AssetFolderTableView::doubleClicked,
                this,
                [this](const QModelIndex& index)
                {
                    auto indexData = index.data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserEntry*>();
                    emit entryDoubleClicked(indexData);
                });

            connect(
                m_tableViewWidget,
                &AzQtComponents::AssetFolderTableView::customContextMenuRequested,
                this,
                [this](const QPoint& pos)
                {
                    if (auto index = m_tableViewWidget->indexAt(pos); index.isValid())
                    {
                        QMenu menu(this);
                        const auto& entries = GetSelectedAssets();
                        AssetBrowserInteractionNotificationBus::Broadcast(
                            &AssetBrowserInteractionNotificationBus::Events::AddContextMenuActions, this, &menu, entries);

                        if (!menu.isEmpty())
                        {
                            menu.exec(QCursor::pos());
                        }
                    }
                    else if (!index.isValid() && m_assetTreeView)
                    {
                        m_assetTreeView->OnContextMenu(pos);
                    }
                });

            connect(
                m_tableViewDelegate,
                &TableViewDelegate::renameTableEntry,
                this,
                [this](QString name)
                {
                    AfterRename(name);
                });

            AssignWidgetToActionContextHelper(EditorIdentifiers::EditorAssetBrowserActionContextIdentifier, this);

            QAction* deleteAction = new QAction("Delete Action", this);
            deleteAction->setShortcut(QKeySequence::Delete);
            deleteAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
            connect(
                deleteAction,
                &QAction::triggered,
                this,
                [this]()
                {
                    DeleteEntries();
                });
            addAction(deleteAction);

            QAction* renameAction = new QAction("Rename Action", this);
            renameAction->setShortcut(Qt::Key_F2);
            renameAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
            connect(
                renameAction,
                &QAction::triggered,
                this,
                [this]()
                {
                    RenameEntry();
                });
            addAction(renameAction);

            QAction* duplicateAction = new QAction("Duplicate Action", this);
            duplicateAction->setShortcut(QKeySequence("Ctrl+D"));
            duplicateAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
            connect(
                duplicateAction,
                &QAction::triggered,
                this,
                [this]()
                {
                    DuplicateEntries();
                });
            addAction(duplicateAction);

            connect(
                m_tableViewWidget,
                &AzQtComponents::AssetFolderTableView::showInTableFolderTriggered,
                this,
                [this](const QModelIndex& index)
                {
                    auto indexData = index.data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserEntry*>();
                    emit showInFolderTriggered(indexData);
                });

            // Track the root index on the proxy model as well so it can provide info such as whether an entry is first level or not
            connect(
                m_tableViewWidget,
                &AzQtComponents::AssetFolderTableView::tableRootIndexChanged,
                m_tableViewProxyModel,
                &AssetBrowserTableViewProxyModel::SetRootIndex);

            auto layout = new QVBoxLayout();
            layout->addWidget(m_tableViewWidget);
            setLayout(layout);

            setAcceptDrops(true);
        }

        AssetBrowserTableView::~AssetBrowserTableView()
        {
            RemoveWidgetFromActionContextHelper(EditorIdentifiers::EditorAssetBrowserActionContextIdentifier, this);
        }

        AssetBrowserEntry::AssetEntrySortMode AssetBrowserTableView::ColumnToSortMode(const int columnIndex) const
        {
            switch (columnIndex)
            {
            case AssetBrowserTableViewProxyModel::Type:
                return AssetBrowserEntry::AssetEntrySortMode::FileType;
            case AssetBrowserTableViewProxyModel::DiskSize:
                return AssetBrowserEntry::AssetEntrySortMode::Size;
            case AssetBrowserTableViewProxyModel::Vertices:
                return AssetBrowserEntry::AssetEntrySortMode::Vertices;
            case AssetBrowserTableViewProxyModel::ApproxSize:
                return AssetBrowserEntry::AssetEntrySortMode::Dimensions;
            default:
                return AssetBrowserEntry::AssetEntrySortMode::Name;
            }
        }

        int AssetBrowserTableView::SortModeToColumn(const AssetBrowserEntry::AssetEntrySortMode sortMode) const
        {
            switch (sortMode)
            {
            case AssetBrowserEntry::AssetEntrySortMode::FileType:
                return AssetBrowserTableViewProxyModel::Type;
            case AssetBrowserEntry::AssetEntrySortMode::Size:
                return AssetBrowserTableViewProxyModel::DiskSize;
            case AssetBrowserEntry::AssetEntrySortMode::Vertices:
                return AssetBrowserTableViewProxyModel::Vertices;
            case AssetBrowserEntry::AssetEntrySortMode::Dimensions:
                return AssetBrowserTableViewProxyModel::ApproxSize;
            default:
                return 0;
            }
        }

        AzQtComponents::AssetFolderTableView* AssetBrowserTableView::GetTableViewWidget() const
        {
            return m_tableViewWidget;
        }

        void AssetBrowserTableView::SetName(const QString& name)
        {
            m_name = name;
        }

        const QString& AssetBrowserTableView::GetName() const
        {
            return m_name;
        }

        void AssetBrowserTableView::SetIsAssetBrowserMainView()
        {
            SetName(TableViewMainViewName);
        }

        bool AssetBrowserTableView::GetIsAssetBrowserMainView() const
        {
            return GetName() == TableViewMainViewName;
        }

        void AssetBrowserTableView::SetTableViewActive(bool isActiveView)
        {
            m_isActiveView = isActiveView;
        }

        bool AssetBrowserTableView::GetTableViewActive() const
        {
            return m_isActiveView;
        }

        void AssetBrowserTableView::DeleteEntries()
        {
            const auto& entries = GetSelectedAssets();
            AssetBrowserViewUtils::DeleteEntries(entries, this);
        }

        void AssetBrowserTableView::MoveEntries()
        {
            const auto& entries = GetSelectedAssets();
            AssetBrowserViewUtils::MoveEntries(entries, this);
        }

        void AssetBrowserTableView::DuplicateEntries()
        {
            const auto& entries = GetSelectedAssets();
            AssetBrowserViewUtils::DuplicateEntries(entries);
        }

        void AssetBrowserTableView::RenameEntry()
        {
            const auto& entries = GetSelectedAssets();
            if (AssetBrowserViewUtils::RenameEntry(entries, this))
            {
                QModelIndex selectedIndex = m_tableViewWidget->selectionModel()->selectedIndexes()[0];
                m_tableViewWidget->edit(selectedIndex);
            }
        }

        void AssetBrowserTableView::AfterRename(QString newVal)
        {
            const auto& entries = GetSelectedAssets();
            AssetBrowserViewUtils::AfterRename(newVal, entries, this);
        }

        AZStd::vector<const AssetBrowserEntry*> AssetBrowserTableView::GetSelectedAssets() const
        {
            // No need to check for product assets since they do not appear in the table view
            AZStd::vector<const AssetBrowserEntry*> entries;
            if (m_tableViewWidget->selectionModel())
            {
                const auto& indexes = m_tableViewWidget->selectionModel()->selectedRows();
                entries.reserve(indexes.size());

                for (const auto& index : indexes)
                {
                    const AssetBrowserEntry* item = index.data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserEntry*>();
                    if (item)
                    {
                        entries.push_back(item);
                    }
                }
            }
            return entries;
        }

        void AssetBrowserTableView::OpenItemForEditing(const QModelIndex& index)
        {
            QModelIndex proxyIndex = m_tableViewProxyModel->mapFromSource(m_assetFilterModel->mapFromSource(index));

            if (proxyIndex.isValid())
            {
                m_tableViewWidget->selectionModel()->select(
                    proxyIndex, QItemSelectionModel::SelectionFlag::ClearAndSelect | QItemSelectionModel::Rows);

                m_tableViewWidget->scrollTo(proxyIndex, QAbstractItemView::ScrollHint::PositionAtCenter);

                RenameEntry();
            }
        }

        void AssetBrowserTableView::dragEnterEvent(QDragEnterEvent* event)
        {
            if (event->mimeData()->hasFormat(SourceAssetBrowserEntry::GetMimeType()) ||
                event->mimeData()->hasFormat(ProductAssetBrowserEntry::GetMimeType()))
            {
                event->accept();
                return;
            }

            using namespace AzQtComponents;
            DragAndDropContextBase context;
            DragAndDropEventsBus::Event(DragAndDropContexts::EditorMainWindow, &DragAndDropEvents::DragEnter, event, context);
        }

        void AssetBrowserTableView::dragMoveEvent(QDragMoveEvent* event)
        {
            if (event->mimeData()->hasFormat(SourceAssetBrowserEntry::GetMimeType()) ||
                event->mimeData()->hasFormat(ProductAssetBrowserEntry::GetMimeType()))
            {
                event->accept();
                return;
            }

            using namespace AzQtComponents;
            DragAndDropContextBase context;
            DragAndDropEventsBus::Event(DragAndDropContexts::EditorMainWindow, &DragAndDropEvents::DragMove, event, context);
        }

        void AssetBrowserTableView::dropEvent(QDropEvent* event)
        {
            if (event->mimeData()->hasFormat(SourceAssetBrowserEntry::GetMimeType()) ||
                event->mimeData()->hasFormat(ProductAssetBrowserEntry::GetMimeType()))
            {
                event->accept();
                return;
            }

            const AssetBrowserEntry* item = m_tableViewProxyModel->mapToSource(m_tableViewProxyModel->GetRootIndex())
                                                .data(AssetBrowserModel::Roles::EntryRole)
                                                .value<const AssetBrowserEntry*>();
            AZStd::string pathName = item->GetFullPath();

            using namespace AzQtComponents;
            DragAndDropContextBase context;
            DragAndDropEventsBus::Event(
                DragAndDropContexts::EditorMainWindow, &DragAndDropEvents::DropAtLocation, event, context, QString(pathName.data()));
        }

        void AssetBrowserTableView::dragLeaveEvent(QDragLeaveEvent* event)
        {
            using namespace AzQtComponents;
            DragAndDropEventsBus::Event(DragAndDropContexts::EditorMainWindow, &DragAndDropEvents::DragLeave, event);
        }

        void AssetBrowserTableView::SetAssetTreeView(AssetBrowserTreeView* treeView)
        {
            if (m_assetTreeView)
            {
                disconnect(m_assetTreeView, &AssetBrowserTreeView::selectionChangedSignal, this, nullptr);
                auto treeViewFilterModel = qobject_cast<AssetBrowserFilterModel*>(m_assetTreeView->model());
                if (treeViewFilterModel)
                {
                    disconnect(
                        treeViewFilterModel,
                        &AssetBrowserFilterModel::filterChanged,
                        this,
                        &AssetBrowserTableView::UpdateFilterInLocalFilterModel);
                }
            }

            m_assetTreeView = treeView;
            if (!m_assetTreeView)
            {
                return;
            }

            m_assetTreeView->SetAttachedTableView(this);

            auto treeViewFilterModel = qobject_cast<AssetBrowserFilterModel*>(m_assetTreeView->model());
            if (!treeViewFilterModel)
            {
                return;
            }

            auto treeViewModel = qobject_cast<AssetBrowserModel*>(treeViewFilterModel->sourceModel());
            if (!treeViewModel)
            {
                return;
            }

            m_assetFilterModel->setSourceModel(treeViewModel);
            UpdateFilterInLocalFilterModel();

            connect(
                treeViewFilterModel,
                &AssetBrowserFilterModel::filterChanged,
                this,
                &AssetBrowserTableView::UpdateFilterInLocalFilterModel);

            connect(
                m_assetTreeView,
                &AssetBrowserTreeView::selectionChangedSignal,
                this,
                &AssetBrowserTableView::HandleTreeViewSelectionChanged);
        }

        void AssetBrowserTableView::setSelectionMode(QAbstractItemView::SelectionMode mode)
        {
            m_tableViewWidget->setSelectionMode(mode);
        }

        QAbstractItemView::SelectionMode AssetBrowserTableView::selectionMode() const
        {
            return m_tableViewWidget->selectionMode();
        }

        void AssetBrowserTableView::SelectEntry(QString assetName)
        {
            QModelIndex rootIndex = m_tableViewProxyModel->GetRootIndex();

            auto model = GetTableViewWidget()->model();

            for (int rowIndex = 0; rowIndex < model->rowCount(rootIndex); rowIndex++)
            {
                auto index = model->index(rowIndex, 0, rootIndex);
                if (!index.isValid())
                {
                    continue;
                }

                auto str = index.data().toString();
                if (assetName == str)
                {
                    m_tableViewWidget->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
                    m_tableViewWidget->scrollTo(index, QAbstractItemView::ScrollHint::PositionAtCenter);
                }
            }
        }

        void AssetBrowserTableView::HandleTreeViewSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
        {
            Q_UNUSED(deselected);

            auto treeViewFilterModel = qobject_cast<AssetBrowserFilterModel*>(m_assetTreeView->model());
            if (!treeViewFilterModel)
            {
                return;
            }

            const auto& selectedIndexes = selected.indexes();
            if (!selectedIndexes.empty())
            {
                auto newRootIndex = m_tableViewProxyModel->mapFromSource(
                    m_assetFilterModel->mapFromSource(treeViewFilterModel->mapToSource(selectedIndexes[0])));
                m_tableViewWidget->setRootIndex(newRootIndex);
            }
            else
            {
                m_tableViewWidget->setRootIndex({});
            }

            m_assetFilterModel->sort(0, Qt::DescendingOrder);
        }

        void AssetBrowserTableView::UpdateFilterInLocalFilterModel()
        {
            if (!m_assetTreeView)
            {
                return;
            }

            auto treeViewFilterModel = qobject_cast<AssetBrowserFilterModel*>(m_assetTreeView->model());
            if (!treeViewFilterModel)
            {
                return;
            }

            auto filter = qobject_cast<const CompositeFilter*>(treeViewFilterModel->GetFilter().get());
            if (!filter)
            {
                return;
            }

            bool hasString{ false };
            const QString tagString("String");
            const QString tagFolder("Folder");
            auto clonedFilter = new CompositeFilter(CompositeFilter::LogicOperatorType::AND);
            for (const auto& subFilter : filter->GetSubFilters())
            {
                if (subFilter->GetTag() == tagString)
                {
                    auto stringCompFilter = qobject_cast<const CompositeFilter*>(subFilter.get());
                    hasString |= stringCompFilter && !stringCompFilter->GetSubFilters().empty();
                }

                // Skip the folder filter on the table view so that we can see files
                if (subFilter->GetTag() != tagFolder)
                {
                    clonedFilter->AddFilter(FilterConstType(subFilter->Clone()));
                }
            }

            // Switch between "search mode" where all results in the asset folder tree are shown,
            // and "normal mode", where only contents for a single folder are shown, depending on
            // whether there is an active string search ongoing.
            m_tableViewProxyModel->SetShowSearchResultsMode(hasString);
            m_tableViewWidget->SetShowSearchResultsMode(hasString);

            if (hasString)
            {
                // With string searches enabled we disable filter propagation to only show exact mattress.
                for (auto subFilter : clonedFilter->GetSubFilters())
                {
                    if (auto clonedCompositeFilter = qobject_cast<const CompositeFilter*>(subFilter.get()); clonedCompositeFilter)
                    {
                        auto modifiedCompositeFilter = const_cast<CompositeFilter*>(clonedCompositeFilter);
                        modifiedCompositeFilter->SetFilterPropagation(AssetBrowserEntryFilter::PropagateDirection::None);
                    }
                }

                // With string searches enabled we only want to show sources and products in this view.
                auto customTypeFilter = new CustomFilter(
                    [](const AssetBrowserEntry* entry)
                    {
                        switch (entry->GetEntryType())
                        {
                        case AssetBrowserEntry::AssetEntryType::Product:
                        case AssetBrowserEntry::AssetEntryType::Source:
                            return true;
                        }
                        return false;
                    });
                clonedFilter->AddFilter(FilterConstType(customTypeFilter));

                clonedFilter->SetFilterPropagation(AssetBrowserEntryFilter::PropagateDirection::Down);
                m_tableViewWidget->expandAll();
                m_tableViewProxyModel->setSourceModel(m_treeToTableProxyModel);
                m_treeToTableProxyModel->ConnectSignals();
                m_assetFilterModel->SetFilter(FilterConstType(clonedFilter));
            }
            else
            {
                clonedFilter->SetFilterPropagation(AssetBrowserEntryFilter::PropagateDirection::Down);
                m_assetFilterModel->SetFilter(FilterConstType(clonedFilter));
                m_tableViewWidget->collapseAll();
                m_tableViewProxyModel->setSourceModel(m_assetFilterModel);
                m_treeToTableProxyModel->DisconnectSignals();
            }
        }

        void AssetBrowserTableView::SetSortMode(const AssetBrowserEntry::AssetEntrySortMode mode)
        {
            // If the sort mode is being set to the same value then alternate the sort order
            if (m_assetFilterModel->GetSortMode() == mode)
            {
                m_assetFilterModel->SetSortOrder(
                    m_assetFilterModel->GetSortOrder() == Qt::DescendingOrder ? Qt::AscendingOrder : Qt::DescendingOrder);
            }

            m_assetFilterModel->SetSortMode(mode);
            m_assetFilterModel->sort(0, m_assetFilterModel->GetSortOrder());
            m_assetFilterModel->setDynamicSortFilter(true);

            m_tableViewWidget->header()->setSortIndicator(
                SortModeToColumn(m_assetFilterModel->GetSortMode()), m_assetFilterModel->GetSortOrder());
        }

        AssetBrowserEntry::AssetEntrySortMode AssetBrowserTableView::GetSortMode() const
        {
            return m_assetFilterModel->GetSortMode();
        }

        void AssetBrowserTableView::SetSearchString(const QString& searchString)
        {
            m_tableViewProxyModel->SetSearchString(searchString);
        }

        TableViewDelegate::TableViewDelegate(QWidget* parent)
            : QStyledItemDelegate(parent)
        {
        }

        void TableViewDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
        {
            const QVariant text = index.data(Qt::DisplayRole);
            if (text.isValid())
            {
                QStyleOptionViewItem options{ option };
                initStyleOption(&options, index);
                int height = options.rect.height();
                QRect iconRect(0, options.rect.y() + 5, height - 10, height - 10);
                QSize iconSize = iconRect.size();

                const auto& qVariant = index.data(Qt::UserRole + 1);
                if (!qVariant.isNull())
                {
                    QIcon icon;
                    if (const auto& path = qVariant.value<QString>(); !path.isEmpty())
                    {
                        icon.addFile(path, iconSize, QIcon::Normal, QIcon::Off);
                        icon.paint(painter, iconRect, Qt::AlignLeft | Qt::AlignVCenter);
                    }
                    else if (const auto& pixmap = qVariant.value<QPixmap>(); !pixmap.isNull())
                    {
                        icon.addPixmap(pixmap.scaled(iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation), QIcon::Normal, QIcon::Off);
                        icon.paint(painter, iconRect, Qt::AlignLeft | Qt::AlignVCenter);
                    }
                }
                QRect textRect{ options.rect };
                textRect.setX(textRect.x() + 4);

                QStyleOptionViewItem optionV4{ option };
                initStyleOption(&optionV4, index);
                optionV4.state &= ~(QStyle::State_HasFocus | QStyle::State_Selected);

                RichTextHighlighter::PaintHighlightedRichText(text.toString(), painter, optionV4, textRect);
            }
        }

        QWidget* TableViewDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
        {
            QWidget* widget = QStyledItemDelegate::createEditor(parent, option, index);
            if (auto* lineEdit = qobject_cast<QLineEdit*>(widget))
            {
                connect(
                    lineEdit,
                    &QLineEdit::editingFinished,
                    this,
                    [this]()
                    {
                        auto sendingLineEdit = qobject_cast<QLineEdit*>(sender());
                        if (sendingLineEdit)
                        {
                            emit renameTableEntry(sendingLineEdit->text());
                        }
                    });
            }
            return widget;
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
