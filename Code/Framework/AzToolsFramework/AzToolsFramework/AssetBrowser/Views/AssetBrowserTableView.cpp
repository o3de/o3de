/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTableView.h>

#include <AzToolsFramework/ActionManager/HotKey/HotKeyManagerInterface.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserTableFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserTableViewProxyModel.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTreeView.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserViewUtils.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerUtils.h>
#include <AzCore/Utils/Utils.h>

#include <AzQtComponents/Components/Widgets/AssetFolderTableView.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzFramework/StringFunc/StringFunc.h>

#if !defined(Q_MOC_RUN)
#include <QVBoxLayout>
#include <QtWidgets/QApplication>
#include <QHeaderView>
#include <QLineEdit>
#include <QMenu>
#endif

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        static constexpr const char* const ExpandedTableViewMainViewName = "AssetBrowserExpandedTableView_main";

        AssetBrowserTableView::AssetBrowserTableView(QWidget* parent)
            : QWidget(parent)
            , m_expandedTableViewWidget(new AzQtComponents::AssetFolderTableView(parent))
            , m_expandedTableViewProxyModel(new AssetBrowserTableViewProxyModel(parent))
            , m_assetFilterModel(new AssetBrowserTableFilterModel(parent))
            , m_expandedTableViewDelegate(new ExpandedTableViewDelegate(m_expandedTableViewWidget))
        {
            // Using our own instance of AssetBrowserFilterModel to be able to show also files when the main model
            // only lists directories, and at the same time get sort and filter entries features from AssetBrowserFilterModel.
            m_assetFilterModel->sort(0, Qt::DescendingOrder);
            m_expandedTableViewProxyModel->setSourceModel(m_assetFilterModel);
            m_expandedTableViewWidget->setModel(m_expandedTableViewProxyModel);
            m_expandedTableViewWidget->setItemDelegateForColumn(0, m_expandedTableViewDelegate);
            for (int i = 0; i < m_expandedTableViewWidget->header()->count(); ++i)
            {
                m_expandedTableViewWidget->header()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
            }

            connect(
                m_expandedTableViewWidget,
                &AzQtComponents::AssetFolderTableView::clicked,
                this,
                [this](const QModelIndex& index)
                {
                    auto indexData = index.data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserEntry*>();
                    if (indexData->GetEntryType() != AssetBrowserEntry::AssetEntryType::Folder)
                    {
                        AssetBrowserPreviewRequestBus::Broadcast(&AssetBrowserPreviewRequest::PreviewAsset, indexData);
                    }
                    emit entryClicked(indexData);
                });

             connect(
                m_expandedTableViewWidget,
                &AzQtComponents::AssetFolderTableView::doubleClicked,
                this,
                [this](const QModelIndex& index)
                {
                    auto indexData = index.data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserEntry*>();
                    emit entryDoubleClicked(indexData);
                });

             connect(
                 m_expandedTableViewWidget,
                 &AzQtComponents::AssetFolderTableView::customContextMenuRequested,
                 this,
                 [this](const QPoint& pos)
                 {
                     if (auto index = m_expandedTableViewWidget->indexAt(pos); index.isValid())
                     {
                         QMenu menu(this);
                         AZStd::vector<const AssetBrowserEntry*> entries = GetSelectedAssets();
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
                 m_expandedTableViewDelegate,
                 &ExpandedTableViewDelegate::renameTableEntry, this,
                 [this](QString name)
                 {
                     AfterRename(name);
                 });

              if (AzToolsFramework::IsNewActionManagerEnabled())
              {
                AssignWidgetToActionContextHelper(EditorIdentifiers::EditorAssetBrowserActionContextIdentifier, this);
              }

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
                m_expandedTableViewWidget,
                 &AzQtComponents::AssetFolderTableView::showInTableFolderTriggered,
                this,
                [this](const QModelIndex& index)
                {
                    auto indexData = index.data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserEntry*>();
                    emit showInFolderTriggered(indexData);
                });

            // Track the root index on the proxy model as well so it can provide info such as whether an entry is first level or not
            connect(
                m_expandedTableViewWidget,
                &AzQtComponents::AssetFolderTableView::tableRootIndexChanged,
                m_expandedTableViewProxyModel,
                &AssetBrowserTableViewProxyModel::SetRootIndex);

            auto layout = new QVBoxLayout();
            layout->addWidget(m_expandedTableViewWidget);
            setLayout(layout);
        }

        AssetBrowserTableView::~AssetBrowserTableView()
        {
            if (AzToolsFramework::IsNewActionManagerEnabled())
            {
                RemoveWidgetFromActionContextHelper(EditorIdentifiers::EditorAssetBrowserActionContextIdentifier, this);
            }
        }

        AzQtComponents::AssetFolderTableView* AssetBrowserTableView::GetExpandedTableViewWidget() const
        {
            return m_expandedTableViewWidget;
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
            SetName(ExpandedTableViewMainViewName);
        }

        bool AssetBrowserTableView::GetIsAssetBrowserMainView() const
        {
            return GetName() == ExpandedTableViewMainViewName;
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
            auto entries = GetSelectedAssets();

            AssetBrowserViewUtils::DeleteEntries(entries, this);
        }

        void AssetBrowserTableView::MoveEntries()
        {
            auto entries = GetSelectedAssets();

            AssetBrowserViewUtils::MoveEntries(entries, this);
        }

        void AssetBrowserTableView::DuplicateEntries()
        {
            auto entries = GetSelectedAssets();
            AssetBrowserViewUtils::DuplicateEntries(entries);
        }

        void AssetBrowserTableView::RenameEntry()
        {
            auto entries = GetSelectedAssets();

            if (AssetBrowserViewUtils::RenameEntry(entries, this))
            {
                QModelIndex selectedIndex = m_expandedTableViewWidget->selectionModel()->selectedIndexes()[0];
                m_expandedTableViewWidget->edit(selectedIndex);
            }
        }

        void AssetBrowserTableView::AfterRename(QString newVal)
        {
            auto entries = GetSelectedAssets();

            AssetBrowserViewUtils::AfterRename(newVal, entries, this);
        }

        AZStd::vector<const AssetBrowserEntry*> AssetBrowserTableView::GetSelectedAssets() const
        {
            // No need to check for product assets since they do not appear in the table view
            AZStd::vector<const AssetBrowserEntry*> entries;
            if (m_expandedTableViewWidget->selectionModel())
            {
                auto indexes = m_expandedTableViewWidget->selectionModel()->selectedRows();
                for (const auto index : indexes)
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
            QModelIndex proxyIndex = m_expandedTableViewProxyModel->mapFromSource(m_assetFilterModel->mapFromSource(index));

            if (proxyIndex.isValid())
            {
                m_expandedTableViewWidget->selectionModel()->select(proxyIndex, QItemSelectionModel::SelectionFlag::ClearAndSelect | QItemSelectionModel::Rows);

                m_expandedTableViewWidget->scrollTo(proxyIndex, QAbstractItemView::ScrollHint::PositionAtCenter);

                RenameEntry();
            }
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
            m_expandedTableViewWidget->setSelectionMode(mode);
        }

        QAbstractItemView::SelectionMode AssetBrowserTableView::selectionMode() const
        {
            return m_expandedTableViewWidget->selectionMode();
        }

        void AssetBrowserTableView::SelectEntry(QString assetName)
        {
            QModelIndex rootIndex = m_expandedTableViewProxyModel->GetRootIndex();

            auto model = GetExpandedTableViewWidget()->model();

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
                    m_expandedTableViewWidget->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
                    m_expandedTableViewWidget->scrollTo(index, QAbstractItemView::ScrollHint::PositionAtCenter);
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

            auto selectedIndexes = selected.indexes();
            if (selectedIndexes.count() > 0)
            {
                auto newRootIndex = m_expandedTableViewProxyModel->mapFromSource(
                    m_assetFilterModel->mapFromSource(treeViewFilterModel->mapToSource(selectedIndexes[0])));
                m_expandedTableViewWidget->setRootIndex(newRootIndex);
            }
            else
            {
                m_expandedTableViewWidget->setRootIndex({});
            }
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

            auto filterCopy = new CompositeFilter(CompositeFilter::LogicOperatorType::AND);
            for (const auto& subFilter : filter->GetSubFilters())
            {
                // Switch between "search mode" where all results in the asset folder tree are shown,
                // and "normal mode", where only contents for a single folder are shown, depending on
                // whether there is an active string search ongoing.
                if (subFilter->GetTag() == "String")
                {
                    auto stringCompFilter = qobject_cast<const CompositeFilter*>(subFilter.get());
                    if (!stringCompFilter)
                    {
                        continue;
                    }

                    auto stringSubFilters = stringCompFilter->GetSubFilters();

                    m_expandedTableViewProxyModel->SetShowSearchResultsMode(stringSubFilters.count() != 0);
                    m_expandedTableViewWidget->SetShowSearchResultsMode(stringSubFilters.count() != 0);
                }

                // Skip the folder filter on the thumbnail view so that we can see files
                if (subFilter->GetTag() != "Folder")
                {
                    filterCopy->AddFilter(subFilter);
                }
            }
            filterCopy->SetFilterPropagation(AssetBrowserEntryFilter::Up | AssetBrowserEntryFilter::Down);
            m_assetFilterModel->SetFilter(FilterConstType(filterCopy));
        }

        ExpandedTableViewDelegate::ExpandedTableViewDelegate(QWidget* parent)
            : QStyledItemDelegate(parent)

        {
        }

        void ExpandedTableViewDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
        {
            const QVariant text = index.data(Qt::DisplayRole);
            if (text.isValid())
            {
                QStyleOptionViewItem options{ option };
                initStyleOption(&options, index);
                int height = options.rect.height();
                QRect iconRect(0, options.rect.y() + 5, height - 10, height - 10);
                QSize iconSize = iconRect.size();
                QStyle* style = options.widget ? options.widget->style() : qApp->style();

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
                QRect textRect{options.rect};
                textRect.setX(textRect.x() + 4);
                style->drawItemText(
                    painter, options.rect, Qt::AlignLeft | Qt::AlignVCenter, options.palette, options.state & QStyle::State_Enabled, text.toString());
            }
        }

        QWidget* ExpandedTableViewDelegate::createEditor(
            QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
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
