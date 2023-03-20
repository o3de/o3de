/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserExpandedTableView.h>

#include <AzToolsFramework/ActionManager/HotKey/HotKeyManagerInterface.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserExpandedFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserExpandedTableViewProxyModel.h>
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

        AssetBrowserExpandedTableView::AssetBrowserExpandedTableView(QWidget* parent)
            : QWidget(parent)
            , m_expandedTableViewWidget(new AzQtComponents::AssetFolderTableView(parent))
            , m_expandedTableViewProxyModel(new AssetBrowserExpandedTableViewProxyModel(parent))
            , m_assetFilterModel(new AssetBrowserExpandedFilterModel(parent))
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
                    AssetBrowserPreviewRequestBus::Broadcast(&AssetBrowserPreviewRequest::PreviewAsset, indexData);
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
                         const AssetBrowserEntry* entry = index.data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserEntry*>();
                         AZStd::vector<const AssetBrowserEntry*> entries{ entry };
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
                if (auto hotKeyManagerInterface = AZ::Interface<AzToolsFramework::HotKeyManagerInterface>::Get())
                {
                    // Assign this widget to the Editor Asset Browser Action Context.
                    hotKeyManagerInterface->AssignWidgetToActionContext(EditorIdentifiers::EditorAssetBrowserActionContextIdentifier, this);
                }
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
                &AssetBrowserExpandedTableViewProxyModel::SetRootIndex);

            auto layout = new QVBoxLayout();
            layout->addWidget(m_expandedTableViewWidget);
            setLayout(layout);
        }

        AssetBrowserExpandedTableView::~AssetBrowserExpandedTableView()
        {
            if (AzToolsFramework::IsNewActionManagerEnabled())
            {
                if (auto hotKeyManagerInterface = AZ::Interface<AzToolsFramework::HotKeyManagerInterface>::Get())
                {
                    hotKeyManagerInterface->RemoveWidgetFromActionContext(
                        EditorIdentifiers::EditorAssetBrowserActionContextIdentifier, this);
                }
            }
        }

        AzQtComponents::AssetFolderTableView* AssetBrowserExpandedTableView::GetExpandedTableViewWidget() const
        {
            return m_expandedTableViewWidget;
        }

         void AssetBrowserExpandedTableView::SetName(const QString& name)
        {
            m_name = name;
        }

        const QString& AssetBrowserExpandedTableView::GetName() const
        {
            return m_name;
        }

        void AssetBrowserExpandedTableView::SetIsAssetBrowserMainView()
        {
            SetName(ExpandedTableViewMainViewName);
        }

        bool AssetBrowserExpandedTableView::GetIsAssetBrowserMainView() const
        {
            return GetName() == ExpandedTableViewMainViewName;
        }

        void AssetBrowserExpandedTableView::SetExpandedTableViewActive(bool isActiveView)
        {
            m_isActiveView = isActiveView;
        }

        bool AssetBrowserExpandedTableView::GetExpandedTableViewActive() const
        {
            return m_isActiveView;
        }

        void AssetBrowserExpandedTableView::DeleteEntries()
        {
            auto entries = GetSelectedAssets();

            AssetBrowserViewUtils::DeleteEntries(entries, this);
        }

        void AssetBrowserExpandedTableView::MoveEntries()
        {
            auto entries = GetSelectedAssets();

            AssetBrowserViewUtils::MoveEntries(entries, this);
        }

        void AssetBrowserExpandedTableView::DuplicateEntries()
        {
            auto entries = GetSelectedAssets();
            AssetBrowserViewUtils::DuplicateEntries(entries);
        }

        void AssetBrowserExpandedTableView::RenameEntry()
        {
            auto entries = GetSelectedAssets();

            if (AssetBrowserViewUtils::RenameEntry(entries, this))
            {
                QModelIndex selectedIndex = m_expandedTableViewWidget->selectionModel()->selectedIndexes()[0];
                m_expandedTableViewWidget->edit(selectedIndex);
            }
        }

        void AssetBrowserExpandedTableView::AfterRename(QString newVal)
        {
            auto entries = GetSelectedAssets();

            AssetBrowserViewUtils::AfterRename(newVal, entries, this);
        }

        AZStd::vector<const AssetBrowserEntry*> AssetBrowserExpandedTableView::GetSelectedAssets() const
        {
            // There are no product assets in the expanded table view, just get the first item selected
            AZStd::vector<const AssetBrowserEntry*> entries;
            if (m_expandedTableViewWidget->selectionModel())
            {
                auto index = m_expandedTableViewWidget->selectionModel()->selectedIndexes()[0];
                const AssetBrowserEntry* item = index.data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserEntry*>();
                if (item)
                {
                    entries.push_back(item);
                }
            }
            return entries;
        }

        void AssetBrowserExpandedTableView::OpenItemForEditing(const QModelIndex& index)
        {
            QModelIndex proxyIndex = m_expandedTableViewProxyModel->mapFromSource(m_assetFilterModel->mapFromSource(index));

            if (proxyIndex.isValid())
            {
                m_expandedTableViewWidget->selectionModel()->select(proxyIndex, QItemSelectionModel::SelectionFlag::ClearAndSelect);

                m_expandedTableViewWidget->scrollTo(proxyIndex, QAbstractItemView::ScrollHint::PositionAtCenter);

                RenameEntry();
            }
        }

        void AssetBrowserExpandedTableView::SetAssetTreeView(AssetBrowserTreeView* treeView)
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
                        &AssetBrowserExpandedTableView::UpdateFilterInLocalFilterModel);
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
                &AssetBrowserExpandedTableView::UpdateFilterInLocalFilterModel);

            connect(
                m_assetTreeView,
                &AssetBrowserTreeView::selectionChangedSignal,
                this,
                &AssetBrowserExpandedTableView::HandleTreeViewSelectionChanged);
        }

        void AssetBrowserExpandedTableView::setSelectionMode(QAbstractItemView::SelectionMode mode)
        {
            m_expandedTableViewWidget->setSelectionMode(mode);
        }

        QAbstractItemView::SelectionMode AssetBrowserExpandedTableView::selectionMode() const
        {
            return m_expandedTableViewWidget->selectionMode();
        }

        void AssetBrowserExpandedTableView::HandleTreeViewSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
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

        void AssetBrowserExpandedTableView::UpdateFilterInLocalFilterModel()
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
                QString iconPathToUse;
                QStyleOptionViewItem options{ option };
                initStyleOption(&options, index);

                const QVariant path = index.data(Qt::UserRole);

                AZ::EBusAggregateResults<SourceFileDetails> results;
                AssetBrowserInteractionNotificationBus::BroadcastResult(
                                    results, &AssetBrowserInteractionNotificationBus::Events::GetSourceFileDetails, path.toString().toUtf8().constData());

                auto it = AZStd::find_if( results.values.begin(), results.values.end(),
                    [](const SourceFileDetails& details)
                    {
                        return !details.m_sourceThumbnailPath.empty();
                    });

                const bool isFolder = index.data(Qt::UserRole + 1).toBool();

                if (it != results.values.end() || isFolder)
                {
                    static constexpr const char* FolderIconPath = "Icons/AssetBrowser/Folder_16.svg";
                    const char* resultPath = isFolder ? FolderIconPath : it->m_sourceThumbnailPath.c_str();
                    // its an ordered bus, though, so first one wins.
                    // we have to massage this though.  there are three valid possibilities
                    // 1. its a relative path to source, in which case we have to find the full path
                    // 2. its an absolute path, in which case we use it as-is
                    // 3. its an embedded resource, in which case we use it as is.

                    // is it an embedded resource or absolute path?
                    if ((resultPath[0] == ':') || (!AzFramework::StringFunc::Path::IsRelative(resultPath)))
                    {
                        iconPathToUse = QString::fromUtf8(resultPath);
                    }
                    else
                    {
                        // getting here means its a relative path.  Can we find the real path of the file?  This also searches in gems for
                        // sources.
                        bool foundIt = false;
                        AZ::Data::AssetInfo info;
                        AZStd::string watchFolder;
                        AssetSystemRequestBus::BroadcastResult(
                            foundIt, &AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, resultPath, info, watchFolder);

                        AZ_WarningOnce(
                            "Asset Browser", foundIt, "Unable to find source icon file in any source folders or gems: %s\n", resultPath);

                        if (foundIt)
                        {
                            // the absolute path is join(watchfolder, relativepath); // since its relative to the watch folder.
                            AZStd::string finalPath;
                            AzFramework::StringFunc::Path::Join(watchFolder.c_str(), info.m_relativePath.c_str(), finalPath);
                            iconPathToUse = QString::fromUtf8(finalPath.c_str());
                        }
                    }
                }

                if (iconPathToUse.isEmpty())
                {
                    static constexpr const char* DefaultFileIconPath = "Assets/Editor/Icons/AssetBrowser/Default_16.svg";
                    AZ::IO::FixedMaxPath engineRoot = AZ::Utils::GetEnginePath();
                    AZ_Assert(!engineRoot.empty(), "Engine Root not initialized");
                    iconPathToUse = (engineRoot / DefaultFileIconPath).c_str();
                }

                QIcon icon;
                icon.addFile(iconPathToUse, QSize(), QIcon::Normal, QIcon::Off);
                int height = options.rect.height();
                QRect iconRect(0, options.rect.y() + 5, height - 10, height - 10);
                QSize iconSize = iconRect.size();
                QStyle* style = options.widget ? options.widget->style() : qApp->style();
                style->drawItemPixmap(painter, iconRect, Qt::AlignLeft | Qt::AlignVCenter, icon.pixmap(iconSize));
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
