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
#include <QSortFilterProxyModel>
#include <QPointer>
#endif

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserFilterModel;
        class AssetBrowserEntry;

        class AssetBrowserTableModel
            : public QSortFilterProxyModel
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(AssetBrowserTableModel, AZ::SystemAllocator, 0);
            explicit AssetBrowserTableModel(QObject* parent = nullptr);
            ////////////////////////////////////////////////////////////////////
            // QSortFilterProxyModel
            void setSourceModel(QAbstractItemModel* sourceModel) override;
            QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;
            QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
            QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        public Q_SLOTS:
            void UpdateTableModelMaps();
        protected:
            int rowCount(const QModelIndex& parent = QModelIndex()) const override;
            QVariant headerData(int section, Qt::Orientation orientation, int role /* = Qt::DisplayRole */) const override;
            ////////////////////////////////////////////////////////////////////

        private:
            AssetBrowserEntry* GetAssetEntry(QModelIndex index) const;
            int BuildTableModelMap(const QAbstractItemModel* model, const QModelIndex& parent = QModelIndex(), int row = 0);
        private:
            QPointer<AssetBrowserFilterModel> m_filterModel;
            QMap<int, QModelIndex> m_indexMap;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
