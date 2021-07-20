/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include <AzCore/base.h>
#include <AzCore/Memory/Memory.h>

#include "ClassReferenceFilter.hxx"

void ClassReferenceFilterModel::SetFilter(QString newFilter)
{
    m_Filter = newFilter;
    m_filteredRows.clear();

    CacheFilteredData();

    invalidate();
}

void ClassReferenceFilterModel::CacheFilteredData()
{
    if (m_Filter.isEmpty())
    {
        return;
    }

    int rootCount = sourceModel()->rowCount();
    for (int rootRow = 0; rootRow < rootCount; ++rootRow)
    {
        const QModelIndex rootIndex = sourceModel()->index(rootRow, 0);
        // Always show roots
        m_filteredRows.insert(rootIndex);

        TraverseChildren(rootIndex, true, false);
    }
}

bool ClassReferenceFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    // default QT behavior is to hide all children if the parent does not match
    // this is unacceptable for our purpose
    // therefore we recurse all children and Accept the parent if any child matches
    // NB: children that don't match will get Denied during their own callback

    if (m_Filter.isEmpty())
    {
        return true;
    }

    QModelIndex rowIndex = sourceModel()->index(sourceRow, 0, sourceParent);
    return m_filteredRows.contains(rowIndex);
}

ClassReferenceFilterModel::ClassReferenceFilterModel(QObject* pParent)
    : QSortFilterProxyModel(pParent)
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setDynamicSortFilter(true);
}

bool ClassReferenceFilterModel::TraverseChildren(const QModelIndex& indexParent, bool recurse, bool parentMatches)
{
    const int rowCount = sourceModel()->rowCount(indexParent);

    bool matchedAnyChild = false;

    for (int i = 0; i < rowCount; ++i)
    {
        const QModelIndex rowIndex = indexParent.model()->index(i, indexParent.column(), indexParent);
        if (rowIndex.isValid())
        {
            bool rowMatches = false;
            if (parentMatches || m_filteredRows.contains(rowIndex)) 
            {
                rowMatches = true;
            }
            else
            {
                const QString name = sourceModel()->data(rowIndex).toString();
                if (name.contains(m_Filter, Qt::CaseInsensitive))
                {
                    rowMatches = true;
                }
            }

            if (recurse)
            {
                // If any child matched, then this row is filtered in
                rowMatches |= TraverseChildren(rowIndex, true, rowMatches);
            }

            if (rowMatches)
            {
                matchedAnyChild = true;
                m_filteredRows.insert(rowIndex);
            }
        }
    }

    return matchedAnyChild;
}

