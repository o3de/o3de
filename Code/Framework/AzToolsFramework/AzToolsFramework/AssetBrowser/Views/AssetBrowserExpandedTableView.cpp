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
#include <AzToolsFramework/AssetBrowser/Previewer/PreviewerFrame.h>
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
        AssetBrowserExpandedTableView::AssetBrowserExpandedTableView(QWidget* parent)
            : QWidget(parent)
            , m_expandedTableViewWidget(new AzQtComponents::AssetFolderExpandedTableView(parent))
            , m_expandedTableViewProxyModel(new AssetBrowserExpandedTableViewProxyModel(parent))
            , m_assetFilterModel(new AssetBrowserFilterModel(parent))
        {
            // Using our own instance of AssetBrowserFilterModel to be able to show also files when the main model
            // only lists directories, and at the same time get sort and filter entries features from AssetBrowserFilterModel.
            m_assetFilterModel->sort(0, Qt::DescendingOrder);
            m_expandedTableViewProxyModel->setSourceModel(m_assetFilterModel);
            m_expandedTableViewWidget->setModel(m_expandedTableViewProxyModel);

            connect(
                m_expandedTableViewWidget,
                &AzQtComponents::AssetFolderExpandedTableView::IndexClicked,
                this,
                [this](const QModelIndex& index)
                {
                    if (!m_previewerFrame)
                    {
                        return;
                    }

                    auto indexData = index.data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserEntry*>();
                    if (indexData->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source)
                    {
                        m_previewerFrame->Display(indexData);
                    }
                });

            connect(
                m_expandedTableViewWidget,
                &AzQtComponents::AssetFolderExpandedTableView::IndexDoubleClicked,
                this,
                [this](const QModelIndex& index)
                {
                    if (!m_assetTreeView)
                    {
                        return;
                    }

                    auto indexData = index.data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserEntry*>();
                    if (indexData->GetEntryType() != AssetBrowserEntry::AssetEntryType::Folder)
                    {
                        return;
                    }

                    auto treeViewFilterModel = qobject_cast<AssetBrowserFilterModel*>(m_assetTreeView->model());
                    if (!treeViewFilterModel)
                    {
                        return;
                    }

                    auto selectionModel = m_assetTreeView->selectionModel();

                    auto targetIndex =
                        treeViewFilterModel->mapFromSource(m_assetFilterModel->mapToSource(m_expandedTableViewProxyModel->mapToSource(index)));

                    selectionModel->select(targetIndex, QItemSelectionModel::ClearAndSelect);

                    auto targetIndexAncestor = targetIndex.parent();
                    while (targetIndexAncestor.isValid())
                    {
                        m_assetTreeView->expand(targetIndexAncestor);
                        targetIndexAncestor = targetIndexAncestor.parent();
                    }

                    m_assetTreeView->scrollTo(targetIndex);
                });

            auto layout = new QVBoxLayout();
            layout->addWidget(m_expandedTableViewWidget);
            setLayout(layout);
        }

        AssetBrowserExpandedTableView::~AssetBrowserExpandedTableView() = default;

        void AssetBrowserExpandedTableView::SetPreviewerFrame(PreviewerFrame* previewerFrame)
        {
            m_previewerFrame = previewerFrame;
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
                m_expandedTableViewProxyModel->SetSourceModelCurrentSelection(
                    m_assetFilterModel->mapFromSource(treeViewFilterModel->mapToSource(selectedIndexes[0])));
            }
            else
            {
                m_expandedTableViewProxyModel->SetSourceModelCurrentSelection({});
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
                if (subFilter->GetName() != "Folder")
                {
                    filterCopy->AddFilter(subFilter);
                }
            }
            m_assetFilterModel->SetFilter(FilterConstType(filterCopy));
        }
#pragma optimize("", on)
    } // namespace AssetBrowser
} // namespace AzToolsFramework
