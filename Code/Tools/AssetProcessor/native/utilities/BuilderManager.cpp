/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "BuilderManager.h"
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Utils/Utils.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <native/connection/connectionManager.h>
#include <native/connection/connection.h>
#include <native/utilities/AssetBuilderInfo.h>
#include <QCoreApplication>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AssetBuilder/AssetBuilderStatic.h>

namespace AssetProcessor
{
    //! Amount of time in milliseconds to wait between checking the status of the AssetBuilder process and pumping the stdout/err pipes
    static const int s_MaximumSleepTimeMS = 10;

    //! Time in milliseconds to wait after each message pump cycle
    static const int s_IdleBuilderPumpingDelayMS = 100;

    static const int s_MillisecondsInASecond = 1000;

    static const char* s_buildersFolderName = "Builders";

    bool Builder::IsConnected() const
    {
        return m_connectionId > 0;
    }

    bool Builder::WaitForConnection()
    {
        if (m_startupWaitTimeS == 0)
        {
            const auto* settingsRegistry = AZ::SettingsRegistry::Get();
            if (settingsRegistry)
            {
                settingsRegistry->Get(m_startupWaitTimeS, "/Amazon/AssetProcessor/Settings/BuilderManager/StartupTimeoutSeconds");
            }
        }

        if (m_connectionId == 0)
        {
            bool result = false;
            QElapsedTimer ticker;

            ticker.start();

            while (!result)
            {
                result = m_connectionEvent.try_acquire_for(AZStd::chrono::milliseconds(s_MaximumSleepTimeMS));

                PumpCommunicator();

                if (ticker.elapsed() > m_startupWaitTimeS * s_MillisecondsInASecond
                    || m_quitListener.WasQuitRequested()
                    || !IsRunning())
                {
                    break;
                }
            }

            PumpCommunicator();
            FlushCommunicator();

            if (result)
            {
                return true;
            }

            AZ::u32 exitCode;

            if (m_quitListener.WasQuitRequested())
            {
                AZ_TracePrintf(AssetProcessor::DebugChannel, "Aborting waiting for builder, quit requested\n");
            }
            else if (!IsRunning(&exitCode))
            {
                AZ_Error("Builder", false, "AssetBuilder terminated during start up with exit code %d", exitCode);
            }
            else
            {
                AZ_Error("Builder", false, "AssetBuilder failed to connect within %d seconds", m_startupWaitTimeS);
            }

            return false;
        }

        return true;
    }

    void Builder::SetConnection(AZ::u32 connId)
    {
        m_connectionId = connId;
        m_connectionEvent.release();
    }

    AZ::u32 Builder::GetConnectionId() const
    {
        return m_connectionId;
    }

    AZ::Uuid Builder::GetUuid() const
    {
        return m_uuid;
    }

    AZStd::string Builder::UuidString() const
    {
        return m_uuid.ToString<AZStd::string>(false, false);
    }

    void Builder::PumpCommunicator() const
    {
        if (m_tracePrinter)
        {
            m_tracePrinter->Pump();
        }
    }

    void Builder::FlushCommunicator() const
    {
        if (m_tracePrinter)
        {
            // flush both STDOUT and STDERR
            m_tracePrinter->WriteCurrentString(true);
            m_tracePrinter->WriteCurrentString(false);
        }
    }

    void Builder::TerminateProcess(AZ::u32 exitCode) const
    {
        if (m_processWatcher)
        {
            m_processWatcher->TerminateProcess(exitCode);
        }
    }

    bool Builder::Start(BuilderPurpose purpose)
    {
        // Get the current BinXXX folder based on the current running AP
        QString applicationDir = QCoreApplication::instance()->applicationDirPath();

        // Construct the Builders subfolder path
        AZStd::string buildersFolder;
        AzFramework::StringFunc::Path::Join(applicationDir.toUtf8().constData(), s_buildersFolderName, buildersFolder);

        // Construct the full exe for the builder.exe
        const AZStd::string fullExePathString = QDir(applicationDir).absoluteFilePath(AssetProcessor::s_assetBuilderRelativePath).toUtf8().constData();

        if (m_quitListener.WasQuitRequested())
        {
            return false;
        }

        const AZStd::vector<AZStd::string> params = BuildParams("resident", buildersFolder.c_str(), UuidString(), "", "", purpose);

        m_processWatcher = LaunchProcess(fullExePathString.c_str(), params);

        if (!m_processWatcher)
        {
            return false;
        }

        m_tracePrinter = AZStd::make_unique<ProcessCommunicatorTracePrinter>(m_processWatcher->GetCommunicator(), "AssetBuilder");

        return WaitForConnection();
    }

    bool Builder::IsValid() const
    {
        return m_connectionId != 0 && IsRunning();
    }

    bool Builder::IsRunning(AZ::u32* exitCode) const
    {
        return !m_processWatcher || (m_processWatcher && m_processWatcher->IsProcessRunning(exitCode));
    }

    AZStd::vector<AZStd::string> Builder::BuildParams(const char* task, const char* moduleFilePath, const AZStd::string& builderGuid, const AZStd::string& jobDescriptionFile, const AZStd::string& jobResponseFile, BuilderPurpose purpose) const
    {
        QDir projectCacheRoot;
        AssetUtilities::ComputeProjectCacheRoot(projectCacheRoot);

        AZ::SettingsRegistryInterface::FixedValueString projectName = AZ::Utils::GetProjectName();
        AZ::IO::FixedMaxPathString projectPath = AZ::Utils::GetProjectPath();
        AZ::IO::FixedMaxPathString enginePath = AZ::Utils::GetEnginePath();

        int portNumber = 0;
        ApplicationServerBus::BroadcastResult(portNumber, &ApplicationServerBus::Events::GetServerListeningPort);

        AZStd::vector<AZStd::string> params;
        params.emplace_back(AZStd::string::format(R"(-task="%s")", task));
        params.emplace_back(AZStd::string::format(R"(-id="%s")", builderGuid.c_str()));
        params.emplace_back(AZStd::string::format(R"(-project-name="%s")", projectName.c_str()));
        params.emplace_back(AZStd::string::format(R"(-project-cache-path="%s")", projectCacheRoot.absolutePath().toUtf8().constData()));
        params.emplace_back(AZStd::string::format(R"(-project-path="%s")", projectPath.c_str()));
        params.emplace_back(AZStd::string::format(R"(-engine-path="%s")", enginePath.c_str()));
        params.emplace_back(AZStd::string::format("-port=%d", portNumber));

        if (purpose == BuilderPurpose::Registration)
        {
            params.emplace_back("--register");
        }

        if (moduleFilePath && moduleFilePath[0])
        {
            params.emplace_back(AZStd::string::format(R"(-module="%s")", moduleFilePath));
        }

        if (!jobDescriptionFile.empty() && !jobResponseFile.empty())
        {
            params.emplace_back(AZStd::string::format(R"(-input="%s")", jobDescriptionFile.c_str()));
            params.emplace_back(AZStd::string::format(R"(-output="%s")", jobResponseFile.c_str()));
        }

        auto settingsRegistry = AZ::SettingsRegistry::Get();
        AZ::CommandLine commandLine;
        AZ::SettingsRegistryMergeUtils::GetCommandLineFromRegistry(*settingsRegistry, commandLine);
        AZStd::fixed_vector optionKeys{ "regset", "regremove" };
        for (auto&& optionKey : optionKeys)
        {
            size_t commandOptionCount = commandLine.GetNumSwitchValues(optionKey);
            for (size_t optionIndex = 0; optionIndex < commandOptionCount; ++optionIndex)
            {
                const AZStd::string& optionValue = commandLine.GetSwitchValue(optionKey, optionIndex);
                params.emplace_back(AZStd::string::format(R"(--%s="%s")", optionKey, optionValue.c_str()));
            }
        }

        return params;
    }

    AZStd::unique_ptr<AzFramework::ProcessWatcher> Builder::LaunchProcess(const char* fullExePath, const AZStd::vector<AZStd::string>& params) const
    {
        AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;
        processLaunchInfo.m_processExecutableString = fullExePath;

        AZStd::vector<AZStd::string> commandLineArray{ fullExePath };
        commandLineArray.insert(commandLineArray.end(), params.begin(), params.end());
        processLaunchInfo.m_commandlineParameters = AZStd::move(commandLineArray);
        processLaunchInfo.m_showWindow = false;
        processLaunchInfo.m_processPriority = AzFramework::ProcessPriority::PROCESSPRIORITY_IDLE;

        AZ_TracePrintf(AssetProcessor::DebugChannel, "Executing AssetBuilder with parameters: %s\n", processLaunchInfo.GetCommandLineParametersAsString().c_str());

        auto processWatcher = AZStd::unique_ptr<AzFramework::ProcessWatcher>(AzFramework::ProcessWatcher::LaunchProcess(processLaunchInfo, AzFramework::ProcessCommunicationType::COMMUNICATOR_TYPE_STDINOUT));

        AZ_Error(AssetProcessor::ConsoleChannel, processWatcher, "Failed to start %s", fullExePath);

        return processWatcher;
    }

    BuilderRunJobOutcome Builder::WaitForBuilderResponse(AssetBuilderSDK::JobCancelListener* jobCancelListener, AZ::u32 processTimeoutLimitInSeconds, AZStd::binary_semaphore* waitEvent) const
    {
        AZ::u32 exitCode = 0;
        bool finishedOK = false;
        QElapsedTimer ticker;

        ticker.start();

        while (!finishedOK)
        {
            finishedOK = waitEvent->try_acquire_for(AZStd::chrono::milliseconds(s_MaximumSleepTimeMS));

            PumpCommunicator();

            if (!IsValid() || ticker.elapsed() > processTimeoutLimitInSeconds * s_MillisecondsInASecond || (jobCancelListener && jobCancelListener->IsCancelled()))
            {
                break;
            }
        }

        PumpCommunicator();
        FlushCommunicator();


        if (finishedOK)
        {
            return BuilderRunJobOutcome::Ok;
        }
        else if (!IsConnected())
        {
            AZ_Error("Builder", false, "Lost connection to asset builder");
            return BuilderRunJobOutcome::LostConnection;
        }
        else if (!IsRunning(&exitCode))
        {
            // these are written to the debug channel because other messages are given for when asset builders die
            // that are more appropriate
            AZ_Error("Builder", false, "AssetBuilder terminated with exit code %d", exitCode);
            return BuilderRunJobOutcome::ProcessTerminated;
        }
        else if (jobCancelListener && jobCancelListener->IsCancelled())
        {
            AZ_Error("Builder", false, "Job request was canceled");
            TerminateProcess(AZ::u32(-1)); // Terminate the builder. Even if it isn't deadlocked, we can't put it back in the pool while it's busy.
            return BuilderRunJobOutcome::JobCancelled;
        }
        else
        {
            AZ_Error("Builder", false, "AssetBuilder failed to respond within %d seconds", processTimeoutLimitInSeconds);
            TerminateProcess(AZ::u32(-1)); // Terminate the builder. Even if it isn't deadlocked, we can't put it back in the pool while it's busy.
            return BuilderRunJobOutcome::ResponseFailure;
        }
    }

    //////////////////////////////////////////////////////////////////////////

    BuilderRef::BuilderRef(const AZStd::shared_ptr<Builder>& builder)
        : m_builder(builder)
    {
        if (m_builder)
        {
            m_builder->m_busy = true;
        }
    }

    BuilderRef::BuilderRef(BuilderRef&& rhs)
        : m_builder(AZStd::move(rhs.m_builder))
    {
    }

    BuilderRef& BuilderRef::operator=(BuilderRef&& rhs)
    {
        m_builder = AZStd::move(rhs.m_builder);
        return *this;
    }

    BuilderRef::~BuilderRef()
    {
        release();
    }

    const Builder* BuilderRef::operator->() const
    {
        return m_builder.get();
    }

    BuilderRef::operator bool() const
    {
        return m_builder != nullptr;
    }

    void BuilderRef::release()
    {
        if (m_builder)
        {
            AZ_Warning("BuilderRef", m_builder->m_busy, "Builder reference is valid but is already set to not busy");

            m_builder->m_busy = false;
            m_builder = nullptr;
        }
    }

    //////////////////////////////////////////////////////////////////////////

    BuilderManager::BuilderManager(ConnectionManager* connectionManager)
    {
        using namespace AZStd::placeholders;
        connectionManager->RegisterService(AssetBuilder::BuilderHelloRequest::MessageType(), AZStd::bind(&BuilderManager::IncomingBuilderPing, this, _1, _2, _3, _4, _5));

        // Setup a background thread to pump the idle builders so they don't get blocked trying to output to stdout/err
        AZStd::thread_desc desc;
        desc.m_name = "BuilderManager Idle Pump";
        m_pollingThread = AZStd::thread(desc, [this]()
            {
                while (!m_quitListener.WasQuitRequested())
                {
                    PumpIdleBuilders();
                    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(s_IdleBuilderPumpingDelayMS));
                }
            });

        m_quitListener.BusConnect();
        BusConnect();
    }

    BuilderManager::~BuilderManager()
    {
        PrintDebugOutput();

        BusDisconnect();
        m_quitListener.BusDisconnect();
        m_quitListener.ApplicationShutdownRequested();

        if (m_pollingThread.joinable())
        {
            m_pollingThread.join();
        }
    }

    void BuilderManager::ConnectionLost(AZ::u32 connId)
    {
        AZ_Assert(connId > 0, "ConnectionId was 0");
        AZStd::lock_guard<AZStd::mutex> lock(m_buildersMutex);

        for (auto itr = m_builders.begin(); itr != m_builders.end(); ++itr)
        {
            auto& builder = itr->second;

            if (builder->GetConnectionId() == connId)
            {
                AZ_TracePrintf("BuilderManager", "Lost connection to builder %s\n", builder->UuidString().c_str());
                builder->m_connectionId = 0;
                m_builders.erase(itr);
                break;
            }
        }
    }

    void BuilderManager::IncomingBuilderPing(AZ::u32 connId, AZ::u32 /*type*/, AZ::u32 serial, QByteArray payload, QString platform)
    {
        AssetBuilder::BuilderHelloRequest requestPing;
        AssetBuilder::BuilderHelloResponse responsePing;

        if (!AZ::Utils::LoadObjectFromBufferInPlace(payload.data(), payload.length(), requestPing))
        {
            AZ_Error("BuilderManager", false,
                "Failed to deserialize BuilderHelloRequest.\n"
                "Your builder(s) may need recompilation to function correctly as this kind of failure usually indicates that "
                "there is a disparity between the version of asset processor running and the version of builder dll files present in the "
                "'builders' subfolder.");
        }
        else
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_buildersMutex);

            AZStd::shared_ptr<Builder> builder;
            auto itr = m_builders.find(requestPing.m_uuid);

            if (itr != m_builders.end())
            {
                builder = itr->second;
            }
            else if (m_allowUnmanagedBuilderConnections)
            {
                AZ_TracePrintf("BuilderManager", "External builder connection accepted\n");
                builder = AddNewBuilder();
            }
            else
            {
                AZ_Warning("BuilderManager", false, "Received request ping from builder but could not match uuid %s", requestPing.m_uuid.ToString<AZStd::string>().c_str());
            }

            if (builder)
            {
                if (builder->IsConnected())
                {
                    AZ_Error("BuilderManager", false, "Builder %s is already connected and should not be sending another ping.  Something has gone wrong.  There may be multiple builders with the same UUID", builder->UuidString().c_str());
                }
                else
                {
                    AZ_TracePrintf("BuilderManager", "Builder %s connected, connId: %d\n", builder->UuidString().c_str(), connId);
                    builder->SetConnection(connId);
                    responsePing.m_accepted = true;
                    responsePing.m_uuid = builder->GetUuid();
                }
            }
        }

        AssetProcessor::ConnectionBus::Event(connId, &AssetProcessor::ConnectionBusTraits::SendResponse, serial, responsePing);
    }

    AZStd::shared_ptr<Builder> BuilderManager::AddNewBuilder()
    {
        AZ::Uuid builderUuid;
        // Make sure that we don't already have a builder with the same UUID.  If we do, try generating another one
        constexpr int MaxRetryCount = 10;
        int retriesRemaining = MaxRetryCount;

        do
        {
            builderUuid = AZ::Uuid::CreateRandom();
            --retriesRemaining;
        } while (m_builders.find(builderUuid) != m_builders.end() && retriesRemaining > 0);

        if(m_builders.find(builderUuid) != m_builders.end())
        {
            AZ_Error("BuilderManager", false, "Failed to generate a unique id for new builder after %d attempts.  All attempted random ids were already taken.", MaxRetryCount);
            return {};
        }

        auto builder = AZStd::make_shared<Builder>(m_quitListener, builderUuid);

        m_builders.insert({ builder->GetUuid(), builder });

        return builder;
    }

    void BuilderManager::AddAssetToBuilderProcessedList(const AZ::Uuid& builderId, const AZStd::string& sourceAsset)
    {
        m_builderDebugOutput[builderId].m_assetsProcessed.push_back(sourceAsset);
    }

    BuilderRef BuilderManager::GetBuilder(BuilderPurpose purpose)
    {
        AZStd::shared_ptr<Builder> newBuilder;
        BuilderRef builderRef;

        {
            AZStd::unique_lock<AZStd::mutex> lock(m_buildersMutex);

            if (purpose != BuilderPurpose::Registration)
            {
                for (auto itr = m_builders.begin(); itr != m_builders.end();)
                {
                    if (itr == m_builders.begin() && purpose != BuilderPurpose::CreateJobs)
                    {
                        // The first builder is always reserved for create jobs.
                        // Since there is only ever 1 CreateJobs process happening at once, this means CreateJobs will never have to wait to
                        // start up a new builder. This is important since CreateJobs is meant to be quick.
                        ++itr;
                        continue;
                    }

                    auto& builder = itr->second;

                    if (!builder->m_busy)
                    {
                        builder->PumpCommunicator();

                        if (builder->IsValid())
                        {
                            return BuilderRef(builder);
                        }
                        else
                        {
                            itr = m_builders.erase(itr);
                        }
                    }
                    else
                    {
                        ++itr;
                    }
                }
            }

            AZ_TracePrintf("BuilderManager", "Starting new builder for job request\n");

            // None found, start up a new one
            newBuilder = AddNewBuilder();

            // Grab a reference so no one else can take it while we're outside the lock
            builderRef = BuilderRef(newBuilder);
        }

        if (!newBuilder->Start(purpose))
        {
            AZ_Error("BuilderManager", false, "Builder failed to start");

            AZStd::unique_lock<AZStd::mutex> lock(m_buildersMutex);

            builderRef = {}; // Release after the lock to make sure no one grabs it before we can delete it

            m_builders.erase(newBuilder->GetUuid());
        }
        else
        {
            AZ_TracePrintf("BuilderManager", "Builder started successfully\n");
        }

        return builderRef;
    }

    void BuilderManager::PumpIdleBuilders()
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_buildersMutex);

        for (auto pair : m_builders)
        {
            auto builder = pair.second;

            if (!builder->m_busy)
            {
                builder->PumpCommunicator();
            }
        }
    }

    void BuilderManager::PrintDebugOutput()
    {
        // If debug output was tracked, print it on shutdown.
        // This prints each asset that was processed by each builder, in the order they were processed.
        // This is useful for tracing issues like memory leaks across assets processed by the same builder.
        for (auto builderInfo : m_builderDebugOutput)
        {
            AZ_TracePrintf("BuilderManager", "Builder %.*s processed these assets:\n",
                AZ_STRING_ARG(builderInfo.first.ToString<AZStd::string>()));
            for (auto asset : builderInfo.second.m_assetsProcessed)
            {
                AZ_TracePrintf(
                    "BuilderManager", "Builder with ID %.*s processed %.*s\n",
                    AZ_STRING_ARG(builderInfo.first.ToFixedString()),
                    AZ_STRING_ARG(asset));
            }
        }
    }

} // namespace AssetProcessor
