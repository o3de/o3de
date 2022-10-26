/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ui/ProcessesModel.h>
#include <QColor>

namespace AssetProcessor
{
    ProcessesModel::ProcessesModel()
    {
        connect(
            &m_debounceTimer,
            &QTimer::timeout,
            [this]()
            {
                m_debounceTimer.stop();

                decltype(m_pendingUpdates) swapped;
                swapped.swap(m_pendingUpdates);

                for (const auto& [id, update] : swapped)
                {
                    const auto& [uuid, process, connection, busy, file] = update;

                    if (!uuid.IsNull())
                    {
                        DoUpdate(uuid, process, connection, busy, file);
                    }
                }
            });
    }

    void ProcessesModel::OnBuilderAdded(AZ::Uuid uuid, AZStd::shared_ptr<const AssetProcessor::Builder> builder)
    {
        Q_EMIT beginInsertRows(QModelIndex(), m_builders.size(), m_builders.size());

        m_builders.emplace_back(uuid, false, false, false, "");
        connect(builder.get(), &AssetProcessor::Builder::StatusUpdate, this, &ProcessesModel::OnStatusUpdate, Qt::QueuedConnection);

        Q_EMIT endInsertRows();
    }

    void ProcessesModel::OnBuilderRemoved(AZ::Uuid builderId)
    {
        auto itr = AZStd::find_if(
            m_builders.begin(),
            m_builders.end(),
            [builderId](auto& obj)
            {
                return AZStd::get<0>(obj) == builderId;
            });

        if (itr != m_builders.end())
        {
            auto rowNumber = AZStd::distance(m_builders.begin(), itr);
            Q_EMIT beginRemoveRows(QModelIndex(), rowNumber, rowNumber);
            m_builders.erase(itr);
            Q_EMIT endRemoveRows();
        }
    }

    void ProcessesModel::DoUpdate(AZ::Uuid builderId, bool process, bool connection, bool busy, QString file)
    {
        auto itr = AZStd::find_if(
            m_builders.begin(),
            m_builders.end(),
            [builderId](auto& obj)
            {
                return AZStd::get<0>(obj) == builderId;
            });

        if (itr != m_builders.end())
        {
            auto rowNumber = AZStd::distance(m_builders.begin(), itr);
            auto& [builderUuid, builderProcess, builderConnection, builderBusy, builderFile] = *itr;

            builderProcess = process;
            builderConnection = connection;
            builderBusy = busy;
            builderFile = file;

            Q_EMIT dataChanged(index(rowNumber, 0), index(rowNumber, 0));
        }

        const int numberBusyBuilders = AZStd::count_if(
            m_builders.begin(),
            m_builders.end(),
            [](auto& obj)
            {
                return AZStd::get<3>(obj);
            });

        Q_EMIT UtilizationUpdate(m_builders.size(), numberBusyBuilders);
    }

    void ProcessesModel::OnStatusUpdate(AZ::Uuid builderId, bool process, bool connection, bool busy, QString file)
    {
        if (m_debounceTimer.isActive())
        {
            m_pendingUpdates[builderId] = { builderId, process, connection, busy, file };
            return;
        }

        DoUpdate(builderId, process, connection, busy, file);
        m_debounceTimer.start(200);
    }

    int ProcessesModel::rowCount(const QModelIndex& index) const
    {
        return index.isValid() ? 0 : m_builders.size();
    }

    QVariant ProcessesModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
        {
            return QVariant();
        }

        if (index.row() >= m_builders.size())
        {
            return QVariant();
        }

        auto& [uuid, process, connection, busy, file] = m_builders.at(index.row());

        if (role == Qt::DisplayRole)
        {
            QString status = busy ? (m_showFilename ? file : "Busy") : connection ? "Idle" : "Boot";

            return QStringLiteral("#%1 %2").arg(index.row(), 2, 10, QLatin1Char('0')).arg(status);
        }

        if (role == Qt::BackgroundRole)
        {
            if (busy)
            {
                return QColor(255, 236, 31);
            }
            if (connection)
            {
                return QColor(139, 207, 29);
            }

            return QColor(255, 98, 62);
        }

        if (role == Qt::ForegroundRole)
        {
            return QColor(0, 0, 0);
        }

        return QVariant();
    }

    void ProcessesModel::ShowFilename(bool show)
    {
        m_showFilename = show;
    }
} // namespace AssetProcessor
