/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "ComponentEntityEditorPlugin_precompiled.h"

#include "OutlinerSortFilterProxyModel.hxx"

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>

#include "OutlinerListModel.hxx"

OutlinerSortFilterProxyModel::OutlinerSortFilterProxyModel(QObject* pParent)
    : QSortFilterProxyModel(pParent)
{
}

void OutlinerSortFilterProxyModel::UpdateFilter()
{
    invalidateFilter();
}

bool OutlinerSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    QVariant visibilityData = sourceModel()->data(index, OutlinerListModel::VisibilityRole);
    return visibilityData.isValid() ? visibilityData.toBool() : true;
}

bool OutlinerSortFilterProxyModel::lessThan(const QModelIndex& leftIndex, const QModelIndex& rightIndex) const
{
    if (leftIndex.isValid() && rightIndex.isValid())
    {
        QVariant leftData = sourceModel()->data(leftIndex);
        QVariant rightData = sourceModel()->data(rightIndex);

        // make sure to compare the correct data types for sorting the current column
        AZ_Assert(leftData.type() == rightData.type(), "OutlinerSortFilterProxyModel::lessThan types do not agree!");
        if (static_cast<QMetaType::Type>(leftData.type()) == QMetaType::QString)
        {
            return leftData.toString() < rightData.toString();
        }
        else if (static_cast<QMetaType::Type>(leftData.type()) == QMetaType::ULongLong)
        {
            return leftData.toULongLong() < rightData.toULongLong();
        }
        AZ_Error("Editor", false, "Error! Unhandled type \"%s\" in OutlinerSortFilterProxyModel::lessThan", leftData.typeName());
    }
    return false;
}

void OutlinerSortFilterProxyModel::sort(int /*column*/, Qt::SortOrder /*order*/)
{
    // override any attempts to change sort
    QSortFilterProxyModel::sort(OutlinerListModel::ColumnSortIndex, Qt::SortOrder::AscendingOrder);
}

#include <UI/Outliner/moc_OutlinerSortFilterProxyModel.cpp>

