/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <utilities/assetUtils.h>
#include <AzFramework/Process/ProcessWatcher.h>
#include <AzFramework/Process/ProcessCommunicatorTracePrinter.h>

namespace AssetProcessor
{
    //! Enum used to indicate the purpose of a builder which may result in special handling
    enum class BuilderPurpose
    {
        CreateJobs,
        ProcessJob,
        Registration
    };

    enum class BuilderRunJobOutcome
    {
        Ok,
        LostConnection,
        ProcessTerminated,
        JobCancelled,
        ResponseFailure,
        FailedToDecodeResponse,
        FailedToWriteDebugRequest
    };

    //! Wrapper for managing a single builder process and sending job requests to it
    class Builder
    {
        friend class BuilderManager;
        friend struct BuilderRef;
        friend class BuilderList;

    public:
        Builder(const AssetUtilities::QuitListener& quitListener, AZ::Uuid uuid)
            : m_uuid(uuid)
            , m_quitListener(quitListener)
        {
        }
        virtual ~Builder() = default;

        // Disable copy and move (can't move a semaphore)
        AZ_DISABLE_COPY_MOVE(Builder);

        //! Returns true if the builder has a valid connection id and, if there is a process associated, the process is running
        bool IsValid() const;

        //! Returns true if the builder has a process watcher and the process is running OR does not have a process watcher.  Returns false
        //! otherwise
        bool IsRunning(AZ::u32* exitCode = nullptr) const;

        //! Returns true if the builder exe has established a connection
        bool IsConnected() const;

        //! Blocks waiting for the builder to establish a connection
        AZ::Outcome<void, AZStd::string> WaitForConnection();

        AZ::u32 GetConnectionId() const;
        AZ::Uuid GetUuid() const;
        AZStd::string UuidString() const;

        void PumpCommunicator() const;
        void FlushCommunicator() const;
        void TerminateProcess(AZ::u32 exitCode) const;

        //! Sends the job over to the builder and blocks until the response is received or the builder crashes/times out
        template<typename TNetRequest, typename TNetResponse, typename TRequest, typename TResponse>
        BuilderRunJobOutcome RunJob(
            const TRequest& request,
            TResponse& response,
            AZ::u32 processTimeoutLimitInSeconds,
            const AZStd::string& task,
            const AZStd::string& modulePath,
            AssetBuilderSDK::JobCancelListener* jobCancelListener = nullptr,
            AZStd::string tempFolderPath = AZStd::string()) const;

    protected:
        //! Starts the builder process and waits for it to connect
        virtual AZ::Outcome<void, AZStd::string> Start(BuilderPurpose purpose);

        //! Sets the connection id and signals that the builder has connected
        void SetConnection(AZ::u32 connId);

        AZStd::vector<AZStd::string> BuildParams(
            const char* task,
            const char* moduleFilePath,
            const AZStd::string& builderGuid,
            const AZStd::string& jobDescriptionFile,
            const AZStd::string& jobResponseFile,
            BuilderPurpose purpose) const;
        AZStd::unique_ptr<AzFramework::ProcessWatcher> LaunchProcess(
            const char* fullExePath, const AZStd::vector<AZStd::string>& params) const;

        //! Waits for the builder exe to send the job response and pumps stdout/err
        BuilderRunJobOutcome WaitForBuilderResponse(
            AssetBuilderSDK::JobCancelListener* jobCancelListener,
            AZ::u32 processTimeoutLimitInSeconds,
            AZStd::binary_semaphore* waitEvent) const;

        //! Writes the request out to disk for debug purposes and logs info on how to manually run the asset builder
        template<typename TRequest>
        bool DebugWriteRequestFile(
            QString tempFolderPath,
            const TRequest& request,
            const AZStd::string& task,
            const AZStd::string& modulePath,
            BuilderPurpose purpose) const;

        const AZ::Uuid m_uuid;

        //! Indicates if the builder is currently in use
        bool m_busy = false;

        AZStd::atomic<AZ::u32> m_connectionId = 0;

        //! Signals the exe has successfully established a connection
        AZStd::binary_semaphore m_connectionEvent;

        //! Optional process watcher
        AZStd::unique_ptr<AzFramework::ProcessWatcher> m_processWatcher = nullptr;

        //! Optional communicator, only available if we have a process watcher
        AZStd::unique_ptr<ProcessCommunicatorTracePrinter> m_tracePrinter = nullptr;

        const AssetUtilities::QuitListener& m_quitListener;

        //! Time to wait in seconds for a builder to startup before timing out.
        AZ::s64 m_startupWaitTimeS = 0;
    };

    //! Scoped reference to a builder. Destructor returns the builder to the free builders pool
    struct BuilderRef
    {
        BuilderRef() = default;
        explicit BuilderRef(const AZStd::shared_ptr<Builder>& builder);
        ~BuilderRef();

        // Disable copy
        BuilderRef(const BuilderRef&) = delete;
        BuilderRef& operator=(const BuilderRef&) = delete;

        // Allow move
        BuilderRef(BuilderRef&&);
        BuilderRef& operator=(BuilderRef&&);

        const Builder* operator->() const;

        explicit operator bool() const;

        void release();

    private:
        AZStd::shared_ptr<Builder> m_builder = nullptr;
    };
} // namespace AssetProcessor
