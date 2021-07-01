/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/Logging/LogTableModel.h>
#include <AzToolsFramework/UI/Logging/LogLine.h>

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'QDateTime::d': class 'QSharedDataPointer<QDateTimePrivate>' needs to have dll-interface to be used by clients of class 'QDateTime'
#include <QDateTime>
AZ_POP_DISABLE_WARNING
#include <QPixmap>

AZ_PUSH_DISABLE_WARNING(4800 4251, "-Wunknown-warning-option") // 4800: 'int': forcing value to bool 'true' or 'false' (performance warning)
                                                               // 4251: class 'QScopedPointer<QDebugStateSaverPrivate,QScopedPointerDeleter<T>>' needs to have dll-interface to be used by clients of class 'QDebugStateSaver'
#include <QDebug>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    namespace Logging
    {
        LogTableModel::LogTableModel(QObject* parent)
            : AzQtComponents::TableViewModel(parent)
            , m_errorImage(QStringLiteral(":/stylesheet/img/logging/error.svg"))
            , m_warningImage(QStringLiteral(":/stylesheet/img/logging/warning.svg"))
            , m_debugImage(QStringLiteral(":/stylesheet/img/table_debug.png"))
            , m_infoImage(QStringLiteral(":/stylesheet/img/logging/information.svg"))
        {
        }

        int LogTableModel::rowCount(const QModelIndex& parent) const
        {
            return parent.isValid() ? 0 : m_lines.size() - m_pendingLines;
        }

        int LogTableModel::columnCount(const QModelIndex& parent) const
        {
            return parent.isValid() ? 0 : ColumnCount;
        }

        QVariant LogTableModel::data(const QModelIndex& index, int role) const
        {
            if (hasIndex(index.row(), index.column(), index.parent()))
            {
                switch (role)
                {
                    case Qt::DecorationRole:
                        return dataDecorationRole(index);
                    case Qt::DisplayRole:
                        return dataDisplayRole(index);
                    case DetailsRole:
                        return dataDetailsRole(index);
                    case LogLineTextRole:
                    case CompleteLogLineTextRole:
                        return const_cast<LogTableModel*>(this)->logLineToStringAt(index.row(), role == CompleteLogLineTextRole);
                    case LogLineRole:
                        return dataLogLineRole(index);
                    case LogTypeRole:
                        return m_lines[index.row()].GetLogType();
                }
            }

            return AzQtComponents::TableViewModel::data(index, role);
        }

        QVariant LogTableModel::dataDecorationRole(const QModelIndex& index) const
        {
            if (index.column() != ColumnType)
            {
                return {};
            }

            switch (m_lines[index.row()].GetLogType())
            {
                case LogLine::TYPE_DEBUG:
                    return m_debugImage;

                case LogLine::TYPE_MESSAGE: // Intentional fall-through
                case LogLine::TYPE_CONTEXT:
                    return m_infoImage;
                case LogLine::TYPE_WARNING:
                    return m_warningImage;
                case LogLine::TYPE_ERROR:
                    return m_errorImage;
            }

            return {};
        }

        QVariant LogTableModel::dataDisplayRole(const QModelIndex& index) const
        {
            switch (index.column())
            {
                case ColumnType:
                    return QLocale::system().toString(QDateTime::fromMSecsSinceEpoch(m_lines[index.row()].GetLogTime()), QLocale::ShortFormat).toUtf8().data();
                case ColumnWindow:
                    return m_lines[index.row()].GetLogWindow().c_str();
                case ColumnMessage:
                    return QString::fromUtf8(m_lines[index.row()].GetLogMessage().c_str());
            }
            return {};
        }

        QVariant LogTableModel::dataDetailsRole(const QModelIndex& index) const
        {
            const auto it = m_details.constFind(index.row());
            if (it != m_details.constEnd())
            {
                return QVariant::fromValue(it.value());
            }

            return {};
        }

        QVariant LogTableModel::dataLogLineRole(const QModelIndex& index) const
        {
            return QVariant::fromValue<const LogLine*>(&(m_lines[index.row()]));
        }

        QVariant LogTableModel::headerData(int section, Qt::Orientation orientation, int role) const
        {
            if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
            {
                switch (section)
                {
                    case ColumnType:
                        return "Status";
                    case ColumnWindow:
                        return "Source";
                    case ColumnMessage:
                        return "Message";
                    default:
                        break;
                }
            }

            return AzQtComponents::TableViewModel::headerData(section, orientation, role);
        }

        void LogTableModel::AppendLine(const LogLine& logLine)
        {
            AppendLineAsync(logLine);
            CommitLines();
        }

        void LogTableModel::AppendLineAsync(const LogLine& logLine)
        {
            if (logLine.GetLogType() == LogLine::LogType::TYPE_CONTEXT)
            {
                std::pair<QString, QString> result;
                if (LogLine::ParseContextLogLine(logLine, result))
                {
                    m_tmpDetails[result.first] = result.second;
                }
            }
            else
            {
                m_lines.push_back(std::move(logLine));
                m_pendingLines++;
                if (!m_tmpDetails.isEmpty())
                {
                    m_details.insert(m_lines.count() - 1, m_tmpDetails);
                    m_tmpDetails.clear();
                }
            }
        }

        void LogTableModel::CommitLines()
        {
            if (m_pendingLines > 0)
            {
                beginInsertRows({}, m_lines.count() - m_pendingLines, m_lines.count() - 1);
                m_pendingLines = 0;
                endInsertRows();
            }
        }

        void LogTableModel::Clear()
        {
            if (!m_lines.isEmpty())
            {
                beginResetModel();
                AZ_PUSH_DISABLE_WARNING(4800, "-Wunknown-warning-option")
                m_lines.clear();
                AZ_POP_DISABLE_WARNING
                m_details.clear();
                m_tmpDetails.clear();
                m_pendingLines = 0;
                endResetModel();
            }
        }

        QString LogTableModel::toString(bool details)
        {
            QStringList entries;

            for (int i = 0; i < m_lines.count(); ++i)
            {
                entries << logLineToStringAt(i, details);
            }

            return entries.join(QLatin1Char('\n'));
        }

        QString LogTableModel::logLineToStringAt(int row, bool details)
        {
            auto &line = m_lines[row];
            QString result = QString::fromUtf8(line.ToString().c_str()).trimmed();

            if (details)
            {
                const auto dit = m_details.constFind(row);
                if (dit != m_details.constEnd())
                {
                    const auto &keys = dit.value();

                    for (auto it = keys.constBegin(), end = keys.constEnd(); it != end; ++it)
                    {
                        result.append(QString::fromLatin1("\n[%1] = %2").arg(it.key(), it.value()));
                    }
                }
            }

            return result;
        }

        int ContextDetailsLogTableModel::rowCount(const QModelIndex& parent) const
        {
            return parent.isValid() ? 0 : m_data.count();
        }

        int ContextDetailsLogTableModel::columnCount(const QModelIndex& parent) const
        {
            return parent.isValid() ? 0 : ColumnCount;
        }

        QVariant ContextDetailsLogTableModel::data(const QModelIndex& index, int role) const
        {
            if (hasIndex(index.row(), index.column(), index.parent()))
            {
                switch (role)
                {
                case Qt::DisplayRole:
                {
                    if (index.column() == ColumnKey)
                    {
                        return (m_data.constBegin() +index.row()).key();
                    }
                    else if (index.column() == ColumnValue)
                    {
                        return (m_data.constBegin() +index.row()).value();
                    }

                    break;
                }

                default:
                    break;
                }
            }

            return TableViewModel::data(index, role);
        }

        QVariant ContextDetailsLogTableModel::headerData(int section, Qt::Orientation orientation, int role) const
        {
            if (orientation == Qt::Horizontal && section >= 0 && section <= ColumnCount)
            {
                if (role == Qt::DisplayRole)
                {
                    if (section == ColumnKey)
                    {
                        return tr("Key");
                    }
                    else if (section == ColumnValue)
                    {
                        return tr("Value");
                    }
                }
            }

            return AzQtComponents::TableViewModel::headerData(section, orientation, role);
        }

        void ContextDetailsLogTableModel::SetDetails(const QMap<QString, QString> &details)
        {
            beginResetModel();
            m_data = details;
            endResetModel();
        }

        const QMap<QString, QString> &ContextDetailsLogTableModel::GetDetails() const
        {
            return m_data;
        }

        QString ContextDetailsLogTableModel::toString() const
        {
            QStringList entries;

            for (auto it = m_data.constBegin(), end = m_data.constEnd(); it != end; ++it)
            {
                entries << QString::fromLatin1("%1 = %2").arg(it.key(), it.value());
            }

            return entries.join("\n");
        }
    }
} // namespace AzToolsFramework

#include "UI/Logging/moc_LogTableModel.cpp"
