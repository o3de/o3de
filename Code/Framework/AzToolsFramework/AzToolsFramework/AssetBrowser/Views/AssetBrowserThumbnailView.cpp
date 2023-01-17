/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserThumbnailView.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserThumbnailViewProxyModel.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Previewer/PreviewerFrame.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTreeView.h>

#include <AzQtComponents/Components/Widgets/AssetFolderThumbnailView.h>

#if !defined(Q_MOC_RUN)
#include <QVBoxLayout>

#endif

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        AssetBrowserThumbnailView::AssetBrowserThumbnailView(QWidget* parent)
            : QWidget(parent)
            , m_thumbnailViewWidget(new AzQtComponents::AssetFolderThumbnailView(parent))
            , m_thumbnailViewProxyModel(new AssetBrowserThumbnailViewProxyModel(parent))
            , m_assetFilterModel(new AssetBrowserFilterModel(parent))
        {
            // Using our own instance of AssetBrowserFilterModel to be able to show also files when the main model
            // only lists directories, and at the same time get sort and filter entries features from AssetBrowserFilterModel.
            m_assetFilterModel->sort(0, Qt::DescendingOrder);
            m_thumbnailViewProxyModel->setSourceModel(m_assetFilterModel);
            m_thumbnailViewWidget->setModel(m_thumbnailViewProxyModel);

            connect(
                m_thumbnailViewWidget,
                &AzQtComponents::AssetFolderThumbnailView::IndexClicked,
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
                m_thumbnailViewWidget,
                &AzQtComponents::AssetFolderThumbnailView::IndexDoubleClicked,
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
                        treeViewFilterModel->mapFromSource(m_assetFilterModel->mapToSource(m_thumbnailViewProxyModel->mapToSource(index)));

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
            layout->addWidget(m_thumbnailViewWidget);
            setLayout(layout);
        }

        AssetBrowserThumbnailView::~AssetBrowserThumbnailView() = default;

        void AssetBrowserThumbnailView::SetPreviewerFrame(PreviewerFrame* previewerFrame)
        {
            m_previewerFrame = previewerFrame;
        }

        void AssetBrowserThumbnailView::SetAssetTreeView(AssetBrowserTreeView* treeView)
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
                        &AssetBrowserThumbnailView::UpdateFilterInLocalFilterModel);
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
                &AssetBrowserThumbnailView::UpdateFilterInLocalFilterModel);

            connect(
                m_assetTreeView,
                &AssetBrowserTreeView::selectionChangedSignal,
                this,
                &AssetBrowserThumbnailView::HandleTreeViewSelectionChanged);
        }

        void AssetBrowserThumbnailView::HandleTreeViewSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
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
                m_thumbnailViewProxyModel->SetSourceModelCurrentSelection(
                    m_assetFilterModel->mapFromSource(treeViewFilterModel->mapToSource(selectedIndexes[0])));
            }
            else
            {
                m_thumbnailViewProxyModel->SetSourceModelCurrentSelection({});
            }
        }

        void AssetBrowserThumbnailView::UpdateFilterInLocalFilterModel()
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
    } // namespace AssetBrowser
} // namespace AzToolsFramework
