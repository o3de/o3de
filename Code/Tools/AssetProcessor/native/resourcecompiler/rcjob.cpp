/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "rcjob.h"

#include <AzToolsFramework/UI/Logging/LogLine.h>
#include <AzToolsFramework/Metadata/UuidUtils.h>

#include <native/utilities/BuilderManager.h>
#include <native/utilities/ThreadHelper.h>

#include <QtConcurrent/QtConcurrentRun>
#include <QElapsedTimer>

#include "native/utilities/JobDiagnosticTracker.h"

#include <qstorageinfo.h>
#include <native/utilities/ProductOutputUtil.h>

namespace
{
    bool s_typesRegistered = false;
    // You have up to 60 minutes to finish processing an asset.
    // This was increased from 10 to account for PVRTC compression
    // taking up to an hour for large normal map textures, and should
    // be reduced again once we move to the ASTC compression format, or
    // find another solution to reduce processing times to be reasonable.
    const unsigned int g_jobMaximumWaitTime = 1000 * 60 * 60;

    const unsigned int g_sleepDurationForLockingAndFingerprintChecking = 100;

    const unsigned int g_timeoutInSecsForRetryingCopy = 30;

    const char* const s_tempString = "%TEMP%";
    const char* const s_jobLogFileName = "jobLog.xml";

    bool MoveCopyFile(QString sourceFile, QString productFile, bool isCopyJob = false)
    {
        if (!isCopyJob && (AssetUtilities::MoveFileWithTimeout(sourceFile, productFile, g_timeoutInSecsForRetryingCopy)))
        {
            //We do not want to rename the file if it is a copy job
            return true;
        }
        else if (AssetUtilities::CopyFileWithTimeout(sourceFile, productFile, g_timeoutInSecsForRetryingCopy))
        {
            // try to copy instead
            return true;
        }
        AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Failed to move OR copy file from Source directory: %s  to Destination Directory: %s", sourceFile.toUtf8().data(), productFile.toUtf8().data());
        return false;
    }
}

using namespace AssetProcessor;

bool Params::IsValidParams() const
{
    return !m_cacheOutputDir.empty() && !m_intermediateOutputDir.empty() && !m_relativePath.empty();
}

bool RCParams::IsValidParams() const
{
    return (
        (!m_rcExe.isEmpty()) &&
        (!m_rootDir.isEmpty()) &&
        (!m_inputFile.isEmpty()) &&
        Params::IsValidParams()
        );
}

namespace AssetProcessor
{
    RCJob::RCJob(QObject* parent)
        : QObject(parent)
        , m_timeCreated(QDateTime::currentDateTime())
        , m_scanFolderID(0)
    {
        m_jobState = RCJob::pending;

        if (!s_typesRegistered)
        {
            qRegisterMetaType<RCParams>("RCParams");
            qRegisterMetaType<BuilderParams>("BuilderParams");
            qRegisterMetaType<JobOutputInfo>("JobOutputInfo");
            s_typesRegistered = true;
        }
    }

    RCJob::~RCJob()
    {
    }

    void RCJob::Init(JobDetails& details)
    {
        m_jobDetails = AZStd::move(details);
        m_queueElementID = QueueElementID(GetJobEntry().m_sourceAssetReference, GetPlatformInfo().m_identifier.c_str(), GetJobKey());
    }

    const JobEntry& RCJob::GetJobEntry() const
    {
        return m_jobDetails.m_jobEntry;
    }

    bool RCJob::HasMissingSourceDependency() const
    {
        return m_jobDetails.m_hasMissingSourceDependency;
    }

    QDateTime RCJob::GetTimeCreated() const
    {
        return m_timeCreated;
    }

    void RCJob::SetTimeCreated(const QDateTime& timeCreated)
    {
        m_timeCreated = timeCreated;
    }

    QDateTime RCJob::GetTimeLaunched() const
    {
        return m_timeLaunched;
    }

    void RCJob::SetTimeLaunched(const QDateTime& timeLaunched)
    {
        m_timeLaunched = timeLaunched;
    }

    QDateTime RCJob::GetTimeCompleted() const
    {
        return m_timeCompleted;
    }

    void RCJob::SetTimeCompleted(const QDateTime& timeCompleted)
    {
        m_timeCompleted = timeCompleted;
    }

    AZ::u32 RCJob::GetOriginalFingerprint() const
    {
        return m_jobDetails.m_jobEntry.m_computedFingerprint;
    }

    void RCJob::SetOriginalFingerprint(unsigned int fingerprint)
    {
        m_jobDetails.m_jobEntry.m_computedFingerprint = fingerprint;
    }

    RCJob::JobState RCJob::GetState() const
    {
        return m_jobState;
    }

    void RCJob::SetState(const JobState& state)
    {
        bool wasPending = (m_jobState == pending);
        m_jobState = state;

        if ((wasPending)&&(m_jobState == cancelled))
        {
            // if we were pending (had not started yet) and we are now canceled, we still have to emit the finished signal
            // so that all the various systems waiting for us can do their housekeeping.
            Q_EMIT Finished();
        }
    }

    void RCJob::SetJobEscalation(int jobEscalation)
    {
        m_JobEscalation = jobEscalation;
    }

    void RCJob::SetCheckExclusiveLock(bool value)
    {
        m_jobDetails.m_jobEntry.m_checkExclusiveLock = value;
    }

    QString RCJob::GetStateDescription(const RCJob::JobState& state)
    {
        switch (state)
        {
        case RCJob::pending:
            return tr("Pending");
        case RCJob::processing:
            return tr("Processing");
        case RCJob::completed:
            return tr("Completed");
        case RCJob::crashed:
            return tr("Crashed");
        case RCJob::terminated:
            return tr("Terminated");
        case RCJob::failed:
            return tr("Failed");
        case RCJob::cancelled:
            return tr("Cancelled");
        }
        return QString();
    }

    const AZ::Uuid& RCJob::GetInputFileUuid() const
    {
        return m_jobDetails.m_jobEntry.m_sourceFileUUID;
    }

    AZ::IO::Path RCJob::GetCacheOutputPath() const
    {
        return m_jobDetails.m_cachePath;
    }

    AZ::IO::Path RCJob::GetIntermediateOutputPath() const
    {
        return m_jobDetails.m_intermediatePath;
    }

    AZ::IO::Path RCJob::GetRelativePath() const
    {
        return m_jobDetails.m_relativePath;
    }

    const AssetBuilderSDK::PlatformInfo& RCJob::GetPlatformInfo() const
    {
        return m_jobDetails.m_jobEntry.m_platformInfo;
    }

    AssetBuilderSDK::ProcessJobResponse& RCJob::GetProcessJobResponse()
    {
        return m_processJobResponse;
    }

    void RCJob::PopulateProcessJobRequest(AssetBuilderSDK::ProcessJobRequest& processJobRequest)
    {
        processJobRequest.m_jobDescription.m_critical = IsCritical();
        processJobRequest.m_jobDescription.m_additionalFingerprintInfo = m_jobDetails.m_extraInformationForFingerprinting;
        processJobRequest.m_jobDescription.m_jobKey = GetJobKey().toUtf8().data();
        processJobRequest.m_jobDescription.m_jobParameters = AZStd::move(m_jobDetails.m_jobParam);
        processJobRequest.m_jobDescription.SetPlatformIdentifier(GetPlatformInfo().m_identifier.c_str());
        processJobRequest.m_jobDescription.m_priority = GetPriority();
        processJobRequest.m_platformInfo = GetPlatformInfo();
        processJobRequest.m_builderGuid = GetBuilderGuid();
        processJobRequest.m_sourceFile = GetJobEntry().m_sourceAssetReference.RelativePath().c_str();
        processJobRequest.m_sourceFileUUID = GetInputFileUuid();
        processJobRequest.m_watchFolder = GetJobEntry().m_sourceAssetReference.ScanFolderPath().c_str();
        processJobRequest.m_fullPath = GetJobEntry().GetAbsoluteSourcePath().toUtf8().data();
        processJobRequest.m_jobId = GetJobEntry().m_jobRunKey;
    }

    QString RCJob::GetJobKey() const
    {
        return m_jobDetails.m_jobEntry.m_jobKey;
    }

    AZ::Uuid RCJob::GetBuilderGuid() const
    {
        return m_jobDetails.m_jobEntry.m_builderGuid;
    }

    bool RCJob::IsCritical() const
    {
        return m_jobDetails.m_critical;
    }

    bool RCJob::IsAutoFail() const
    {
        return m_jobDetails.m_autoFail;
    }

    int RCJob::GetPriority() const
    {
        return m_jobDetails.m_priority;
    }

    const AZStd::vector<AssetProcessor::JobDependencyInternal>& RCJob::GetJobDependencies()
    {
        return m_jobDetails.m_jobDependencyList;
    }

    void RCJob::Start()
    {
        // the following trace can be uncommented if there is a need to deeply inspect job running.
        //AZ_TracePrintf(AssetProcessor::DebugChannel, "JobTrace Start(%i %s,%s,%s)\n", this, GetInputFileAbsolutePath().toUtf8().data(), GetPlatform().toUtf8().data(), GetJobKey().toUtf8().data());

        AssetUtilities::QuitListener listener;
        listener.BusConnect();
        RCParams rc(this);
        BuilderParams builderParams(this);

        //Create the process job request
        AssetBuilderSDK::ProcessJobRequest processJobRequest;
        PopulateProcessJobRequest(processJobRequest);

        builderParams.m_processJobRequest = processJobRequest;
        builderParams.m_cacheOutputDir = GetCacheOutputPath();
        builderParams.m_intermediateOutputDir = GetIntermediateOutputPath();
        builderParams.m_relativePath = GetRelativePath();
        builderParams.m_assetBuilderDesc = m_jobDetails.m_assetBuilderDesc;
        builderParams.m_sourceUuid = m_jobDetails.m_sourceUuid;

        // when the job finishes, record the results and emit Finished()
        connect(this, &RCJob::JobFinished, this, [this](AssetBuilderSDK::ProcessJobResponse result)
            {
                m_processJobResponse = AZStd::move(result);
                switch (m_processJobResponse.m_resultCode)
                {
                case AssetBuilderSDK::ProcessJobResult_Crashed:
                    {
                        SetState(crashed);
                    }
                    break;
                case AssetBuilderSDK::ProcessJobResult_Success:
                    {
                        SetState(completed);
                    }
                    break;
                case AssetBuilderSDK::ProcessJobResult_Cancelled:
                    {
                        SetState(cancelled);
                    }
                    break;
                default:
                    {
                        SetState(failed);
                    }
                    break;
                }
                Q_EMIT Finished();
            });

        if (!listener.WasQuitRequested())
        {
            QtConcurrent::run(&RCJob::ExecuteBuilderCommand, builderParams);
        }
        else
        {
            AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Job canceled due to quit being requested.");
            SetState(terminated);
            Q_EMIT Finished();
        }
        listener.BusDisconnect();
    }

    void RCJob::ExecuteBuilderCommand(BuilderParams builderParams)
    {
        // Note: this occurs inside a worker thread.

        // Signal start and end of the job
        ScopedJobSignaler signaler;

        // listen for the user quitting (CTRL-C or otherwise)
        AssetUtilities::QuitListener listener;
        listener.BusConnect();
        QElapsedTimer ticker;
        ticker.start();
        AssetBuilderSDK::ProcessJobResponse result;
        AssetBuilderSDK::JobCancelListener cancelListener(builderParams.m_processJobRequest.m_jobId);

        if (builderParams.m_rcJob->m_jobDetails.m_autoFail)
        {
            // if this is an auto-fail job, we should avoid doing any additional work besides the work required to fail the job and
            // write the details into its log.  This is because Auto-fail jobs have 'incomplete' job descriptors, and only exist to
            // force a job to fail with a reasonable log file stating the reason for failure.  An example of where it is useful to
            // use auto-fail jobs is when, after compilation was successful, something goes wrong integrating the result into the
            // cache.  (For example, files collide, or the product file name would be too long).  The job will have at that point
            // already completed, the thread long gone, so we can 'append' to the log in this manner post-build by creating a new
            // job that will automatically fail and ingest the old (success) log along with additional fail reasons and then fail.
            AutoFailJob(builderParams);
            result.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            Q_EMIT builderParams.m_rcJob->JobFinished(result);
            return;
        }

        // If requested, make sure we can open the file with exclusive permissions
        QString inputFile = builderParams.m_rcJob->GetJobEntry().GetAbsoluteSourcePath();
        if (builderParams.m_rcJob->GetJobEntry().m_checkExclusiveLock && QFile::exists(inputFile))
        {
            // We will only continue once we get exclusive lock on the source file
            while (!AssetUtilities::CheckCanLock(inputFile))
            {
                // Wait for a while before checking again, we need to let some time pass for the other process to finish whatever work it is doing
                QThread::msleep(g_sleepDurationForLockingAndFingerprintChecking);

                // If AP shutdown is requested, the job is canceled or we exceeded the max wait time, abort the loop and mark the job as canceled
                if (listener.WasQuitRequested() || cancelListener.IsCancelled() || (ticker.elapsed() > g_jobMaximumWaitTime))
                {
                    result.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
                    Q_EMIT builderParams.m_rcJob->JobFinished(result);
                    return;
                }
            }
        }

        Q_EMIT builderParams.m_rcJob->BeginWork();
        // We will actually start working on the job after this point and even if RcController gets the same job again, we will put it in the queue for processing
        builderParams.m_rcJob->DoWork(result, builderParams, listener);
        Q_EMIT builderParams.m_rcJob->JobFinished(result);
    }

    void RCJob::AutoFailJob(BuilderParams& builderParams)
    {
        // force the fail data to be captured to the log file.
        // because this is being executed in a thread worker, this won't stomp the main thread's job id.
        AssetProcessor::SetThreadLocalJobId(builderParams.m_rcJob->GetJobEntry().m_jobRunKey);
        AssetUtilities::JobLogTraceListener jobLogTraceListener(builderParams.m_rcJob->m_jobDetails.m_jobEntry);

#if defined(AZ_ENABLE_TRACING)
        QString sourceFullPath(builderParams.m_processJobRequest.m_fullPath.c_str());
        auto failReason = builderParams.m_processJobRequest.m_jobDescription.m_jobParameters.find(AZ_CRC_CE(AssetProcessor::AutoFailReasonKey));
        if (failReason != builderParams.m_processJobRequest.m_jobDescription.m_jobParameters.end())
        {
            // you are allowed to have many lines in your fail reason.
            AZ_Error(AssetBuilderSDK::ErrorWindow, false, "Failed processing %s", sourceFullPath.toUtf8().data());
            AZStd::vector<AZStd::string> delimited;
            AzFramework::StringFunc::Tokenize(failReason->second.c_str(), delimited, "\n");
            for (const AZStd::string& token : delimited)
            {
                AZ_Error(AssetBuilderSDK::ErrorWindow, false, "%s", token.c_str());
            }
        }
        else
        {
            // since we didn't have a custom auto-fail reason, add a token to the log file that will help with
            // forensic debugging to differentiate auto-fails from regular fails (although it should also be
            // obvious from the output in other ways)
            AZ_TracePrintf("Debug", "(auto-failed)\n");
        }
        auto failLogFile = builderParams.m_processJobRequest.m_jobDescription.m_jobParameters.find(AZ_CRC_CE(AssetProcessor::AutoFailLogFile));
        if (failLogFile != builderParams.m_processJobRequest.m_jobDescription.m_jobParameters.end())
        {
            AzToolsFramework::Logging::LogLine::ParseLog(failLogFile->second.c_str(), failLogFile->second.size(),
                [](AzToolsFramework::Logging::LogLine& target)
            {
                switch (target.GetLogType())
                {
                case AzToolsFramework::Logging::LogLine::TYPE_DEBUG:
                    AZ_TracePrintf(target.GetLogWindow().c_str(), "%s", target.GetLogMessage().c_str());
                    break;
                case AzToolsFramework::Logging::LogLine::TYPE_MESSAGE:
                    AZ_TracePrintf(target.GetLogWindow().c_str(), "%s", target.GetLogMessage().c_str());
                    break;
                case AzToolsFramework::Logging::LogLine::TYPE_WARNING:
                    AZ_Warning(target.GetLogWindow().c_str(), false, "%s", target.GetLogMessage().c_str());
                    break;
                case AzToolsFramework::Logging::LogLine::TYPE_ERROR:
                    AZ_Error(target.GetLogWindow().c_str(), false, "%s", target.GetLogMessage().c_str());
                    break;
                case AzToolsFramework::Logging::LogLine::TYPE_CONTEXT:
                    AZ_TracePrintf(target.GetLogWindow().c_str(), " %s", target.GetLogMessage().c_str());
                    break;
                }
            });
        }
#endif

        // note that this line below is printed out to be consistent with the output from a job that normally failed, so
        // applications reading log file will find it.
        AZ_Error(AssetBuilderSDK::ErrorWindow, false, "Builder indicated that the job has failed.\n");

        if (builderParams.m_processJobRequest.m_jobDescription.m_jobParameters.find(AZ_CRC_CE(AssetProcessor::AutoFailOmitFromDatabaseKey)) != builderParams.m_processJobRequest.m_jobDescription.m_jobParameters.end())
        {
            // we don't add Auto-fail jobs to the database if they have asked to be emitted.
            builderParams.m_rcJob->m_jobDetails.m_jobEntry.m_addToDatabase = false;
        }

        AssetProcessor::SetThreadLocalJobId(0);
    }


    void RCJob::DoWork(AssetBuilderSDK::ProcessJobResponse& result, BuilderParams& builderParams, AssetUtilities::QuitListener& listener)
    {
        // Setting job id for logging purposes
        AssetProcessor::SetThreadLocalJobId(builderParams.m_rcJob->GetJobEntry().m_jobRunKey);
        AssetUtilities::JobLogTraceListener jobLogTraceListener(builderParams.m_rcJob->m_jobDetails.m_jobEntry);

        {
            AssetBuilderSDK::JobCancelListener JobCancelListener(builderParams.m_rcJob->m_jobDetails.m_jobEntry.m_jobRunKey);
            result.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed; // failed by default

#if defined(AZ_ENABLE_TRACING)
            for (const auto& warningMessage : builderParams.m_rcJob->m_jobDetails.m_warnings)
            {
                // you are allowed to have many lines in your warning message.
                AZStd::vector<AZStd::string> delimited;
                AzFramework::StringFunc::Tokenize(warningMessage.c_str(), delimited, "\n");
                for (const AZStd::string& token : delimited)
                {
                    AZ_Warning(AssetBuilderSDK::WarningWindow, false, "%s", token.c_str());
                }
                jobLogTraceListener.AddWarning();
            }
#endif

            // create a temporary directory for Builder to work in.
            // lets make it as a subdir of a known temp dir

            QString workFolder;

            if (!AssetUtilities::CreateTempWorkspace(workFolder))
            {
                AZ_Error(AssetBuilderSDK::ErrorWindow, false, "Could not create temporary directory for Builder!\n");
                result.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                Q_EMIT builderParams.m_rcJob->JobFinished(result);
                return;
            }

            builderParams.m_processJobRequest.m_tempDirPath = AZStd::string(workFolder.toUtf8().data());

            QString sourceFullPath(builderParams.m_processJobRequest.m_fullPath.c_str());

            if (sourceFullPath.length() >= ASSETPROCESSOR_WARN_PATH_LEN && sourceFullPath.length() < ASSETPROCESSOR_TRAIT_MAX_PATH_LEN)
            {
                AZ_Warning(
                    AssetBuilderSDK::WarningWindow,
                    false,
                    "Source Asset: %s filepath length %d exceeds the suggested max path length (%d). This may not work on all platforms.\n",
                    sourceFullPath.toUtf8().data(),
                    sourceFullPath.length(),
                    ASSETPROCESSOR_WARN_PATH_LEN);
            }
            if (sourceFullPath.length() >= ASSETPROCESSOR_TRAIT_MAX_PATH_LEN)
            {
                AZ_Warning(
                    AssetBuilderSDK::WarningWindow,
                    false,
                    "Source Asset: %s filepath length %d exceeds the maximum path length (%d) allowed.\n",
                    sourceFullPath.toUtf8().data(),
                    sourceFullPath.length(),
                    ASSETPROCESSOR_TRAIT_MAX_PATH_LEN);
                result.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            }
            else
            {
                if (!JobCancelListener.IsCancelled())
                {
                    bool runProcessJob = true;
                    if (m_jobDetails.m_checkServer)
                    {
                        AssetServerMode assetServerMode = AssetServerMode::Inactive;
                        AssetServerBus::BroadcastResult(assetServerMode, &AssetServerBus::Events::GetRemoteCachingMode);

                        QFileInfo fileInfo(builderParams.m_processJobRequest.m_sourceFile.c_str());
                        builderParams.m_serverKey = QString("%1_%2_%3_%4")
                            .arg(fileInfo.completeBaseName(),
                                 builderParams.m_processJobRequest.m_jobDescription.m_jobKey.c_str(),
                                 builderParams.m_processJobRequest.m_platformInfo.m_identifier.c_str())
                            .arg(builderParams.m_rcJob->GetOriginalFingerprint());
                        bool operationResult = false;
                        if (assetServerMode == AssetServerMode::Server)
                        {
                            // sending process job command to the builder
                            builderParams.m_assetBuilderDesc.m_processJobFunction(builderParams.m_processJobRequest, result);
                            runProcessJob = false;
                            if (result.m_resultCode == AssetBuilderSDK::ProcessJobResult_Success)
                            {
                                auto beforeStoreResult = BeforeStoringJobResult(builderParams, result);
                                if (beforeStoreResult.IsSuccess())
                                {
                                    AssetProcessor::AssetServerBus::BroadcastResult(operationResult, &AssetProcessor::AssetServerBusTraits::StoreJobResult, builderParams, beforeStoreResult.GetValue());
                                }
                                else
                                {
                                    AZ_Warning(AssetBuilderSDK::WarningWindow, false, "Failed preparing store result for %s", builderParams.m_processJobRequest.m_sourceFile.c_str());
                                }

                                if (!operationResult)
                                {
                                    AZ_TracePrintf(AssetProcessor::DebugChannel, "Unable to save job (%s, %s, %s) with fingerprint (%u) to the server.\n",
                                        builderParams.m_rcJob->GetJobEntry().m_sourceAssetReference.AbsolutePath().c_str(), builderParams.m_rcJob->GetJobKey().toUtf8().data(),
                                        builderParams.m_rcJob->GetPlatformInfo().m_identifier.c_str(), builderParams.m_rcJob->GetOriginalFingerprint());
                                }
                                else
                                {
                                    for (auto& product : result.m_outputProducts)
                                    {
                                        product.m_outputFlags |= AssetBuilderSDK::ProductOutputFlags::CachedAsset;
                                    }
                                }
                            }
                        }
                        else if (assetServerMode == AssetServerMode::Client)
                        {
                            // running as client, check with the server whether it has already
                            // processed this asset, if not or if the operation fails then process locally
                            AssetProcessor::AssetServerBus::BroadcastResult(operationResult, &AssetProcessor::AssetServerBusTraits::RetrieveJobResult, builderParams);

                            if (operationResult)
                            {
                                operationResult = AfterRetrievingJobResult(builderParams, jobLogTraceListener, result);
                            }
                            else
                            {
                                AZ_TracePrintf(AssetProcessor::DebugChannel, "Unable to get job (%s, %s, %s) with fingerprint (%u) from the server. Processing locally.\n",
                                    builderParams.m_rcJob->GetJobEntry().m_sourceAssetReference.AbsolutePath().c_str(), builderParams.m_rcJob->GetJobKey().toUtf8().data(),
                                    builderParams.m_rcJob->GetPlatformInfo().m_identifier.c_str(), builderParams.m_rcJob->GetOriginalFingerprint());
                            }

                            if (operationResult)
                            {
                                for (auto& product : result.m_outputProducts)
                                {
                                    product.m_outputFlags |= AssetBuilderSDK::ProductOutputFlags::CachedAsset;
                                }
                            }

                            runProcessJob = !operationResult;
                        }
                    }

                    if(runProcessJob)
                    {
                        result.m_outputProducts.clear();
                        // sending process job command to the builder
                        builderParams.m_assetBuilderDesc.m_processJobFunction(builderParams.m_processJobRequest, result);
                    }
                }
            }

            if (JobCancelListener.IsCancelled())
            {
                result.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            }
        }

        bool shouldRemoveTempFolder = true;

        if (result.m_resultCode == AssetBuilderSDK::ProcessJobResult_Success)
        {
            // do a final check of this job to make sure its not making colliding subIds.
            AZStd::unordered_map<AZ::u32, AZStd::string> subIdsFound;
            for (const AssetBuilderSDK::JobProduct& product : result.m_outputProducts)
            {
                if (!subIdsFound.insert({ product.m_productSubID, product.m_productFileName }).second)
                {
                    // if this happens the element was already in the set.
                    AZ_Error(AssetBuilderSDK::ErrorWindow, false,
                        "The builder created more than one asset with the same subID (%u) when emitting product %.*s, colliding with %.*s\n  Builders should set a unique m_productSubID value for each product, as this is used as part of the address of the asset.",
                        product.m_productSubID,
                        AZ_STRING_ARG(product.m_productFileName),
                        AZ_STRING_ARG(subIdsFound[product.m_productSubID]));
                    result.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                    break;
                }
            }
        }

        if(result.m_resultCode == AssetBuilderSDK::ProcessJobResult_Success)
        {
            bool handledDependencies = true; // True in case there are no outputs

            for (const AssetBuilderSDK::JobProduct& jobProduct : result.m_outputProducts)
            {
                handledDependencies = false; // False by default since there are outputs

                if(jobProduct.m_dependenciesHandled)
                {
                    handledDependencies = true;
                    break;
                }
            }

            if(!handledDependencies)
            {
                AZ_Warning(AssetBuilderSDK::WarningWindow, false, "The builder (%s) has not indicated it handled outputting product dependencies for file %s.  This is a programmer error.", builderParams.m_assetBuilderDesc.m_name.c_str(), builderParams.m_processJobRequest.m_sourceFile.c_str());
                AZ_Warning(AssetBuilderSDK::WarningWindow, false, "For builders that output AZ serialized types, it is recommended to use AssetBuilderSDK::OutputObject which will handle outputting product depenedencies and creating the JobProduct.  This is fine to use even if your builder never has product dependencies.");
                AZ_Warning(AssetBuilderSDK::WarningWindow, false, "For builders that need custom depenedency parsing that cannot be handled by AssetBuilderSDK::OutputObject or ones that output non-AZ serialized types, add the dependencies to m_dependencies and m_pathDependencies on the JobProduct and then set m_dependenciesHandled to true.");
                jobLogTraceListener.AddWarning();
            }

            WarningLevel warningLevel = WarningLevel::Default;
            JobDiagnosticRequestBus::BroadcastResult(warningLevel, &JobDiagnosticRequestBus::Events::GetWarningLevel);
            const bool hasErrors = jobLogTraceListener.GetErrorCount() > 0;
            const bool hasWarnings = jobLogTraceListener.GetWarningCount() > 0;

            if(warningLevel == WarningLevel::FatalErrors && hasErrors)
            {
                AZ_Error(AssetBuilderSDK::ErrorWindow, false, "Failing job, fatal errors setting is enabled");
                result.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            }
            else if(warningLevel == WarningLevel::FatalErrorsAndWarnings && (hasErrors || hasWarnings))
            {
                AZ_Error(AssetBuilderSDK::ErrorWindow, false, "Failing job, fatal errors and warnings setting is enabled");
                result.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            }
        }

        switch (result.m_resultCode)
        {
        case AssetBuilderSDK::ProcessJobResult_Success:
            // make sure there's no subid collision inside a job.
            {
                if (!CopyCompiledAssets(builderParams, result))
                {
                    result.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                    shouldRemoveTempFolder = false;
                }
                shouldRemoveTempFolder = shouldRemoveTempFolder && !result.m_keepTempFolder && !s_createRequestFileForSuccessfulJob;
            }
            break;

        case AssetBuilderSDK::ProcessJobResult_Crashed:
            AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Builder indicated that its process crashed!");
            break;

        case AssetBuilderSDK::ProcessJobResult_Cancelled:
            AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Builder indicates that the job was cancelled.");
            break;

        case AssetBuilderSDK::ProcessJobResult_Failed:
            AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Builder indicated that the job has failed.");
            shouldRemoveTempFolder = false;
            break;
        }

        if ((shouldRemoveTempFolder) || (listener.WasQuitRequested()))
        {
            QDir workingDir(QString(builderParams.m_processJobRequest.m_tempDirPath.c_str()));
            workingDir.removeRecursively();
        }

        // Setting the job id back to zero for error detection
        AssetProcessor::SetThreadLocalJobId(0);
        listener.BusDisconnect();

        JobDiagnosticRequestBus::Broadcast(&JobDiagnosticRequestBus::Events::RecordDiagnosticInfo, builderParams.m_rcJob->GetJobEntry().m_jobRunKey, JobDiagnosticInfo(aznumeric_cast<AZ::u32>(jobLogTraceListener.GetWarningCount()), aznumeric_cast<AZ::u32>(jobLogTraceListener.GetErrorCount())));
    }

    bool RCJob::CopyCompiledAssets(BuilderParams& params, AssetBuilderSDK::ProcessJobResponse& response)
    {
        if (response.m_outputProducts.empty())
        {
            // early out here for performance - no need to do anything at all here so don't waste time with IsDir or Exists or anything.
            return true;
        }

        AZ::IO::Path cacheDirectory = params.m_cacheOutputDir;
        AZ::IO::Path intermediateDirectory = params.m_intermediateOutputDir;
        AZ::IO::Path relativeFilePath = params.m_relativePath;
        QString tempFolder = params.m_processJobRequest.m_tempDirPath.c_str();
        QDir tempDir(tempFolder);

        if (params.m_cacheOutputDir.empty() || params.m_intermediateOutputDir.empty())
        {
            AZ_Assert(false, "CopyCompiledAssets:  params.m_finalOutputDir or m_intermediateOutputDir is empty for an asset processor job.  This should not happen and is because of a recent code change.  Check history of any new builders or rcjob.cpp\n");
            return false;
        }

        if (!tempDir.exists())
        {
            AZ_Assert(false, "CopyCompiledAssets:  params.m_processJobRequest.m_tempDirPath is empty for an asset processor job.  This should not happen and is because of a recent code change!  Check history of RCJob.cpp and any new builder code changes.\n");
            return false;
        }

        // copy the built products into the appropriate location in the real cache and update the job status accordingly.
        // note that we go to the trouble of first doing all the checking for disk space and existence of the source files
        // before we notify the AP or start moving any of the files so that failures cause the least amount of damage possible.

        // this vector is a set of pairs where the first of each pair is the source file (absolute) we intend to copy
        // and  the second is the product destination we intend to copy it to.
        QList< QPair<QString, QString> > outputsToCopy;
        outputsToCopy.reserve(static_cast<int>(response.m_outputProducts.size()));
        QList<QPair<QString, AZ::Uuid>> intermediateOutputPaths;
        qint64 fileSizeRequired = 0;

        bool needCacheDirectory = false;
        bool needIntermediateDirectory = false;

        for (AssetBuilderSDK::JobProduct& product : response.m_outputProducts)
        {
            // each Output Product communicated by the builder will either be
            // * a relative path, which means we assume its relative to the temp folder, and we attempt to move the file
            // * an absolute path in the temp folder, and we attempt to move also
            // * an absolute path outside the temp folder, in which we assume you'd like to just copy a file somewhere.

            QString outputProduct = QString::fromUtf8(product.m_productFileName.c_str()); // could be a relative path.
            QFileInfo fileInfo(outputProduct);

            if (fileInfo.isRelative())
            {
                // we assume that its relative to the TEMP folder.
                fileInfo = QFileInfo(tempDir.absoluteFilePath(outputProduct));
            }

            QString absolutePathOfSource = fileInfo.absoluteFilePath();
            QString outputFilename = fileInfo.fileName();

            bool outputToCache = (product.m_outputFlags & AssetBuilderSDK::ProductOutputFlags::ProductAsset) == AssetBuilderSDK::ProductOutputFlags::ProductAsset;
            bool outputToIntermediate = (product.m_outputFlags & AssetBuilderSDK::ProductOutputFlags::IntermediateAsset) ==
                AssetBuilderSDK::ProductOutputFlags::IntermediateAsset;

            if (outputToCache && outputToIntermediate)
            {
                // We currently do not support both since intermediate outputs require the Common platform, which is not supported for cache outputs yet
                AZ_Error(AssetProcessor::ConsoleChannel, false, "Outputting an asset as both a product and intermediate is not supported.  To output both, please split the job into two separate ones.");
                return false;
            }

            if (!outputToCache && !outputToIntermediate)
            {
                AZ_Error(AssetProcessor::ConsoleChannel, false, "An output asset must be flagged as either a product or an intermediate asset.  "
                    "Please update the output job to include either AssetBuilderSDK::ProductOutputFlags::ProductAsset "
                    "or AssetBuilderSDK::ProductOutputFlags::IntermediateAsset");
                return false;
            }

            // Intermediates are required to output for the common platform only
            if (outputToIntermediate && params.m_processJobRequest.m_platformInfo.m_identifier != AssetBuilderSDK::CommonPlatformName)
            {
                AZ_Error(AssetProcessor::ConsoleChannel, false, "Intermediate outputs are only supported for the %s platform.  "
                    "Either change the Job platform to %s or change the output flag to AssetBuilderSDK::ProductOutputFlags::ProductAsset",
                    AssetBuilderSDK::CommonPlatformName,
                    AssetBuilderSDK::CommonPlatformName);
                return false;
            }

            // Common platform is not currently supported for product assets
            if (outputToCache && params.m_processJobRequest.m_platformInfo.m_identifier == AssetBuilderSDK::CommonPlatformName)
            {
                AZ_Error(
                    AssetProcessor::ConsoleChannel, false,
                    "Product asset outputs are not currently supported for the %s platform.  "
                    "Either change the Job platform to a normal platform or change the output flag to AssetBuilderSDK::ProductOutputFlags::IntermediateAsset",
                    AssetBuilderSDK::CommonPlatformName);
                return false;
            }

            const bool isSourceMetadataEnabled = !params.m_sourceUuid.IsNull();

            if (isSourceMetadataEnabled)
            {
                // For metadata enabled files, the output file needs to be prefixed to handle multiple files with the same relative path.
                // This phase will just use a temporary prefix which is longer and less likely to result in accidental conflicts.
                // During AssetProcessed_Impl in APM, the prefixing will be resolved to figure out which file is highest priority and gets renamed
                // back to the non-prefixed, backwards compatible format and every other file with the same rel path will be re-prefixed to a finalized form.
                ProductOutputUtil::GetInterimProductPath(outputFilename, params.m_rcJob->GetJobEntry().m_sourceAssetReference.ScanFolderId());
            }

            if(outputToCache)
            {
                needCacheDirectory = true;

                if(!product.m_outputPathOverride.empty())
                {
                    AZ_Error(AssetProcessor::ConsoleChannel, false, "%s specified m_outputPathOverride on a ProductAsset.  This is not supported."
                    "  Please update the builder accordingly.", params.m_processJobRequest.m_sourceFile.c_str());
                    return false;
                }

                if (!VerifyOutputProduct(
                    QDir(cacheDirectory.c_str()), outputFilename, absolutePathOfSource, fileSizeRequired,
                    outputsToCopy))
                {
                    return false;
                }
            }

            if(outputToIntermediate)
            {
                needIntermediateDirectory = true;

                if(!product.m_outputPathOverride.empty())
                {
                    relativeFilePath = product.m_outputPathOverride;
                }

                if (VerifyOutputProduct(
                        QDir(intermediateDirectory.c_str()), outputFilename, absolutePathOfSource, fileSizeRequired, outputsToCopy))
                {
                    // A null uuid indicates the source is not using metadata files.
                    // The assumption for the UUID generated below is that the source UUID will not change.  A type which has no metadata
                    // file currently may be updated later to have a metadata file, which would break that assumption.  In that case, stick
                    // with the default path-based UUID.
                    if (isSourceMetadataEnabled)
                    {
                        // Generate a UUID for the intermediate as:
                        // SourceUuid:BuilderUuid:SubId
                        auto uuid = AZ::Uuid::CreateName(AZStd::string::format(
                            "%s:%s:%d",
                            params.m_sourceUuid.ToFixedString().c_str(),
                            params.m_assetBuilderDesc.m_busId.ToFixedString().c_str(),
                            product.m_productSubID));

                        // Add the product absolute path to the list of intermediates
                        intermediateOutputPaths.append(QPair(outputsToCopy.back().second, uuid));
                    }
                }
                else
                {
                    return false;
                }
            }

            // update the productFileName to be the scanfolder relative path (without the platform)
            product.m_productFileName = (relativeFilePath / outputFilename.toUtf8().constData()).c_str();
        }

        // now we can check if there's enough space for ALL the files before we copy any.

        bool hasSpace = false;

        auto* diskSpaceInfoInterface = AZ::Interface<AssetProcessor::IDiskSpaceInfo>::Get();

        if (diskSpaceInfoInterface)
        {
            hasSpace = diskSpaceInfoInterface->CheckSufficientDiskSpace(fileSizeRequired, false);
        }

        if (!hasSpace)
        {
            AZ_Error(
                AssetProcessor::ConsoleChannel, false,
                "Cannot save file(s) to cache, not enough disk space to save all the products of %s.  Total needed: %lli bytes",
                params.m_processJobRequest.m_sourceFile.c_str(), fileSizeRequired);
            return false;
        }

        // if we get here, we are good to go in terms of disk space and sources existing, so we make the best attempt we can.

        // if outputDirectory does not exist then create it
        unsigned int waitTimeInSecs = 3;
        if (needCacheDirectory && !AssetUtilities::CreateDirectoryWithTimeout(QDir(cacheDirectory.AsPosix().c_str()), waitTimeInSecs))
        {
            AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Failed to create output directory: %s\n", cacheDirectory.c_str());
            return false;
        }

        if (needIntermediateDirectory && !AssetUtilities::CreateDirectoryWithTimeout(QDir(intermediateDirectory.AsPosix().c_str()), waitTimeInSecs))
        {
            AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Failed to create intermediate directory: %s\n", intermediateDirectory.c_str());
            return false;
        }

        auto* uuidInterface = AZ::Interface<AzToolsFramework::IUuidUtil>::Get();

        if (!uuidInterface)
        {
            AZ_Assert(false, "Programmer Error - IUuidUtil interface is not available");
            return false;
        }

        // Go through all the intermediate products and output the assigned UUID
        for (auto [intermediateProduct, uuid] : intermediateOutputPaths)
        {
            if(!uuidInterface->CreateSourceUuid(intermediateProduct.toUtf8().constData(), uuid))
            {
                AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Failed to create metadata file for intermediate product " AZ_STRING_FORMAT, AZ_STRING_ARG(intermediateProduct));
            }
        }

        bool anyFileFailed = false;

        for (const QPair<QString, QString>& filePair : outputsToCopy)
        {
            const QString& sourceAbsolutePath = filePair.first;
            const QString& productAbsolutePath = filePair.second;

            bool isCopyJob = !(sourceAbsolutePath.startsWith(tempFolder, Qt::CaseInsensitive));
            isCopyJob |= response.m_keepTempFolder; // Copy instead of Move if the builder wants to keep the Temp Folder. 

            if (!MoveCopyFile(sourceAbsolutePath, productAbsolutePath, isCopyJob)) // this has its own traceprintf for failure
            {
                // MoveCopyFile will have output to the log.  No need to double output here.
                anyFileFailed = true;
                continue;
            }

            //we now ensure that the file is writable - this is just a warning if it fails, not a complete failure.
            if (!AssetUtilities::MakeFileWritable(productAbsolutePath))
            {
                AZ_TracePrintf(AssetBuilderSDK::WarningWindow, "Unable to change permission for the file: %s.\n", productAbsolutePath.toUtf8().data());
            }
        }

        return !anyFileFailed;
    }

    bool RCJob::VerifyOutputProduct(
        QDir outputDirectory,
        QString outputFilename,
        QString absolutePathOfSource,
        qint64& totalFileSizeRequired,
        QList<QPair<QString, QString>>& outputsToCopy)
    {
        QString productFile = AssetUtilities::NormalizeFilePath(outputDirectory.filePath(outputFilename.toLower()));

        // Don't make productFile all lowercase for case-insensitive as this
        // breaks macOS. The case is already setup properly when the job
        // was created.

        if (productFile.length() >= ASSETPROCESSOR_WARN_PATH_LEN && productFile.length() < ASSETPROCESSOR_TRAIT_MAX_PATH_LEN)
        {
            AZ_Warning(
                AssetBuilderSDK::WarningWindow,
                false,
                "Product '%s' path length (%d) exceeds the suggested max path length (%d). This may not work on all platforms.\n",
                productFile.toUtf8().data(),
                productFile.length(),
                ASSETPROCESSOR_WARN_PATH_LEN);
        }

        if (productFile.length() >= ASSETPROCESSOR_TRAIT_MAX_PATH_LEN)
        {
            AZ_Error(
                AssetBuilderSDK::ErrorWindow,
                false,
                "Cannot copy file: Product '%s' path length (%d) exceeds the max path length (%d) allowed on disk\n",
                productFile.toUtf8().data(),
                productFile.length(),
                ASSETPROCESSOR_TRAIT_MAX_PATH_LEN);
            return false;
        }

        QFileInfo inFile(absolutePathOfSource);
        if (!inFile.exists())
        {
            AZ_Error(
                AssetBuilderSDK::ErrorWindow, false,
                "Cannot copy file - product file with absolute path '%s' attempting to save into cache could not be found",
                absolutePathOfSource.toUtf8().constData());
            return false;
        }

        totalFileSizeRequired += inFile.size();
        outputsToCopy.push_back(qMakePair(absolutePathOfSource, productFile));

        return true;
    }

    AZ::Outcome<AZStd::vector<AZStd::string>> RCJob::BeforeStoringJobResult(const BuilderParams& builderParams, AssetBuilderSDK::ProcessJobResponse jobResponse)
    {
        AZStd::string normalizedTempFolderPath = builderParams.m_processJobRequest.m_tempDirPath;
        AzFramework::StringFunc::Path::Normalize(normalizedTempFolderPath);

        AZStd::vector<AZStd::string> sourceFiles;
        for (AssetBuilderSDK::JobProduct& product : jobResponse.m_outputProducts)
        {
            // Try to handle Absolute paths within the temp folder
            AzFramework::StringFunc::Path::Normalize(product.m_productFileName);
            if (!AzFramework::StringFunc::Replace(product.m_productFileName, normalizedTempFolderPath.c_str(), s_tempString))
            {
                // From CopyCompiledAssets:

                // each Output Product communicated by the builder will either be
                // * a relative path, which means we assume its relative to the temp folder, and we attempt to move the file
                // * an absolute path in the temp folder, and we attempt to move also
                // * an absolute path outside the temp folder, in which we assume you'd like to just copy a file somewhere.

                // We need to handle case 3 here (Case 2 was above, case 1 is treated as relative within temp)
                // If the path was not absolute within the temp folder and not relative it should be an absolute path beneath our source (Including the source)
                // meaning a copy job which needs to be added to our archive.
                if (!AzFramework::StringFunc::Path::IsRelative(product.m_productFileName.c_str()))
                {
                    AZStd::string sourceFile{ builderParams.m_rcJob->GetJobEntry().GetAbsoluteSourcePath().toUtf8().data() };
                    AzFramework::StringFunc::Path::Normalize(sourceFile);
                    AzFramework::StringFunc::Path::StripFullName(sourceFile);

                    size_t sourcePathPos = product.m_productFileName.find(sourceFile.c_str());
                    if(sourcePathPos != AZStd::string::npos)
                    {
                        sourceFiles.push_back(product.m_productFileName.substr(sourceFile.size()).c_str());
                        AzFramework::StringFunc::Path::Join(s_tempString, product.m_productFileName.substr(sourceFile.size()).c_str(), product.m_productFileName);
                    }
                    else
                    {
                        AZ_Warning(AssetBuilderSDK::WarningWindow, false,
                            "Failed to find source path %s or temp path %s in non relative path in %s",
                            sourceFile.c_str(), normalizedTempFolderPath.c_str(), product.m_productFileName.c_str());
                    }
                }
            }
        }
        AZStd::string responseFilePath;
        AzFramework::StringFunc::Path::ConstructFull(builderParams.m_processJobRequest.m_tempDirPath.c_str(), AssetBuilderSDK::s_processJobResponseFileName, responseFilePath, true);
        //Save ProcessJobResponse to disk
        if (!AZ::Utils::SaveObjectToFile(responseFilePath, AZ::DataStream::StreamType::ST_XML, &jobResponse))
        {
            return AZ::Failure();
        }
        AzToolsFramework::AssetSystem::JobInfo jobInfo;
        AzToolsFramework::AssetSystem::AssetJobLogResponse jobLogResponse;
        jobInfo.m_sourceFile = builderParams.m_rcJob->GetJobEntry().m_sourceAssetReference.RelativePath().c_str();
        jobInfo.m_platform = builderParams.m_rcJob->GetPlatformInfo().m_identifier.c_str();
        jobInfo.m_jobKey = builderParams.m_rcJob->GetJobKey().toUtf8().data();
        jobInfo.m_builderGuid = builderParams.m_rcJob->GetBuilderGuid();
        jobInfo.m_jobRunKey = builderParams.m_rcJob->GetJobEntry().m_jobRunKey;
        jobInfo.m_watchFolder = builderParams.m_processJobRequest.m_watchFolder;
        AssetUtilities::ReadJobLog(jobInfo, jobLogResponse);

        //Save joblog to disk
        AZStd::string jobLogFilePath;
        AzFramework::StringFunc::Path::ConstructFull(builderParams.m_processJobRequest.m_tempDirPath.c_str(), s_jobLogFileName, jobLogFilePath, true);
        if (!AZ::Utils::SaveObjectToFile(jobLogFilePath, AZ::DataStream::StreamType::ST_XML, &jobLogResponse))
        {
            return AZ::Failure();
        }
        return AZ::Success(sourceFiles);
    }

    bool RCJob::AfterRetrievingJobResult(const BuilderParams& builderParams, AssetUtilities::JobLogTraceListener& jobLogTraceListener, AssetBuilderSDK::ProcessJobResponse& jobResponse)
    {
        AZStd::string responseFilePath;
        AzFramework::StringFunc::Path::ConstructFull(builderParams.m_processJobRequest.m_tempDirPath.c_str(), AssetBuilderSDK::s_processJobResponseFileName, responseFilePath, true);
        if (!AZ::Utils::LoadObjectFromFileInPlace(responseFilePath.c_str(), jobResponse))
        {
            return false;
        }

        //Ensure that ProcessJobResponse have the correct absolute paths
        for (AssetBuilderSDK::JobProduct& product : jobResponse.m_outputProducts)
        {
            AzFramework::StringFunc::Replace(product.m_productFileName, s_tempString, builderParams.m_processJobRequest.m_tempDirPath.c_str(), s_tempString);
        }

        AZStd::string jobLogFilePath;
        AzFramework::StringFunc::Path::ConstructFull(builderParams.m_processJobRequest.m_tempDirPath.c_str(), s_jobLogFileName, jobLogFilePath, true);
        AzToolsFramework::AssetSystem::AssetJobLogResponse jobLogResponse;

        if (!AZ::Utils::LoadObjectFromFileInPlace(jobLogFilePath.c_str(), jobLogResponse))
        {
            return false;
        }

        if (!jobLogResponse.m_isSuccess)
        {
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Job log request was unsuccessful for job (%s, %s, %s) from the server.\n",
                builderParams.m_rcJob->GetJobEntry().m_sourceAssetReference.AbsolutePath().c_str(), builderParams.m_rcJob->GetJobKey().toUtf8().data(),
                builderParams.m_rcJob->GetPlatformInfo().m_identifier.c_str());

            if(jobLogResponse.m_jobLog.find("No log file found") != AZStd::string::npos)
            {
                AZ_TracePrintf(AssetProcessor::DebugChannel, "Unable to find job log from the server. This could happen if you are trying to use the server cache with a copy job, "
                    "please check the assetprocessorplatformconfig.ini file and ensure that server cache is disabled for the job.\n");
            }

            return false;
        }

        // writing server logs
        AZ_TracePrintf(AssetProcessor::DebugChannel, "------------SERVER BEGIN----------\n");
        AzToolsFramework::Logging::LogLine::ParseLog(jobLogResponse.m_jobLog.c_str(), jobLogResponse.m_jobLog.size(),
            [&jobLogTraceListener](AzToolsFramework::Logging::LogLine& line)
            {
                jobLogTraceListener.AppendLog(line);
            });
        AZ_TracePrintf(AssetProcessor::DebugChannel, "------------SERVER END----------\n");
        return true;
    }

    AZStd::string BuilderParams::GetTempJobDirectory() const
    {
        return m_processJobRequest.m_tempDirPath;
    }

    QString BuilderParams::GetServerKey() const
    {
        return m_serverKey;
    }

} // namespace AssetProcessor


//////////////////////////////////////////////////////////////////////////

