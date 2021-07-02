/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AssetBundlerFileTableFilterModel.h"

#include <source/models/AssetBundlerAbstractFileTableModel.h>

#include <QDateTime>

namespace AssetBundler
{
    AssetBundlerFileTableFilterModel::AssetBundlerFileTableFilterModel(QObject* parent, int displayNameCol, int dateTimeCol)
        : QSortFilterProxyModel(parent)
        , m_displayNameCol(displayNameCol)
        , m_dateTimeCol(dateTimeCol)
    {
    }

    void AssetBundlerFileTableFilterModel::FilterChanged(const QString& newFilter)
    {
        setFilterRegExp(newFilter.toLower());
        invalidateFilter();
    }

    bool AssetBundlerFileTableFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
    {
        // Override the default implemention since we want to define custom rules here 
        QModelIndex index = sourceModel()->index(sourceRow, m_displayNameCol, sourceParent);
        QRegExp filter(filterRegExp());

        return sourceModel()->data(index).toString().toLower().contains(filter);
    }

    bool AssetBundlerFileTableFilterModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
    {
        // Any column displaying a DateTime string needs to be compared as a DateTime object to ensure
        // proper sorting
        if (left.column() != m_dateTimeCol || right.column() != m_dateTimeCol)
        {
            return QSortFilterProxyModel::lessThan(left, right);
        }

        QVariant leftTime = sourceModel()->data(left, AssetBundlerAbstractFileTableModel::SortRole);
        QVariant rightTime = sourceModel()->data(right, AssetBundlerAbstractFileTableModel::SortRole);

        if (leftTime.type() != QVariant::DateTime || rightTime.type() != QVariant::DateTime)
        {
            return QSortFilterProxyModel::lessThan(left, right);
        }

        return leftTime.toDateTime() < rightTime.toDateTime();
    }
}
