/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <native/ui/BuilderData.h>
#include <utilities/AssetUtilEBusHelper.h>
#include <native/ui/BuilderInfoMetricsItem.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>

namespace AssetProcessor
{
    BuilderData::BuilderData(AZStd::shared_ptr<AzToolsFramework::AssetDatabase::AssetDatabaseConnection> dbConnection, QObject* parent)
        : m_dbConnection(dbConnection)
        , QObject(parent)
    {
    }

    //! This method runs when this model is initialized. It gets the list of builders, gets existing stats about analysis jobs and
    //! processing jobs, and matches stats with builders and save them appropriately for future use.
    void BuilderData::Reset()
    {
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
    }

    void BuilderData::OnProcessJobDurationChanged([[maybe_unused]] JobEntry jobEntry, [[maybe_unused]] int value)
    {
        AZ_Printf("Asset Processor", "hi, OnProcessJobDurationChanged got called!\n");
    }
}
