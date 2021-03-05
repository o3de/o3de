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
    AZ_CLASS_ALLOCATOR(ClassReferenceFilterModel, AZ::SystemAllocator, 0);
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

