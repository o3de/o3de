/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <qglobal.h> // For Q_OS_WIN

#include <AzQtComponents/Components/StyledDetailsTableModel.h>

#include <QDebug>

namespace AzQtComponents
{
    StyledDetailsTableModel::StyledDetailsTableModel(QObject* parent)
        : QAbstractListModel(parent)
        , m_detailsChangedRoleVector(1, StyledDetailsTableModel::Details)
    {
        RegisterStatusIcon(StatusError, QPixmap(QStringLiteral(":/stylesheet/img/table_error.png")));
        RegisterStatusIcon(StatusWarning, QPixmap(QStringLiteral(":/stylesheet/img/table_warning.png")));
        RegisterStatusIcon(StatusSuccess, QPixmap(QStringLiteral(":/stylesheet/img/table_success.png")));
    }

    StyledDetailsTableModel::~StyledDetailsTableModel()
    {
        qDeleteAll(m_entries);
    }

    int StyledDetailsTableModel::AddColumn(const QString& name, StyledDetailsTableModel::ColumnStyle style)
    {
        const int pos = m_columns.size();
        beginInsertColumns({}, pos, pos);
        m_columns.resize(pos + 1);
        auto& col = m_columns.last();
        col.name = name;
        col.style = style;
        endInsertColumns();
        return pos;
    }

    void StyledDetailsTableModel::MoveColumn(const QString& name, int toIndex)
    {
        if (toIndex < 0 || toIndex >= m_columns.size())
        {
            qWarning() << "Cannot move column out of bounds" << name << toIndex;
            return;
        }
        const int oldIndex = GetColumnIndex(name);
        if (oldIndex == -1)
        {
            qWarning() << "Cannot move non-existent column" << name;
            return;
        }
        if (oldIndex == toIndex)
        {
            return;
        }
        const QModelIndex parent;
        beginMoveColumns(parent, oldIndex, oldIndex, parent, toIndex + 1);
        m_columns.insert(toIndex, m_columns.takeAt(oldIndex));
        endMoveColumns();
    }

    void StyledDetailsTableModel::AddColumnAlias(const QString& aliasName, const QString& columnName)
    {
        m_columnAliases.insert(aliasName, columnName);
    }

    int StyledDetailsTableModel::GetColumnIndex(const QString& name) const
    {
        auto searchName = name;
        const auto alias = m_columnAliases.find(name);
        if (alias != m_columnAliases.end())
        {
            searchName = alias.value();
        }
        for (int i = 0; i < m_columns.size(); ++i)
        {
            if (m_columns.at(i).name == searchName)
            {
                return i;
            }
        }
        return -1;
    }

    void StyledDetailsTableModel::AddEntry(const TableEntry& entry)
    {
        InternalTableEntry internalEntry;
        for (const auto& t : entry.m_entries)
        {
            internalEntry.push_back({t.first, t.second});
        }

        const auto sortColName = GetColumnName(m_sortColumn);
        int pos;
        for (pos = m_entries.size() - 1; pos >= 0; --pos)
        {
            if (!AppearsAbove(&internalEntry, m_entries.at(pos), sortColName, m_sortOrder))
            {
                break;
            }
        }
        ++pos;
        beginInsertRows({}, pos, pos);
        m_entries.insert(pos, new InternalTableEntry(internalEntry));
        endInsertRows();
    }

    void StyledDetailsTableModel::AddPrioritizedKey(const QString& key)
    {
        if (!m_prioritizedKeys.contains(key))
        {
            m_prioritizedKeys.append(key);
            DetailsUpdated();
        }
    }

    void StyledDetailsTableModel::RemovePrioritizedKey(const QString& key)
    {
        const int index = m_prioritizedKeys.indexOf(key);
        if (index != -1)
        {
            m_prioritizedKeys.removeAt(index);
            DetailsUpdated();
        }
    }

    void StyledDetailsTableModel::AddDeprioritizedKey(const QString& key)
    {
        if (!m_deprioritizedKeys.contains(key))
        {
            m_deprioritizedKeys.append(key);
            DetailsUpdated();
        }
    }

    void StyledDetailsTableModel::RemoveDeprioritizedKey(const QString& key)
    {
        const int index = m_deprioritizedKeys.indexOf(key);
        if (index != -1)
        {
            m_deprioritizedKeys.removeAt(index);
            DetailsUpdated();
        }
    }

    QVariant StyledDetailsTableModel::GetEntryData(const StyledDetailsTableModel::InternalTableEntry* entry, const QString& column) const
    {
        const auto match = [&column](const StyledDetailsTableModel::InternalTableData& data)
        {
            return data.key == column;
        };

        const auto canonical = std::find_if(entry->begin(), entry->end(), match);
        if (canonical != entry->end())
        {
            return canonical->value;
        }

        const auto aliases = m_columnAliases.keys(column);
        for (const auto& alias: aliases)
        {
            const auto aliasMatch = [&alias](const StyledDetailsTableModel::InternalTableData& data)
            {
                return data.key == alias;
            };

            const auto keyIt = std::find_if(entry->begin(), entry->end(), aliasMatch);
            if (keyIt != entry->end())
            {
                return keyIt->value;
            }
        }

        return {};
    }

    bool StyledDetailsTableModel::AppearsAbove(const StyledDetailsTableModel::InternalTableEntry* lhs,
                                        const StyledDetailsTableModel::InternalTableEntry* rhs,
                                        const QString& column, Qt::SortOrder order) const
    {
        const auto lhsVal = GetEntryData(lhs, column);
        const auto rhsVal = GetEntryData(rhs, column);
        if (lhsVal.toString() < rhsVal.toString())
        {
            return order == Qt::AscendingOrder;
        }
        if (lhsVal.toString() > rhsVal.toString())
        {
            return order == Qt::DescendingOrder;
        }
        return false;
    }

    void StyledDetailsTableModel::DetailsUpdated()
    {
        const int rows = rowCount();
        const int columns = columnCount();
        if (rows > 0 && columns > 0)
        {
            emit dataChanged(index(0, 0), index(rows - 1, columns - 1), m_detailsChangedRoleVector);
        }
    }

    void StyledDetailsTableModel::sort(int colIndex, Qt::SortOrder order)
    {
        const auto columnName = GetColumnName(colIndex);
        if (columnName.isEmpty())
        {
            return;
        }

        const QList<QPersistentModelIndex> parents = { {} };
        const auto oldEntries = m_entries;

        const auto compare = [columnName, order, this](const InternalTableEntry* lhs, const InternalTableEntry* rhs)
        {
            return AppearsAbove(lhs, rhs, columnName, order);
        };

        emit layoutAboutToBeChanged(parents, VerticalSortHint);

        std::stable_sort(m_entries.begin(), m_entries.end(), compare);

        const int colCount = m_columns.size();
        for (int newRow = 0, endPos = m_entries.size(); newRow < endPos; ++newRow)
        {
            const auto oldRow = oldEntries.indexOf(m_entries[newRow]);
            if (newRow == oldRow)
            {
                continue;
            }
            for (int col = 0; col < colCount; ++col)
            {
                changePersistentIndex(index(oldRow, col), index(newRow, col));
            }
        }

        m_sortColumn = colIndex;
        m_sortOrder = order;
        emit layoutChanged(parents);
    }

    QVariant StyledDetailsTableModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (section < 0 || section >= m_columns.size() || orientation != Qt::Horizontal || role != Qt::DisplayRole)
        {
            return {};
        }

        return m_columns[section].name;
    }

    QVariant StyledDetailsTableModel::data(const QModelIndex& index, int role) const
    {
        if (!hasIndex(index.row(), index.column()))
        {
            return {};
        }

        const auto& column = m_columns[index.column()];
        const auto& entry = m_entries[index.row()];

        if (role == StyledDetailsTableModel::HasOnlyDetails)
        {
            for (const auto& col: m_columns)
            {
                if (GetEntryData(entry, col.name).isValid())
                {
                    return false;
                }
            }
            return true;
        }

        if (role == StyledDetailsTableModel::Details)
        {
            QStringList prioritized, normal, deprioritized;
            for (auto it = entry->begin(), end = entry->end(); it != end; ++it)
            {
                if (GetColumnIndex(it->key) == -1)
                {
                    QString line = QString("%1 - %2").arg(it->key, it->value.toString());
                    (m_prioritizedKeys.contains(it->key) ? prioritized
                        : m_deprioritizedKeys.contains(it->key) ? deprioritized
                        : normal).append(line);
                }
            }
            return (prioritized + normal + deprioritized).join(QStringLiteral("\n"));
        }

        switch (column.style)
        {
            case TextString:
                if (role == Qt::DisplayRole)
                {
                    return GetEntryData(entry, column.name);
                }
                break;
            case StatusIcon:
                if (role == Qt::DecorationRole)
                {
                    int statusType = GetEntryData(entry, column.name).toInt();
                    return m_statusIcons[statusType];
                }
                break;
        }
        return {};
    }

    int StyledDetailsTableModel::columnCount(const QModelIndex& index) const
    {
        return index.isValid() ? 0 : m_columns.size();
    }

    int StyledDetailsTableModel::rowCount(const QModelIndex& index) const
    {
        return index.isValid() ? 0 : m_entries.size();
    }

    bool StyledDetailsTableModel::removeRows(int row, int count, const QModelIndex& parent)
    {
        if (m_entries.size() == 0 || row < 0 || row >= m_entries.size() || row+count > m_entries.size())
        {
            return false;
        }
        beginRemoveRows(parent, row, row+count-1);
        m_entries.remove(row, count);
        endRemoveRows();
        return true;
    }

    void StyledDetailsTableModel::RegisterStatusIcon(int statusType, const QPixmap& icon)
    {
        m_statusIcons.insert(statusType, icon);
    }

    QString StyledDetailsTableModel::GetColumnName(int colIndex) const
    {
        const bool outOfRange = colIndex < 0 || colIndex >= m_columns.size();
        return outOfRange ? QString() : m_columns[colIndex].name;
    }

} // namespace AzQtComponents

#include "Components/moc_StyledDetailsTableModel.cpp"
