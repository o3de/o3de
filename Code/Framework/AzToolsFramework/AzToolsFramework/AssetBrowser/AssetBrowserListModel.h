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
#include <QSortFilterProxyModel>
#include <QPersistentModelIndex>
#include <QPointer>
#endif
#include <Editor/EditorSettingsAPIBus.h>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserFilterModel;
        class AssetBrowserEntry;

        class AssetBrowserListModel
            : public QSortFilterProxyModel
            , public AssetBrowserComponentNotificationBus::Handler
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(AssetBrowserListModel, AZ::SystemAllocator);
            explicit AssetBrowserListModel(QObject* parent = nullptr);

            ////////////////////////////////////////////////////////////////////
            // QSortFilterProxyModel
            void setSourceModel(QAbstractItemModel* sourceModel) override;
            QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;
            QModelIndex mapFromSource(const QModelIndex &sourceIndex) const override;
            QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
            QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
            QModelIndex parent(const QModelIndex& child) const override;
            QModelIndex sibling(int row, int column, const QModelIndex& idx) const override;
            int rowCount(const QModelIndex& parent = QModelIndex()) const override;

            // QAbstractTableModel overrides ...
            Qt::DropActions supportedDropActions() const override;
            Qt::ItemFlags flags(const QModelIndex& index) const override;
            bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
            QMimeData* mimeData(const QModelIndexList& indexes) const override;
            QStringList mimeTypes() const override;

        protected:
            bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
            QVariant headerData(int section, Qt::Orientation orientation, int role /* = Qt::DisplayRole */) const override;
            void timerEvent(QTimerEvent* event) override;
            ////////////////////////////////////////////////////////////////////

            void StartUpdateModelMapTimer();
            AssetBrowserEntry* GetAssetEntry(QModelIndex index) const;
            int BuildListModelMap(const QAbstractItemModel* model, const QModelIndex& parent = QModelIndex(), int row = 0);

        public slots:
            void UpdateListModelMaps();

        private slots:
            void SourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);

        private:
            AZ::u64 m_numberOfItemsDisplayed = 50;
            int m_displayedItemsCounter = 0;
            int m_updateModelMapTimerId = 0;
            QPointer<AssetBrowserFilterModel> m_filterModel;
            QMap<int, QPersistentModelIndex> m_indexMap;
            QMap<QModelIndex, int> m_rowMap;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
