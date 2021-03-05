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
#ifndef COLUMNGROUPPROXYMODEL_H
#define COLUMNGROUPPROXYMODEL_H

#if !defined(Q_MOC_RUN)
#include "AbstractGroupProxyModel.h"

#include <QVector>
#endif

class ColumnSortProxyModel;

class ColumnGroupProxyModel
    : public AbstractGroupProxyModel
{
    Q_OBJECT

public:
    ColumnGroupProxyModel(QObject* parent = nullptr);

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    void setSourceModel(QAbstractItemModel* sourceModel) override;

    void AddGroup(int column);
    void RemoveGroup(int column);
    void SetGroups(const QVector<int>& columns);
    void ClearGroups();
    QVector<int> Groups() const;

    bool IsColumnSorted(int col) const;
    Qt::SortOrder SortOrder(int col) const;

protected:
    QStringList GroupForSourceIndex(const QModelIndex& sourceIndex) const override;

signals:
    void GroupsChanged();
    void SortChanged();

private:
    ColumnSortProxyModel* m_sortModel;
    QVector<int> m_groups;
    int m_freeSortColumn;
};

#endif // COLUMNGROUPPROXYMODEL_H