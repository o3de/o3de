/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "FindEntitySortFilterProxyModel.h"
#include "FindEntityItemModel.h"

FindEntitySortFilterProxyModel::FindEntitySortFilterProxyModel(QObject* pParent)
    : QSortFilterProxyModel(pParent)
{
}

void FindEntitySortFilterProxyModel::UpdateFilter()
{
    invalidateFilter();
}

bool FindEntitySortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    QVariant visibilityData = sourceModel()->data(index, FindEntityItemModel::VisibilityRole);
    return visibilityData.isValid() ? visibilityData.toBool() : true;
}

#include <moc_FindEntitySortFilterProxyModel.cpp>
