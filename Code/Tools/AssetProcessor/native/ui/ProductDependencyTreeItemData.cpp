/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <native/ui/ProductDependencyTreeItemData.h>

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Casting/numeric_cast.h>

#include <QDir>
#include <QStack>
#include <QVariant>

namespace AssetProcessor
{
    ProductDependencyTreeItemData::ProductDependencyTreeItemData(
        QString name, AZStd::string productName)
        : m_name(name)
        , m_productName(productName)
    {
    }

    ProductDependencyTreeItem::ProductDependencyTreeItem(
        AZStd::shared_ptr<ProductDependencyTreeItemData> data, ProductDependencyTreeItem* parentItem)
        : m_data(data)
        , m_parent(parentItem)
    {
    }

    ProductDependencyTreeItem::~ProductDependencyTreeItem()
    {
    }

    ProductDependencyTreeItem* ProductDependencyTreeItem::CreateChild(AZStd::shared_ptr<ProductDependencyTreeItemData> data)
    {
        return m_childItems.emplace_back(new ProductDependencyTreeItem(data, this)).get();
    }

    ProductDependencyTreeItem* ProductDependencyTreeItem::GetChild(int row) const
    {
        if (row < 0 || row >= getChildCount())
        {
            return nullptr;
        }
        return m_childItems.at(row).get();
    }

    void ProductDependencyTreeItem::EraseChild(ProductDependencyTreeItem* child)
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

    int ProductDependencyTreeItem::getChildCount() const
    {
        return aznumeric_cast<int>(m_childItems.size());
    }

    int ProductDependencyTreeItem::GetRow() const
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

    QVariant ProductDependencyTreeItem::GetDataForColumn(int column) const
    {
        if (!m_data)
        {
            return QVariant();
        }
        switch (column)
        {
        case static_cast<int>(ProductDependencyTreeColumns::Name):
            return m_data->m_name;
        default:
            AZ_Warning("AssetProcessor", false, "Unhandled ProductDependencyTreeItem column %d", column);
            break;
        }
        return QVariant();
    }

    ProductDependencyTreeItem* ProductDependencyTreeItem::GetParent() const
    {
        return m_parent;
    }

} // namespace AssetProcessor
