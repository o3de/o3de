/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef COLUMNSORTPROXYMODEL_H
#define COLUMNSORTPROXYMODEL_H

#if !defined(Q_MOC_RUN)
#include <QAbstractProxyModel>

#include <QVector>
#endif

class ColumnGroupProxyModel;

/*!
    \brief Proxy model used to sort on multiple columns

    It's using a stable sort to sort on multiple columns.
    Every time a value is changed (on one of the sorted columns), it will resort everything.
    THis model does not work with tree models!
*/

class ColumnSortProxyModel
    : public QAbstractProxyModel
{
    Q_OBJECT

public:
    ColumnSortProxyModel(QObject* parent = nullptr);
    ~ColumnSortProxyModel();

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    int rowCount(const QModelIndex& index = QModelIndex()) const override;
    int columnCount(const QModelIndex& index = QModelIndex()) const override;

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;

    QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;
    QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;

    void setSourceModel(QAbstractItemModel* sourceModel) override;

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    void AddColumn(int column, Qt::SortOrder order = Qt::AscendingOrder);
    void RemoveColumn(int column);
    void SetColumns(const QVector<int>& columns);
    void ClearColumns();

    bool IsColumnSorted(int col) const;
    Qt::SortOrder SortOrder(int col) const;

signals:
    void SortChanged();

private:
    friend class ColumnGroupProxyModel;
    void AddColumnWithoutSorting(int column, Qt::SortOrder order = Qt::AscendingOrder);
    void RemoveColumnWithoutSorting(int column);

private slots:
    void SortModel();
    void SourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);

private:
    struct Column
    {
        int column;
        Qt::SortOrder sortOrder;
    };
    int ColumnContains(int col) const;

    QVector<Column> m_columns;
    QVector<int> m_mappingToSource;
};

#endif //COLUMNSORTPROXYMODEL_H
