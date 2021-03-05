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

#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/RoleFilterProxyModel.h>

namespace EMStudio
{
    RoleFilterProxyModel::RoleFilterProxyModel(QAbstractItemModel& sourceModel, QObject* parent)
        : QAbstractItemModel(parent)
        , m_sourceModel(sourceModel)
    {}

    void RoleFilterProxyModel::setFilteredRoles(const AZStd::vector<int>& roles)
    {
        m_filterRoles = roles;
    }

    QModelIndex RoleFilterProxyModel::index(int row, int column, const QModelIndex& parent) const
    {
        return m_sourceModel.index(row, column, parent);
    }

    QModelIndex RoleFilterProxyModel::parent(const QModelIndex& child) const
    {
        return m_sourceModel.parent(child);
    }

    int RoleFilterProxyModel::rowCount(const QModelIndex& parent) const
    {
        return m_sourceModel.rowCount(parent);
    }

    int RoleFilterProxyModel::columnCount(const QModelIndex& parent) const
    {
        return m_sourceModel.columnCount(parent);
    }

    QVariant RoleFilterProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        return m_sourceModel.headerData(section, orientation, role);
    }
    
    QVariant RoleFilterProxyModel::data(const QModelIndex& index, int role) const
    {
        for (const int filteredRole : m_filterRoles)
        {
            if (filteredRole == role)
            {
                return QVariant();
            }
        }
        return m_sourceModel.data(index, role);
    }

    QModelIndex RoleFilterProxyModel::mapFromSource(const QModelIndex &sourceIndex) const
    {
        return createIndex(sourceIndex.row(), sourceIndex.column(), sourceIndex.internalPointer());
    }

} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/moc_RoleFilterProxyModel.cpp>
