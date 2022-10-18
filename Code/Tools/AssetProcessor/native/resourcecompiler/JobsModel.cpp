/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/resourcecompiler/JobsModel.h>
#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include <native/utilities/assetUtils.h>
#include <native/AssetDatabase/AssetDatabase.h>
#include <native/resourcecompiler/RCJobSortFilterProxyModel.h>
#include <native/utilities/JobDiagnosticTracker.h>
#include <AzCore/StringFunc/StringFunc.h>

namespace AssetProcessor
{
    JobsModel::JobsModel(QObject* parent)
        : QAbstractItemModel(parent)
        , m_pendingIcon(QStringLiteral(":/stylesheet/img/logging/pending.svg"))
        , m_errorIcon(QStringLiteral(":/stylesheet/img/logging/error.svg"))
        , m_failureIcon(QStringLiteral(":/stylesheet/img/logging/failure.svg"))
        , m_warningIcon(QStringLiteral(":/stylesheet/img/logging/warning.svg"))
        , m_okIcon(QStringLiteral(":/stylesheet/img/logging/valid.svg"))
        , m_processingIcon(QStringLiteral(":/stylesheet/img/logging/processing.svg"))
    {
    }

    JobsModel::~JobsModel()
    {
        for (int idx = 0; idx < m_cachedJobs.size(); idx++)
        {
            delete m_cachedJobs[idx];
        }

        m_cachedJobs.clear();
        m_cachedJobsLookup.clear();
    }

    QModelIndex JobsModel::parent(const QModelIndex& index) const
    {
        AZ_UNUSED(index);
        return QModelIndex();
    }

    QModelIndex JobsModel::index(int row, int column, const QModelIndex& parent) const
    {
        if (row >= rowCount(parent) || column >= columnCount(parent))
        {
            return QModelIndex();
        }
        return createIndex(row, column);
    }

    int JobsModel::rowCount(const QModelIndex& parent) const
    {
        return parent.isValid() ? 0 : itemCount();
    }

    int JobsModel::columnCount(const QModelIndex& parent) const
    {
        return parent.isValid() ? 0 : Column::Max;
    }

    QVariant JobsModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation != Qt::Horizontal)
        {
            return QAbstractItemModel::headerData(section, orientation, role);
        }

        switch (role)
        {
            case Qt::DisplayRole:
            {
                switch (section)
                {
                case ColumnStatus:
                    return tr("Status");
                case ColumnSource:
                    return tr("Source");
                case ColumnPlatform:
                    return tr("Platform");
                case ColumnJobKey:
                    return tr("Job Key");
                case ColumnCompleted:
                    return tr("Completed");
                case ColumnProcessDuration:
                    return tr("Last Processing Job Duration");
                default:
                    break;
                }
            }

            case Qt::TextAlignmentRole:
            {
                return Qt::AlignLeft + Qt::AlignVCenter;
            }

            default:
                break;
        }

        return QAbstractItemModel::headerData(section, orientation, role);
    }

    int JobsModel::itemCount() const
    {
        return aznumeric_caster(m_cachedJobs.size());
    }


    QVariant JobsModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
        {
            return QVariant();
        }

        if (index.row() >= itemCount())
        {
            return QVariant();
        }

        switch (role)
        {
        case Qt::DecorationRole:
        {
            if (index.column() == ColumnStatus) {
                using namespace AzToolsFramework::AssetSystem;

                switch (getItem(index.row())->m_jobState) {
                case JobStatus::Queued:
                    return m_pendingIcon;
                case JobStatus::Failed_InvalidSourceNameExceedsMaxLimit:  // fall through intentional
                case JobStatus::Failed:
                    return m_failureIcon;
                case JobStatus::Completed:
                {
                    CachedJobInfo* jobInfo = getItem(index.row());

                    if(jobInfo->m_errorCount > 0)
                    {
                        return m_errorIcon;
                    }

                    if(jobInfo->m_warningCount > 0)
                    {
                        return m_warningIcon;
                    }

                    return m_okIcon;
                }
                case JobStatus::InProgress:
                    return m_processingIcon;
                }
            }

            break;
        }
        case Qt::DisplayRole:
        case SortRole:
            switch (index.column())
            {
            case ColumnStatus:
            {
                CachedJobInfo* jobInfo = getItem(index.row());
                return GetStatusInString(jobInfo->m_jobState, jobInfo->m_warningCount, jobInfo->m_errorCount);
            }
            case ColumnSource:
                return getItem(index.row())->m_elementId.GetSourceAssetReference().RelativePath().c_str();
            case ColumnPlatform:
                return getItem(index.row())->m_elementId.GetPlatform();
            case ColumnJobKey:
                return getItem(index.row())->m_elementId.GetJobDescriptor();
            case ColumnCompleted:
                if (role == SortRole)
                {
                    return getItem(index.row())->m_completedTime;
                }
                else
                {
                    return getItem(index.row())->m_completedTime.toString("hh:mm:ss.zzz MMM dd, yyyy");
                }
            case ColumnProcessDuration:
                if (role == SortRole)
                {
                    return getItem(index.row())->m_processDuration;
                }
                else
                {
                    QTime processDuration = getItem(index.row())->m_processDuration;
                    if (!processDuration.isValid())
                    {
                        return "";
                    }

                    return processDuration.toString("hh:mm:ss.zzz");
                }
            default:
                break;
            }
        case logRole:
        {
            using namespace AzToolsFramework::AssetSystem;
            using namespace AssetUtilities;

            JobInfo jobInfo;
            AssetJobLogResponse jobLogResponse;
            auto* cachedJobInfo = getItem(index.row());
            jobInfo.m_sourceFile = cachedJobInfo->m_elementId.GetSourceAssetReference().RelativePath().Native();
            jobInfo.m_watchFolder = cachedJobInfo->m_elementId.GetSourceAssetReference().ScanFolderPath().Native();
            jobInfo.m_platform = cachedJobInfo->m_elementId.GetPlatform().toUtf8().data();
            jobInfo.m_jobKey = cachedJobInfo->m_elementId.GetJobDescriptor().toUtf8().data();
            jobInfo.m_builderGuid = cachedJobInfo->m_builderGuid;
            jobInfo.m_jobRunKey = cachedJobInfo->m_jobRunKey;
            jobInfo.m_warningCount = cachedJobInfo->m_warningCount;
            jobInfo.m_errorCount = cachedJobInfo->m_errorCount;

            ReadJobLogResult readJobLogResult = ReadJobLog(jobInfo, jobLogResponse);

            const char* jobLogData = jobLogResponse.m_jobLog.c_str();

            // ReadJobLog prepends the result with Error: if it can't find the file, even if the job was
            // completed successfully or is still pending, so we detect that and give a less panicky response
            // to the end user.
            if (readJobLogResult == ReadJobLogResult::MissingLogFile)
            {
                switch (cachedJobInfo->m_jobState)
                {
                    case JobStatus::Completed:
                        jobLogData = "The log file from the last (successful) run of this job could not be found.\nLogs are not always generated for successful jobs and this does not indicate an error.";
                        break;

                    case JobStatus::InProgress:
                    case JobStatus::Queued:
                        jobLogData = "The job is still processing and the log file has not yet been created";
                        break;

                    default:
                        // leave the job log as it is
                        break;
                }
            }

            return QVariant(jobLogData);
        }
        case Qt::TextAlignmentRole:
        {
            return Qt::AlignLeft + Qt::AlignVCenter;
        }
        case statusRole:
        {
            CachedJobInfo* jobInfo = getItem(index.row());
            return QVariant::fromValue(JobStatusInfo{ jobInfo->m_jobState, jobInfo->m_warningCount, jobInfo->m_errorCount });
        }
        case logFileRole:
        {
            AzToolsFramework::AssetSystem::JobInfo jobInfo;
            CachedJobInfo* cachedJobInfo = getItem(index.row());
            jobInfo.m_sourceFile = cachedJobInfo->m_elementId.GetSourceAssetReference().RelativePath().Native();
            jobInfo.m_watchFolder = cachedJobInfo->m_elementId.GetSourceAssetReference().ScanFolderPath().Native();
            jobInfo.m_platform = cachedJobInfo->m_elementId.GetPlatform().toUtf8().data();
            jobInfo.m_jobKey = cachedJobInfo->m_elementId.GetJobDescriptor().toUtf8().data();
            jobInfo.m_builderGuid = cachedJobInfo->m_builderGuid;
            jobInfo.m_jobRunKey = cachedJobInfo->m_jobRunKey;
            jobInfo.m_warningCount = cachedJobInfo->m_warningCount;
            jobInfo.m_errorCount = cachedJobInfo->m_errorCount;

            AZStd::string logFile = AssetUtilities::ComputeJobLogFolder() + "/" + AssetUtilities::ComputeJobLogFileName(jobInfo);
            return QVariant(logFile.c_str());
        }

        default:
            break;
        }

        return QVariant();
    }

    CachedJobInfo* JobsModel::getItem(int index) const
    {
        if (index >= 0 && index < m_cachedJobs.size())
        {
            return m_cachedJobs[index];
        }

        return nullptr; //invalid index
    }

    void Append(QString& base, const QString& input, const QString& seperator = ", ")
    {
        if(input.isEmpty())
        {
            return;
        }

        if(!base.isEmpty())
        {
            base.append(seperator);
        }

        base.append(input);
    }

    QString JobsModel::GetStatusInString(const AzToolsFramework::AssetSystem::JobStatus& state, AZ::u32 warningCount, AZ::u32 errorCount)
    {
        using namespace AzToolsFramework::AssetSystem;

        switch (state)
        {
            case JobStatus::Queued:
                return tr("Pending");
            case JobStatus::Failed_InvalidSourceNameExceedsMaxLimit:  // fall through intentional
            case JobStatus::Failed:
                return tr("Failed");
            case JobStatus::Completed:
            {
                QString message = tr("Completed");
                QString extra;

                if (warningCount > 0)
                {
                    extra.append(QString("%1 %2").arg(warningCount).arg( warningCount == 1 ? tr("warning") : tr("warnings") ));
                }

                if (errorCount > 0)
                {
                    Append(extra, QString("%1 %2").arg(errorCount).arg( errorCount == 1 ? tr("error") : tr("errors") ));
                }

                Append(message, extra, ": ");

                return message;
            }
            case JobStatus::InProgress:
                return tr("InProgress");
        }

        return QString();
    }

    void JobsModel::PopulateJobsFromDatabase()
    {
        beginResetModel();
        AZStd::string databaseLocation;
        AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Broadcast(&AzToolsFramework::AssetDatabase::AssetDatabaseRequests::GetAssetDatabaseLocation, databaseLocation);

        if (!databaseLocation.empty())
        {
            AssetProcessor::AssetDatabaseConnection assetDatabaseConnection;
            assetDatabaseConnection.OpenDatabase();

            // Get historical ProcessJob stats from asset database
            AZStd::unordered_map<QueueElementID, AZ::s64> historicalStats;
            auto statsFunction = [&historicalStats](AzToolsFramework::AssetDatabase::StatDatabaseEntry entry)
            {
                static constexpr int numTokensExpected = 6;
                AZStd::vector<AZStd::string> tokens;
                AZ::StringFunc::Tokenize(entry.m_statName, tokens, ',');

                if (tokens.size() == numTokensExpected)
                {
                    QueueElementID elementId;
                    elementId.SetSourceAssetReference(SourceAssetReference(tokens[1].c_str(), tokens[2].c_str()));
                    elementId.SetJobDescriptor(tokens[3].c_str());
                    elementId.SetPlatform(tokens[4].c_str());
                    historicalStats[elementId] = entry.m_statValue;
                    AZ_UNUSED(historicalStats);
                }
                else
                {
                    AZ_Warning(
                        "AssetProcessor",
                        false,
                        "ProcessJob stat entry \"%s\" could not be parsed and will not be used. Expected %d tokens, but found %d. A wrong "
                        "stat name may be used in Asset Processor code, or the asset database may be corrupted. If you keep encountering "
                        "this warning, report an issue on GitHub with O3DE version number.",
                        entry.m_statName.c_str(),
                        numTokensExpected,
                        tokens.size());
                }

                return true;
            };
            assetDatabaseConnection.QueryStatLikeStatName("ProcessJob,%", statsFunction);

            // Get jobs from asset database
            auto jobsFunction = [this, &assetDatabaseConnection, &historicalStats](AzToolsFramework::AssetDatabase::JobDatabaseEntry& entry)
            {
                AzToolsFramework::AssetDatabase::SourceDatabaseEntry source;
                assetDatabaseConnection.GetSourceBySourceID(entry.m_sourcePK, source);

                AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry scanFolder;
                assetDatabaseConnection.GetScanFolderByScanFolderID(source.m_scanFolderPK, scanFolder);

                CachedJobInfo* jobInfo = new CachedJobInfo();
                jobInfo->m_elementId.SetSourceAssetReference(SourceAssetReference(scanFolder.m_scanFolder.c_str(), source.m_sourceName.c_str()));
                jobInfo->m_elementId.SetPlatform(entry.m_platform.c_str());
                jobInfo->m_elementId.SetJobDescriptor(entry.m_jobKey.c_str());
                jobInfo->m_jobState = entry.m_status;
                jobInfo->m_jobRunKey = aznumeric_cast<uint32_t>(entry.m_jobRunKey);
                jobInfo->m_builderGuid = entry.m_builderGuid;
                jobInfo->m_completedTime = QDateTime::fromMSecsSinceEpoch(entry.m_lastLogTime);
                jobInfo->m_warningCount = entry.m_warningCount;
                jobInfo->m_errorCount = entry.m_errorCount;
                if (historicalStats.count(jobInfo->m_elementId))
                {
                    jobInfo->m_processDuration =
                        QTime::fromMSecsSinceStartOfDay(aznumeric_cast<int>(historicalStats.at(jobInfo->m_elementId)));
                }
                m_cachedJobs.push_back(jobInfo);
                m_cachedJobsLookup.insert(jobInfo->m_elementId, aznumeric_caster(m_cachedJobs.size() - 1));
                return true;
            };
            assetDatabaseConnection.QueryJobsTable(jobsFunction);
        }

        endResetModel();
    }

    QModelIndex JobsModel::GetJobFromProduct(const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& productEntry, AzToolsFramework::AssetDatabase::AssetDatabaseConnection& assetDatabaseConnection)
    {
        AZStd::string sourceForProduct;
        AZ::s64 scanFolderId;
        assetDatabaseConnection.QuerySourceByProductID(
            productEntry.m_productID,
            [&](AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceEntry)
        {
            sourceForProduct = sourceEntry.m_sourceName;
                scanFolderId = sourceEntry.m_scanFolderPK;
            return false;
        });

        if (sourceForProduct.empty())
        {
            return QModelIndex();
        }

        AZStd::string scanFolderPath;
        assetDatabaseConnection.QueryScanFolderByScanFolderID(scanFolderId, [&](AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry& entry)
        {
            scanFolderPath = entry.m_scanFolder;
            return false;
        });

        if (scanFolderPath.empty())
        {
            return QModelIndex();
        }

        AzToolsFramework::AssetDatabase::JobDatabaseEntry foundJobEntry;
        assetDatabaseConnection.QueryJobByProductID(
            productEntry.m_productID,
            [&](AzToolsFramework::AssetDatabase::JobDatabaseEntry& jobEntry)
        {
            foundJobEntry = jobEntry;
            return false;
        });

        if (foundJobEntry.m_jobID == AzToolsFramework::AssetDatabase::InvalidEntryId)
        {
            return QModelIndex();
        }
        return GetJobFromSourceAndJobInfo(SourceAssetReference(scanFolderPath.c_str(), sourceForProduct.c_str()), foundJobEntry.m_platform, foundJobEntry.m_jobKey);
    }

    QModelIndex JobsModel::GetJobFromSourceAndJobInfo(const SourceAssetReference& sourceAsset, const AZStd::string& platform, const AZStd::string& jobKey)
    {
        QueueElementID elementId(sourceAsset, platform.c_str(), jobKey.c_str());
        auto iter = m_cachedJobsLookup.find(elementId);
        if (iter == m_cachedJobsLookup.end())
        {
            return QModelIndex();
        }

        return index(iter.value(), 0, QModelIndex());
    }

    void JobsModel::OnJobStatusChanged(JobEntry entry, AzToolsFramework::AssetSystem::JobStatus status)
    {
        QueueElementID elementId(entry.m_sourceAssetReference, entry.m_platformInfo.m_identifier.c_str(), entry.m_jobKey);
        CachedJobInfo* jobInfo = nullptr;
        unsigned int jobIndex = 0;

        JobDiagnosticInfo jobDiagnosticInfo{};
        JobDiagnosticRequestBus::BroadcastResult(jobDiagnosticInfo, &JobDiagnosticRequestBus::Events::GetDiagnosticInfo, entry.m_jobRunKey);

        auto iter = m_cachedJobsLookup.find(elementId);
        if (iter == m_cachedJobsLookup.end())
        {
            jobInfo = new CachedJobInfo();
            jobInfo->m_elementId.SetSourceAssetReference(entry.m_sourceAssetReference);
            jobInfo->m_elementId.SetPlatform(entry.m_platformInfo.m_identifier.c_str());
            jobInfo->m_elementId.SetJobDescriptor(entry.m_jobKey.toUtf8().data());
            jobInfo->m_jobRunKey = aznumeric_cast<uint32_t>(entry.m_jobRunKey);
            jobInfo->m_builderGuid = entry.m_builderGuid;
            jobInfo->m_jobState = status;
            jobInfo->m_warningCount = jobDiagnosticInfo.m_warningCount;
            jobInfo->m_errorCount = jobDiagnosticInfo.m_errorCount;
            jobIndex = aznumeric_caster(m_cachedJobs.size());
            beginInsertRows(QModelIndex(), jobIndex, jobIndex);
            m_cachedJobs.push_back(jobInfo);
            m_cachedJobsLookup.insert(jobInfo->m_elementId, jobIndex);
            endInsertRows();
        }
        else
        {
            jobIndex = iter.value();
            jobInfo = m_cachedJobs[jobIndex];
            jobInfo->m_jobState = status;
            jobInfo->m_jobRunKey = aznumeric_cast<uint32_t>(entry.m_jobRunKey);
            jobInfo->m_builderGuid = entry.m_builderGuid;
            jobInfo->m_warningCount = jobDiagnosticInfo.m_warningCount;
            jobInfo->m_errorCount = jobDiagnosticInfo.m_errorCount;
            if (jobInfo->m_jobState == AzToolsFramework::AssetSystem::JobStatus::Completed || jobInfo->m_jobState == AzToolsFramework::AssetSystem::JobStatus::Failed)
            {
                jobInfo->m_completedTime = QDateTime::currentDateTime();
            }
            else
            {
                jobInfo->m_completedTime = QDateTime();
            }
            Q_EMIT dataChanged(index(jobIndex, 0, QModelIndex()), index(jobIndex, columnCount() - 1, QModelIndex()));
        }
    }

    void JobsModel::OnJobProcessDurationChanged(JobEntry jobEntry, int durationMs)
    {
        QueueElementID elementId(jobEntry.m_sourceAssetReference, jobEntry.m_platformInfo.m_identifier.c_str(), jobEntry.m_jobKey);

        if (auto iter = m_cachedJobsLookup.find(elementId); iter != m_cachedJobsLookup.end())
        {
            unsigned int jobIndex = iter.value();
            CachedJobInfo* jobInfo = m_cachedJobs[jobIndex];
            jobInfo->m_processDuration = QTime::fromMSecsSinceStartOfDay(durationMs);
            Q_EMIT dataChanged(
                index(jobIndex, ColumnProcessDuration, QModelIndex()), index(jobIndex, ColumnProcessDuration, QModelIndex()));
        }
    }

    void JobsModel::OnSourceRemoved(const SourceAssetReference& sourceAsset)
    {
        // when a source is removed, we need to eliminate all job entries for that source regardless of all other details of it.
        QList<AssetProcessor::QueueElementID> elementsToRemove;
        for (int index = 0; index < m_cachedJobs.size(); ++index)
        {
            if (m_cachedJobs[index]->m_elementId.GetSourceAssetReference() == sourceAsset)
            {
                elementsToRemove.push_back(m_cachedJobs[index]->m_elementId);
            }
        }

        // now that we've collected all the elements to remove, we can remove them.
        // Doing it this way avoids problems with mutating these cache structures while iterating them.
        for (const AssetProcessor::QueueElementID& removal : elementsToRemove)
        {
            RemoveJob(removal);
        }
    }

    void JobsModel::OnJobRemoved(AzToolsFramework::AssetSystem::JobInfo jobInfo)
    {
        RemoveJob(QueueElementID(SourceAssetReference(jobInfo.m_watchFolder.c_str(), jobInfo.m_sourceFile.c_str()), jobInfo.m_platform.c_str(), jobInfo.m_jobKey.c_str()));
    }

    void JobsModel::RemoveJob(const AssetProcessor::QueueElementID& elementId)
    {
        auto iter = m_cachedJobsLookup.find(elementId);
        if (iter != m_cachedJobsLookup.end())
        {
            unsigned int jobIndex = iter.value();
            CachedJobInfo* jobInfo = m_cachedJobs[jobIndex];
            beginRemoveRows(QModelIndex(), jobIndex, jobIndex);
            m_cachedJobs.erase(m_cachedJobs.begin() + jobIndex);
            delete jobInfo;
            m_cachedJobsLookup.erase(iter);
            // Since we are storing the jobIndex for each job for faster lookup therefore
            // we need to update the jobIndex for jobs that were after the removed job.
            for (int idx = jobIndex; idx < m_cachedJobs.size(); idx++)
            {
                jobInfo = m_cachedJobs[idx];
                auto iterator = m_cachedJobsLookup.find(jobInfo->m_elementId);
                if (iterator != m_cachedJobsLookup.end())
                {
                    unsigned int index = iterator.value();
                    m_cachedJobsLookup[jobInfo->m_elementId] = --index;
                }
            }
            endRemoveRows();
        }
    }
} //namespace AssetProcessor
