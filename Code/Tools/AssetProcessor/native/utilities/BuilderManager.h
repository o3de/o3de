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
#include <AzFramework/Process/ProcessWatcher.h>
#include <AzFramework/Process/ProcessCommunicatorTracePrinter.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <QString>
#include <QByteArray>
#include <native/utilities/assetUtils.h>
#include <QDir>  // used in the inl file.

class ConnectionManager;

namespace AssetProcessor
{
    struct BuilderRef;

    //! Indicates if job request files should be created on success.  Can be useful for debugging
    static const bool s_createRequestFileForSuccessfulJob = false;

    //! This EBUS is used to request a free builder from the builder manager pool
    class BuilderManagerBusTraits
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using MutexType = AZStd::recursive_mutex;

        virtual ~BuilderManagerBusTraits() = default;

        //! Returns a builder for doing work
        virtual BuilderRef GetBuilder() = 0;
    };

    using BuilderManagerBus = AZ::EBus<BuilderManagerBusTraits>;

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

    public:
        Builder(const AssetUtilities::QuitListener& quitListener, AZ::Uuid uuid)
            : m_uuid(uuid),
            m_quitListener(quitListener)
        {}
        ~Builder() = default;

        // Disable copy and move (can't move a semaphore)
        AZ_DISABLE_COPY_MOVE(Builder);

        //! Returns true if the builder has a valid connection id and, if there is a process associated, the process is running
        bool IsValid() const;

        //! Returns true if the builder has a process watcher and the process is running OR does not have a process watcher.  Returns false otherwise
        bool IsRunning(AZ::u32* exitCode = nullptr) const;

        //! Returns true if the builder exe has established a connection
        bool IsConnected() const;

        //! Blocks waiting for the builder to establish a connection
        bool WaitForConnection();

        AZ::u32 GetConnectionId() const;
        AZ::Uuid GetUuid() const;
        AZStd::string UuidString() const;

        void PumpCommunicator() const;
        void FlushCommunicator() const;
        void TerminateProcess(AZ::u32 exitCode) const;

        //! Sends the job over to the builder and blocks until the response is received or the builder crashes/times out
        template<typename TNetRequest, typename TNetResponse, typename TRequest, typename TResponse>
        BuilderRunJobOutcome RunJob(const TRequest& request, TResponse& response, AZ::u32 processTimeoutLimitInSeconds, const AZStd::string& task, const AZStd::string& modulePath, AssetBuilderSDK::JobCancelListener* jobCancelListener = nullptr, AZStd::string tempFolderPath = AZStd::string()) const;

    private:

        //! Starts the builder process and waits for it to connect
        bool Start();

        //! Sets the connection id and signals that the builder has connected
        void SetConnection(AZ::u32 connId);

        AZStd::string BuildParams(const char* task, const char* moduleFilePath, const AZStd::string& builderGuid, const AZStd::string& jobDescriptionFile, const AZStd::string& jobResponseFile) const;
        AZStd::unique_ptr<AzFramework::ProcessWatcher> LaunchProcess(const char* fullExePath, const AZStd::string& params) const;

        //! Waits for the builder exe to send the job response and pumps stdout/err
        BuilderRunJobOutcome WaitForBuilderResponse(AssetBuilderSDK::JobCancelListener* jobCancelListener, AZ::u32 processTimeoutLimitInSeconds, AZStd::binary_semaphore* waitEvent) const;

        //! Writes the request out to disk for debug purposes and logs info on how to manually run the asset builder
        template<typename TRequest>
        bool DebugWriteRequestFile(QString tempFolderPath, const TRequest& request, const AZStd::string& task, const AZStd::string& modulePath) const;

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

    private:
        AZStd::shared_ptr<Builder> m_builder = nullptr;
    };

    //! Manages the builder pool
    class BuilderManager
        : public BuilderManagerBus::Handler
    {
    public:
        explicit BuilderManager(ConnectionManager* connectionManager);
        ~BuilderManager();

        // Disable copy
        AZ_DISABLE_COPY_MOVE(BuilderManager);

        void ConnectionLost(AZ::u32 connId);

        //BuilderManagerBus
        BuilderRef GetBuilder() override;

    private:

        //! Makes a new builder, adds it to the pool, and returns a shared pointer to it
        AZStd::shared_ptr<Builder> AddNewBuilder();

        //! Handles incoming builder connections
        void IncomingBuilderPing(AZ::u32 connId, AZ::u32 type, AZ::u32 serial, QByteArray payload, QString platform);

        void PumpIdleBuilders();

        AZStd::mutex m_buildersMutex;

        //! Map of builders, keyed by the builder's unique ID.  Must be locked before accessing
        AZStd::unordered_map<AZ::Uuid, AZStd::shared_ptr<Builder>> m_builders;

        //! Indicates if we allow builders to connect that we haven't started up ourselves.  Useful for debugging
        bool m_allowUnmanagedBuilderConnections = false;

        //! Responsible for going through all the idle builders and pumping their communicators so they don't stall
        AZStd::thread m_pollingThread;

        AssetUtilities::QuitListener m_quitListener;
    };
} // namespace AssetProcessor

#include "native/utilities/BuilderManager.inl"
