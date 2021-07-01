/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"

#include "ColumnSortProxyModel.h"

// Editor
#include "Util/AbstractSortModel.h"

class ColumnSortProxyModelLessThan
{
public:
    inline ColumnSortProxyModelLessThan(int column, const AbstractSortModel* source)
        : sort_column(column)
        , source_model(source) {}

    inline bool operator()(int r1, int r2) const
    {
        QModelIndex i1 = source_model->index(r1, sort_column);
        QModelIndex i2 = source_model->index(r2, sort_column);
        return source_model->LessThan(i1, i2);
    }

private:
    int sort_column;
    const AbstractSortModel* source_model;
};

class ColumnSortProxyModelGreaterThan
{
public:
    inline ColumnSortProxyModelGreaterThan(int column, const AbstractSortModel* source)
        : sort_column(column)
        , source_model(source) {}

    inline bool operator()(int r1, int r2) const
    {
        QModelIndex i1 = source_model->index(r1, sort_column);
        QModelIndex i2 = source_model->index(r2, sort_column);
        return source_model->LessThan(i2, i1);
    }

private:
    int sort_column;
    const AbstractSortModel* source_model;
};


ColumnSortProxyModel::ColumnSortProxyModel(QObject* parent)
    : QAbstractProxyModel(parent)
{
}

ColumnSortProxyModel::~ColumnSortProxyModel()
{
}

QVariant ColumnSortProxyModel::data(const QModelIndex& index, int role) const
{
    Q_ASSERT(index.isValid() && index.model() == this);
    return sourceModel()->data(mapToSource(index), role);
}

QVariant ColumnSortProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return sourceModel()->headerData(section, orientation, role);
}

int ColumnSortProxyModel::rowCount(const QModelIndex& index) const
{
    if (!sourceModel() || index.isValid())
    {
        return 0;
    }
    return sourceModel()->rowCount(index);
}

int ColumnSortProxyModel::columnCount(const QModelIndex& index) const
{
    if (!sourceModel() || index.isValid())
    {
        return 0;
    }
    return sourceModel()->columnCount(index);
}

QModelIndex ColumnSortProxyModel::index(int row, int column, const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    Q_ASSERT(!parent.isValid());
    return createIndex(row, column);
}

QModelIndex ColumnSortProxyModel::parent(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

QModelIndex ColumnSortProxyModel::mapFromSource(const QModelIndex& sourceIndex) const
{
    Q_ASSERT(!sourceIndex.isValid() || sourceIndex.model() == sourceModel());
    if (!sourceIndex.isValid())
    {
        return QModelIndex();
    }
    int row = m_mappingToSource.indexOf(sourceIndex.row());
    return createIndex(row, sourceIndex.column());
}

QModelIndex ColumnSortProxyModel::mapToSource(const QModelIndex& proxyIndex) const
{
    Q_ASSERT(!proxyIndex.isValid() || proxyIndex.model() == this);
    if (!proxyIndex.isValid())
    {
        return QModelIndex();
    }
    int row = m_mappingToSource.at(proxyIndex.row());
    return sourceModel()->index(row, proxyIndex.column());
}

void ColumnSortProxyModel::setSourceModel(QAbstractItemModel* sourceModel)
{
    Q_ASSERT(qobject_cast<AbstractSortModel*>(sourceModel));
    QAbstractProxyModel::setSourceModel(sourceModel);

    connect(sourceModel, &QAbstractItemModel::rowsInserted, this, &ColumnSortProxyModel::SortModel);
    connect(sourceModel, &QAbstractItemModel::rowsRemoved, this, &ColumnSortProxyModel::SortModel);
    connect(sourceModel, &QAbstractItemModel::modelAboutToBeReset, this, &ColumnSortProxyModel::beginResetModel);
    connect(sourceModel, &QAbstractItemModel::modelReset, this, [=]()
        {
            { QSignalBlocker sb(this);
              SortModel();
            } endResetModel();
        });
    connect(sourceModel, &QAbstractItemModel::layoutChanged, this, &ColumnSortProxyModel::SortModel);
    connect(sourceModel, &QAbstractItemModel::dataChanged, this, &ColumnSortProxyModel::SourceDataChanged);

    SortModel();
}

void ColumnSortProxyModel::sort(int column, Qt::SortOrder order)
{
    int id = ColumnContains(column);
    if (id == -1)
    {
        AddColumn(column, order);
    }
    else
    {
        if (m_columns.at(id).sortOrder != order)
        {
            m_columns[id].sortOrder = order;
            SortModel();
        }
    }
}

void ColumnSortProxyModel::AddColumn(int column, Qt::SortOrder order)
{
    if (ColumnContains(column) == -1)
    {
        m_columns.push_front({ column, order });
        SortModel();
    }
}

void ColumnSortProxyModel::AddColumnWithoutSorting(int column, Qt::SortOrder order)
{
    if (ColumnContains(column) == -1)
    {
        m_columns.push_front({ column, order });
    }
}

void ColumnSortProxyModel::RemoveColumn(int column)
{
    int id = ColumnContains(column);
    if (id != -1)
    {
        m_columns.remove(id);
        SortModel();
    }
}

void ColumnSortProxyModel::RemoveColumnWithoutSorting(int column)
{
    int id = ColumnContains(column);
    if (id != -1)
    {
        m_columns.remove(id);
    }
}

void ColumnSortProxyModel::SetColumns(const QVector<int>& columns)
{
    m_columns.clear();
    foreach(int col, columns)
    {
        m_columns.push_back({ col, Qt::AscendingOrder });
    }
    SortModel();
}

void ColumnSortProxyModel::ClearColumns()
{
    SortModel();
}

bool ColumnSortProxyModel::IsColumnSorted(int col) const
{
    int id = ColumnContains(col);
    return (id != -1);
}

Qt::SortOrder ColumnSortProxyModel::SortOrder(int col) const
{
    int id = ColumnContains(col);
    if (id != -1)
    {
        return m_columns.at(id).sortOrder;
    }
    return Qt::AscendingOrder;
}

void ColumnSortProxyModel::SortModel()
{
    emit layoutAboutToBeChanged();

    int size = sourceModel() ? sourceModel()->rowCount() : 0;
    m_mappingToSource.resize(size);
    for (int i = 0; i < size; ++i)
    {
        m_mappingToSource[i] = i;
    }

    foreach(Column col, m_columns)
    {
        if (col.sortOrder == Qt::AscendingOrder)
        {
            ColumnSortProxyModelLessThan lt(col.column, qobject_cast<AbstractSortModel*>(sourceModel()));
            std::stable_sort(m_mappingToSource.begin(), m_mappingToSource.end(), lt);
        }
        else
        {
            ColumnSortProxyModelGreaterThan gt(col.column, qobject_cast<AbstractSortModel*>(sourceModel()));
            std::stable_sort(m_mappingToSource.begin(), m_mappingToSource.end(), gt);
        }
    }

    emit layoutChanged();
    emit SortChanged();
}

void ColumnSortProxyModel::SourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    for (int col = topLeft.column(); col <= bottomRight.column(); ++col)
    {
        if (ColumnContains(col) != -1)
        {
            SortModel();
            return;
        }
    }
}

int ColumnSortProxyModel::ColumnContains(int col) const
{
    for (int i = 0; i < m_columns.count(); ++i)
    {
        if (m_columns.at(i).column == col)
        {
            return i;
        }
    }
    return -1;
}

#include <Util/moc_ColumnSortProxyModel.cpp>
