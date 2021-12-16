/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AssetDatabase/AssetDatabase.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include <native/utilities/ApplicationManagerAPI.h>
#include <QAbstractItemModel>
#include <QFileIconProvider>
#include <QItemSelectionModel>
#include "AssetTreeFilterModel.h"
#include "ProductDependencyTreeItemData.h"

namespace AssetProcessor
{
    class ProductDependencyTreeItemData;

    enum class DependencyTreeType
    {
        Incoming,
        Outgoing
    };

    class ProductDependencyTreeModel
        : public QAbstractItemModel
    {
    public:
        ProductDependencyTreeModel(
            AZStd::shared_ptr<AzToolsFramework::AssetDatabase::AssetDatabaseConnection> sharedDbConnection,
            AssetTreeFilterModel* productFilterModel,
            DependencyTreeType treeType,
            QObject* parent = nullptr);
        virtual ~ProductDependencyTreeModel();

        // QAbstractItemModel
        QModelIndex index(int row, int column, const QModelIndex& parent) const override;

        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
        QModelIndex parent(const QModelIndex& index) const override;
        bool hasChildren(const QModelIndex& parent) const override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;

    public Q_SLOTS:
        void AssetDataSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
    protected:

        void PopulateOutgoingProductDependencies(ProductDependencyTreeItem* parent, AZ::s64 parentProductId);
        void PopulateIncomingProductDependencies(ProductDependencyTreeItem* parent, AZ::s64 parentProductId);

        AZStd::shared_ptr<AzToolsFramework::AssetDatabase::AssetDatabaseConnection> m_sharedDbConnection;
        AssetTreeFilterModel* m_productFilterModel = nullptr;
        ProductDependencyTreeItem* m_currentItem = nullptr;
        AZStd::unique_ptr<ProductDependencyTreeItem> m_root;
        AZStd::unordered_set<AZ::s64> m_trackedProductIds;
        DependencyTreeType m_treeType;
        QIcon m_fileIcon;
    };
}
