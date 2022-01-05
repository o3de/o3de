/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ProductDependencyTreeModel.h"
#include "ProductDependencyTreeItemData.h"
#include "AssetTreeItem.h"
#include "ProductAssetTreeItemData.h"

#include <AzCore/IO/Path/Path.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/std/smart_ptr/make_shared.h>

namespace AssetProcessor
{
    AZ_CVAR_EXTERNED(bool, ap_disableAssetTreeView);

    ProductDependencyTreeModel::ProductDependencyTreeModel(
        AZStd::shared_ptr<AzToolsFramework::AssetDatabase::AssetDatabaseConnection> sharedDbConnection,
        AssetTreeFilterModel* productFilterModel,
        DependencyTreeType treeType,
        QObject* parent)
        : QAbstractItemModel(parent)
        , m_sharedDbConnection(sharedDbConnection)
        , m_productFilterModel(productFilterModel)
        , m_treeType(treeType)
        , m_fileIcon(QIcon(QStringLiteral(":/Gallery/Asset_File.svg")))
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
            Q_ASSERT(checkIndex(index));
            return index;
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

    int ProductDependencyTreeModel::columnCount(const QModelIndex& parent) const
    {
        if (parent.isValid())
        {
            return static_cast<ProductDependencyTreeItem*>(parent.internalPointer())->GetColumnCount();
        }
        if (m_root)
        {
            return m_root->GetColumnCount();
        }
        return 0;
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

    QVariant ProductDependencyTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        {
            return QVariant();
        }
        if (section < 0 || section >= static_cast<int>(ProductDependencyTreeColumns::Max))
        {
            return QVariant();
        }

        switch (section)
        {
        case static_cast<int>(ProductDependencyTreeColumns::Name):
            switch (m_treeType)
            {
            case DependencyTreeType::Incoming:
                return tr("Incoming Product Dependencies");
            case DependencyTreeType::Outgoing:
                return tr("Outgoing Product Dependencies");
            default:
                AZ_Warning("AssetProcessor", false, "Unhandled AssetTree tree type %d", m_treeType);
                return tr("Name");
            }
        default:
            AZ_Warning("AssetProcessor", false, "Unhandled AssetTree section %d", section);
            break;
        }
        return QVariant();
    }

    bool ProductDependencyTreeModel::setData(const QModelIndex& /*index*/, const QVariant& /*value*/, int /*role*/)
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
        Q_ASSERT(checkIndex(parentIndex));
        return parentIndex;
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

    void ProductDependencyTreeModel::AssetDataSelectionChanged(const QItemSelection& selected, const QItemSelection& /*deselected*/)
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

        m_root.reset(new ProductDependencyTreeItem(AZStd::make_shared<ProductDependencyTreeItemData>("", "")));
        m_trackedProductIds.clear();

        ProductDependencyTreeItem* parentItem = m_root.get();

        // Purposely don't put the product asset database name in, so the right click menu disables. No reason to go to the root object.
        AZStd::shared_ptr<ProductDependencyTreeItemData> productDepTreeItemData = ProductDependencyTreeItemData::MakeShared(
            productItemData->m_name, "");
        ProductDependencyTreeItem* productDependenciesChild = parentItem->CreateChild(productDepTreeItemData);
        createIndex(0, 0, productDependenciesChild);
        m_trackedProductIds.insert(productItemData->m_databaseInfo.m_productID);

        switch (m_treeType)
        {
        case DependencyTreeType::Outgoing:
            PopulateOutgoingProductDependencies(productDependenciesChild, productItemData->m_databaseInfo.m_productID);
            break;
        case DependencyTreeType::Incoming:
            PopulateIncomingProductDependencies(productDependenciesChild, productItemData->m_databaseInfo.m_productID);
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

    void ProductDependencyTreeModel::PopulateIncomingProductDependencies(ProductDependencyTreeItem* parent, AZ::s64 parentProductId)
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

                AZ::IO::Path productNamePath(incomingDependency.m_productName, AZ::IO::PosixPathSeparator);
                const AZ::IO::PathView filename = productNamePath.Filename();
                AZStd::shared_ptr<ProductDependencyTreeItemData> productDepTreeItemData = ProductDependencyTreeItemData::MakeShared(
                    AZ::IO::FixedMaxPathString(filename.Native()).c_str(), incomingDependency.m_productName);
                ProductDependencyTreeItem* child = parent->CreateChild(productDepTreeItemData);
                pendingChildren.push_back(ProductDependencyChild(child, incomingDependency.m_productID));
                m_trackedProductIds.insert(incomingDependency.m_productID);

                return true;
            });

        for (auto& child : pendingChildren)
        {
            PopulateIncomingProductDependencies(child.m_treeItem, child.m_productId);
        }
    }

    void ProductDependencyTreeModel::PopulateOutgoingProductDependencies(ProductDependencyTreeItem* parent, AZ::s64 parentProductId)
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
                        if (m_trackedProductIds.contains(product.m_productID))
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

                        AZ::IO::Path productNamePath(product.m_productName, AZ::IO::PosixPathSeparator);
                        const AZ::IO::PathView filename = productNamePath.Filename();
                        AZStd::shared_ptr<ProductDependencyTreeItemData> productDepTreeItemData = ProductDependencyTreeItemData::MakeShared(
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
            PopulateOutgoingProductDependencies(child.m_treeItem, child.m_productId);
        }
    }
} // namespace AssetProcessor
