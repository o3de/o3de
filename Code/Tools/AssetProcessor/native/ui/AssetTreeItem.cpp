/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AssetTreeItem.h"

#include <QApplication>
#include <QFileIconProvider>
#include <QStyle>
#include <QVariant>
#include <AzCore/Casting/numeric_cast.h>

namespace AssetProcessor
{

    AssetTreeItemData::AssetTreeItemData(
        const AZStd::string& assetDbName, QString name, bool isFolder, const AZ::Uuid& uuid, const AZ::s64 scanFolderID)
        :
        m_assetDbName(assetDbName),
        m_name(name),
        m_isFolder(isFolder),
        m_uuid(uuid),
        m_scanFolderID(scanFolderID)
    {
        QFileInfo fileInfo(name);
        m_extension = fileInfo.suffix();
    }

    int AssetTreeItemData::GetColumnCount() const
    {
        return aznumeric_cast<int>(AssetTreeColumns::Max);
    }

    QVariant AssetTreeItemData::GetDataForColumn(int column) const
    {
        switch (column)
        {
        case aznumeric_cast<int>(AssetTreeColumns::Name):
            return m_name;
        case aznumeric_cast<int>(AssetTreeColumns::Extension):
            if (m_isFolder)
            {
                return QVariant();
            }
            return m_extension;
        default:
            AZ_Warning("AssetProcessor", false, "Unhandled AssetTree column %d", column);
            break;
        }
        return QVariant();
    }

    AssetTreeItem::AssetTreeItem(
        AZStd::shared_ptr<AssetTreeItemData> data,
        QIcon errorIcon,
        QIcon folderIcon,
        QIcon fileIcon,
        AssetTreeItem* parentItem) :
        m_data(data),
        m_parent(parentItem),
        m_errorIcon(errorIcon), // QIcon is implicitly shared.
        m_folderIcon(folderIcon),
        m_fileIcon(fileIcon)
    {

    }

    AssetTreeItem::~AssetTreeItem()
    {
    }

    AssetTreeItem* AssetTreeItem::CreateChild(AZStd::shared_ptr<AssetTreeItemData> data)
    {
        m_childItems.emplace_back(new AssetTreeItem(data, m_errorIcon, m_folderIcon, m_fileIcon, this));
        return m_childItems.back().get();
    }

    AssetTreeItem* AssetTreeItem::GetChild(int row) const
    {
        if (row < 0 || row >= getChildCount())
        {
            return nullptr;
        }
        return m_childItems.at(row).get();
    }

    void AssetTreeItem::EraseChild(AssetTreeItem* child)
    {
        for (auto& item : m_childItems)
        {
            if (item.get() == child)
            {
                m_childItems.erase(&item);
                break;
            }
        }
    }

    int AssetTreeItem::getChildCount() const
    {
        return static_cast<int>(m_childItems.size());
    }

    int AssetTreeItem::GetRow() const
    {
        if (m_parent)
        {
            int index = 0;
            for (const auto& item : m_parent->m_childItems)
            {
                if (item.get() == this)
                {
                    return index;
                }
                ++index;
            }
        }
        return 0;
    }

    int AssetTreeItem::GetColumnCount() const
    {
        return m_data->GetColumnCount();
    }

    QVariant AssetTreeItem::GetDataForColumn(int column) const
    {
        if (column < 0 || column >= GetColumnCount() || !m_data)
        {
            return QVariant();
        }

        return m_data->GetDataForColumn(column);
    }

    QIcon AssetTreeItem::GetIcon() const
    {
        if (!m_data)
        {
            return QIcon();
        }
        if (m_data->m_assetHasUnresolvedIssue)
        {
            return m_errorIcon;
        }
        if (m_data->m_isFolder)
        {
            return m_folderIcon;
        }
        else
        {
            return m_fileIcon;
        }
    }

    AssetTreeItem* AssetTreeItem::GetParent() const
    {
        return m_parent;
    }

    AssetTreeItem* AssetTreeItem::GetChildFolder(QString folder) const
    {
        for (const auto& item : m_childItems)
        {
            if (!item->m_data ||
                !item->m_data->m_isFolder)
            {
                continue;
            }
            if (item->m_data->m_name == folder)
            {
                return item.get();
            }
        }
        return nullptr;
    }

}
