/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "ProductDependencyTreeItemData.h"

#include "native/utilities/assetUtils.h"
#include <AzCore/std/smart_ptr/make_shared.h>

AZ_PUSH_DISABLE_WARNING(4127 4251 4800 4244, "-Wunknown-warning-option")
#include <QDir>
#include <QStack>
AZ_POP_DISABLE_WARNING

namespace AssetProcessor
{
    AZStd::shared_ptr<ProductDependencyTreeItemData> ProductDependencyTreeItemData::MakeShared(
        QString name, AZStd::string productName)
    {
        return AZStd::make_shared<ProductDependencyTreeItemData>(name, productName);
    }

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
        m_childItems.emplace_back(new ProductDependencyTreeItem(data, this));
        return m_childItems.back().get();
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
        return static_cast<int>(m_childItems.size());
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

    int ProductDependencyTreeItem::GetColumnCount() const
    {
        return static_cast<int>(ProductDependencyTreeColumns::Max);
    }

    QVariant ProductDependencyTreeItem::GetDataForColumn(int column) const
    {
        if (column < 0 || column >= GetColumnCount() || !m_data)
        {
            return QVariant();
        }
        switch (column)
        {
        case static_cast<int>(AssetTreeColumns::Name):
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
