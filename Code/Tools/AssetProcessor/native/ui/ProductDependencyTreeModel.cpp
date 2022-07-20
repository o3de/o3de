/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/ui/ProductDependencyTreeModel.h>
#include <native/ui/ProductDependencyTreeItemData.h>
#include <native/ui/AssetTreeItem.h>
#include <native/ui/ProductAssetTreeItemData.h>

#include <QIcon>
#include <QItemSelection>

#include <AzCore/IO/Path/Path.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/std/smart_ptr/make_shared.h>

namespace AssetProcessor
{
    ProductDependencyTreeModel::ProductDependencyTreeModel(
        AZStd::shared_ptr<AzToolsFramework::AssetDatabase::AssetDatabaseConnection> sharedDbConnection,
        AssetTreeFilterModel* productFilterModel,
        DependencyTreeType treeType,
        QObject* parent)
        : QAbstractItemModel(parent)
        , m_sharedDbConnection(sharedDbConnection)
        , m_productFilterModel(productFilterModel)
        , m_treeType(treeType)
        , m_fileIcon(QIcon(QStringLiteral(":/AssetProcessor_goto.svg")))
    {
        m_root.reset(new ProductDependencyTreeItem(AZStd::make_shared<ProductDependencyTreeItemData>("", "")));
    }

    ProductDependencyTreeModel::~ProductDependencyTreeModel()
    {
    }

    QModelIndex ProductDependencyTreeModel::index(int row, int column, const QModelIndex& parent) const
    {
        if (!hasIndex(row, column, parent))
        {
            return QModelIndex();
        }

        ProductDependencyTreeItem* parentItem = nullptr;

        if (!parent.isValid())
        {
            parentItem = m_root.get();
        }
        else
        {
            parentItem = static_cast<ProductDependencyTreeItem*>(parent.internalPointer());
        }

        if (!parentItem)
        {
            return QModelIndex();
        }

        ProductDependencyTreeItem* childItem = parentItem->GetChild(row);

        if (childItem)
        {
            QModelIndex index = createIndex(row, column, childItem);
            if (checkIndex(index))
            {
                return index;
            }
        }
        return QModelIndex();
    }

    int ProductDependencyTreeModel::rowCount(const QModelIndex& parent) const
    {
        if (parent.column() > 0)
        {
            return 0;
        }

        ProductDependencyTreeItem* parentItem = nullptr;
        if (!parent.isValid())
        {
            parentItem = m_root.get();
        }
        else
        {
            parentItem = static_cast<ProductDependencyTreeItem*>(parent.internalPointer());
        }

        if (!parentItem)
        {
            return 0;
        }
        return parentItem->getChildCount();
    }

    int ProductDependencyTreeModel::columnCount([[maybe_unused]] const QModelIndex& parent) const
    {
        return static_cast<int>(ProductDependencyTreeColumns::Max);
    }

    QVariant ProductDependencyTreeModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
        {
            return QVariant();
        }

        ProductDependencyTreeItem* item = static_cast<ProductDependencyTreeItem*>(index.internalPointer());
        switch (role)
        {
        case Qt::DisplayRole:
            return item->GetDataForColumn(index.column());
        case Qt::DecorationRole:
            // Only show the icon in the name column
            if (index.column() == static_cast<int>(AssetTreeColumns::Name))
            {
                return m_fileIcon;
            }
            break;
        case Qt::ToolTipRole:
            // Purposely return an empty string, so mousing over rows clear out.
            return QString("");
        }
        return QVariant();
    }

    bool ProductDependencyTreeModel::setData(
        [[maybe_unused]] const QModelIndex& index, [[maybe_unused]] const QVariant& value, [[maybe_unused]] int role)
    {
        return false;
    }

    Qt::ItemFlags ProductDependencyTreeModel::flags(const QModelIndex& index) const
    {
        return Qt::ItemIsSelectable | QAbstractItemModel::flags(index);
    }

    QModelIndex ProductDependencyTreeModel::parent(const QModelIndex& index) const
    {
        if (!index.isValid())
        {
            return QModelIndex();
        }

        ProductDependencyTreeItem* childItem = static_cast<ProductDependencyTreeItem*>(index.internalPointer());
        ProductDependencyTreeItem* parentItem = childItem->GetParent();

        if (parentItem == m_root.get() || parentItem == nullptr)
        {
            return QModelIndex();
        }
        QModelIndex parentIndex = createIndex(parentItem->GetRow(), 0, parentItem);
        if (checkIndex(parentIndex))
        {
            return parentIndex;
        }
        return QModelIndex();
    }

    bool ProductDependencyTreeModel::hasChildren(const QModelIndex& parent) const
    {
        ProductDependencyTreeItem* parentItem = nullptr;

        if (!parent.isValid())
        {
            parentItem = m_root.get();
        }
        else
        {
            parentItem = static_cast<ProductDependencyTreeItem*>(parent.internalPointer());
        }

        if (!parentItem)
        {
            return false;
        }

        return parentItem->getChildCount() > 0;
    }

    void ProductDependencyTreeModel::AssetDataSelectionChanged(const QItemSelection& selected, [[maybe_unused]] const QItemSelection& deselected)
    {
        // Even if multi-select is enabled, only display the first selected item.
        if (selected.indexes().count() == 0 || !selected.indexes()[0].isValid())
        {
            // ResetText();
            return;
        }

        QModelIndex productModelIndex = m_productFilterModel->mapToSource(selected.indexes()[0]);

        if (!productModelIndex.isValid())
        {
            return;
        }
        const AssetTreeItem* assetTreeItem = static_cast<const AssetTreeItem*>(productModelIndex.internalPointer());
        const AZStd::shared_ptr<const ProductAssetTreeItemData> productItemData =
            AZStd::rtti_pointer_cast<const ProductAssetTreeItemData>(assetTreeItem->GetData());
        beginResetModel();

        ProductDependencyTreeItem* productDependencies =
            new ProductDependencyTreeItem(AZStd::make_shared<ProductDependencyTreeItemData>(productItemData->m_name, ""));
        m_root.reset(productDependencies);
        m_trackedProductIds.clear();
        createIndex(0, 0, productDependencies);
        m_trackedProductIds.insert(productItemData->m_databaseInfo.m_productID);

        switch (m_treeType)
        {
        case DependencyTreeType::Outgoing:
            PopulateOutgoingProductDependencies(productDependencies, productItemData->m_databaseInfo.m_productID, {});
            break;
        case DependencyTreeType::Incoming:
            PopulateIncomingProductDependencies(productDependencies, productItemData->m_databaseInfo.m_productID, {});
            break;
        }
        endResetModel();
    }

    struct ProductDependencyChild
    {
        ProductDependencyChild(ProductDependencyTreeItem* treeItem, AZ::s64 productId)
            : m_treeItem(treeItem)
            , m_productId(productId)
        {
        }
        ProductDependencyTreeItem* m_treeItem;
        AZ::s64 m_productId;
    };

    void ProductDependencyTreeModel::PopulateIncomingProductDependencies(ProductDependencyTreeItem* parent, AZ::s64 parentProductId, QSet<AZ::s64> visitedDependencies)
    {
        AZ::Data::AssetId assetId;
        m_sharedDbConnection->QueryProductByProductID(
            parentProductId,
            [&](AzToolsFramework::AssetDatabase::ProductDatabaseEntry& productEntry)
            {
                assetId.m_subId = productEntry.m_subID;
                return true;
            });

        m_sharedDbConnection->QuerySourceByProductID(
            parentProductId,
            [&](AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceEntry)
            {
                assetId.m_guid = sourceEntry.m_sourceGuid;
                return true;
            });


        AZStd::string platform;
        m_sharedDbConnection->QueryJobByProductID(
            parentProductId,
            [&](AzToolsFramework::AssetDatabase::JobDatabaseEntry& jobEntry)
            {
                platform = jobEntry.m_platform;
                return true;
            });


        AZStd::vector<ProductDependencyChild> pendingChildren;

        m_sharedDbConnection->QueryDirectReverseProductDependenciesBySourceGuidSubId(
            assetId.m_guid, assetId.m_subId,
            [&](AzToolsFramework::AssetDatabase::ProductDatabaseEntry& incomingDependency)
            {
                // Make sure we haven't already encountered this product before
                // If we have, that means there's a dependency loop, so just abort
                if (visitedDependencies.contains(incomingDependency.m_productID))
                {
                    return true;
                }

                bool platformMatches = false;
                m_sharedDbConnection->QueryJobByJobID(
                    incomingDependency.m_jobPK,
                    [&](AzToolsFramework::AssetDatabase::JobDatabaseEntry& jobEntry)
                    {
                        if (platform.compare(jobEntry.m_platform) == 0)
                        {
                            platformMatches = true;
                        }
                        return true;
                    });
                if (!platformMatches)
                {
                    return true;
                }

                visitedDependencies.insert(incomingDependency.m_productID);

                AZ::IO::Path productNamePath(incomingDependency.m_productName, AZ::IO::PosixPathSeparator);
                const AZ::IO::PathView filename = productNamePath.Filename();
                AZStd::shared_ptr<ProductDependencyTreeItemData> productDepTreeItemData = AZStd::make_shared<ProductDependencyTreeItemData>(
                    AZ::IO::FixedMaxPathString(filename.Native()).c_str(), incomingDependency.m_productName);
                ProductDependencyTreeItem* child = parent->CreateChild(productDepTreeItemData);
                pendingChildren.push_back(ProductDependencyChild(child, incomingDependency.m_productID));
                m_trackedProductIds.insert(incomingDependency.m_productID);

                return true;
            });

        for (auto& child : pendingChildren)
        {
            PopulateIncomingProductDependencies(child.m_treeItem, child.m_productId, visitedDependencies);
        }
    }

    void ProductDependencyTreeModel::PopulateOutgoingProductDependencies(ProductDependencyTreeItem* parent, AZ::s64 parentProductId, QSet<AZ::s64> visitedDependencies)
    {
        AZStd::string platform;
        m_sharedDbConnection->QueryJobByProductID(
            parentProductId,
            [&](AzToolsFramework::AssetDatabase::JobDatabaseEntry& jobEntry)
            {
                platform = jobEntry.m_platform;
                return true;
            });

        AZStd::vector<ProductDependencyChild> pendingChildren;
        m_sharedDbConnection->QueryProductDependencyByProductId(
            parentProductId,
            [&](AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry& dependency)
            {
                if (dependency.m_dependencySourceGuid.IsNull())
                {
                    return true;
                }
                m_sharedDbConnection->QueryProductBySourceGuidSubID(
                    dependency.m_dependencySourceGuid, dependency.m_dependencySubID,
                    [&](AzToolsFramework::AssetDatabase::ProductDatabaseEntry& product)
                    {
                        // Make sure we haven't already encountered this product before
                        // If we have, that means there's a dependency loop, so just abort
                        if (visitedDependencies.contains(product.m_productID))
                        {
                            return true;
                        }
                        bool platformMatches = false;
                        m_sharedDbConnection->QueryJobByJobID(
                            product.m_jobPK,
                            [&](AzToolsFramework::AssetDatabase::JobDatabaseEntry& jobEntry)
                            {
                                if (platform.compare(jobEntry.m_platform) == 0)
                                {
                                    platformMatches = true;
                                }
                                return true;
                            });
                        if (!platformMatches)
                        {
                            return true;
                        }

                        visitedDependencies.insert(product.m_productID);

                        AZ::IO::Path productNamePath(product.m_productName, AZ::IO::PosixPathSeparator);
                        const AZ::IO::PathView filename = productNamePath.Filename();
                        AZStd::shared_ptr<ProductDependencyTreeItemData> productDepTreeItemData =
                            AZStd::make_shared<ProductDependencyTreeItemData>(
                                AZ::IO::FixedMaxPathString(filename.Native()).c_str(), product.m_productName);
                        ProductDependencyTreeItem* child = parent->CreateChild(productDepTreeItemData);
                        pendingChildren.push_back(ProductDependencyChild(child, product.m_productID));
                        m_trackedProductIds.insert(product.m_productID);
                        return true;
                    });
                return true;
            });
        for (auto& child : pendingChildren)
        {
            PopulateOutgoingProductDependencies(child.m_treeItem, child.m_productId, visitedDependencies);
        }
    }

} // namespace AssetProcessor
