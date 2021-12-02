/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AssetTreeFilterModel.h"

#include "AssetTreeItem.h"

namespace AssetProcessor
{

    AssetTreeFilterModel::AssetTreeFilterModel(QObject* parent) : QSortFilterProxyModel(parent)
    {

    }

    void AssetTreeFilterModel::FilterChanged(const QString& newFilter)
    {
        // If the search was changed, clear the asset that had visibility forced.
        m_pathToForceVisibleAsset.clear();
        setFilterRegExp(newFilter);
        setFilterCaseSensitivity(Qt::CaseInsensitive);
        invalidateFilter();
    }

    bool AssetTreeFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
    {
        QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

        AssetTreeItem* assetTreeItem = static_cast<AssetTreeItem*>(index.internalPointer());
        if (!assetTreeItem)
        {
            return false;
        }

        for (const auto& forceVisible : m_pathToForceVisibleAsset)
        {
            if (forceVisible.get() == assetTreeItem->GetData().get())
            {
                return true;
            }
        }

        QRegExp filter(filterRegExp());
        if (filter.isEmpty())
        {
            return true;
        }

        // It's common to find assets referenced by UUID and not by name or path.
        // For example, asset references in AZ data files (like slices) are stored on disk as UUIDs.
        // It's useful to be able to search for an asset by UUID.
        QString searchStr = filter.pattern();
        // If a subId is provided just ignore that bit and pass in the UUID string since that's what we're
        // going to search against
        auto subidPos = searchStr.indexOf(':');
        if (subidPos != -1)
        {
            searchStr = searchStr.mid(0, subidPos);
        }

         // Cap the string to some reasonable length, we don't want to try parsing an entire book
        size_t cappedStringLength = searchStr.length() > 60 ? 60 : searchStr.length();
        AZ::Uuid filterAsUuid = AZ::Uuid::CreateStringPermissive(searchStr.toUtf8(), cappedStringLength);

        return DescendantMatchesFilter(*assetTreeItem, filter, filterAsUuid);
    }

    bool AssetTreeFilterModel::DescendantMatchesFilter(const AssetTreeItem& assetTreeItem, const QRegExp& filter, const AZ::Uuid& filterAsUuid) const
    {
        if (filter.isEmpty())
        {
            // Match everything if there is no filter.
            return true;
        }

        if (!filterAsUuid.IsNull())
        {
            if (assetTreeItem.GetData()->m_uuid == filterAsUuid)
            {
                return true;
            }
        }

        if (assetTreeItem.GetData()->m_name.contains(filter))
        {
            return true;
        }
        if (!assetTreeItem.GetData()->m_isFolder)
        {
            return false;
        }

        for (int childIndex = 0; childIndex < assetTreeItem.getChildCount(); ++childIndex)
        {
            const AssetTreeItem* childItem = assetTreeItem.GetChild(childIndex);
            if (!childItem)
            {
                continue;
            }
            if (DescendantMatchesFilter(*childItem, filter, filterAsUuid))
            {
                return true;
            }
        }
        return false;
    }

    bool AssetTreeFilterModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
    {
        if (!left.isValid())
        {
            return false;
        }
        else if (!right.isValid())
        {
            return true;
        }

        AssetTreeItem* leftItem = static_cast<AssetTreeItem*>(left.internalPointer());
        AssetTreeItem* rightItem = static_cast<AssetTreeItem*>(right.internalPointer());

        // Always sort folders separately.
        if (leftItem && rightItem && leftItem->GetData()->m_isFolder != rightItem->GetData()->m_isFolder)
        {
            // Sort folders before files.
            return rightItem->GetData()->m_isFolder;
        }

        QVariant leftData = sourceModel()->data(left);
        QVariant rightData = sourceModel()->data(right);

        return rightData.toString() < leftData.toString();
    }

    void AssetTreeFilterModel::ForceModelIndexVisible(const QModelIndex& sourceIndex)
    {
        if (!sourceIndex.isValid())
        {
            return;
        }
        m_pathToForceVisibleAsset.clear();

        for (AssetTreeItem* item = static_cast<AssetTreeItem*>(sourceIndex.internalPointer());
            item != nullptr;
            item = item->GetParent())
        {
            m_pathToForceVisibleAsset.push_front(item->GetData());
        }

        invalidateFilter();
    }

}

