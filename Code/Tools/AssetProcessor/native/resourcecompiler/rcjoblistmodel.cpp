/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "rcjoblistmodel.h"


// uncomment this in order to add additional verbose log output for this class.
// can drastically slow down function since this class is a hotspot
// #define DEBUG_RCJOB_MODEL

namespace AssetProcessor
{
    RCJobListModel::RCJobListModel(QObject* parent)
        : QAbstractItemModel(parent)
    {

    }

    int RCJobListModel::rowCount(const QModelIndex& parent) const
    {
        if (parent.isValid())
        {
            return 0;
        }
        return itemCount();
    }

    QModelIndex RCJobListModel::parent(const QModelIndex& /*index*/) const
    {
        return QModelIndex();
    }

    QModelIndex RCJobListModel::index(int row, int column, const QModelIndex& parent) const
    {
        if (row >= rowCount(parent) || column >= columnCount(parent))
        {
            return QModelIndex();
        }
        return createIndex(row, column);
    }

    int RCJobListModel::columnCount(const QModelIndex& parent) const
    {
        return parent.isValid() ? 0 : Column::Max;
    }

    QVariant RCJobListModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        {
            switch (section)
            {
            case ColumnState:
                return tr("State");
            case ColumnJobId:
                return tr("Job Id");
            case ColumnCommand:
                return tr("Asset");
            case ColumnCompleted:
                return tr("Completed");
            case ColumnPlatform:
                return tr("Platform");
            default:
                break;
            }
        }

        return QAbstractItemModel::headerData(section, orientation, role);
    }

    unsigned int RCJobListModel::jobsInFlight() const
    {
        return m_jobsInFlight.size();
    }

    unsigned int RCJobListModel::jobsInQueueWithoutMissingDependencies() const
    {
        unsigned int jobsWithNoMissingDependencies = 0;
        for (const auto& job : m_jobsInQueueLookup)
        {
            if (!job->HasMissingSourceDependency())
            {
                ++jobsWithNoMissingDependencies;
            }
        }
        return jobsWithNoMissingDependencies;
    }

    unsigned int RCJobListModel::jobsPendingCatalog() const
    {
        return m_finishedJobsNotInCatalog.count();
    }

    void RCJobListModel::UpdateJobEscalation(AssetProcessor::RCJob* rcJob, int jobEscalation)
    {
        for (int idx = 0; idx < rowCount(); ++idx)
        {
            RCJob* job = getItem(idx);
            if (job == rcJob)
            {
                rcJob->SetJobEscalation(jobEscalation);
                Q_EMIT dataChanged(index(idx, 0), index(idx, columnCount() - 1));
                break;
            }
        }
    }

    void RCJobListModel::UpdateRow(int jobIndex)
    {
        Q_EMIT dataChanged(index(jobIndex, 0), index(jobIndex, columnCount() - 1));
    }

    QVariant RCJobListModel::data(const QModelIndex& index, int role) const
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
        case jobIndexRole:
            return getItem(index.row())->GetJobEntry().m_jobRunKey;
        case stateRole:
            return RCJob::GetStateDescription(getItem(index.row())->GetState());
        case displayNameRole:
            return getItem(index.row())->GetJobEntry().m_sourceAssetReference.RelativePath().c_str();
        case timeCreatedRole:
            return getItem(index.row())->GetTimeCreated().toString("hh:mm:ss.zzz");
        case timeLaunchedRole:
            return getItem(index.row())->GetTimeLaunched().toString("hh:mm:ss.zzz");
        case timeCompletedRole:
            return getItem(index.row())->GetTimeCompleted().toString("hh:mm:ss.zzz");

        case Qt::DisplayRole:
            switch (index.column())
            {
            case ColumnJobId:
                return getItem(index.row())->GetJobEntry().m_jobRunKey;
            case ColumnState:
                return RCJob::GetStateDescription(getItem(index.row())->GetState());
            case ColumnCommand:
                return getItem(index.row())->GetJobEntry().m_sourceAssetReference.RelativePath().c_str();
            case ColumnCompleted:
                return getItem(index.row())->GetTimeCompleted().toString("hh:mm:ss.zzz");
            case ColumnPlatform:
                return QString::fromUtf8(getItem(index.row())->GetPlatformInfo().m_identifier.c_str());
            default:
                break;
            }
        default:
            break;
        }

        return QVariant();
    }

    int RCJobListModel::itemCount() const
    {
        return aznumeric_caster(m_jobs.size());
    }

    RCJob* RCJobListModel::getItem(int index) const
    {
        if (index >= 0 && index < m_jobs.size())
        {
            return m_jobs[index];
        }

        return nullptr; //invalid index
    }

    bool RCJobListModel::isEmpty()
    {
        return m_jobs.empty();
    }

    void RCJobListModel::addNewJob(RCJob* rcJob)
    {
        int posForInsert = aznumeric_caster(m_jobs.size());
        beginInsertRows(QModelIndex(), posForInsert, posForInsert);

        m_jobs.push_back(rcJob);
#if defined(DEBUG_RCJOB_MODEL)
        AZ_TracePrintf(AssetProcessor::DebugChannel, "JobTrace AddNewJob(%i  %s,%s,%s)\n", rcJob, rcJob->GetInputFileAbsolutePath().toUtf8().constData(), rcJob->GetPlatformInfo().m_identifier.c_str(), rcJob->GetJobKey().toUtf8().constData());
#endif

        if (rcJob->GetState() == RCJob::pending)
        {
            m_jobsInQueueLookup.insert(rcJob->GetElementID(), rcJob);
        }
        endInsertRows();
    }

    void RCJobListModel::markAsProcessing(RCJob* rcJob)
    {
#if defined(DEBUG_RCJOB_MODEL)
        AZ_TracePrintf(AssetProcessor::DebugChannel, "JobTrace markAsProcessing(%i %s,%s,%s)\n", rcJob, rcJob->GetJobEntry().m_databaseSourceName.toUtf8().constData(), rcJob->GetPlatformInfo().m_identifier.c_str(), rcJob->GetJobKey().toUtf8().constData());
#endif

        rcJob->SetState(RCJob::processing);
        rcJob->SetTimeLaunched(QDateTime::currentDateTime());

        m_jobsInFlight.insert(rcJob);

        for(int jobIndex = static_cast<int>(m_jobs.size()) - 1; jobIndex >= 0; --jobIndex)
        {
            if(m_jobs[jobIndex] == rcJob)
            {
                Q_EMIT dataChanged(index(jobIndex, 0, QModelIndex()), index(jobIndex, 0, QModelIndex()));
                return;
            }
        }

        AZ_TracePrintf(AssetProcessor::DebugChannel, "JobTrace jobIndex == -1!!! (%i %s,%s,%s)\n",
            rcJob, rcJob->GetJobEntry().GetAbsoluteSourcePath().toUtf8().constData(),
            rcJob->GetPlatformInfo().m_identifier.c_str(),
            rcJob->GetJobKey().toUtf8().constData());
        AZ_Assert(false, "Job not found!!!");
    }

    void RCJobListModel::markAsStarted(RCJob* rcJob)
    {
#if defined(DEBUG_RCJOB_MODEL)
        AZ_TracePrintf(AssetProcessor::DebugChannel, "JobTrace markAsStarted(%i %s,%s,%s)\n", rcJob, rcJob->GetInputFileAbsolutePath().toUtf8().constData(), rcJob->GetPlatformInfo().m_identifier.c_str(), rcJob->GetJobKey().toUtf8().constData());
#endif

        auto foundInQueue = m_jobsInQueueLookup.find(rcJob->GetElementID());
        while ((foundInQueue != m_jobsInQueueLookup.end()) && (foundInQueue.value() == rcJob))
        {
            foundInQueue = m_jobsInQueueLookup.erase(foundInQueue);
        }
    }

    void RCJobListModel::markAsCompleted(RCJob* rcJob)
    {
#if defined(DEBUG_RCJOB_MODEL)
        AZ_TracePrintf(AssetProcessor::DebugChannel, "JobTrace markAsCompleted(%i %s,%s,%s)\n", rcJob, rcJob->GetInputFileAbsolutePath().toUtf8().constData(), rcJob->GetPlatformInfo().m_identifier.c_str(), rcJob->GetJobKey().toUtf8().constData());
#endif
        rcJob->SetTimeCompleted(QDateTime::currentDateTime());

        auto foundInQueue = m_jobsInQueueLookup.find(rcJob->GetElementID());
        while ((foundInQueue != m_jobsInQueueLookup.end()) && (foundInQueue.value() == rcJob))
        {
            foundInQueue = m_jobsInQueueLookup.erase(foundInQueue);
        }

        for (int jobIndex = static_cast<int>(m_jobs.size()) - 1; jobIndex >= 0; --jobIndex)
        {
            if(m_jobs[jobIndex] == rcJob)
            {
                m_jobsInFlight.remove(rcJob);

                // remove it from the list and delete it - there is a separate model that keeps track for the GUI so no need to keep jobs around.
                {
#if defined(DEBUG_RCJOB_MODEL)
                    AZ_TracePrintf(AssetProcessor::DebugChannel, "JobTrace =>JobCompleted(%i %s,%s,%s)\n", rcJob, rcJob->GetJobEntry().m_databaseSourceName.toUtf8().constData(), rcJob->GetPlatformInfo().m_identifier.c_str(), rcJob->GetJobKey().toUtf8().constData());
#endif
                    beginRemoveRows(QModelIndex(), jobIndex, jobIndex);
                    m_jobs.erase(m_jobs.begin() + jobIndex);
                    endRemoveRows();

                    // Only completed jobs need to wait on a catalog write
                    if (rcJob->GetState() == RCJob::completed)
                    {
                        const auto& id = rcJob->GetElementID();

                        auto itr = m_finishedJobsNotInCatalog.find(id);
                        if (itr != m_finishedJobsNotInCatalog.end())
                        {
                            itr.value()++;
                        }
                        else
                        {
                            m_finishedJobsNotInCatalog.insert(id, 1);
                        }
                    }

                    rcJob->deleteLater();
                }

                return;
            }
        }

        AZ_Error(
            AssetProcessor::ConsoleChannel,
            false,
            "Programmer Error: Could not mark job for file %s as completed, job was not tracked in the m_jobs container. It was either already finished, or never queued. (platform:%s, job key:%s)\n",
            rcJob->GetJobEntry().GetAbsoluteSourcePath().toUtf8().constData(),
            rcJob->GetPlatformInfo().m_identifier.c_str(),
            rcJob->GetJobKey().toUtf8().constData());
    }

    void RCJobListModel::markAsCataloged(const AssetProcessor::QueueElementID& check)
    {
        auto itr = m_finishedJobsNotInCatalog.find(check);

        if(itr == m_finishedJobsNotInCatalog.end())
        {
            AZ_Assert(false, "Attempting to mark a job as written to the catalog before the job has been put in the waiting queue! %s", check.GetSourceAssetReference().AbsolutePath().c_str());
            return;
        }

        itr.value()--;

        if (itr.value() == 0)
        {
            m_finishedJobsNotInCatalog.erase(itr);
        }
    }

    bool RCJobListModel::isInFlight(const AssetProcessor::QueueElementID& check) const
    {
        for (auto rcJob : m_jobsInFlight)
        {
            if (check == rcJob->GetElementID())
            {
                return true;
            }
        }
        return false;
    }

    int RCJobListModel::GetIndexOfJobByState(const QueueElementID& elementId, RCJob::JobState jobState)
    {
        for (int idx = 0; idx < rowCount(); ++idx)
        {
            RCJob* job = getItem(idx);
            if (job->GetState() == jobState && job->GetElementID() == elementId)
            {
                return idx;
                break;
            }
        }

        return -1; // invalid index
    }

    void RCJobListModel::EraseJobs(const SourceAssetReference& sourceAsset, AZStd::vector<RCJob*>& pendingJobs)
    {
        for (int jobIdx = 0; jobIdx < rowCount(); ++jobIdx)
        {
            RCJob* job = getItem(jobIdx);
            if (job->GetJobEntry().m_sourceAssetReference == sourceAsset)
            {
                const QueueElementID& target = job->GetElementID();
                if ((isInQueue(target)) || (isInFlight(target)))
                {
                    // Its important that this still follows the 'cancelled' flow, so that other parts of the code can update their "in progress" and other maps.
                    AZ_TracePrintf(
                        AssetProcessor::DebugChannel,
                        "Cancelling Job [%s, %s, %s] because the source file no longer exists.\n",
                        target.GetSourceAssetReference().AbsolutePath().c_str(),
                        target.GetPlatform().toUtf8().data(),
                        target.GetJobDescriptor().toUtf8().data());

                    // if a job is pending, it was never started and thus will never enter Finished state,
                    // so simply changing its state to cancelled is not enough, collect them and return to rccontroller to process manually
                    if (job->GetState() == RCJob::JobState::pending)
                    {
                        pendingJobs.push_back(job);
                    }

                    job->SetState(RCJob::JobState::cancelled);
                    AssetBuilderSDK::JobCommandBus::Event(job->GetJobEntry().m_jobRunKey, &AssetBuilderSDK::JobCommandBus::Events::Cancel);
                    UpdateRow(jobIdx);
                }
            }
        }

    }

    bool RCJobListModel::isInQueue(const AssetProcessor::QueueElementID& check) const
    {
        return m_jobsInQueueLookup.contains(check);
    }

    bool RCJobListModel::isWaitingOnCatalog(const QueueElementID& check) const
    {
        return m_finishedJobsNotInCatalog.contains(check);
    }

    void RCJobListModel::PerformHeuristicSearch(QString searchTerm, QString platform, QSet<QueueElementID>& found, AssetProcessor::JobIdEscalationList& escalationList, bool isStatusRequest, int searchRules)
    {
        int escalationValue = 0;
        if (isStatusRequest)
        {
            escalationValue = AssetProcessor::JobEscalation::ProcessAssetRequestStatusEscalation;
        }
        else
        {
            escalationValue = AssetProcessor::JobEscalation::ProcessAssetRequestSyncEscalation;
        }

        // try to narrowly exact-match the search term in case the search term refers to a specific actual source file:
        for (const RCJob* rcJob : m_jobs)
        {
            if ((platform != rcJob->GetPlatformInfo().m_identifier.c_str()) || (rcJob->GetState() != RCJob::pending))
            {
                continue;
            }
            QString input = rcJob->GetJobEntry().m_sourceAssetReference.RelativePath().c_str();
            if (input.endsWith(searchTerm, Qt::CaseInsensitive))
            {
                AZ_TracePrintf(
                    AssetProcessor::DebugChannel,
                    "Job Queue: Heuristic search found exact match (%s,%s,%s).\n",
                    rcJob->GetJobEntry().GetAbsoluteSourcePath().toUtf8().constData(),
                    rcJob->GetPlatformInfo().m_identifier.c_str(),
                    rcJob->GetJobKey().toUtf8().constData());
                found.insert(QueueElementID(rcJob->GetJobEntry().m_sourceAssetReference, platform, rcJob->GetJobKey()));
                escalationList.append(qMakePair(rcJob->GetJobEntry().m_jobRunKey, escalationValue));
            }
        }

        for (const RCJob* rcJob : m_jobsInFlight)
        {
            if (platform != rcJob->GetPlatformInfo().m_identifier.c_str())
            {
                continue;
            }
            QString input = rcJob->GetJobEntry().m_sourceAssetReference.RelativePath().c_str();
            if (input.endsWith(searchTerm, Qt::CaseInsensitive))
            {
                AZ_TracePrintf(
                    AssetProcessor::DebugChannel,
                    "Job Queue: Heuristic search found exact match (%s,%s,%s).\n",
                    rcJob->GetJobEntry().GetAbsoluteSourcePath().toUtf8().constData(),
                    rcJob->GetPlatformInfo().m_identifier.c_str(),
                    rcJob->GetJobKey().toUtf8().constData());
                found.insert(QueueElementID(rcJob->GetJobEntry().m_sourceAssetReference, platform, rcJob->GetJobKey()));
            }
        }

        if (!found.isEmpty() || searchRules == AzFramework::AssetSystem::RequestAssetStatus::SearchType::Exact)
        {
            return;
        }

        // broaden the heuristic.  Try without extensions - that is, ignore everything after the dot.
        // if there are dashes, ignore them also.  This is how you match "blah.dds" to actually mean "blah.tif"
        // since we have no idea what products will be generated by a source still in the queue until it runs
        int dotIndex = searchTerm.lastIndexOf('.');
        QStringRef searchTermWithNoExtension = searchTerm.midRef(0, dotIndex);

        if (dotIndex != -1)
        {
            for (const auto& rcJob : m_jobs)
            {
                if ((platform != rcJob->GetPlatformInfo().m_identifier.c_str()) || (rcJob->GetState() != RCJob::pending))
                {
                    continue;
                }
                QString input = rcJob->GetJobEntry().m_sourceAssetReference.RelativePath().c_str();
                dotIndex = input.lastIndexOf('.');
                if (dotIndex != -1)
                {
                    QStringRef testref = input.midRef(0, dotIndex);
                    if (testref.endsWith(searchTermWithNoExtension, Qt::CaseInsensitive))
                    {
                        AZ_TracePrintf(
                            AssetProcessor::DebugChannel,
                            "Job Queue: Heuristic search found broad match (%s,%s,%s).\n",
                            rcJob->GetJobEntry().GetAbsoluteSourcePath().toUtf8().constData(),
                            rcJob->GetPlatformInfo().m_identifier.c_str(),
                            rcJob->GetJobKey().toUtf8().constData());
                        found.insert(QueueElementID(rcJob->GetJobEntry().m_sourceAssetReference, platform, rcJob->GetJobKey()));
                        escalationList.append(qMakePair(rcJob->GetJobEntry().m_jobRunKey, escalationValue));
                    }
                }
            }

            for (const RCJob* rcJob : m_jobsInFlight)
            {
                if (platform != rcJob->GetPlatformInfo().m_identifier.c_str())
                {
                    continue;
                }
                QString input = rcJob->GetJobEntry().m_sourceAssetReference.RelativePath().c_str();
                dotIndex = input.lastIndexOf('.');
                if (dotIndex != -1)
                {
                    QStringRef testref = input.midRef(0, dotIndex);
                    if (testref.endsWith(searchTermWithNoExtension, Qt::CaseInsensitive))
                    {
                        AZ_TracePrintf(
                            AssetProcessor::DebugChannel,
                            "Job Queue: Heuristic search found broad match (%s,%s,%s).\n",
                            rcJob->GetJobEntry().GetAbsoluteSourcePath().toUtf8().constData(),
                            rcJob->GetPlatformInfo().m_identifier.c_str(),
                            rcJob->GetJobKey().toUtf8().constData());
                        found.insert(QueueElementID(rcJob->GetJobEntry().m_sourceAssetReference, platform, rcJob->GetJobKey()));
                    }
                }
            }
        }

        if (!found.isEmpty())
        {
            return;
        }

        // broaden the heuristic further.  Eliminate anything after the last underscore in the file name
        // (so blahblah_diff.dds just becomes blahblah) and then allow anything which has that file somewhere in it.

        int slashIndex = searchTerm.lastIndexOf('/');
        int dashIndex = searchTerm.lastIndexOf('_');
        QStringRef searchTermWithNoSuffix = searchTermWithNoExtension;
        if ((dashIndex != -1) && (slashIndex == -1) || (dashIndex > slashIndex))
        {
            searchTermWithNoSuffix = searchTermWithNoSuffix.mid(0, dashIndex);
        }

        for (const auto& rcJob : m_jobs)
        {
            if ((platform != rcJob->GetPlatformInfo().m_identifier.c_str()) || (rcJob->GetState() != RCJob::pending))
            {
                continue;
            }
            QString input = rcJob->GetJobEntry().m_sourceAssetReference.RelativePath().c_str();
            if (input.contains(searchTermWithNoSuffix, Qt::CaseInsensitive)) //notice here that we use simply CONTAINS instead of endswith - this can potentially be very broad!
            {
                AZ_TracePrintf(
                    AssetProcessor::DebugChannel,
                    "Job Queue: Heuristic search found ultra-broad match (%s,%s,%s).\n",
                    rcJob->GetJobEntry().GetAbsoluteSourcePath().toUtf8().constData(),
                    rcJob->GetPlatformInfo().m_identifier.c_str(),
                    rcJob->GetJobKey().toUtf8().constData());
                found.insert(QueueElementID(rcJob->GetJobEntry().m_sourceAssetReference, platform, rcJob->GetJobKey()));
                escalationList.append(qMakePair(rcJob->GetJobEntry().m_jobRunKey, escalationValue));
            }
        }

        for (const RCJob* rcJob : m_jobsInFlight)
        {
            if (platform != rcJob->GetPlatformInfo().m_identifier.c_str())
            {
                continue;
            }

            QString input = rcJob->GetJobEntry().m_sourceAssetReference.RelativePath().c_str();
            if (input.contains(searchTermWithNoSuffix, Qt::CaseInsensitive)) //notice here that we use simply CONTAINS instead of endswith - this can potentially be very broad!
            {
                AZ_TracePrintf(
                    AssetProcessor::DebugChannel,
                    "Job Queue: Heuristic search found ultra-broad match (%s,%s,%s).\n",
                    rcJob->GetJobEntry().GetAbsoluteSourcePath().toUtf8().constData(),
                    rcJob->GetPlatformInfo().m_identifier.c_str(),
                    rcJob->GetJobKey().toUtf8().constData());
                found.insert(QueueElementID(rcJob->GetJobEntry().m_sourceAssetReference, platform, rcJob->GetJobKey()));
            }
        }
    }

    void RCJobListModel::PerformUUIDSearch(AZ::Uuid searchUuid, QString platform, QSet<QueueElementID>& found, AssetProcessor::JobIdEscalationList& escalationList, bool isStatusRequest)
    {
        int escalationValue = 0;
        if (isStatusRequest)
        {
            escalationValue = AssetProcessor::JobEscalation::ProcessAssetRequestStatusEscalation;
        }
        else
        {
            escalationValue = AssetProcessor::JobEscalation::ProcessAssetRequestSyncEscalation;
        }

        for (const RCJob* rcJob : m_jobs)
        {
            if ((platform != rcJob->GetPlatformInfo().m_identifier.c_str()) || (rcJob->GetState() != RCJob::pending))
            {
                continue;
            }

            if (rcJob->GetJobEntry().m_sourceFileUUID == searchUuid)
            {
                found.insert(QueueElementID(rcJob->GetJobEntry().m_sourceAssetReference, platform, rcJob->GetJobKey()));
                escalationList.append(qMakePair(rcJob->GetJobEntry().m_jobRunKey, escalationValue));
            }
        }
    }
}// namespace AssetProcessor

