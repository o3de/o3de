/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/containers/vector.h>
#include <QtCore/QSortFilterProxyModel>
#endif


namespace EMStudio
{
    // This class serves as a proxy model for filtering roles.
    // Can be used if a view is not interested in having some role being drawn (like BackgroundRole or DecorationRole)
    // This was implemented as a QAbstractItemModel to avoid the whole cost of mapping, we just want to filter roles so we 
    // implement the proxy our own way.
    //
    class RoleFilterProxyModel
    : public QAbstractItemModel
    {
        Q_OBJECT

    public:
        RoleFilterProxyModel(QAbstractItemModel& sourceModel, QObject * parent = nullptr);
        ~RoleFilterProxyModel() override {}

        void setFilteredRoles(const AZStd::vector<int>& roles);

        QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
        QModelIndex parent(const QModelIndex& child) const override;

        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;

        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

        QModelIndex mapFromSource(const QModelIndex& sourceIndex) const;

    private:
        // If not empty, will filter-in (leave) the node entries that match the specified type
        AZStd::vector<int> m_filterRoles;

        QAbstractItemModel& m_sourceModel;
    };
}   // namespace EMStudio
