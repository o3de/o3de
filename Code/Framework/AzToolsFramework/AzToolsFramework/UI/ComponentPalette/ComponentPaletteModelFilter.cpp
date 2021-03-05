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

#include "AzToolsFramework_precompiled.h"
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