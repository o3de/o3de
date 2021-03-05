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
#include "UiCanvasEditor_precompiled.h"

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
