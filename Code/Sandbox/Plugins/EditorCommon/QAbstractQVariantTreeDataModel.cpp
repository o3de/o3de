/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include <QAbstractQVariantTreeDataModel.h>


QAbstractQVariantTreeDataModel::Item* QAbstractQVariantTreeDataModel::itemFromIndex(const QModelIndex& index) const
{
    if (index.isValid())
    {
        return (Item*)index.internalPointer();
    }
    return m_root.get();
}

QModelIndex QAbstractQVariantTreeDataModel::indexFromItem(QAbstractQVariantTreeDataModel::Item* item, int col /*= 0*/) const
{
    if (0 == item)
    {
        return QModelIndex();
    }
    if (!item->m_parent || !item->m_parent->asFolder())
    {
        return QModelIndex();
    }
    int row = item->m_parent->asFolder()->row(item);
    return createIndex(row, col, item);
}

QModelIndex QAbstractQVariantTreeDataModel::index(int row, int column, const QModelIndex& parent /*= QModelIndex()*/) const
{
    Item* parentItem = itemFromIndex(parent);
    if (parentItem && parentItem->asFolder() && row < parentItem->asFolder()->m_children.size())
    {
        Item* item = parentItem->asFolder()->m_children[row].get();
        return createIndex(row, column, item);
    }
    return QModelIndex();
}

QModelIndex QAbstractQVariantTreeDataModel::parent(const QModelIndex& child) const
{
    Item* item = itemFromIndex(child);
    return item && item->m_parent && item->m_parent->asFolder() ? indexFromItem(item->m_parent) : QModelIndex();
}

bool QAbstractQVariantTreeDataModel::hasChildren(const QModelIndex& parent /* = QModelIndex() */) const
{
    Item* item = itemFromIndex(parent);
    return item && item->asFolder() && item->asFolder()->m_children.size();
}

int QAbstractQVariantTreeDataModel::rowCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
    Item* item = itemFromIndex(parent);
    int res = 0;
    if (item && item->asFolder())
    {
        res = (int) item->asFolder()->m_children.size();
    }
    return res;
}

int QAbstractQVariantTreeDataModel::columnCount([[maybe_unused]] const QModelIndex& parent /*= QModelIndex()*/) const
{
    return 1;
}
