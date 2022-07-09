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
#include <QAbstractItemModel>
#include <QIcon>
#include "AssetTreeFilterModel.h"

namespace AssetProcessor
{
    class ProductDependencyTreeItem;
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
        bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
        QModelIndex parent(const QModelIndex& index) const override;
        bool hasChildren(const QModelIndex& parent) const override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;

    public Q_SLOTS:
        void AssetDataSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
    protected:

        // visitedDependencies is intentionally a copy because we need to independently track the chain for each branch in the graph
        void PopulateOutgoingProductDependencies(ProductDependencyTreeItem* parent, AZ::s64 parentProductId, QSet<AZ::s64> visitedDependencies);
        void PopulateIncomingProductDependencies(ProductDependencyTreeItem* parent, AZ::s64 parentProductId, QSet<AZ::s64> visitedDependencies);

        AZStd::shared_ptr<AzToolsFramework::AssetDatabase::AssetDatabaseConnection> m_sharedDbConnection;
        
        // Note that the following pointer is passed in down from the main window.
        // We cache the raw pointer here as this window does not own it and is not
        // responsible for destroying it (the main window will).
        AssetTreeFilterModel* m_productFilterModel = nullptr;
        
        AZStd::unique_ptr<ProductDependencyTreeItem> m_root;
        AZStd::unordered_set<AZ::s64> m_trackedProductIds;
        DependencyTreeType m_treeType;
        QIcon m_fileIcon;
    };
}
