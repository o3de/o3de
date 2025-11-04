/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>
#include <QIcon>
#include <QString>

class QFileIconProvider;
class QVariant;

namespace AssetProcessor
{
    class AssetTreeItemData
    {
    public:
        AZ_RTTI(AssetTreeItemData, "{5660BA97-C4B0-4E3B-A03B-9ACD9C67841B}");

        AssetTreeItemData(const AZStd::string& assetDbName, QString name, bool isFolder, const AZ::Uuid& uuid, const AZ::s64 scanFolderID);
        virtual ~AssetTreeItemData() {}
        virtual int GetColumnCount() const;
        virtual QVariant GetDataForColumn(int column) const;

        AZStd::string m_assetDbName;
        AZ::s64 m_scanFolderID;
        QString m_name;
        QString m_extension;
        AZ::Uuid m_uuid;
        bool m_isFolder = false;
        bool m_assetHasUnresolvedIssue = false;
        QString m_unresolvedIssuesTooltip;
    };

    enum class AssetTreeColumns
    {
        Name,
        Extension,
        Max
    };

    class AssetTreeItem
    {
    public:
        explicit AssetTreeItem(
            AZStd::shared_ptr<AssetTreeItemData> data,
            QIcon errorIcon,
            QIcon folderIcon,
            QIcon fileIcon,
            AssetTreeItem* parentItem = nullptr);
        virtual ~AssetTreeItem();

        AssetTreeItem* CreateChild(AZStd::shared_ptr<AssetTreeItemData> data);
        AssetTreeItem* GetChild(int row) const;

        void EraseChild(AssetTreeItem* child);

        int getChildCount() const;
        int GetColumnCount() const;
        int GetRow() const;
        QVariant GetDataForColumn(int column) const;
        QIcon GetIcon() const;
        AssetTreeItem* GetParent() const;
        AssetTreeItem* GetChildFolder(QString folder) const;

        AZStd::shared_ptr<AssetTreeItemData> GetData() const { return m_data; }

    private:
        AZStd::vector<AZStd::unique_ptr<AssetTreeItem>> m_childItems;
        AZStd::shared_ptr<AssetTreeItemData> m_data;
        AssetTreeItem* m_parent = nullptr;
        QIcon m_errorIcon;
        QIcon m_folderIcon;
        QIcon m_fileIcon;
    };
}
