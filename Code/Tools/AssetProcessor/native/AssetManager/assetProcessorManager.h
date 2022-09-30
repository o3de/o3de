/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QString>
#include <QByteArray>
#include <QQueue>
#include <QVector>
#include <QHash>
#include <QDir>
#include <QSet>
#include <QMap>
#include <QPair>
#include <QMutex>

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/containers/unordered_map.h>

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>

#include "native/assetprocessor.h"
#include "native/utilities/AssetUtilEBusHelper.h"
#include "native/utilities/MissingDependencyScanner.h"
#include "native/utilities/ThreadHelper.h"
#include "native/AssetManager/AssetCatalog.h"
#include "native/AssetDatabase/AssetDatabase.h"
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/map.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzCore/IO/SystemFile.h> // for AZ_MAX_PATH_LEN

#include "AssetRequestHandler.h"
#include "native/utilities/JobDiagnosticTracker.h"
#include "SourceFileRelocator.h"

#include <AssetManager/ExcludedFolderCache.h>
#include <AssetManager/ProductAsset.h>
#endif

class FileWatcher;

namespace AzFramework
{
    namespace AssetSystem
    {
        class BaseAssetProcessorMessage;

        class GetRelativeProductPathFromFullSourceOrProductPathRequest;
        class GetRelativeProductPathFromFullSourceOrProductPathResponse;

        class GenerateRelativeSourcePathRequest;
        class GenerateRelativeSourcePathResponse;

        class GetFullSourcePathFromRelativeProductPathRequest;
        class GetFullSourcePathFromRelativeProductPathResponse;
        class AssetNotificationMessage;
    } // namespace AssetSystem
} // namespace AzFramework

namespace AzToolsFramework
{
    namespace AssetSystem
    {
        class AssetJobLogRequest;
        class AssetJobLogResponse;

        class AssetJobsInfoRequest;
        class AssetJobsInfoResponse;

        class GetAbsoluteAssetDatabaseLocationRequest;
        class GetAbsoluteAssetDatabaseLocationResponse;
    } // namespace AssetSystem
} // namespace AzToolsFramework

namespace AssetProcessor
{
    class AssetProcessingStateData;
    struct AssetRecognizer;
    class PlatformConfiguration;
    class ScanFolderInfo;
    class PathDependencyManager;
    class LfsPointerFileValidator;

    //! The Asset Processor Manager is the heart of the pipeline
    //! It is what makes the critical decisions about what should and should not be processed
    //! It emits signals when jobs need to be performed and when assets are complete or have failed.
    class AssetProcessorManager
        : public QObject
        , public AssetProcessor::ProcessingJobInfoBus::Handler
    {
        using BaseAssetProcessorMessage = AzFramework::AssetSystem::BaseAssetProcessorMessage;
        using AssetJobsInfoRequest = AzToolsFramework::AssetSystem::AssetJobsInfoRequest;
        using AssetJobsInfoResponse = AzToolsFramework::AssetSystem::AssetJobsInfoResponse;
        using JobInfo = AzToolsFramework::AssetSystem::JobInfo;
        using JobStatus = AzToolsFramework::AssetSystem::JobStatus;
        using AssetJobLogRequest = AzToolsFramework::AssetSystem::AssetJobLogRequest;
        using AssetJobLogResponse = AzToolsFramework::AssetSystem::AssetJobLogResponse;
        using GetAbsoluteAssetDatabaseLocationRequest = AzToolsFramework::AssetSystem::GetAbsoluteAssetDatabaseLocationRequest;
        using GetAbsoluteAssetDatabaseLocationResponse = AzToolsFramework::AssetSystem::GetAbsoluteAssetDatabaseLocationResponse;
        using GetRelativeProductPathFromFullSourceOrProductPathRequest = AzFramework::AssetSystem::GetRelativeProductPathFromFullSourceOrProductPathRequest;
        using GetRelativeProductPathFromFullSourceOrProductPathResponse = AzFramework::AssetSystem::GetRelativeProductPathFromFullSourceOrProductPathResponse;
        using GenerateRelativeSourcePathRequest = AzFramework::AssetSystem::GenerateRelativeSourcePathRequest;
        using GenerateRelativeSourcePathResponse = AzFramework::AssetSystem::GenerateRelativeSourcePathResponse;
        using GetFullSourcePathFromRelativeProductPathRequest = AzFramework::AssetSystem::GetFullSourcePathFromRelativeProductPathRequest;
        using GetFullSourcePathFromRelativeProductPathResponse = AzFramework::AssetSystem::GetFullSourcePathFromRelativeProductPathResponse;

        Q_OBJECT

    private:

        struct FileEntry
        {
            QString m_fileName;
            bool m_isDelete = false;
            bool m_isFromScanner = false;
            AZStd::chrono::steady_clock::time_point m_initialProcessTime{};

            FileEntry() = default;

            FileEntry(const QString& fileName, bool isDelete, bool isFromScanner = false, AZStd::chrono::steady_clock::time_point initialProcessTime = {})
                : m_fileName(fileName)
                , m_isDelete(isDelete)
                , m_isFromScanner(isFromScanner)
                , m_initialProcessTime(initialProcessTime)
            {
            }
        };

        struct AssetProcessedEntry
        {
            JobEntry m_entry;
            AssetBuilderSDK::ProcessJobResponse m_response;

            AssetProcessedEntry() = default;
            AssetProcessedEntry(JobEntry& entry, AssetBuilderSDK::ProcessJobResponse& response)
                : m_entry(AZStd::move(entry))
                , m_response(AZStd::move(response))
            {
            }

            AssetProcessedEntry(const AssetProcessedEntry& other) = default;
            AssetProcessedEntry(AssetProcessedEntry&& other)
                : m_entry(AZStd::move(other.m_entry))
                , m_response(AZStd::move(other.m_response))
            {
            }

            AssetProcessedEntry& operator=(AssetProcessedEntry&& other)
            {
                if (this != &other)
                {
                    m_entry = AZStd::move(other.m_entry);
                    m_response = AZStd::move(other.m_response);
                }
                return *this;
            }
        };

        //! Internal structure that will hold all the necessary source info
        struct SourceFileInfo
        {
            SourceAssetReference m_sourceAssetReference;
            AZ::Uuid m_uuid;
            const ScanFolderInfo* m_scanFolder{ nullptr };
        };

    public:
        explicit AssetProcessorManager(AssetProcessor::PlatformConfiguration* config, QObject* parent = nullptr);

        virtual ~AssetProcessorManager();
        bool IsIdle();
        bool HasProcessedCriticalAssets() const;

        //////////////////////////////////////////////////////////////////////////
        // ProcessingJobInfoBus::Handler overrides
        void BeginCacheFileUpdate(const char* productPath) override;
        void EndCacheFileUpdate(const char* productPath, bool queueAgainForDeletion) override;
        AZ::u32 GetJobFingerprint(const AssetProcessor::JobIndentifier& jobIndentifier) override;
        //////////////////////////////////////////////////////////////////////////

        //! Controls whether or not we are allowed to skip analysis on a file when the source files modtimes have not changed
        //! and neither have any builders.
        void SetEnableModtimeSkippingFeature(bool enable);

        //! Query logging will log every asset database query.
        void SetQueryLogging(bool enableLogging);

        void SetBuilderDebugFlag(bool enabled);
        bool GetBuilderDebugFlag() const { return m_builderDebugFlag; }

        //! Scans assets that match the given pattern for content that looks like a missing product dependency.
        //! Note that the database pattern is used as an SQL query, so use SQL syntax for the search (wildcard is %, not *).
        //! FilePattern is just a normal wildcard pattern that can be used to filter files in the provided scan folders.
        void ScanForMissingProductDependencies(QString dbPattern, QString filePattern, const AZStd::vector<AZStd::string>& dependencyAdditionalScanFolders, int maxScanIteration=AssetProcessor::MissingDependencyScanner::DefaultMaxScanIteration);

        AZStd::shared_ptr<AssetDatabaseConnection> GetDatabaseConnection() const;

        void EmitResolvedDependency(const AZ::Data::AssetId& assetId, const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry& entry);

        //! Internal structure that will hold all the necessary information to process jobs later.
        //! We need to hold these jobs because they have declared either source dependency on other sources
        //! or a job dependency and we can only resolve these dependencies once all the create jobs are completed.
        struct JobToProcessEntry
        {
            bool operator<(const JobToProcessEntry& other)
            {
                return m_sourceFileInfo.m_sourceAssetReference.RelativePath() < other.m_sourceFileInfo.m_sourceAssetReference.RelativePath();
            }

            SourceFileInfo m_sourceFileInfo;
            AZStd::vector<JobDetails> m_jobsToAnalyze;
            // a vector of pairs of <builder which emitted it, the dependency>
            AZStd::vector<AZStd::pair<AZ::Uuid, AssetBuilderSDK::SourceFileDependency>> m_sourceFileDependencies;
        };

        //! Request to invalidate and reprocess a source asset or folder containing source assets
        AZ::u64 RequestReprocess(const QString& sourcePath);
        AZ::u64 RequestReprocess(const AZStd::list<AZStd::string>& reprocessList);

        //! Retrieves the scan folder ID for the intermediate asset scan folder, if available.
        //! Calls GetIntermediateAssetsScanFolderId for the platform config, which returns an optional.
        //! If the scan folder ID is not available, returns nullopt, otherwise returns the scan folder ID.
        //! The scan folder ID may not be available if the platform config is not available,
        //! or the scan folder ID hasn't been set for the platform config.
        AZStd::optional<AZ::s64> GetIntermediateAssetScanFolderId() const;

    Q_SIGNALS:
        void NumRemainingJobsChanged(int newNumJobs);

        void AssetToProcess(JobDetails jobDetails);

        //! Emit whenever a new asset is found or an existing asset is updated
        void AssetMessage(AzFramework::AssetSystem::AssetNotificationMessage message);

        // InputAssetProcessed - uses absolute asset path of input file
        void InputAssetProcessed(QString fullAssetPath, QString platform);

        void RequestInputAssetStatus(QString inputAssetPath, QString platform, QString jobDescription);
        void RequestPriorityAssetCompile(QString inputAssetPath, QString platform, QString jobDescription);

        //! AssetProcessorManagerIdleState is emitted when APM idle state changes, we emit true when
        //! APM is waiting for outside stimulus i.e its has eaten through all of its queues and is only waiting for
        //! responses back from other systems (like its waiting for responses back from the compiler)
        void AssetProcessorManagerIdleState(bool state);
        void ReadyToQuit(QObject* source);

        void CreateAssetsRequest(unsigned int nonce, QString name, QString platform, bool onlyExactMatch = true, bool syncRequest = false);

        void SendAssetExistsResponse(NetworkRequestID groupID, bool exists);

        void FenceFileDetected(unsigned int fenceId);

        void EscalateJobs(AssetProcessor::JobIdEscalationList jobIdEscalationList);

        void SourceDeleted(SourceAssetReference sourceAsset);
        void SourceFolderDeleted(QString folderPath);
        void SourceQueued(AZ::Uuid sourceUuid, AZ::Uuid legacyUuid, SourceAssetReference sourceAssetReference);
        void SourceFinished(AZ::Uuid sourceUuid, AZ::Uuid legacyUuid);
        void JobRemoved(AzToolsFramework::AssetSystem::JobInfo jobInfo);

        void JobComplete(JobEntry jobEntry, AzToolsFramework::AssetSystem::JobStatus status);
        void JobProcessDurationChanged(JobEntry jobEntry, int durationMs);
        void CreateJobsDurationChanged(QString sourceName);

        //! Send a message when a new path dependency is resolved, so that downstream tools know the AssetId of the resolved dependency.
        void PathDependencyResolved(const AZ::Data::AssetId& assetId, const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry& entry);

        void AddedToCatalog(JobEntry jobEntry);

        //! Fired when FinishAnalysis is run for a file to notify that a source file has completely finished processing.
        //! count is the number of files remaining waiting for FinishAnalysis to be called
        void FinishedAnalysis(int count);

    public Q_SLOTS:
        void AssetProcessed(JobEntry jobEntry, AssetBuilderSDK::ProcessJobResponse response);
        void AssetProcessed_Impl();

        void AssetFailed(JobEntry jobEntry);
        void AssetCancelled(JobEntry jobEntry);

        void AssessFilesFromScanner(QSet<AssetFileInfo> filePaths);
        void RecordFoldersFromScanner(QSet<AssetFileInfo> folderPaths);

        virtual void AssessModifiedFile(QString filePath);
        virtual void AssessAddedFile(QString filePath);
        virtual void AssessDeletedFile(QString filePath);
        void OnAssetScannerStatusChange(AssetProcessor::AssetScanningStatus status);
        void FinishAssetScan();
        void OnJobStatusChanged(JobEntry jobEntry, JobStatus status);

        void CheckAssetProcessorIdleState();

        void QuitRequested();

        //! A network request came in asking, for a given input asset, what the status is of any jobs related to that request
        AssetJobsInfoResponse ProcessGetAssetJobsInfoRequest(MessageData<AssetJobsInfoRequest> messageData);

        //! A network request came in, Given a JOB ID (from the above Job Request), asking for the actual log for that job.
        AssetJobLogResponse ProcessGetAssetJobLogRequest(MessageData<AssetJobLogRequest> messageData);

        //! A network request came in asking for asset database location
        GetAbsoluteAssetDatabaseLocationResponse ProcessGetAbsoluteAssetDatabaseLocationRequest(MessageData<GetAbsoluteAssetDatabaseLocationRequest> messageData);

        //! This request comes in and is expected to do whatever heuristic is required in order to determine if an asset actually exists in the database.
        void OnRequestAssetExists(NetworkRequestID requestId, QString platform, QString searchTerm, AZ::Data::AssetId assetId);

        //! Searches the product and source asset tables to try and find a match
        QString GuessProductOrSourceAssetName(QString searchTerm,  QString platform, bool useLikeSearch);

        void ProcessFilesToExamineQueue();
        void CheckForIdle();
        void CheckMissingFiles();
        void ProcessGetAssetJobsInfoRequest(AssetJobsInfoRequest& request, AssetJobsInfoResponse& response);
        void ProcessGetAssetJobLogRequest(const AssetJobLogRequest& request, AssetJobLogResponse& response);
        void ScheduleNextUpdate();
        void ProcessJobs();
        void RemoveEmptyFolders();

        void OnBuildersRegistered();

    private:
        template <class R>
        bool Recv(unsigned int connId, QByteArray payload, R& request);
        void AssessFileInternal(QString fullFile, bool isDelete, bool fromScanner = false);
        void CheckSource(const FileEntry& source);
        void CheckMissingJobs(QString relativeSourceFile, const ScanFolderInfo* scanFolder, const AZStd::vector<JobDetails>& jobsThisTime);
        void CheckDeletedProductFile(QString normalizedPath);
        void CheckDeletedSourceFile(
            const SourceAssetReference& sourceAsset,
            AZStd::chrono::steady_clock::time_point initialProcessTime);
        void CheckModifiedSourceFile(QString normalizedPath, QString databaseSourceFile, const ScanFolderInfo* scanFolderInfo);
        bool AnalyzeJob(JobDetails& details);
        void CheckDeletedCacheFolder(QString normalizedPath);
        void CheckDeletedSourceFolder(QString normalizedPath, QString relativePath, const ScanFolderInfo* scanFolderInfo);
        void CheckCreatedSourceFolder(QString normalizedPath);
        void FailTopLevelSourceForIntermediate(const SourceAssetReference& intermediateAsset, AZStd::string_view errorMessage);
        void CheckMetaDataRealFiles(QString relativePath);
        bool DeleteProducts(const AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& products);
        void DispatchFileChange();
        bool InitializeCacheRoot();
        void PopulateJobStateCache();
        void AutoFailJob(
            AZStd::string_view consoleMsg,
            AZStd::string_view autoFailReason,
            const AZ::IO::Path& absoluteFilePath,
            JobEntry jobEntry,
            AZStd::string_view jobLog = "");
        void AutoFailJob(AZStd::string_view consoleMsg, AZStd::string_view autoFailReason, const AZStd::vector<AssetProcessedEntry>::iterator& assetIter);

        using ProductInfoList = AZStd::vector<AZStd::pair<AzToolsFramework::AssetDatabase::ProductDatabaseEntry, const AssetBuilderSDK::JobProduct*>>;

        void WriteProductTableInfo(AZStd::pair<AzToolsFramework::AssetDatabase::ProductDatabaseEntry, const AssetBuilderSDK::JobProduct*>& pair, AZStd::vector<AZ::u32>& subIds, AZStd::unordered_set<AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry>& dependencyContainer, const AZStd::string& platform);

        //! given a full absolute path to a file, add any metadata files you find that apply.
        void AddMetadataFilesForFingerprinting(QString absolutePathToFileToCheck, SourceFilesForFingerprintingContainer& outFilesToFingerprint);

        // given a file name and a root to not go beyond, add the parent folder and its parent folders recursively
        // to the list of known folders.
        void AddKnownFoldersRecursivelyForFile(QString file, QString root);
        void CleanEmptyFolder(QString folder, QString root);

        void ProcessBuilders(QString normalizedPath, QString relativePathToFile, const ScanFolderInfo* scanFolder, const AssetProcessor::BuilderInfoList& builderInfoList);
        AZStd::vector<AZStd::string> GetExcludedFolders();

        struct SourceInfoWithFingerprints
        {
            SourceAssetReference m_sourceAssetReference;
            QString m_analysisFingerprint;
        };

        struct ConflictResult
        {
            enum class ConflictType
            {
                None,
                //! Indicates the conflict occurred because of a new intermediate overriding an existing source
                Intermediate,
            };

            ConflictType m_type;

            //! The file that has caused the conflict.  If ConflictType == Intermediate, this is the source
            SourceAssetReference m_conflictingFile;
        };

        //! Search the database and the the source dependency maps for the the sourceUuid. if found returns the cached info
        bool SearchSourceInfoBySourceUUID(const AZ::Uuid& sourceUuid, SourceAssetReference& result);

        //!  Adds the source to the database and returns the corresponding sourceDatabase Entry
        void AddSourceToDatabase(AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceDatabaseEntry, const ScanFolderInfo* scanFolder, const SourceAssetReference& sourceAsset);

        // ! Get the engine, project and active gem root directories which could potentially be separate repositories.
        AZStd::vector<AZStd::string> GetPotentialRepositoryRoots();

    protected:
        // given a set of file info that definitely exist, warm the file cache up so
        // that we only query them once.
        void WarmUpFileCache(QSet<AssetFileInfo> filePaths);
        // Checks whether or not a file can be skipped for processing (ie, file content hasn't changed, builders haven't been added/removed, builders for the file haven't changed)
        bool CanSkipProcessingFile(const AssetFileInfo &fileInfo, AZ::u64& fileHash);

        AZ::s64 GenerateNewJobRunKey();
        // Attempt to erase a log file.  Failing to erase it is not a critical problem, but should be logged.
        // returns true if there is no log file there after this operation completes
        bool EraseLogFile(const char* fileName);

        // Load the old scan folders and match them up with new scan folders.  Make sure they're
        bool MigrateScanFolders();

        //! Checks whether the AP is aware of any source file that has indicated the inputted
        //! source file as its dependency, and if found do we need to put that file back in the asset pipeline queue again
        QStringList GetSourceFilesWhichDependOnSourceFile(const QString& sourcePath, const ProductInfoList& updatedProducts);

        /** Given a BuilderSDK SourceFileDependency, try to find out what actual database source name is.
        *   If it cannot be resolved but a UUID is available, the string result will contain the UUID (and we will return true).
        *   If there's a problem that makes it unusable (such as no fields being filled in), the string will be blank
        *     and this function will return false.
        */
        bool ResolveSourceFileDependencyPath(const AssetBuilderSDK::SourceFileDependency& sourceDependency, QString& resultDatabaseSourceNames, QStringList& resolvedDependencyList);
        //! Updates the database with all the changes related to source dependency / job dependency:
        void UpdateSourceFileDependenciesDatabase(JobToProcessEntry& entry);

        //! Analyze JobDetail for every hold jobs
        void AnalyzeJobDetail(JobToProcessEntry& jobEntry);

        void UpdateJobDependency(JobDetails& jobDetails);
        void QueueIdleCheck();
        void UpdateWildcardDependencies(JobDetails& job, size_t jobDependencySlot, QStringList& resolvedDependencyList);

        //! Check whether the job can be analyzed by APM,
        //! A job cannot be analyzed if any of its dependent job hasn't been fingerprinted
        bool CanAnalyzeJob(const JobDetails& jobDetails);

        //! Analyzes and forward the job to the RCController if the job requires processing
        void ProcessJob(JobDetails& jobDetails);

        // Returns true if the path is inside the Cache and *not* inside the Intermediate Assets folder
        bool IsInCacheFolder(AZ::IO::PathView path) const;

        // Returns true if the path is inside the Intermediate Assets folder
        bool IsInIntermediateAssetsFolder(AZ::IO::PathView path) const;
        bool IsInIntermediateAssetsFolder(QString path) const;

        // Checks if an intermediate product conflicts with an existing source asset
        // searchSourcePath should be the path of the intermediate to use to search for existing sources of the same name
        ConflictResult CheckIntermediateProductConflict(const char* searchSourcePath);

        bool CheckForIntermediateAssetLoop(const SourceAssetReference& sourceAsset, const SourceAssetReference& productAsset);

        void UpdateForCacheServer(JobDetails& jobDetails);

        //! Check whether the specified file is an LFS pointer file.
        bool IsLfsPointerFile(const AZStd::string& filePath);

        AssetProcessor::PlatformConfiguration* m_platformConfig = nullptr;

        bool m_queuedExamination = false;
        bool m_hasProcessedCriticalAssets = false;

        QQueue<FileEntry> m_activeFiles;
        QSet<QString> m_alreadyActiveFiles; // a simple optimization to only do the exhaustive search if we know its there.
        AZStd::vector<AssetProcessedEntry> m_assetProcessedList;
        AZStd::shared_ptr<AssetDatabaseConnection> m_stateData;
        ThreadController<AssetCatalog>* m_assetCatalog;
        typedef QHash<QString, FileEntry> FileExamineContainer;
        FileExamineContainer m_filesToExamine; // order does not actually matter in this (yet)

        // this map contains a list of source files that were discovered in the database before asset scanning began.
        // (so files from a previous run).
        // as asset scanning encounters files, it will remove them from this map, and when its done,
        // it will thus contain only the files that were in the database from last time, but were NOT found during file scan
        // in other words, files that have been deleted from disk since last run.
        // the key to this map is the absolute path of the file from last run, but with the current scan folder setup
        QMap<QString, SourceInfoWithFingerprints> m_sourceFilesInDatabase;

        // this map contains modtimes of all files AP processed last time it ran
        AZStd::unordered_map<AZStd::string, AZ::u64> m_fileModTimes;

        // this map contains hashes of all files AP processed last time it ran
        AZStd::unordered_map<AZStd::string, AZ::u64> m_fileHashes;

        QSet<QString> m_knownFolders; // a cache of all known folder names, normalized to have forward slashes.
        typedef AZStd::unordered_map<AZ::u64, AzToolsFramework::AssetSystem::JobInfo> JobRunKeyToJobInfoMap;  // for when network requests come in about the jobInfo

        JobRunKeyToJobInfoMap m_jobRunKeyToJobInfoMap;
        AZStd::multimap<AZStd::string, AZ::u64> m_jobKeyToJobRunKeyMap;

        using SourceUUIDToSourceInfoMap = AZStd::unordered_map<AZ::Uuid, SourceAssetReference>;
        SourceUUIDToSourceInfoMap m_sourceUUIDToSourceInfoMap; // contains Source Asset UUID -> SourceAssetReference
        AZStd::mutex m_sourceUUIDToSourceInfoMapMutex;

        QString m_normalizedCacheRootPath;
        QDir m_cacheRootDir;
        bool m_isCurrentlyScanning = false;
        bool m_quitRequested = false;
        bool m_processedQueued = false;
        bool m_AssetProcessorIsBusy = true;

        bool m_alreadyScheduledUpdate = false;
        QMutex m_processingJobMutex;
        AZStd::unordered_set<AZStd::string> m_processingProductInfoList;
        AZ::s64 m_highestJobRunKeySoFar = 0;
        AZStd::vector<JobToProcessEntry> m_jobEntries;
        AZStd::unordered_set<JobDetails> m_jobsToProcess;
        //! This map is required to prevent multiple sourceFile modified events been send by the APM
        AZStd::unordered_map<AZ::Uuid, qint64> m_sourceFileModTimeMap;
        AZStd::unordered_map<JobIndentifier, AZ::u32> m_jobFingerprintMap;
        AZStd::unordered_map<JobDesc, AZStd::unordered_set<AZ::Uuid>> m_jobDescToBuilderUuidMap;

        AZStd::unique_ptr<PathDependencyManager> m_pathDependencyManager;
        AZStd::unique_ptr<SourceFileRelocator> m_sourceFileRelocator;
        AZStd::unique_ptr<LfsPointerFileValidator> m_lfsPointerFileValidator;

        JobDiagnosticTracker m_jobDiagnosticTracker{};

        QSet<QString> m_checkFoldersToRemove; //!< List of folders that needs to be checked for removal later by AP
        //! List of all scanfolders that are present in the database but not currently watched by AP
        AZStd::unordered_map<AZStd::string, AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry> m_scanFoldersInDatabase;

        int m_numOfJobsToAnalyze = 0;
        bool m_alreadyQueuedCheckForIdle = false;

        //////////////////// Analysis Early-Out feature ///////////////////
        // ComputeBuilderDirty builds the maps of which builders are dirty and how they have changed.
        // note that until ComputeBuilderDirty is called, it is assumed that *all* are dirty, to be conservative.

        // The data we actually care about for this feature:
        struct BuilderData
        {
            AZ::u8 m_flags = 0; // the flags from the builder registration
            AZ::Uuid m_fingerprint; // a hash of the fingerprint and version info
            bool m_isDirty = false;
        };

        void ComputeBuilderDirty();
        AZStd::string ComputeRecursiveDependenciesFingerprint(const AZStd::string& fileAbsolutePath, const AZStd::string& fileDatabaseName);
        AZStd::unordered_map<AZ::Uuid, BuilderData>  m_builderDataCache;
        bool m_buildersAddedOrRemoved = true; //< true if any new builders exist.  If this happens we actually need to re-analyze everything.
        bool m_anyBuilderChange = true;

        // Checks whether any of the builders specified have changed their fingerprint
        bool AreBuildersUnchanged(AZStd::string_view builderEntries, int& numBuildersEmittingSourceDependencies);

        /** Utility function:  Given the input database row (from sources table), return an (ordered) set of all dependencies
          * including dependencies-of-dependencies.  These will be absolute paths to the dependency file on disk.
          * Note that the output also includes the initial inputDatabasePath asset (but expanded to be absolute)
          * if a file does not exist, it will still in the list at the absolute path to where it may appear, so that
          * this result set can still use that for hashing.
          * if a source file is missing from disk, it will not be included in the result set, since this returns
          * full absolute paths.
          */
        void QueryAbsolutePathDependenciesRecursive(AZ::Uuid sourceUuid, SourceFilesForFingerprintingContainer& finalDependencyList, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::TypeOfDependency dependencyType);

        // we can't write a job to the database as not needing analysis the next time around,
        // until all jobs related to it are finished.  This is becuase the jobs themselves are not written to the database
        // so until all jobs are finished, we need to re-analyze the source file next time.
        // (since if you terminate the asset processor while its still processing, we don't want it to skip over those
        // source files next time).  So we keep a map of how many remaining outstanding jobs exist for a given
        // source file.  Once the outstanding jobs hit zero we compute a final source fingerprint for analysis and save it.
        struct AnalysisTracker
        {
            int m_remainingJobsSpawned = 0;
            AZ::s64 m_databaseScanFolderId = -1;
            AZStd::string m_databaseSourceName;
            AZStd::set<AZ::Uuid> m_buildersInvolved; // this is intentionally a sorted set, since its used to generate a stable hash
            bool failedStatus = false; // if it fails, we avoid writing anything to the database, so that next time around, we reprocess the file.
        };

        // maps "absolute source path to file (normalized)" to tracking infomation struct above.
        using JobCounter = AZStd::unordered_map<AZStd::string, AnalysisTracker> ;
        JobCounter m_remainingJobsForEachSourceFile;

        // utility function: finds the source in the above map and updates it.
        enum class AnalysisTrackerUpdateType
        {
            JobFailed,
            JobStarted,
            JobFinished,
        };

        // ideally you would already have the absolute path to the file, and call this function with it:
        void UpdateAnalysisTrackerForFile(const char* fullPathToFile, AnalysisTrackerUpdateType updateType);

        // convenience overload of the above function when you have a jobEntry but no absolute path to the file.
        void UpdateAnalysisTrackerForFile(const JobEntry &entry, AnalysisTrackerUpdateType updateType);

        // Used to scan through products for anything that looks like a missing product dependency;
        MissingDependencyScanner m_missingDependencyScanner;

        // Metrics
        int m_numTotalSourcesFound = 0;
        int m_numSourcesNeedingFullAnalysis = 0;
        int m_numSourcesNotHandledByAnyBuilder = 0;
        bool m_reportedAnalysisMetrics = false;

        // cache these so we don't have to check them each time during analysis:
        QSet<QString> m_metaFilesWhichActuallyExistOnDisk;
        bool m_cachedMetaFilesExistMap = false;

        // when true, only processes files if their modtime or builder(s) have changed
        // defaults to true (in the settings) for GUI mode, false for batch mode
        bool m_allowModtimeSkippingFeature = false;

        // when true, a flag will be sent to builders process job indicating debug output/mode should be used
        bool m_builderDebugFlag = false;

        AZStd::unique_ptr<ExcludedFolderCache> m_excludedFolderCache{};

protected Q_SLOTS:
        void FinishAnalysis(AZStd::string fileToCheck);
        //////////////////////////////////////////////////////////
    };
} // namespace AssetProcessor

