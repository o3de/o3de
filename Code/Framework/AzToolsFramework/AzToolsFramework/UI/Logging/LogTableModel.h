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

#if !defined(Q_MOC_RUN)
#include <AzCore/PlatformDef.h>

AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option") // conditional expression is constant
#include <AzQtComponents/Components/Widgets/TableView.h>
AZ_POP_DISABLE_WARNING

#include <QVector>
#include <QAbstractTableModel>
#include <QIcon>
#endif

namespace AzToolsFramework
{
    namespace Logging
    {
        class LogLine;

        class LogTableModel
            : public AzQtComponents::TableViewModel
        {
            Q_OBJECT

        public:
            enum LogTabModelRole
            {
                DetailsRole = AzQtComponents::TableViewModel::IsSelectedRole + 1,
                LogLineTextRole,
                CompleteLogLineTextRole,
                LogLineRole,
                LogTypeRole
            };

            enum Columns
            {
                ColumnType,
                ColumnWindow,
                ColumnMessage,
                ColumnCount
            };

            LogTableModel(QObject* parent = nullptr);

            // QAbstractTableModel
            int rowCount(const QModelIndex& parent = {}) const override;
            int columnCount(const QModelIndex& parent = {}) const override;
            QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
            QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

            void AppendLine(const LogLine& logLine);
            void AppendLineAsync(const LogLine& logLine);
            void CommitLines();
            void Clear();

            QString toString(bool details);

        private:
            AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option") // conditional expression is constant
            QVector<LogLine> m_lines;
            AZ_POP_DISABLE_WARNING
            using ContextDetails = QMap<QString, QString>;
            QHash<int, ContextDetails> m_details;
            ContextDetails m_tmpDetails;

            QString logLineToStringAt(int row, bool details);
            QVariant dataDecorationRole(const QModelIndex& index) const;
            QVariant dataDisplayRole(const QModelIndex& index) const;
            QVariant dataDetailsRole(const QModelIndex& index) const;
            QVariant dataLogLineRole(const QModelIndex& index) const;

            int m_pendingLines = 0;

            QIcon m_errorImage;
            QIcon m_warningImage;
            QIcon m_debugImage;
            QIcon m_infoImage;
        };

        class ContextDetailsLogTableModel
            : public AzQtComponents::TableViewModel
        {
            Q_OBJECT

        public:
            enum Columns
            {
                ColumnKey,
                ColumnValue,
                ColumnCount
            };

            using AzQtComponents::TableViewModel::TableViewModel;

            // QAbstractTableModel
            int rowCount(const QModelIndex& parent = {}) const override;
            int columnCount(const QModelIndex& parent = {}) const override;
            QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
            QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

            void SetDetails(const QMap<QString, QString>& details);
            const QMap<QString, QString>& GetDetails() const;

            QString toString() const;

        private:
            QMap<QString, QString> m_data;
        };

    } // namespace Logging

} // namespace AzQtComponents
