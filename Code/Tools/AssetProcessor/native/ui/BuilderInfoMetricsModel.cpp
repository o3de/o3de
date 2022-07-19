/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <ui/BuilderInfoMetricsModel.h>
#include <ui/BuilderInfoMetricsItem.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <utilities/AssetUtilEBusHelper.h>

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

    BuilderInfoMetricsModel::BuilderInfoMetricsModel(
        AZStd::shared_ptr<AzToolsFramework::AssetDatabase::AssetDatabaseConnection> dbConnection, QObject* parent)
        : QAbstractItemModel(parent)
        , m_dbConnection(dbConnection)
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
            parentItem = m_root.get();
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
            parentItem = m_root.get();
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
        auto rootItem = m_root.get();
            
        if (parentItem == rootItem || parentItem == nullptr)
        {
            return QModelIndex();
        }

        QModelIndex parentIndex = createIndex(parentItem->GetRow(), 0, parentItem);
        Q_ASSERT(checkIndex(parentIndex));
        return parentIndex;
    }

    //! This method runs when this model is initialized. It gets the list of builders, gets existing stats about analysis jobs and processing
    //! jobs, and matches stats with builders and save them appropriately for future use.
    void BuilderInfoMetricsModel::Reset()
    {
        beginResetModel();
        BuilderInfoList builders;
        AssetBuilderInfoBus::Broadcast(&AssetBuilderInfoBus::Events::GetAllBuildersInfo, builders);

        m_root.reset(new BuilderInfoMetricsItem(BuilderInfoMetricsItem::ItemType::InvisibleRoot, "", 0, 0, nullptr));
        
        m_allBuildersMetrics.reset(new BuilderInfoMetricsItem(BuilderInfoMetricsItem::ItemType::Builder, "All builders", 0, 0, m_root));
        m_singleBuilderMetrics.clear();
        m_builderGuidToIndex.clear();
        m_builderNameToIndex.clear();
        m_currentSelectedBuilderIndex = -1;

        for (int i = 0; i < builders.size(); ++i)
        {
            m_singleBuilderMetrics.emplace_back(
                new BuilderInfoMetricsItem(BuilderInfoMetricsItem::ItemType::Builder, builders[i].m_name, 0, 0, m_root));
            m_builderGuidToIndex[builders[i].m_busId] = i;
            m_builderNameToIndex[builders[i].m_name] = i;
        }

        // Analysis jobs stat
        m_dbConnection->QueryStatLikeStatName(
            "CreateJobs,%",
            [this](AzToolsFramework::AssetDatabase::StatDatabaseEntry entry)
            {
                AZStd::vector<AZStd::string> tokens;
                AZ::StringFunc::Tokenize(entry.m_statName, tokens, ',');
                if (tokens.size() == 3) // CreateJobs,filePath,builderName
                {
                    const auto& sourceName = tokens[1];
                    const auto& builderName = tokens[2];
                    if (m_builderNameToIndex.contains(builderName))
                    {
                        m_singleBuilderMetrics[m_builderNameToIndex[builderName]]->UpdateOrInsertEntry(
                            BuilderInfoMetricsItem::JobType::AnalysisJob, sourceName, 1, entry.m_statValue);
                        m_allBuildersMetrics->UpdateOrInsertEntry(
                            BuilderInfoMetricsItem::JobType::AnalysisJob, builderName + "," + sourceName, 1, entry.m_statValue);
                    }
                    else
                    {
                        AZ_Warning("AssetProcessor", false, "No builder found for an analysis job stat!!!\n");
                    }
                }
                return true;
            });

        // Processing job stat
        m_dbConnection->QueryStatLikeStatName(
            "ProcessJob,%",
            [this](AzToolsFramework::AssetDatabase::StatDatabaseEntry stat)
            {
                AZStd::vector<AZStd::string> tokens;
                AZ::StringFunc::Tokenize(stat.m_statName, tokens, ',');
                if (tokens.size() == 5) // ProcessJob,sourceName,jobKey,platform,builderGuid
                {
                    // const auto& sourceName = tokens[1];
                    // const auto& jobKey = tokens[2];
                    // const auto& platform = tokens[3];
                    const auto& builderGuid = AZ::Uuid::CreateString(tokens[4].c_str());
                    
                    if (m_builderGuidToIndex.contains(builderGuid))
                    {
                        AZStd::string entryName;
                        AZ::StringFunc::Join(entryName, tokens.begin() + 1, tokens.begin() + 4, ',');
                        m_singleBuilderMetrics[m_builderGuidToIndex[builderGuid]]->UpdateOrInsertEntry(
                            BuilderInfoMetricsItem::JobType::ProcessingJob, entryName, 1, stat.m_statValue);
                        m_allBuildersMetrics->UpdateOrInsertEntry(
                            BuilderInfoMetricsItem::JobType::ProcessingJob, entryName, 1, stat.m_statValue);
                    }
                    else
                    {
                        AZ_Warning("AssetProcessor", false, "No builder found for a processing job stat!!!\n");
                    }
                }
                return true;
            });
        endResetModel();
    }
    void BuilderInfoMetricsModel::OnBuilderSelectionChanged(const AssetBuilderSDK::AssetBuilderDesc& builder)
    {
        // TODO: need to handle the "all builders" case

        if (m_builderGuidToIndex.contains(builder.m_busId))
        {
            m_currentSelectedBuilderIndex = m_builderGuidToIndex[builder.m_busId];
            m_root->SetChild(m_singleBuilderMetrics[m_currentSelectedBuilderIndex]);
        }
        else
        {
            AZ_Warning(
                "Asset Processor",
                false,
                "BuilderInfoMetricsModel cannot find the GUID of the builder selected by the user (%s) in itself. No metrics will be "
                "shown in the builder tab.",
                builder.m_busId.ToString<AZStd::string>().c_str());
            m_currentSelectedBuilderIndex = -2;
        }
        
        beginResetModel();
        endResetModel();
    }

    void BuilderInfoMetricsModel::OnJobProcessingStatChanged(JobEntry jobEntry, int value)
    {
        if (m_builderGuidToIndex.contains(jobEntry.m_builderGuid))
        {
            int builderIndex = m_builderGuidToIndex[jobEntry.m_builderGuid];
            
            AZStd::string entryName = jobEntry.m_databaseSourceName.toUtf8().constData();
            entryName.append(",");
            entryName.append(jobEntry.m_jobKey.toUtf8().constData());
            entryName.append(",");
            entryName.append(jobEntry.m_platformInfo.m_identifier);
            m_singleBuilderMetrics[builderIndex]->UpdateOrInsertEntry(BuilderInfoMetricsItem::JobType::ProcessingJob, entryName, 1, value);
            m_allBuildersMetrics->UpdateOrInsertEntry(BuilderInfoMetricsItem::JobType::ProcessingJob, entryName, 1, value);
        }
    }
}
