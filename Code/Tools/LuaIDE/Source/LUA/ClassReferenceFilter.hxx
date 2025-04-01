/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <QtCore/QSortFilterProxyModel>
#include <QtWidgets/QMainWindow>
#include <QtCore/QSet>


class ClassReferenceFilterModel : public QSortFilterProxyModel
{
public:
    AZ_CLASS_ALLOCATOR(ClassReferenceFilterModel, AZ::SystemAllocator);
    ClassReferenceFilterModel( QObject *pParent );

    void SetFilter( QString newFilter );
    QString GetFilter() const { return m_Filter; }
    bool TraverseChildren( const QModelIndex &, bool traverseGrandchildren, bool parentMatches);

    void CacheFilteredData();

protected:
    virtual bool filterAcceptsRow( int source_row, const QModelIndex & source_parent ) const;

private:
    QString m_Filter;
    QSet<QModelIndex> m_filteredRows;
};

