/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/parallel/semaphore.h>
#include <AzCore/std/parallel/atomic.h>

#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzToolsFramework/SourceControl/LocalFileSCComponent.h>

namespace AzToolsFramework
{
    class PerforceConnection;

    class PerforceJobRequest
    {
    public:
        enum RequestType
        {
            PJR_Invalid = 0,
            PJR_Stat,
            PJR_StatBulk,
            PJR_Add,
            PJR_Edit,
            PJR_EditBulk,
            PJR_Delete,
            PJR_DeleteBulk,
            PJR_Revert,
            PJR_Rename,
            PJR_RenameBulk,
            PJR_Sync,
        };

        RequestType m_requestType { PJR_Invalid };
        AZStd::string m_requestPath;
        AZStd::string m_targetPath;
        AZStd::unordered_set<AZStd::string> m_bulkFilePaths;
        bool m_allowMultiCheckout{};
        bool m_skipReadonly{};
        SourceControlResponseCallback m_callback{ nullptr };
        SourceControlResponseCallbackBulk m_bulkCallback{ nullptr };

        PerforceJobRequest() = default;

        PerforceJobRequest(RequestType requestType, const AZStd::string& requestPath, SourceControlResponseCallback responseCB)
            : m_requestType(requestType)
            , m_requestPath(requestPath)
            , m_callback(AZStd::move(responseCB))
        {}

        PerforceJobRequest(RequestType requestType, const AZStd::string& requestPath, SourceControlResponseCallbackBulk responseCB)
            : m_requestType(requestType)
            , m_requestPath(requestPath)
            , m_bulkCallback(AZStd::move(responseCB))
        {}

        PerforceJobRequest(RequestType requestType, const AZStd::unordered_set<AZStd::string>& bulkFilePaths, SourceControlResponseCallbackBulk responseCB)
            : m_requestType(requestType)
            , m_bulkFilePaths(bulkFilePaths)
            , m_bulkCallback(AZStd::move(responseCB))
        {}
    };

    class PerforceJobResult
    {
    public:

        SourceControlResponseCallback m_callback{ nullptr };
        SourceControlResponseCallbackBulk m_bulkCallback{ nullptr };
        bool m_succeeded{};
        SourceControlFileInfo m_fileInfo;
        AZStd::vector<SourceControlFileInfo> m_bulkFileInfo;

        PerforceJobResult() = default;
    };

    class PerforceSettingResult
    {
    public:
        PerforceSettingResult() = default;
        void UpdateSettingInfo(const AZStd::string& value);

        SourceControlSettingCallback m_callback = nullptr;
        SourceControlSettingInfo m_settingInfo;
    };

    typedef AZStd::unordered_map<AZStd::string, AZStd::string> PerforceMap;

    // the perforce component's job is to manage perforce connectivity and execute perforce commands
    // it parses the status of perforce commands and returns results.
    // it has helpers to determine what needs to be done with a file in order to remove it or to add it.
    // for example, if a file is checked out and needs to be deleted, it knows that it will need to both revert the file and mark it for delete
    // in order to get to where we want it to be, from where we are now.
    // it does not keep track of individual files or watch directories or anything
    class PerforceComponent
        : public AZ::Component
        , private SourceControlCommandBus::Handler
        , private SourceControlConnectionRequestBus::Handler
    {
    public:
        AZ_COMPONENT(PerforceComponent, "{680C2C8B-37CA-42EB-9E50-06AB2474201E}")

        AZStd::string m_autoChangelistDescription;

        PerforceComponent() = default;
        ~PerforceComponent() override = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component overrides
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////
    protected:
        void SetConnection(PerforceConnection* connection);

    private:
        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        //////////////////////////////////////////////////////////////////////////
        // SourceControlCommandBus::Handler overrides
        void GetFileInfo(const char* fullFilePath, const SourceControlResponseCallback& respCallback) override;
        void GetBulkFileInfo(const AZStd::unordered_set<AZStd::string>& fullFilePaths, const SourceControlResponseCallbackBulk& respCallback) override;
        void RequestEdit(const char* fullFilePath, bool allowMultiCheckout, const SourceControlResponseCallback& respCallback) override;
        void RequestEditBulk(const AZStd::unordered_set<AZStd::string>& fullFilePaths, bool allowMultiCheckout, const SourceControlResponseCallbackBulk& respCallback) override;
        void RequestDelete(const char* fullFilePath, const SourceControlResponseCallback& respCallback) override;
        void RequestDeleteExtended(const char* fullFilePath, bool skipReadOnly, const SourceControlResponseCallback& respCallback) override;
        void RequestDeleteBulk(const char* fullFilePath, const SourceControlResponseCallbackBulk& respCallback) override;
        void RequestDeleteBulkExtended(const char* fullFilePath, bool skipReadOnly, const SourceControlResponseCallbackBulk& respCallback) override;
        void RequestRevert(const char* fullFilePath, const SourceControlResponseCallback& respCallback) override;
        void RequestLatest(const char* fullFilePath, const SourceControlResponseCallback& respCallback) override;
        void RequestRename(const char* sourcePathFull, const char* destPathFull, const SourceControlResponseCallback& respCallback) override;
        void RequestRenameExtended(const char* sourcePathFull, const char* destPathFull, bool skipReadOnly, const SourceControlResponseCallback& respCallback) override;
        void RequestRenameBulk(const char* sourcePathFull, const char* destPathFull, const SourceControlResponseCallbackBulk& respCallback) override;
        void RequestRenameBulkExtended(const char* sourcePathFull, const char* destPathFull, bool skipReadOnly, const SourceControlResponseCallbackBulk& respCallback) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // SourceControlConnectionRequestBus::Handler overrides
        void EnableSourceControl(bool enable) override;
        bool IsActive() const override { return m_connectionState == SourceControlState::Active; }
        void EnableTrust(bool enable, AZStd::string fingerprint) override;
        void SetConnectionSetting(const char* key, const char* value, const SourceControlSettingCallback& respCallBack) override;
        void GetConnectionSetting(const char* key, const SourceControlSettingCallback& respCallBack) override;
        void OpenSettings() override;
        SourceControlState GetSourceControlState() const override { return m_connectionState; }
        //////////////////////////////////////////////////////////////////////////

        SourceControlFileInfo GetFileInfo(const char*);
        AZStd::vector<SourceControlFileInfo> GetBulkFileInfo(const char* requestPath) const;
        AZStd::vector<SourceControlFileInfo> GetBulkFileInfo(const AZStd::unordered_set<AZStd::string>& requestPaths) const;

        //! Attempt to checkout a file
        //! - If the file is marked for add, nothing will occur, its already writable for you.
        //! - If the file is marked for delete, it will be reverted, then checked out, so that its writable and ready to add.
        //! - If the file is marked for checkout already, nothing will occur (but we'll return true)
        //! - If the file is not in perforce, nothing will occur (but we'll return true).
        //! Error conditions will occur (false returned) when someone else has the file checked out, the file is locked, or from insufficient permissions
        //! Note that calling this function will 'claim' the file into the editor's changelist, even if its already open on another changelist.
        //! Note You cant check a file out if you don't have latest.
        bool RequestEdit(const char* fullFilePath, bool allowMultiCheckout);

        bool RequestEditBulk(const AZStd::unordered_set<AZStd::string>& fullFilePath, bool allowMultiCheckout);

        //! Attempt to delete a file from both perforce and local
        bool RequestDelete(const char* fullFilePath);

        //! Attempt to delete a file from both perforce and local
        bool RequestDeleteBulk(const char* fullFilePath);

        //! Attempt to get the latest revision of the file
        bool RequestLatest(const char* fullFilePath);

        //! Attempt to revert a file to its last changelist
        bool RequestRevert(const char* fullFilePath);

        //! Attempt to rename a file
        bool RequestRename(const char* sourcePathFull, const char* destPathFull);

        //! Attempt to rename a file
        bool RequestRenameBulk(const char* sourcePathFull, const char* destPathFull);

        bool ClaimChangedFile(const char* fullFilePath, int changelistTarget);

        bool ExecuteAdd(const char* filePath);
        bool ExecuteEdit(const char* filePath, bool allowMultiCheckout, bool allowAdd);
        bool ExecuteEditBulk(const AZStd::unordered_set<AZStd::string>& filePaths, bool allowMultiCheckout, bool allowAdd);
        bool ExecuteDelete(const char* filePath);
        bool ExecuteDeleteBulk(const char* filePath);
        bool ExecuteSync(const char* filePath);
        bool ExecuteRevert(const char* filePath);
        bool ExecuteMove(const char* sourcePath, const char* destPath);
        bool ExecuteMoveBulk(const char* sourcePath, const char* destPath);

        void QueueJobRequest(PerforceJobRequest&& jobRequest);
        void QueueSettingResponse(const PerforceSettingResult& result);

        bool CheckConnectivityForAction(const char* actionDesc, const char* filePath) const;
        bool ExecuteAndParseFstat(const char* filePath, bool& sourceAwareFile);
        bool ExecuteAndParseFstat(const char* filePath, AZStd::vector<PerforceMap>& commandMap) const;
        bool ExecuteAndParseFstat(const AZStd::unordered_set<AZStd::string>& filePaths, AZStd::vector<PerforceMap>& commandMap) const;
        bool ExecuteAndParseSet(const char* key, const char* value) const;
        bool CommandSucceeded();

        bool UpdateTrust();
        bool IsTrustKeyValid() const;
        void TestConnectionTrust(bool attemptResolve);
        void VerifyP4PortIsSet();

        bool IsConnectionValid() const;
        bool CacheClientConfig() const;
        void TestConnectionValid();
        bool UpdateConnectivity();
        void DropConnectivity();

        int GetOrCreateOurChangelist();
        int FindExistingChangelist();

        void ThreadWorker();

        AZStd::thread m_WorkerThread;
        AZStd::semaphore m_WorkerSemaphore;

        AZStd::queue<PerforceJobRequest> m_workerQueue;
        AZStd::queue<PerforceJobResult> m_resultQueue;
        AZStd::queue<PerforceSettingResult> m_settingsQueue;

        AZStd::mutex m_WorkerQueueMutex;
        AZStd::mutex m_ResultQueueMutex;
        AZStd::mutex m_SettingsQueueMutex;

        AZStd::atomic_bool m_shutdownThreadSignal;
        AZStd::atomic_bool m_waitingOnTrust;

        void ProcessJob(const PerforceJobRequest& request);

        void ProcessJobOffline(const PerforceJobRequest& request);

        void ProcessResultQueue();
        AZStd::thread::id m_ProcessThreadID; // used for debugging!

        bool ParseOutput(PerforceMap& perforceMap, AZStd::string& perforceOutput, const char* lineDelim = nullptr) const;
        bool ParseDuplicateOutput(AZStd::vector<PerforceMap>& perforceMapList, AZStd::string& perforceOutput) const;

        LocalFileSCComponent m_localFileSCComponent;
        AZStd::atomic_bool m_offlineMode;

        AZStd::atomic_bool m_resolveKey;
        AZStd::atomic_bool m_trustedKey;
        AZStd::atomic_bool m_testTrust;

        AZStd::atomic_bool m_testConnection;
        AZStd::atomic_bool m_validConnection;

        SourceControlState m_connectionState;
    };
} // namespace AzToolsFramework
