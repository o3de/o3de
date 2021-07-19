/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
