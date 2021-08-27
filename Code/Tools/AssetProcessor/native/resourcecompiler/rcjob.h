/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef RCJOB_H
#define RCJOB_H

#if !defined(Q_MOC_RUN)
#include <QObject>
#include <QString>
#include <QDateTime>
#include <QStringList>
#include <AzCore/base.h>
#include "RCCommon.h"
#include "native/utilities/PlatformConfiguration.h"
#include <AzCore/Math/Uuid.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include "native/assetprocessor.h"
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include <QFileInfoList>
#endif

namespace AssetProcessor
{
    struct AssetRecognizer;
    class RCJob;

    //! Params Base class
    struct Params
    {
        Params(AssetProcessor::RCJob* job = nullptr)
            : m_rcJob(job)
        {}
        virtual ~Params() = default;

        AssetProcessor::RCJob* m_rcJob;
        QString m_finalOutputDir;

        Params(const Params&) = default;

        virtual bool IsValidParams() const;
    };

    //! RCParams contains info that is required by the rc
    struct RCParams
        : public Params
    {
        QString m_rootDir;
        QString m_rcExe;
        QString m_inputFile;
        QString m_platformIdentifier;
        QString m_params;

        RCParams(AssetProcessor::RCJob* job = nullptr)
            : Params(job)
        {}

        RCParams(const RCParams&) = default;

        bool IsValidParams() const override;
    };

    //! BuilderParams contains info that is required by the builders
    struct BuilderParams
        : public Params
    {
        AssetBuilderSDK::ProcessJobRequest m_processJobRequest;
        AssetBuilderSDK::AssetBuilderDesc  m_assetBuilderDesc;
        QString m_serverKey;

        BuilderParams(AssetProcessor::RCJob* job = nullptr)
            : Params(job)
        {}

        BuilderParams(const BuilderParams&) = default;

        AZStd::string GetTempJobDirectory() const;
        QString GetServerKey() const;
    };

    //! JobOutputInfo is used to store job related messages.
    //! Messages can be an error or just some information.
    struct JobOutputInfo
    {
        QString m_windowName; // window name is used to specify whether it is an error or not
        QString m_message;// the actual message

        JobOutputInfo() = default;

        JobOutputInfo(QString window, QString message)
            : m_windowName(window)
            , m_message(message)
        {
        }
    };


    /**
    * The RCJob class contains all the necessary information about a single RC job
    */
    class RCJob
        : public QObject
    {
        Q_OBJECT

    public:
        enum JobState
        {
            pending,
            processing,
            completed,
            crashed,
            terminated,
            cancelled,
            failed,
        };

        explicit RCJob(QObject* parent = 0);

        virtual ~RCJob();

        void Init(JobDetails& details);

        QString Params() const;
        QString CommandLine() const;

        QDateTime GetTimeCreated() const;
        void SetTimeCreated(const QDateTime& timeCreated);

        QDateTime GetTimeLaunched() const;
        void SetTimeLaunched(const QDateTime& timeLaunched);

        QDateTime GetTimeCompleted() const;
        void SetTimeCompleted(const QDateTime& timeCompleted);

        void SetOriginalFingerprint(AZ::u32 originalFingerprint);
        AZ::u32 GetOriginalFingerprint() const;

        QString GetConsoleOutput() const;
        void SetConsoleOutput(QString rcOut);

        JobState GetState() const;
        void SetState(const JobState& state);

        const AZ::Uuid& GetInputFileUuid() const;

        QString GetDestination() const;

        //! the final output path is where the actual outputs are copied when processing succeeds
        //! this will be in the asset cache, in the gamename / platform / gamename folder.
        QString GetFinalOutputPath() const;

        const AssetProcessor::AssetRecognizer* GetRecognizer() const;
        void SetRecognizer(const AssetProcessor::AssetRecognizer* value);

        const AssetBuilderSDK::PlatformInfo& GetPlatformInfo() const;

        // intentionally non-const to move.
        AssetBuilderSDK::ProcessJobResponse& GetProcessJobResponse();

        const JobEntry& GetJobEntry() const;

        void Start();

        const QueueElementID& GetElementID() const { return m_queueElementID; }

        const int JobEscalation() { return m_JobEscalation; }

        void SetJobEscalation(int jobEscalation);

        void SetCheckExclusiveLock(bool value);

    Q_SIGNALS:
        //! This signal will be emitted when we make sure that no other application has a lock on the source file 
        //! and also that the fingerprint of the source file is stable and not changing.
        //! This will basically indicate that we are starting to perform work on the current job
        void BeginWork();
        void Finished();
        void JobFinished(AssetBuilderSDK::ProcessJobResponse result);

    public:
        static QString GetStateDescription(const JobState& state);
        static void ExecuteBuilderCommand(BuilderParams builderParams);
        static void AutoFailJob(BuilderParams& builderParams);
        static bool CopyCompiledAssets(BuilderParams& params, AssetBuilderSDK::ProcessJobResponse& response);
        //! This method will save the processJobResponse and the job log to the temp directory as xml files.
        //! We will be modifying absolute paths in processJobResponse before saving it to the disk.
        static AZ::Outcome<AZStd::vector<AZStd::string>> BeforeStoringJobResult(const BuilderParams& builderParams, AssetBuilderSDK::ProcessJobResponse jobResponse);
        //! This method will retrieve the processJobResponse and the job log from the temp directory.
        //! This method is also responsible for emitting the server job logs to the local job log file.
        static bool AfterRetrievingJobResult(const BuilderParams& builderParams, AssetUtilities::JobLogTraceListener& jobLogTraceListener, AssetBuilderSDK::ProcessJobResponse& jobResponse);

        QString GetJobKey() const;
        AZ::Uuid GetBuilderGuid() const;
        bool IsCritical() const;
        bool IsAutoFail() const;
        int GetPriority() const;
        const AZStd::vector<JobDependencyInternal>& GetJobDependencies();

    protected:
        //! DoWork ensure that the job is ready for being processing and than makes the actual builder call   
        virtual void DoWork(AssetBuilderSDK::ProcessJobResponse& result, BuilderParams& builderParams, AssetUtilities::QuitListener& listener);
        void PopulateProcessJobRequest(AssetBuilderSDK::ProcessJobRequest& processJobRequest);

    private:
        JobDetails m_jobDetails;
        JobState m_jobState;

        QueueElementID m_queueElementID; // cached to prevent lots of construction of this all over the place

        int m_JobEscalation = AssetProcessor::JobEscalation::Default; // Escalation indicates how important the job is and how soon it needs processing, the greater the number the greater the escalation  

        QDateTime m_timeCreated;
        QDateTime m_timeLaunched;
        QDateTime m_timeCompleted;

        unsigned int m_exitCode = 0;

        AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer m_products;

        AssetBuilderSDK::ProcessJobResponse m_processJobResponse;

        AZ::u32 m_scanFolderID;
    };
} // namespace AssetProcessor

Q_DECLARE_METATYPE(AssetProcessor::BuilderParams);
Q_DECLARE_METATYPE(AssetProcessor::JobOutputInfo);
Q_DECLARE_METATYPE(AssetProcessor::RCParams);

#endif // RCJOB_H
