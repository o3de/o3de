/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <native/ui/BuilderData.h>
#include <utilities/AssetUtilEBusHelper.h>
#include <native/ui/BuilderDataItem.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/std/smart_ptr/weak_ptr.h>

namespace AssetProcessor
{
    static constexpr char builderNotFoundWarningMessage[] =
        "Found a %s metric entry with builder %s \"%s\", but Asset Processor does not recognize this "
        "builder. Ensure this builder is in the asset folders and its name is shown "
        "in the Builders tab. If this builder was removed intentionally in the past, you can safely ignore this "
        "warning.\n";

    BuilderData::BuilderData(AZStd::shared_ptr<AzToolsFramework::AssetDatabase::AssetDatabaseConnection> dbConnection, QObject* parent)
        : m_dbConnection(dbConnection)
        , QObject(parent)
    {
    }

    void BuilderData::Reset()
    {
        BuilderInfoList builders;
        AssetBuilderInfoBus::Broadcast(&AssetBuilderInfoBus::Events::GetAllBuildersInfo, builders);

        m_root.reset(new BuilderDataItem(BuilderDataItem::ItemType::InvisibleRoot, "", 0, 0, AZStd::weak_ptr<BuilderDataItem>()));

        m_allBuildersMetrics.reset(new BuilderDataItem(BuilderDataItem::ItemType::Builder, "All builders", 0, 0, m_root));
        m_allBuildersMetrics->InitializeBuilder(m_allBuildersMetrics);
        m_singleBuilderMetrics.clear();
        m_builderGuidToIndex.clear();
        m_builderNameToIndex.clear();
        m_currentSelectedBuilderIndex = aznumeric_cast<int>(BuilderSelection::AllBuilders);

        for (int i = 0; i < builders.size(); ++i)
        {
            auto builder = m_singleBuilderMetrics.emplace_back(
                new BuilderDataItem(BuilderDataItem::ItemType::Builder, builders[i].m_name, 0, 0, m_root));
            builder->InitializeBuilder(builder);
            m_builderGuidToIndex[builders[i].m_busId] = i;
            m_builderNameToIndex[builders[i].m_name] = i;
        }

        // CreateJobs stat
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
                            BuilderDataItem::JobType::CreateJobs, sourceName, 1, entry.m_statValue);
                        m_allBuildersMetrics->UpdateOrInsertEntry(
                            BuilderDataItem::JobType::CreateJobs, builderName + "," + sourceName, 1, entry.m_statValue);
                    }
                    else
                    {
                        AZ_Warning("AssetProcessor", false, builderNotFoundWarningMessage, "CreateJobs", "name", builderName.c_str());
                    }
                }
                return true;
            });

        // ProcessJob stat
        m_dbConnection->QueryStatLikeStatName(
            "ProcessJob,%",
            [this](AzToolsFramework::AssetDatabase::StatDatabaseEntry stat)
            {
                AZStd::vector<AZStd::string> tokens;
                AZ::StringFunc::Tokenize(stat.m_statName, tokens, ',');
                if (tokens.size() == 5) // ProcessJob,sourceName,jobKey,platform,builderGuid
                {
                    const auto& builderGuid = AZ::Uuid::CreateString(tokens[4].c_str());

                    if (m_builderGuidToIndex.contains(builderGuid))
                    {
                        AZStd::string entryName;
                        AZ::StringFunc::Join(entryName, tokens.begin() + 1, tokens.begin() + 4, ',');
                        m_singleBuilderMetrics[m_builderGuidToIndex[builderGuid]]->UpdateOrInsertEntry(
                            BuilderDataItem::JobType::ProcessJob, entryName, 1, stat.m_statValue);
                        m_allBuildersMetrics->UpdateOrInsertEntry(
                            BuilderDataItem::JobType::ProcessJob, entryName, 1, stat.m_statValue);
                    }
                    else
                    {
                        AZ_Warning("AssetProcessor", false, builderNotFoundWarningMessage, "ProcessJob", "bus ID", tokens[4].c_str());
                    }
                }
                return true;
            });
    }

    void BuilderData::OnCreateJobsDurationChanged(QString sourceName)
    {
        QString statKey = QString("CreateJobs,").append(sourceName).append("%");
        m_dbConnection->QueryStatLikeStatName(
            statKey.toUtf8().constData(),
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
                        AZStd::shared_ptr<BuilderDataItem> item = nullptr;
                        item = m_singleBuilderMetrics[m_builderNameToIndex[builderName]]->UpdateOrInsertEntry(
                            BuilderDataItem::JobType::CreateJobs, sourceName, 1, entry.m_statValue);
                        Q_EMIT DurationChanged(item.get());
                        item = m_allBuildersMetrics->UpdateOrInsertEntry(
                            BuilderDataItem::JobType::CreateJobs, builderName + "," + sourceName, 1, entry.m_statValue);
                        Q_EMIT DurationChanged(item.get());
                    }
                    else
                    {
                        AZ_Warning("AssetProcessor", false, builderNotFoundWarningMessage, "CreateJobs", "name", builderName.c_str());
                    }
                }
                return true;
            });
    }

    void BuilderData::OnProcessJobDurationChanged(JobEntry jobEntry, int value)
    {
        if (m_builderGuidToIndex.contains(jobEntry.m_builderGuid))
        {
            int builderIndex = m_builderGuidToIndex[jobEntry.m_builderGuid];

            AZStd::string entryName = AZStd::string::format(
                "%s,%s,%s",
                jobEntry.m_databaseSourceName.toUtf8().constData(),
                jobEntry.m_jobKey.toUtf8().constData(),
                jobEntry.m_platformInfo.m_identifier.c_str());

            AZStd::shared_ptr<BuilderDataItem> item = nullptr;
            item = m_singleBuilderMetrics[builderIndex]->UpdateOrInsertEntry(BuilderDataItem::JobType::ProcessJob, entryName, 1, value);
            Q_EMIT DurationChanged(item.get());
            item = m_allBuildersMetrics->UpdateOrInsertEntry(BuilderDataItem::JobType::ProcessJob, entryName, 1, value);
            Q_EMIT DurationChanged(item.get());
        }
    }
}
