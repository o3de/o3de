/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "DeepFilterProxyModel.h"
#include <QPalette>

DeepFilterProxyModel::DeepFilterProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
}

void DeepFilterProxyModel::setFilterString(const QString& filter)
{
    m_filter = filter;
    m_filterParts = m_filter.split(' ', Qt::SkipEmptyParts);
    m_acceptCache.clear();
}

void DeepFilterProxyModel::invalidate()
{
    QSortFilterProxyModel::invalidate();
    m_acceptCache.clear();
}

QVariant DeepFilterProxyModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::ForegroundRole)
    {
        QModelIndex sourceIndex = mapToSource(index);
        if (matchFilter(sourceIndex.row(), sourceIndex.parent()))
        {
            return QSortFilterProxyModel::data(index, role);
        }
        else
        {
            return QPalette().color(QPalette::Disabled, QPalette::Text);
        }
    }
    else
    {
        return QSortFilterProxyModel::data(index, role);
    }
}


void DeepFilterProxyModel::setFilterWildcard(const QString& pattern)
{
    m_acceptCache.clear();
    QSortFilterProxyModel::setFilterWildcard(pattern);
}

bool DeepFilterProxyModel::matchFilter(int sourceRow, const QModelIndex& sourceParent) const
{
    int columnCount = sourceModel()->columnCount(sourceParent);
    for (int i = 0; i < m_filterParts.size(); ++i)
    {
        bool atLeastOneContains = false;
        for (int j = 0; j < columnCount; ++j)
        {
            QModelIndex index = sourceModel()->index(sourceRow, j, sourceParent);
            QVariant data = sourceModel()->data(index, Qt::DisplayRole);
            QString str(data.toString());
            if (str.isEmpty())
            {
                if (m_filterParts.empty())
                {
                    atLeastOneContains = true;
                }
            }
            else if (str.contains(m_filterParts[i], Qt::CaseInsensitive))
            {
                atLeastOneContains = true;
            }
        }
        if (!atLeastOneContains)
        {
            return false;
        }
    }
    return true;
}

bool DeepFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    if (matchFilter(sourceRow, sourceParent))
    {
        return true;
    }

    if (hasAcceptedChildrenCached(sourceRow, sourceParent))
    {
        return true;
    }

    return false;
}

bool DeepFilterProxyModel::hasAcceptedChildrenCached(int sourceRow, const QModelIndex& sourceParent) const
{
    std::pair<QModelIndex, int> indexId = std::make_pair(sourceParent, sourceRow);
    TAcceptCache::iterator it = m_acceptCache.find(indexId);
    if (it == m_acceptCache.end())
    {
        bool result = hasAcceptedChildren(sourceRow, sourceParent);
        m_acceptCache[indexId] = result;
        return result;
    }
    else
    {
        return it->second;
    }
}

bool DeepFilterProxyModel::hasAcceptedChildren(int sourceRow, const QModelIndex& sourceParent) const
{
    QModelIndex item = sourceModel()->index(sourceRow, 0, sourceParent);
    if (!item.isValid())
    {
        return false;
    }

    int childCount = item.model()->rowCount(item);
    if (childCount == 0)
    {
        return false;
    }

    for (int i = 0; i < childCount; ++i)
    {
        if (filterAcceptsRow(i, item))
        {
            return true;
        }
    }

    return false;
}

QModelIndex DeepFilterProxyModel::findFirstMatchingIndex(const QModelIndex& root)
{
    int rowCount = this->rowCount(root);
    for (int i = 0; i < rowCount; ++i)
    {
        QModelIndex index = this->index(i, 0, root);
        if (!index.isValid())
        {
            continue;
        }
        QModelIndex sourceIndex = mapToSource(index);
        if (!sourceIndex.isValid())
        {
            continue;
        }
        if (matchFilter(sourceIndex.row(), sourceIndex.parent()))
        {
            return index;
        }

        QModelIndex child = findFirstMatchingIndex(index);
        if (child.isValid())
        {
            return child;
        }
    }
    return QModelIndex();
}
