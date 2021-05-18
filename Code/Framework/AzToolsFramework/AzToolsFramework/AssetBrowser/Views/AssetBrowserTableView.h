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
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserTableModel.h>

#include <QModelIndex>
#include <QPointer>
#include <QTableView>
#endif

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserEntry;
        class AssetBrowserTableModel;
        class AssetBrowserFilterModel;
        class EntryDelegate;

        class AssetBrowserTableView
            : public QTableView
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

        protected Q_SLOTS:
            void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) override;
            void rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) override;
            void layoutChangedSlot(const QList<QPersistentModelIndex> &parents = QList<QPersistentModelIndex>(), QAbstractItemModel::LayoutChangeHint hint = QAbstractItemModel::NoLayoutChangeHint);

        private:
            QString m_name;
            QPointer<AssetBrowserTableModel> m_tableModel = nullptr;
            QPointer<AssetBrowserFilterModel> m_sourceFilterModel = nullptr;
            EntryDelegate* m_delegate = nullptr;

        private Q_SLOTS:
            void OnContextMenu(const QPoint& point);
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
