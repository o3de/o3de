/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#if !defined(Q_MOC_RUN)
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserTableModel.h>

#include <AzQtComponents/Components/Widgets/TableView.h>

#include <QModelIndex>
#include <QPointer>
#endif

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserEntry;
        class AssetBrowserTableModel;
        class AssetBrowserFilterModel;
        class SearchEntryDelegate;

        class AssetBrowserTableView //! Table view that displays the asset browser entries in a list.
            : public AzQtComponents::TableView
            , public AssetBrowserViewRequestBus::Handler
            , public AssetBrowserComponentNotificationBus::Handler
        {
            Q_OBJECT
        public:
            explicit AssetBrowserTableView(QWidget* parent = nullptr);
            ~AssetBrowserTableView() override;

            void setModel(QAbstractItemModel *model) override;
            void SetName(const QString& name);

            AZStd::vector<AssetBrowserEntry*> GetSelectedAssets() const;

            //////////////////////////////////////////////////////////////////////////
            // AssetBrowserViewRequestBus
            virtual void SelectProduct(AZ::Data::AssetId assetID) override;
            virtual void SelectFileAtPath(const AZStd::string& assetPath) override;
            virtual void ClearFilter() override;
            virtual void Update() override;

            //////////////////////////////////////////////////////////////////////////
            // AssetBrowserComponentNotificationBus
            void OnAssetBrowserComponentReady() override;
            //////////////////////////////////////////////////////////////////////////
        Q_SIGNALS:
            void selectionChangedSignal(const QItemSelection& selected, const QItemSelection& deselected);
            void ClearStringFilter();
            void ClearTypeFilter();

        public Q_SLOTS:
            void UpdateSizeSlot(int newWidth);

        protected Q_SLOTS:
            void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) override;
            void rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) override;
            void layoutChangedSlot(const QList<QPersistentModelIndex> &parents = QList<QPersistentModelIndex>(),
                QAbstractItemModel::LayoutChangeHint hint = QAbstractItemModel::NoLayoutChangeHint);
        private:
            QString m_name;
            QPointer<AssetBrowserTableModel> m_tableModel;
            QPointer<AssetBrowserFilterModel> m_sourceFilterModel;
            SearchEntryDelegate* m_delegate = nullptr;

        private Q_SLOTS:
            void OnContextMenu(const QPoint& point);
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
