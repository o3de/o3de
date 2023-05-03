/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QAbstractListModel>
#include <QString>
#include <QPixmap>
#include <QMap>
#endif

namespace AzQtComponents
{
    // Used in conjunction with a StyledDetailsTableView to display
    // a list of key value pairs. Any keys mapped to columns
    // be displayed in columns, everything else will be combined, in priority
    // order, into the Details view, shown when a row is selected.
    class AZ_QT_COMPONENTS_API StyledDetailsTableModel
        : public QAbstractListModel
    {
        Q_OBJECT

    public:
        enum StyledTableRoles
        {
            Details = Qt::UserRole,
            HasOnlyDetails
        };

        enum ColumnStyle
        {
            /// Display value as text
            TextString,
            /// Display an icon representing value text ("error", "warning" or "status")
            StatusIcon,
        };

        enum StatusType
        {
            StatusError = 100,
            StatusWarning = 200,
            StatusSuccess = 300,

            StatusUser = 400
        };

        class TableEntry
        {
        public:
            void Add(const QString& key, const QString& value)
            {
                m_entries.push_back(QPair<QString, QVariant>(key, value));
            }

            void Add(const QString& key, int statusValue)
            {
                m_entries.push_back(QPair<QString, QVariant>(key, statusValue));
            }

        private:
            QVector<QPair<QString, QVariant>> m_entries;

            friend class StyledDetailsTableModel;
        };

        explicit StyledDetailsTableModel(QObject* parent = nullptr);
        ~StyledDetailsTableModel() override;

        int AddColumn(const QString& name, ColumnStyle style = TextString);
        void MoveColumn(const QString& name, int toIndex);
        void AddColumnAlias(const QString& aliasName, const QString& columnName);

        int GetColumnIndex(const QString& name) const;

        void AddEntry(const TableEntry& entry);

        void AddPrioritizedKey(const QString& key);
        void RemovePrioritizedKey(const QString& key);
        void AddDeprioritizedKey(const QString& key);
        void RemoveDeprioritizedKey(const QString& key);

        void sort(int colIndex, Qt::SortOrder order = Qt::AscendingOrder) override;

        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        int columnCount(const QModelIndex& index = {}) const override;
        int rowCount(const QModelIndex& index = {}) const override;
        bool removeRows(int row, int count, const QModelIndex& parent = {}) override;

        void RegisterStatusIcon(int statusType, const QPixmap& icon);

    private:
        struct InternalTableData
        {
            QString key;
            QVariant value;
        };
        typedef QVector<InternalTableData> InternalTableEntry;

        struct Column
        {
            QString name;
            bool hidden = false;
            ColumnStyle style = TextString;
        };

        enum EntryType
        {
            Prioritized,
            Normal,
            Deprioritized
        };

        QString GetColumnName(int colIndex) const;
        QVariant GetEntryData(const InternalTableEntry* entry, const QString& column) const;

        bool AppearsAbove(const InternalTableEntry* lhs, const InternalTableEntry* rhs, const QString& column, Qt::SortOrder) const;
        void DetailsUpdated();

        int m_sortColumn = -1;
        Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
        AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'AzQtComponents::StyledDetailsTableModel::m_columnAliases': class 'QHash<QString,QString>' needs to have dll-interface to be used by clients of class 'AzQtComponents::StyledDetailsTableModel'
        QHash<QString, QString> m_columnAliases;
        QVector<Column> m_columns;
        QVector<InternalTableEntry*> m_entries;
        QVector<QString> m_prioritizedKeys;
        QVector<QString> m_deprioritizedKeys;

        QMap<int, QPixmap> m_statusIcons;

        // non-static for allocation tracking
        QVector<int> m_detailsChangedRoleVector;
        AZ_POP_DISABLE_WARNING
    };
} // namespace AzQtComponents
