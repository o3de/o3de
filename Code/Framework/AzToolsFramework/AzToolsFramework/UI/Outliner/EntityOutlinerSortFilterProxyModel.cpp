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

#include "EntityOutlinerSortFilterProxyModel.hxx"

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>

#include "EntityOutlinerListModel.hxx"

namespace AzToolsFramework
{

    EntityOutlinerSortFilterProxyModel::EntityOutlinerSortFilterProxyModel(QObject* pParent)
        : QSortFilterProxyModel(pParent)
    {
    }

    void EntityOutlinerSortFilterProxyModel::UpdateFilter()
    {
        invalidateFilter();
    }

    bool EntityOutlinerSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
    {
        QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
        QVariant visibilityData = sourceModel()->data(index, EntityOutlinerListModel::VisibilityRole);
        return visibilityData.isValid() ? visibilityData.toBool() : true;
    }

    bool EntityOutlinerSortFilterProxyModel::lessThan(const QModelIndex& leftIndex, const QModelIndex& rightIndex) const
    {
        return sourceModel()->data(leftIndex).toString() < sourceModel()->data(rightIndex).toString();
    }

    void EntityOutlinerSortFilterProxyModel::sort(int /*column*/, Qt::SortOrder /*order*/)
    {
        // override any attempts to change sort
        QSortFilterProxyModel::sort(EntityOutlinerListModel::ColumnSortIndex, Qt::SortOrder::AscendingOrder);
    }

}

#include <UI/Outliner/moc_EntityOutlinerSortFilterProxyModel.cpp>

