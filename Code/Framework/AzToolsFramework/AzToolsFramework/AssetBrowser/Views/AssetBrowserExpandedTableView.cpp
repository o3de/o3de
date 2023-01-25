/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserExpandedTableView.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserExpandedTableViewProxyModel.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTreeView.h>

#include <AzQtComponents/Components/Widgets/AssetFolderExpandedTableView.h>

#if !defined(Q_MOC_RUN)
#include <QVBoxLayout>

#endif

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
#pragma optimize("", off)
#if 0
        AssetBrowserExpandedTableView::AssetBrowserExpandedTableView(QWidget* parent)
            : QWidget(parent)
            , m_expandedTableViewWidget(new AzQtComponents::AssetFolderExpandedTableView(parent))
            , m_expandedTableViewModel(new AssetBrowserExpandedTableViewModel(parent))
            , m_assetFilterModel(new AssetBrowserFilterModel(parent))
        {
            // Using our own instance of AssetBrowserFilterModel to be able to show also files when the main model
            // only lists directories, and at the same time get sort and filter entries features from AssetBrowserFilterModel.
            m_assetFilterModel->sort(0, Qt::DescendingOrder);
            m_expandedTableViewModel->setSourceModel(m_assetFilterModel);
            m_expandedTableViewWidget->setModel(m_expandedTableViewModel);

                  const QStringList horizontalHeaderLabels = {
                "Name", "Type", "Disk Size", "Related Asset", "Vertices", "Approximate Size", "Source control Status"
            };
            setColumnCount(horizontalHeaderLabels.count());
            setHorizontalHeaderLabels(horizontalHeaderLabels);



            connect(
                m_expandedTableViewWidget,
                &AzQtComponents::AssetFolderExpandedTableView::clicked,
                this,
                [this](const QModelIndex& index)
                {
                    auto indexData = index.data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserEntry*>();
                    if (indexData->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source)
                    {
                        AssetBrowserPreviewRequestBus::Broadcast(&AssetBrowserPreviewRequest::PreviewAsset, indexData);
                    }
                    emit entryClicked(indexData);
                });

            connect(
                m_expandedTableViewWidget,
                &AzQtComponents::AssetFolderExpandedTableView::doubleClicked,
                this,
                [this](const QModelIndex& index)
                {
                    auto indexData = index.data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserEntry*>();
                    emit entryDoubleClicked(indexData);
                });

            connect(
                m_expandedTableViewWidget,
                &AzQtComponents::AssetFolderExpandedTableView::showInFolderTriggered,
                this,
                [this](const QModelIndex& index)
                {
                    auto indexData = index.data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserEntry*>();
                    emit showInFolderTriggered(indexData);
                });

            // Track the root index on the proxy model as well so it can provide info such as whether an entry is first level or not
            connect(
                m_expandedTableViewWidget,
                &AzQtComponents::AssetFolderExpandedTableView::rootIndexChanged,
                m_expandedTableViewModel,
                &AssetBrowserExpandedTableViewModel::SetRootIndex);

            auto layout = new QVBoxLayout();
            layout->addWidget(m_expandedTableViewWidget);
            setLayout(layout);
        }

        AssetBrowserExpandedTableView::~AssetBrowserExpandedTableView() = default;

        AzQtComponents::AssetFolderExpandedTableView* AssetBrowserExpandedTableView::GetExpandedTableViewWidget() const
        {
            return m_expandedTableViewWidget;
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
//JJS                auto newRootIndex = m_expandedTableViewModel->mapFromSource(
//JJS                    m_assetFilterModel->mapFromSource(treeViewFilterModel->mapToSource(selectedIndexes[0])));
                auto newRootIndex = m_assetFilterModel->mapFromSource(treeViewFilterModel->mapToSource(selectedIndexes[0]));
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

                    m_expandedTableViewModel->SetShowSearchResultsMode(stringSubFilters.count() != 0);
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
#else
        AssetBrowserExpandedTableView::AssetBrowserExpandedTableView(QWidget* parent)
            : AzQtComponents::TableView(parent)
            , m_expandedTableViewProxyModel(new AssetBrowserExpandedTableViewProxyModel(parent))
            , m_assetFilterModel(new AssetBrowserFilterModel(parent))
        {
            // Using our own instance of AssetBrowserFilterModel to be able to show also files when the main model
            // only lists directories, and at the same time get sort and filter entries features from AssetBrowserFilterModel.
            m_expandedTableViewProxyModel->setSourceModel(m_assetFilterModel);
            setModel(m_expandedTableViewProxyModel);
        }

        AssetBrowserExpandedTableView::~AssetBrowserExpandedTableView() = default;

        void AssetBrowserExpandedTableView::SetAssetTreeView([[maybe_unused]] AssetBrowserTreeView* treeView)
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

//                    m_expandedTableViewModel->SetShowSearchResultsMode(stringSubFilters.count() != 0);
                    SetShowSearchResultsMode(stringSubFilters.count() != 0);
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
                //auto newRootIndex = m_expandedTableViewModel->mapFromSource(
                //    m_assetFilterModel->mapFromSource(treeViewFilterModel->mapToSource(selectedIndexes[0])));
            }
            else
            {
//                m_expandedTableViewWidget->setRootIndex({});
            }
        }

        void AssetBrowserExpandedTableView::SetShowSearchResultsMode(bool searchMode)
        {
            m_showSearchResultsMode = searchMode;
        }


#endif
#pragma optimize("", on)
    } // namespace AssetBrowser
} // namespace AzToolsFramework
