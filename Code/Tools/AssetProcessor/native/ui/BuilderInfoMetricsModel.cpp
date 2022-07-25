/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <ui/BuilderInfoMetricsModel.h>
#include <ui/BuilderInfoMetricsItem.h>
#include <ui/BuilderData.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <QTime>

namespace AssetProcessor
{
    QString DurationToQString(AZ::s64 durationInMs)
    {
        const AZ::s64 dayInMs = 86400000;
        if (durationInMs < 0)
        {
            return "";
        }

        AZ::s64 dayCount = durationInMs / dayInMs;
        QTime duration = QTime::fromMSecsSinceStartOfDay(durationInMs % dayInMs);
        if (dayCount > 0)
        {
            return duration.toString("zzz' ms, 'ss' sec, 'mm' min, 'hh' hr, %1 day'").arg(dayCount);
        }
        else
        {
            if (!duration.isValid())
            {
                return "";
            }
            if (duration.hour() > 0)
            {
                return duration.toString("zzz' ms, 'ss' sec, 'mm' min, 'hh' hr'");
            }
            if (duration.minute() > 0)
            {
                return duration.toString("zzz' ms, 'ss' sec, 'mm' min'");
            }
            if (duration.second() > 0)
            {
                return duration.toString("zzz' ms, 'ss' sec'");
            }
            return duration.toString("zzz' ms'");
        }
    }

    BuilderInfoMetricsModel::BuilderInfoMetricsModel(BuilderData* builderData, QObject* parent)
        : QAbstractItemModel(parent)
        , m_data(builderData)
    {
    }

    QModelIndex BuilderInfoMetricsModel::index(int row, int column, const QModelIndex& parent) const
    {
        if (!hasIndex(row, column, parent))
        {
            return QModelIndex();
        }

        BuilderInfoMetricsItem* parentItem = nullptr;

        if (!parent.isValid())
        {
            parentItem = m_data->m_root.get();
        }
        else
        {
            parentItem = static_cast<BuilderInfoMetricsItem*>(parent.internalPointer());
        }

        if (!parentItem)
        {
            return QModelIndex();
        }

        BuilderInfoMetricsItem* childItem = parentItem->GetChild(row);

        if (childItem)
        {
            QModelIndex index = createIndex(row, column, childItem);
            Q_ASSERT(checkIndex(index));
            return index;
        }

        return QModelIndex();
    }

    int BuilderInfoMetricsModel::rowCount(const QModelIndex& parent) const
    {
        BuilderInfoMetricsItem* parentItem = nullptr;
        if (!parent.isValid())
        {
            parentItem = m_data->m_root.get();
        }
        else
        {
            parentItem = static_cast<BuilderInfoMetricsItem*>(parent.internalPointer());
        }

        if (!parentItem)
        {
            return 0;
        }
        return parentItem->ChildCount();
    }

    int BuilderInfoMetricsModel::columnCount([[maybe_unused]] const QModelIndex& parent) const
    {
        return aznumeric_cast<int>(Column::Max);
    }

    QVariant BuilderInfoMetricsModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
        {
            return QVariant();
        }

        BuilderInfoMetricsItem* item = static_cast<BuilderInfoMetricsItem*>(index.internalPointer());
        switch (role)
        {
        case aznumeric_cast<int>(Role::SortRole):
            switch (index.column())
            {
            case aznumeric_cast<int>(Column::AverageDuration):
                if (item->GetJobCount() == 0)
                {
                    return QVariant();
                }
                return item->GetTotalDuration() / item->GetJobCount();
            case aznumeric_cast<int>(Column::TotalDuration):
                return item->GetTotalDuration();
            // Other columns are sorted by Qt::DisplayRole immediately below
            }
        case Qt::DisplayRole:
            switch (index.column())
            {
            case aznumeric_cast<int>(Column::Name):
                return item->GetName();
            case aznumeric_cast<int>(Column::JobCount):
                return item->GetJobCount();
            case aznumeric_cast<int>(Column::AverageDuration):
                if (item->GetJobCount() == 0)
                {
                    return QVariant();
                }
                return DurationToQString(item->GetTotalDuration() / item->GetJobCount());
            case aznumeric_cast<int>(Column::TotalDuration):
                return DurationToQString(item->GetTotalDuration());
            default:
                break;
            }
        default:
            break;
        }

        return QVariant();
    }

    QVariant BuilderInfoMetricsModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        {
            return QVariant();
        }
        if (section < 0 || section >= aznumeric_cast<int>(Column::Max))
        {
            return QVariant();
        }

        switch (section)
        {
        case aznumeric_cast<int>(Column::Name):
            return tr("Name");
        case aznumeric_cast<int>(Column::JobCount):
            return tr("Job Count");
        case aznumeric_cast<int>(Column::AverageDuration):
            return tr("Average Duration (ms)");
        case aznumeric_cast<int>(Column::TotalDuration):
            return tr("Total Duration (ms)");
        default:
            AZ_Warning("Asset Processor", false, "Unhandled BuilderInfoMetricsModel header %d", section);
            break;
        }
        return QVariant();
    }

    QModelIndex BuilderInfoMetricsModel::parent(const QModelIndex& index) const
    {
        if (!index.isValid())
        {
            return QModelIndex();
        }

        auto currentItem = static_cast<BuilderInfoMetricsItem*>(index.internalPointer());
        auto parentItem = currentItem->GetParent();
        auto rootItem = m_data->m_root.get();
            
        if (parentItem == rootItem || parentItem == nullptr)
        {
            return QModelIndex();
        }

        QModelIndex parentIndex = createIndex(parentItem->GetRow(), 0, parentItem);
        Q_ASSERT(checkIndex(parentIndex));
        return parentIndex;
    }

    void BuilderInfoMetricsModel::Reset()
    {
        beginResetModel();
        m_data->Reset();
        endResetModel();
    }
    void BuilderInfoMetricsModel::OnBuilderSelectionChanged(const AssetBuilderSDK::AssetBuilderDesc& builder)
    {
        // TODO: move it to BuilderData?
        // TODO: need to handle the "all builders" case
        
        if (m_data->m_builderGuidToIndex.contains(builder.m_busId))
        {
            m_data->m_currentSelectedBuilderIndex = m_data->m_builderGuidToIndex[builder.m_busId];
            m_data->m_root->SetChild(m_data->m_singleBuilderMetrics[m_data->m_currentSelectedBuilderIndex]);
        }
        else
        {
            AZ_Warning(
                "Asset Processor",
                false,
                "BuilderInfoMetricsModel cannot find the GUID of the builder selected by the user (%s) in itself. No metrics will be "
                "shown in the builder tab.",
                builder.m_busId.ToString<AZStd::string>().c_str());
            m_data->m_currentSelectedBuilderIndex = -2;
        }
        
        beginResetModel();
        endResetModel();
    }

    // void BuilderInfoMetricsModel::OnProcessJobDurationChanged([[maybe_unused]] JobEntry jobEntry, [[maybe_unused]] int value)
    // {
    //     // TODO: move it to BuilderData?
    // 
    //     if (m_builderGuidToIndex.contains(jobEntry.m_builderGuid))
    //     {
    //         int builderIndex = m_builderGuidToIndex[jobEntry.m_builderGuid];
    //         
    //         AZStd::string entryName = jobEntry.m_databaseSourceName.toUtf8().constData();
    //         entryName.append(",");
    //         entryName.append(jobEntry.m_jobKey.toUtf8().constData());
    //         entryName.append(",");
    //         entryName.append(jobEntry.m_platformInfo.m_identifier);
    //         m_singleBuilderMetrics[builderIndex]->UpdateOrInsertEntry(BuilderInfoMetricsItem::JobType::ProcessingJob, entryName, 1, value);
    //         m_allBuildersMetrics->UpdateOrInsertEntry(BuilderInfoMetricsItem::JobType::ProcessingJob, entryName, 1, value);
    //     }
    // }

    void BuilderInfoMetricsModel::OnCreateJobsDurationChanged(QString sourceName)
    {
        QString statKey = QString("CreateJobs,").append(sourceName).append("%");
        m_data->m_dbConnection->QueryStatLikeStatName(
            statKey.toUtf8().constData(),
            [this](AzToolsFramework::AssetDatabase::StatDatabaseEntry entry)
            {
                AZStd::vector<AZStd::string> tokens;
                AZ::StringFunc::Tokenize(entry.m_statName, tokens, ',');
                if (tokens.size() == 3) // CreateJobs,filePath,builderName
                {
                    const auto& sourceName = tokens[1];
                    const auto& builderName = tokens[2];
                    if (m_data->m_builderNameToIndex.contains(builderName))
                    {
                        m_data->m_singleBuilderMetrics[m_data->m_builderNameToIndex[builderName]]->UpdateOrInsertEntry(
                            BuilderInfoMetricsItem::JobType::AnalysisJob, sourceName, 1, entry.m_statValue);
                        m_data->m_allBuildersMetrics->UpdateOrInsertEntry(
                            BuilderInfoMetricsItem::JobType::AnalysisJob, builderName + "," + sourceName, 1, entry.m_statValue);
                    }
                    else
                    {
                        AZ_Warning("AssetProcessor", false, "No builder found for an analysis job stat!!!\n");
                    }
                }
                return true;
            });
    }
}
