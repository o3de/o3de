/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "rcjob.h"



#include <AzToolsFramework/UI/Logging/LogLine.h>

#include <native/utilities/BuilderManager.h>
#include <native/utilities/ThreadHelper.h>

#include <QtConcurrent/QtConcurrentRun>
#include <QElapsedTimer>

#include "native/utilities/JobDiagnosticTracker.h"


namespace
{
    unsigned long s_jobSerial = 1;
    bool s_typesRegistered = false;
    // You have up to 60 minutes to finish processing an asset.
    // This was increased from 10 to account for PVRTC compression
    // taking up to an hour for large normal map textures, and should
    // be reduced again once we move to the ASTC compression format, or
    // find another solution to reduce processing times to be reasonable.
    const unsigned int g_jobMaximumWaitTime = 1000 * 60 * 60;

    const unsigned int g_sleepDurationForLockingAndFingerprintChecking = 100;

    const unsigned int g_graceTimeBeforeLockingAndFingerprintChecking = 300;

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
    return (!m_finalOutputDir.isEmpty());
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
        m_queueElementID = QueueElementID(GetJobEntry().m_databaseSourceName, GetPlatformInfo().m_identifier.c_str(), GetJobKey());
    }

    const JobEntry& RCJob::GetJobEntry() const
    {
        return m_jobDetails.m_jobEntry;
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

    QString RCJob::GetFinalOutputPath() const
    {
        return m_jobDetails.m_destinationPath;
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
        processJobRequest.m_sourceFile = GetJobEntry().m_pathRelativeToWatchFolder.toUtf8().data();
        processJobRequest.m_sourceFileUUID = GetInputFileUuid();
        processJobRequest.m_watchFolder = GetJobEntry().m_watchFolderPath.toUtf8().data();
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
        builderParams.m_finalOutputDir = GetFinalOutputPath();
        builderParams.m_assetBuilderDesc = m_jobDetails.m_assetBuilderDesc;

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

        // listen for the user quitting (CTRL-C or otherwise)
        AssetUtilities::QuitListener listener;
        listener.BusConnect();
        QElapsedTimer ticker;
        ticker.start();
        AssetBuilderSDK::ProcessJobResponse result;

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

        // We are adding a grace time before we check exclusive lock and validate the fingerprint of the file.
        // This grace time should prevent multiple jobs from getting added to the queue if the source file is still updating.
        qint64 milliSecsDiff = QDateTime::currentMSecsSinceEpoch() - builderParams.m_rcJob->GetJobEntry().m_computedFingerprintTimeStamp;
        if (milliSecsDiff < g_graceTimeBeforeLockingAndFingerprintChecking)
        {
            QThread::msleep(aznumeric_cast<unsigned long>(g_graceTimeBeforeLockingAndFingerprintChecking - milliSecsDiff));
        }
        // Lock and unlock the source file to ensure it is not still open by another process.
        // This prevents premature processing of some source files that are opened for writing, but are zero bytes for longer than the modification threshhold
        QString inputFile = builderParams.m_rcJob->GetJobEntry().GetAbsoluteSourcePath();
        if (builderParams.m_rcJob->GetJobEntry().m_checkExclusiveLock && QFile::exists(inputFile))
        {
            // We will only continue once we get exclusive lock on the source file
            while (!AssetUtilities::CheckCanLock(inputFile))
            {
                QThread::msleep(g_sleepDurationForLockingAndFingerprintChecking);
                if (listener.WasQuitRequested() || (ticker.elapsed() > g_jobMaximumWaitTime))
                {
                    result.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
                    Q_EMIT builderParams.m_rcJob->JobFinished(result);
                    return;
                }
            }
        }

        // We will only continue once the fingerprint of the file stops changing
        unsigned int fingerprint = AssetUtilities::GenerateFingerprint(builderParams.m_rcJob->m_jobDetails);
        while (fingerprint != builderParams.m_rcJob->GetOriginalFingerprint())
        {
            builderParams.m_rcJob->SetOriginalFingerprint(fingerprint);
            QThread::msleep(g_sleepDurationForLockingAndFingerprintChecking);
            
            if (listener.WasQuitRequested() || (ticker.elapsed() > g_jobMaximumWaitTime))
            {
                result.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
                Q_EMIT builderParams.m_rcJob->JobFinished(result);
                return;
            }

            fingerprint = AssetUtilities::GenerateFingerprint(builderParams.m_rcJob->m_jobDetails);
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
        auto failReason = builderParams.m_processJobRequest.m_jobDescription.m_jobParameters.find(AZ_CRC(AssetProcessor::AutoFailReasonKey));
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
        auto failLogFile = builderParams.m_processJobRequest.m_jobDescription.m_jobParameters.find(AZ_CRC(AssetProcessor::AutoFailLogFile));
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
        
        if (builderParams.m_processJobRequest.m_jobDescription.m_jobParameters.find(AZ_CRC(AssetProcessor::AutoFailOmitFromDatabaseKey)) != builderParams.m_processJobRequest.m_jobDescription.m_jobParameters.end())
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
            auto warningMessage = builderParams.m_processJobRequest.m_jobDescription.m_jobParameters.find(AZ_CRC(AssetProcessor::JobWarningKey));
            if (warningMessage != builderParams.m_processJobRequest.m_jobDescription.m_jobParameters.end())
            {
                // you are allowed to have many lines in your warning message.
                AZStd::vector<AZStd::string> delimited;
                AzFramework::StringFunc::Tokenize(warningMessage->second.c_str(), delimited, "\n");
                for (const AZStd::string& token : delimited)
                {
                    AZ_Warning(AssetBuilderSDK::WarningWindow, false, "%s", token.c_str());
                }
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
            
            if (sourceFullPath.length() >= AP_MAX_PATH_LEN)
            {
                AZ_Warning(AssetBuilderSDK::WarningWindow, false, "Source Asset: %s filepath length %d exceeds the maximum path length (%d) allowed.\n", sourceFullPath.toUtf8().data(), sourceFullPath.length(), AP_MAX_PATH_LEN);
                result.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            }
            else
            {
                if (!JobCancelListener.IsCancelled())
                {
                    bool runProcessJob = true;
                    if (m_jobDetails.m_checkServer)
                    {
                        QFileInfo fileInfo(builderParams.m_processJobRequest.m_sourceFile.c_str());
                        builderParams.m_serverKey = QString("%1_%2_%3_%4").arg(fileInfo.completeBaseName(), builderParams.m_processJobRequest.m_jobDescription.m_jobKey.c_str(), builderParams.m_processJobRequest.m_platformInfo.m_identifier.c_str()).arg(builderParams.m_rcJob->GetOriginalFingerprint());
                        bool operationResult = false;
                        if (AssetUtilities::InServerMode())
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
                                        builderParams.m_rcJob->GetJobEntry().m_pathRelativeToWatchFolder.toUtf8().data(), builderParams.m_rcJob->GetJobKey().toUtf8().data(),
                                        builderParams.m_rcJob->GetPlatformInfo().m_identifier.c_str(), builderParams.m_rcJob->GetOriginalFingerprint());
                                }
                            }
                        }
                        else
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
                                    builderParams.m_rcJob->GetJobEntry().m_pathRelativeToWatchFolder.toUtf8().data(), builderParams.m_rcJob->GetJobKey().toUtf8().data(),
                                    builderParams.m_rcJob->GetPlatformInfo().m_identifier.c_str(), builderParams.m_rcJob->GetOriginalFingerprint());
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
            AZStd::unordered_set<AZ::u32> subIdsFound;
            for (const AssetBuilderSDK::JobProduct& product : result.m_outputProducts)
            {
                if (!subIdsFound.insert(product.m_productSubID).second)
                {
                    // if this happens the element was already in the set.
                    AZ_Error(AssetBuilderSDK::ErrorWindow, false, "The builder created more than one asset with the same subID (%u) when emitting product %s\n  Builders should set a unique m_productSubID value for each product, as this is used as part of the address of the asset.", product.m_productSubID, product.m_productFileName.c_str());
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
                shouldRemoveTempFolder = shouldRemoveTempFolder && !s_createRequestFileForSuccessfulJob;
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

        QDir outputDirectory(params.m_finalOutputDir);
        QString         tempFolder = params.m_processJobRequest.m_tempDirPath.c_str();
        QDir            tempDir(tempFolder);

        if (params.m_finalOutputDir.isEmpty())
        {
            AZ_Assert(false, "CopyCompiledAssets:  params.m_finalOutputDir is empty for an asset processor job.  This should not happen and is because of a recent code change.  Check history of any new builders or rcjob.cpp\n");
            return false;
        }

        if (!tempDir.exists())
        {
            AZ_Assert(false, "PCopyCompiledAssets:  params.m_processJobRequest.m_tempDirPath is empty for an asset processor job.  This should not happen and is because of a recent code change!  Check history of RCJob.cpp and any new builder code changes.\n");
            return false;
        }

        // if outputDirectory does not exist then create it
        unsigned int waitTimeInSecs = 3;
        if (!AssetUtilities::CreateDirectoryWithTimeout(outputDirectory, waitTimeInSecs))
        {
            AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Failed to create output directory: %s\n", outputDirectory.absolutePath().toUtf8().data());
            return false;
        }

        // copy the built products into the appropriate location in the real cache and update the job status accordingly.
        // note that we go to the trouble of first doing all the checking for disk space and existence of the source files
        // before we notify the AP or start moving any of the files so that failures cause the least amount of damage possible.

        // this vector is a set of pairs where the first of each pair is the source file (absolute) we intend to copy
        // and  the second is the product destination we intend to copy it to.
        QList< QPair<QString, QString> > outputsToCopy;
        outputsToCopy.reserve(static_cast<int>(response.m_outputProducts.size()));
        qint64 totalFileSizeRequired = 0;

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
            QString productFile = AssetUtilities::NormalizeFilePath(outputDirectory.filePath(outputFilename.toLower()));

            // Don't make productFile all lowercase for case-insensitive as this
            // breaks macOS. The case is already setup properly when the job
            // was created.

            if (productFile.length() >= AP_MAX_PATH_LEN)
            {
                AZ_Error(AssetBuilderSDK::ErrorWindow, false, "Cannot copy file: Product '%s' path length (%d) exceeds the max path length (%d) allowed on disk\n", productFile.toUtf8().data(), productFile.length(), AP_MAX_PATH_LEN);
                return false;
            }
            
            QFileInfo inFile(absolutePathOfSource);
            if (!inFile.exists())
            {
                AZ_Error(AssetBuilderSDK::ErrorWindow, false, "Cannot copy file - product file with absolute path '%s' attempting to save into cache could not be found", absolutePathOfSource.toUtf8().constData());
                return false;
            }
            
            totalFileSizeRequired += inFile.size();
            outputsToCopy.push_back(qMakePair(absolutePathOfSource, productFile));
            
            // also update the product file name to be the final resting place of this product in the cache (normalized!)
            product.m_productFileName = AssetUtilities::NormalizeFilePath(productFile).toUtf8().constData();
        }

        // now we can check if there's enough space for ALL the files before we copy any.
        bool hasSpace = false;
        AssetProcessor::DiskSpaceInfoBus::BroadcastResult(hasSpace, &AssetProcessor::DiskSpaceInfoBusTraits::CheckSufficientDiskSpace, outputDirectory.absolutePath(), totalFileSizeRequired, false);

        if (!hasSpace)
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, "Cannot save file to cache, not enough disk space to save all the products of %s.  Total needed: %lli bytes", params.m_processJobRequest.m_sourceFile.c_str(), totalFileSizeRequired);
            return false;
        }

        // if we get here, we are good to go in terms of disk space and sources existing, so we make the best attempt we can.

        bool anyFileFailed = false;

        for (const QPair<QString, QString>& filePair : outputsToCopy)
        {
            const QString& sourceAbsolutePath = filePair.first;
            const QString& productAbsolutePath = filePair.second;

            bool isCopyJob = !(sourceAbsolutePath.startsWith(tempFolder, Qt::CaseInsensitive));

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

    AZ::Outcome<AZStd::vector<AZStd::string>> RCJob::BeforeStoringJobResult(const BuilderParams& builderParams, AssetBuilderSDK::ProcessJobResponse jobResponse)
    {
        AZStd::string normalizedTempFolderPath = builderParams.m_processJobRequest.m_tempDirPath;
        AzFramework::StringFunc::Path::Normalize(normalizedTempFolderPath);

        AZStd::vector<AZStd::string> sourceFiles;
        for (AssetBuilderSDK::JobProduct& product : jobResponse.m_outputProducts)
        {
            // Try to handle Absolute paths within the temp folder
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

                    AzFramework::StringFunc::Path::Normalize(product.m_productFileName);
                    size_t sourcePathPos = product.m_productFileName.find(sourceFile.c_str());
                    if(sourcePathPos != AZStd::string::npos)
                    {
                        sourceFiles.push_back(product.m_productFileName.substr(sourceFile.size()).c_str());
                        AzFramework::StringFunc::Path::Join(s_tempString, product.m_productFileName.substr(sourceFile.size()).c_str(), product.m_productFileName);
                    }
                    else
                    {
                        AZ_Warning(AssetBuilderSDK::WarningWindow, false, "Failed to find source path %s or temp path %s in non relative path in %s", sourceFile.c_str(), normalizedTempFolderPath.c_str(), product.m_productFileName.c_str());
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
        jobInfo.m_sourceFile = builderParams.m_rcJob->GetJobEntry().m_databaseSourceName.toUtf8().data();
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
                builderParams.m_rcJob->GetJobEntry().m_pathRelativeToWatchFolder.toUtf8().data(), builderParams.m_rcJob->GetJobKey().toUtf8().data(),
                builderParams.m_rcJob->GetPlatformInfo().m_identifier.c_str());

            if(jobLogResponse.m_jobLog.find("No log file found") != AZStd::string::npos)
            {
                AZ_TracePrintf(AssetProcessor::DebugChannel, "Unable to find job log from the server. This could happen if you are trying to use the server cache with a copy job,\
please check the assetprocessorplatformconfig.ini file and ensure that server cache is disabled for the job.\n");
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

