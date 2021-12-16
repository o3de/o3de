/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include <QIcon>
#include <QString>

namespace AZ
{
    struct Uuid;
}

namespace AssetProcessor
{
    class ProductDependencyTreeItemData
    {
    public:
        AZ_RTTI(ProductDependencyTreeItemData, "{A746FA16-7545-44FB-9454-0D64CBAA7A6E}");

        static AZStd::shared_ptr<ProductDependencyTreeItemData> MakeShared(
            QString name, AZStd::string productName);

        ProductDependencyTreeItemData(
            QString name, AZStd::string productName);

        virtual ~ProductDependencyTreeItemData()
        {
        }

        QString m_name;
        // Product name as it exists in the database,
        // used in the right click menu to jump to this content.
        AZStd::string m_productName;
    };

    enum class ProductDependencyTreeColumns
    {
        Name,
        Max
    };
    class ProductDependencyTreeItem
    {
    public:
        explicit ProductDependencyTreeItem(
            AZStd::shared_ptr<ProductDependencyTreeItemData> data, ProductDependencyTreeItem* parentItem = nullptr);
        virtual ~ProductDependencyTreeItem();

        ProductDependencyTreeItem* CreateChild(AZStd::shared_ptr<ProductDependencyTreeItemData> data);
        ProductDependencyTreeItem* GetChild(int row) const;

        void EraseChild(ProductDependencyTreeItem* child);

        int getChildCount() const;
        int GetColumnCount() const;
        int GetRow() const;
        QVariant GetDataForColumn(int column) const;
        QIcon GetIcon() const;
        ProductDependencyTreeItem* GetParent() const;

        AZStd::shared_ptr<ProductDependencyTreeItemData> GetData() const
        {
            return m_data;
        }


    private:
        AZStd::vector<AZStd::unique_ptr<ProductDependencyTreeItem>> m_childItems;
        AZStd::shared_ptr<ProductDependencyTreeItemData> m_data;
        ProductDependencyTreeItem* m_parent = nullptr;
        QIcon m_fileIcon;
    };
} // namespace AssetProcessor
