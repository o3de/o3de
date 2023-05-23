/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QCoreApplication>
#include <QElapsedTimer>
#include <AzCore/Settings/CommandLine.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/Utils/Utils.h>
#include <utilities/Builder.h>
#include <utilities/AssetBuilderInfo.h>

namespace AssetProcessor
{
    //! Amount of time in milliseconds to wait between checking the status of the AssetBuilder process and pumping the stdout/err pipes
    //! Should be kept fairly low to avoid the process stalling due to a full pipe but not too low to avoid wasting CPU time
    constexpr int MaximumSleepTimeMs = 10;
    constexpr int MillisecondsInASecond = 1000;
    constexpr const char* BuildersFolderName = "Builders";

    bool Builder::IsConnected() const
    {
        return m_connectionId > 0;
    }

    AZ::Outcome<void, AZStd::string> Builder::WaitForConnection()
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
                result = m_connectionEvent.try_acquire_for(AZStd::chrono::milliseconds(MaximumSleepTimeMs));

                PumpCommunicator();

                if (ticker.elapsed() > m_startupWaitTimeS * MillisecondsInASecond || m_quitListener.WasQuitRequested() || !IsRunning())
                {
                    break;
                }
            }

            PumpCommunicator();
            FlushCommunicator();

            if (result)
            {
                return AZ::Success();
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

            return AZ::Failure(AZStd::string::format("Connection failed to builder %.*s", AZ_STRING_ARG(UuidString())));
        }

        return AZ::Success();
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
            m_tracePrinter->Flush();
        }
    }

    void Builder::TerminateProcess(AZ::u32 exitCode) const
    {
        if (m_processWatcher)
        {
            m_processWatcher->TerminateProcess(exitCode);
        }
    }

    AZ::Outcome<void, AZStd::string> Builder::Start(BuilderPurpose purpose)
    {
        // Get the current BinXXX folder based on the current running AP
        QString applicationDir = QCoreApplication::instance()->applicationDirPath();

        // Construct the Builders subfolder path
        AZStd::string buildersFolder;
        AzFramework::StringFunc::Path::Join(applicationDir.toUtf8().constData(), BuildersFolderName, buildersFolder);

        // Construct the full exe for the builder.exe
        const AZStd::string fullExePathString =
            QDir(applicationDir).absoluteFilePath(AssetProcessor::s_assetBuilderRelativePath).toUtf8().constData();

        if (m_quitListener.WasQuitRequested())
        {
            return AZ::Failure(AZStd::string("Cannot start builder, quit was requested"));
        }

        const AZStd::vector<AZStd::string> params = BuildParams("resident", buildersFolder.c_str(), UuidString(), "", "", purpose);

        m_processWatcher = LaunchProcess(fullExePathString.c_str(), params);

        if (!m_processWatcher)
        {
            return AZ::Failure(AZStd::string::format("Failed to start process watcher for Builder %.*s.", AZ_STRING_ARG(UuidString())));
        }

        // Currently, this uses polling for managing the trace printing output because the job log redirections rely on thread local
        // storage to route different jobs to different logs. If the trace printing spins up a new thread for printing, it won't
        // redirect to the correct job logs.
        m_tracePrinter = AZStd::make_unique<ProcessCommunicatorTracePrinter>(
            m_processWatcher->GetCommunicator(), "AssetBuilder", ProcessCommunicatorTracePrinter::TraceProcessing::Poll);

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

    AZStd::vector<AZStd::string> Builder::BuildParams(
        const char* task,
        const char* moduleFilePath,
        const AZStd::string& builderGuid,
        const AZStd::string& jobDescriptionFile,
        const AZStd::string& jobResponseFile,
        BuilderPurpose purpose) const
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

    AZStd::unique_ptr<AzFramework::ProcessWatcher> Builder::LaunchProcess(
        const char* fullExePath, const AZStd::vector<AZStd::string>& params) const
    {
        AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;
        processLaunchInfo.m_processExecutableString = fullExePath;

        AZStd::vector<AZStd::string> commandLineArray{ fullExePath };
        commandLineArray.insert(commandLineArray.end(), params.begin(), params.end());
        processLaunchInfo.m_commandlineParameters = AZStd::move(commandLineArray);
        processLaunchInfo.m_showWindow = false;
        processLaunchInfo.m_processPriority = AzFramework::ProcessPriority::PROCESSPRIORITY_IDLE;

        AZ_TracePrintf(
            AssetProcessor::DebugChannel,
            "Executing AssetBuilder with parameters: %s\n",
            processLaunchInfo.GetCommandLineParametersAsString().c_str());

        auto processWatcher = AZStd::unique_ptr<AzFramework::ProcessWatcher>(AzFramework::ProcessWatcher::LaunchProcess(
            processLaunchInfo, AzFramework::ProcessCommunicationType::COMMUNICATOR_TYPE_STDINOUT));

        AZ_Error(AssetProcessor::ConsoleChannel, processWatcher, "Failed to start %s", fullExePath);

        return processWatcher;
    }

    BuilderRunJobOutcome Builder::WaitForBuilderResponse(
        AssetBuilderSDK::JobCancelListener* jobCancelListener,
        AZ::u32 processTimeoutLimitInSeconds,
        AZStd::binary_semaphore* waitEvent) const
    {
        AZ::u32 exitCode = 0;
        bool finishedOK = false;
        QElapsedTimer ticker;

        AZ_Assert(waitEvent, "WaitEvent must not be null");

        ticker.start();

        while (!finishedOK)
        {
            finishedOK = waitEvent->try_acquire_for(AZStd::chrono::milliseconds(MaximumSleepTimeMs));

            PumpCommunicator();

            if (!IsValid() || ticker.elapsed() > processTimeoutLimitInSeconds * MillisecondsInASecond ||
                (jobCancelListener && jobCancelListener->IsCancelled()))
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
            AZ_Error("Builder", false, "Lost connection to asset builder %s", UuidString().c_str());
            return BuilderRunJobOutcome::LostConnection;
        }
        else if (!IsRunning(&exitCode))
        {
            // these are written to the debug channel because other messages are given for when asset builders die
            // that are more appropriate
            AZ_Error("Builder", false, "AssetBuilder %s terminated with exit code %d", UuidString().c_str(), exitCode);
            return BuilderRunJobOutcome::ProcessTerminated;
        }
        else if (jobCancelListener && jobCancelListener->IsCancelled())
        {
            AZ_Error("Builder", false, "Job request was canceled");
            TerminateProcess(
                AZ::u32(-1)); // Terminate the builder. Even if it isn't deadlocked, we can't put it back in the pool while it's busy.
            return BuilderRunJobOutcome::JobCancelled;
        }
        else
        {
            AZ_Error("Builder", false, "AssetBuilder %s failed to respond within %d seconds", UuidString().c_str(), processTimeoutLimitInSeconds);
            TerminateProcess(
                AZ::u32(-1)); // Terminate the builder. Even if it isn't deadlocked, we can't put it back in the pool while it's busy.
            return BuilderRunJobOutcome::ResponseFailure;
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////

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
}
