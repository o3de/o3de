/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ComponentPaletteModelFilter.hxx"
#include <AzCore/Serialization/SerializeContext.h>

namespace AzToolsFramework
{

    ComponentPaletteModelFilter::ComponentPaletteModelFilter(QObject* parent)
        : QSortFilterProxyModel(parent)
    {
    }

    bool ComponentPaletteModelFilter::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
    {
        const QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
        if (!index.isValid())
        {
            return false;
        }

        if (!filterRegExp().isValid())
        {
            return true;
        }

        auto componentClass = reinterpret_cast<const AZ::SerializeContext::ClassData*>(sourceModel()->data(index, Qt::ItemDataRole::UserRole + 1).toULongLong());
        if (componentClass)
        {
            const QString componentName = sourceModel()->data(index, Qt::DisplayRole).toString();
            return componentName.contains(filterRegExp());
        }

        const int childRowCount = sourceModel()->rowCount(index);
        for (int childRow = 0; childRow < childRowCount; ++childRow)
        {
            if (filterAcceptsRow(childRow, index))
            {
                return true;
            }
        }

        return false;
    }
}

#include "UI/ComponentPalette/moc_ComponentPaletteModelFilter.cpp"
