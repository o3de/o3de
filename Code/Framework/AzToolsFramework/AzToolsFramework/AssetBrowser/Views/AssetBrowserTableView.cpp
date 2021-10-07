/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTableView.h>
#include <AzToolsFramework/AssetBrowser/Views/EntryDelegate.h>

AZ_PUSH_DISABLE_WARNING(
    4244 4251 4800, "-Wunknown-warning-option") // conversion from 'int' to 'float', possible loss of data, needs to have dll-interface to
                                                // be used by clients of class 'QFlags<QPainter::RenderHint>::Int': forcing value to bool
                                                // 'true' or 'false' (performance warning)
#include <QCoreApplication>
#include <QHeaderView>
#include <QMenu>
#include <QResizeEvent>
#include <QTimer>
AZ_POP_DISABLE_WARNING
namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        const float MinHeaderResizeProportion = .25f;
        const float MaxHeaderResizeProportion = .75f;
        const float DefaultHeaderResizeProportion = .5f;

        AssetBrowserTableView::AssetBrowserTableView(QWidget* parent)
            : AzQtComponents::TableView(parent)
            , m_delegate(new SearchEntryDelegate(this))
        {
            setSortingEnabled(false);
            setItemDelegate(m_delegate);
            setRootIsDecorated(false);

            //Styling the header aligning text to the left and using a bold font.
            header()->setDefaultAlignment(Qt::AlignLeft);
            header()->setStyleSheet("QHeaderView { font-weight: bold; };");
            

            setContextMenuPolicy(Qt::CustomContextMenu);

            setMouseTracking(true);
            setSelectionMode(QAbstractItemView::SingleSelection);

            connect(this, &AzQtComponents::TableView::customContextMenuRequested, this, &AssetBrowserTableView::OnContextMenu);

            AssetBrowserViewRequestBus::Handler::BusConnect();
            AssetBrowserComponentNotificationBus::Handler::BusConnect();
        }

        AssetBrowserTableView::~AssetBrowserTableView()
        {
            AssetBrowserViewRequestBus::Handler::BusDisconnect();
            AssetBrowserComponentNotificationBus::Handler::BusDisconnect();
        }

        void AssetBrowserTableView::setModel(QAbstractItemModel* model)
        {
            m_tableModel = qobject_cast<AssetBrowserTableModel*>(model);
            AZ_Assert(m_tableModel, "Expecting AssetBrowserTableModel");
            m_sourceFilterModel = qobject_cast<AssetBrowserFilterModel*>(m_tableModel->sourceModel());
            AzQtComponents::TableView::setModel(model);
            connect(m_tableModel, &AssetBrowserTableModel::layoutChanged, this, &AssetBrowserTableView::layoutChangedSlot);

            header()->setStretchLastSection(true);
            header()->setSectionResizeMode(0, QHeaderView::ResizeMode::Interactive);
            header()->setSectionResizeMode(1, QHeaderView::ResizeMode::Interactive);
            UpdateSizeSlot(parentWidget()->width());
            header()->setSortIndicatorShown(false);
            header()->setSectionsClickable(false);
        }

        void AssetBrowserTableView::SetName(const QString& name)
        {
            m_name = name;
            bool isAssetBrowserComponentReady = false;
            AssetBrowserComponentRequestBus::BroadcastResult(isAssetBrowserComponentReady, &AssetBrowserComponentRequests::AreEntriesReady);
            if (isAssetBrowserComponentReady)
            {
                OnAssetBrowserComponentReady();
            }
        }

        AZStd::vector<AssetBrowserEntry*> AssetBrowserTableView::GetSelectedAssets() const
        {
            QModelIndexList sourceIndexes;
            for (const auto& index : selectedIndexes())
            {
                if (index.column() == 0)
                {
                    sourceIndexes.push_back(m_sourceFilterModel->mapToSource(m_tableModel->mapToSource(index)));
                }
            }

            AZStd::vector<AssetBrowserEntry*> entries;
            AssetBrowserModel::SourceIndexesToAssetDatabaseEntries(sourceIndexes, entries);
            return entries;
        }

        void AssetBrowserTableView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
        {
            AzQtComponents::TableView::selectionChanged(selected, deselected);
            Q_EMIT selectionChangedSignal(selected, deselected);
        }

        void AssetBrowserTableView::rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end)
        {
            // if selected entry is being removed, clear selection so not to select (and attempt to preview) other entries potentially
            // marked for deletion
            if (selectionModel() && selectionModel()->selectedIndexes().size() == 1)
            {
                QModelIndex selectedIndex = selectionModel()->selectedIndexes().first();
                QModelIndex parentSelectedIndex = selectedIndex.parent();
                if (parentSelectedIndex == parent && selectedIndex.row() >= start && selectedIndex.row() <= end)
                {
                    selectionModel()->clear();
                }
            }
            AzQtComponents::TableView::rowsAboutToBeRemoved(parent, start, end);
        }

        void AssetBrowserTableView::layoutChangedSlot(
            [[maybe_unused]] const QList<QPersistentModelIndex>& parents, [[maybe_unused]] QAbstractItemModel::LayoutChangeHint hint)
        {
            scrollToTop();
        }

        void AssetBrowserTableView::SelectProduct([[maybe_unused]] AZ::Data::AssetId assetID)
        {
        }

        void AssetBrowserTableView::SelectFileAtPath([[maybe_unused]] const AZStd::string& assetPath)
        {
        }

        void AssetBrowserTableView::ClearFilter()
        {
            emit ClearStringFilter();
            emit ClearTypeFilter();
            m_sourceFilterModel->FilterUpdatedSlotImmediate();
        }

        void AssetBrowserTableView::Update()
        {
            update();
        }

        void AssetBrowserTableView::OnAssetBrowserComponentReady()
        {
            UpdateSizeSlot(parentWidget()->width());
        }

        void AssetBrowserTableView::UpdateSizeSlot(int newWidth)
        {
            setColumnWidth(0, aznumeric_cast<int>(newWidth * DefaultHeaderResizeProportion));
            header()->setMinimumSectionSize(aznumeric_cast<int>(newWidth * MinHeaderResizeProportion));
            header()->setMaximumSectionSize(aznumeric_cast<int>(newWidth * MaxHeaderResizeProportion));
        }


        void AssetBrowserTableView::OnContextMenu([[maybe_unused]] const QPoint& point)
        {
            const auto& selectedAssets = GetSelectedAssets();
            if (selectedAssets.size() != 1)
            {
                return;
            }

            QMenu menu(this);
            AssetBrowserInteractionNotificationBus::Broadcast(
                &AssetBrowserInteractionNotificationBus::Events::AddContextMenuActions, this, &menu, selectedAssets);
            if (!menu.isEmpty())
            {
                menu.exec(QCursor::pos());
            }
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
#include "AssetBrowser/Views/moc_AssetBrowserTableView.cpp"
