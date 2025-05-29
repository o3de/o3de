/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "BatchApplicationManager.h"

#include <native/resourcecompiler/rccontroller.h>
#include <native/AssetManager/assetScanner.h>
#include <native/utilities/BatchApplicationServer.h>

#include <AzToolsFramework/UI/Logging/LogLine.h>

#include <QCoreApplication>

// in batch mode, we are going to show the log files of up to N failures.
// in order to not spam the logs, we limit this - its possible that something fundamental is broken and EVERY asset is failing
// and we don't want to thus write gigabytes of logs out.
const int s_MaximumFailuresToReport = 10;

#if defined(AZ_PLATFORM_WINDOWS)
namespace BatchApplicationManagerPrivate
{
    BatchApplicationManager* g_appManager = nullptr;
    BOOL WINAPI CtrlHandlerRoutine(DWORD dwCtrlType)
    {
        (void)dwCtrlType;
        AZ_Printf("AssetProcessor", "Asset Processor Batch Processing Interrupted. Quitting.\n");
        QMetaObject::invokeMethod(g_appManager, "QuitRequested", Qt::QueuedConnection);
        return TRUE;
    }
}
#endif  //#if defined(AZ_PLATFORM_WINDOWS)

BatchApplicationManager::BatchApplicationManager(int* argc, char*** argv, QObject* parent)
    : BatchApplicationManager(argc, argv, parent, {})
{
}

BatchApplicationManager::BatchApplicationManager(int* argc, char*** argv, AZ::ComponentApplicationSettings componentAppSettings)
    : BatchApplicationManager(argc, argv, nullptr, AZStd::move(componentAppSettings))
{
}

BatchApplicationManager::BatchApplicationManager(int* argc, char*** argv, QObject* parent, AZ::ComponentApplicationSettings componentAppSettings)
    : ApplicationManagerBase(argc, argv, parent, AZStd::move(componentAppSettings))
{
    AssetProcessor::MessageInfoBus::Handler::BusConnect();
}

BatchApplicationManager::~BatchApplicationManager()
{
    AssetProcessor::MessageInfoBus::Handler::BusDisconnect();
}

void BatchApplicationManager::Destroy()
{
#if defined(AZ_PLATFORM_WINDOWS)
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)BatchApplicationManagerPrivate::CtrlHandlerRoutine, FALSE);
    BatchApplicationManagerPrivate::g_appManager = nullptr;
#endif //#if defined(AZ_PLATFORM_WINDOWS)

    ApplicationManagerBase::Destroy();
}

bool BatchApplicationManager::Activate()
{
#if defined(AZ_PLATFORM_WINDOWS)
    BatchApplicationManagerPrivate::g_appManager = this;
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)BatchApplicationManagerPrivate::CtrlHandlerRoutine, TRUE);
#endif //defined(AZ_PLATFORM_WINDOWS)

    return ApplicationManagerBase::Activate();
}

void BatchApplicationManager::OnErrorMessage([[maybe_unused]] const char* error)
{
    AZ_Error("AssetProcessor", false, "%s", error);
}

void BatchApplicationManager::Reflect()
{
    ApplicationManagerBase::Reflect();

    AZ::SerializeContext* context = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
    AZ_Assert(context, "No serialize context");

    AssetProcessor::PlatformConfiguration::Reflect(context);
}

const char* BatchApplicationManager::GetLogBaseName()
{
    return "AP_Batch";
}

ApplicationManager::RegistryCheckInstructions BatchApplicationManager::PopupRegistryProblemsMessage(QString warningText)
{
    return RegistryCheckInstructions::Exit;
}

void BatchApplicationManager::InitSourceControl()
{
    bool enableSourceControl = false;

    const AzFramework::CommandLine* commandLine = nullptr;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(commandLine, &AzFramework::ApplicationRequests::GetCommandLine);

    if (commandLine->HasSwitch("enablescm"))
    {
        enableSourceControl = true;
    }

    if (enableSourceControl)
    {
        AzToolsFramework::SourceControlConnectionRequestBus::Broadcast(&AzToolsFramework::SourceControlConnectionRequestBus::Events::EnableSourceControl, true);
    }
    else
    {
        Q_EMIT SourceControlReady();
    }
}

void BatchApplicationManager::InitUuidManager()
{
    m_uuidManager = AZStd::make_unique<AssetProcessor::UuidManager>();
    m_assetProcessorManager->SetMetaCreationDelay(0);

    // Note that batch does not set any enabled types and has 0 delay because batch mode is not expected to generate metadata files or handle moving/renaming while running.
}

void BatchApplicationManager::MakeActivationConnections()
{
    QObject::connect(m_rcController, &AssetProcessor::RCController::FileCompiled,
        m_assetProcessorManager, [this](AssetProcessor::JobEntry entry, AssetBuilderSDK::ProcessJobResponse /*response*/)
        {
            m_processedAssetCount++;

            // If a file fails and later succeeds, don't count it as a failure.
            // This avoids marking the entire run as a failure (returning non-zero) when everything compiled successfully *eventually*
            m_failedAssets.erase(entry.GetAbsoluteSourcePath().toUtf8().constData());

            AssetProcessor::JobDiagnosticInfo info{};
            AssetProcessor::JobDiagnosticRequestBus::BroadcastResult(info, &AssetProcessor::JobDiagnosticRequestBus::Events::GetDiagnosticInfo, entry.m_jobRunKey);

            m_warningCount += info.m_warningCount;
            m_errorCount += info.m_errorCount;
        });

    QObject::connect(m_rcController, &AssetProcessor::RCController::FileFailed,
        m_assetProcessorManager, [this](AssetProcessor::JobEntry entry)
        {
            m_failedAssets.emplace(entry.GetAbsoluteSourcePath().toUtf8().constData());

            AssetProcessor::JobDiagnosticInfo info{};
            AssetProcessor::JobDiagnosticRequestBus::BroadcastResult(info, &AssetProcessor::JobDiagnosticRequestBus::Events::GetDiagnosticInfo, entry.m_jobRunKey);

            m_warningCount += info.m_warningCount;
            m_errorCount += info.m_errorCount;

            using AssetJobLogRequest = AzToolsFramework::AssetSystem::AssetJobLogRequest;
            using AssetJobLogResponse = AzToolsFramework::AssetSystem::AssetJobLogResponse;
            if (m_failedAssets.size() < s_MaximumFailuresToReport) // if we're in the situation where many assets are failing we need to stop spamming after a few
            {
                AssetJobLogRequest request;
                AssetJobLogResponse response;
                request.m_jobRunKey = entry.m_jobRunKey;
                QMetaObject::invokeMethod(GetAssetProcessorManager(), "ProcessGetAssetJobLogRequest", Qt::DirectConnection, Q_ARG(const AssetJobLogRequest&, request), Q_ARG(AssetJobLogResponse&, response));
                if (response.m_isSuccess)
                {
                    // write the log to console!
                    AzToolsFramework::Logging::LogLine::ParseLog(response.m_jobLog.c_str(), response.m_jobLog.size(),
                        [](AzToolsFramework::Logging::LogLine& target)
                        {
                            // We're going to output *everything* because when a non-obvious error occurs, even mundane info output can be helpful for diagnosing the cause of the error
                            AZStd::string logString = target.ToString();
                            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "JOB LOG: %s", logString.c_str());
                        });
                }
            }
            else if (m_failedAssets.size() == s_MaximumFailuresToReport)
            {
                // notify the user that we're done here, and will not be notifying any more.
                AZ_Printf(AssetProcessor::ConsoleChannel, "%s\n", QCoreApplication::translate("Batch Mode", "Too Many Compile Errors - not printing out full logs for remaining errors").toUtf8().constData());
            }
        });

    m_connectionsToRemoveOnShutdown << QObject::connect(m_assetScanner, &AssetProcessor::AssetScanner::AssetScanningStatusChanged, this,
        [this](AssetProcessor::AssetScanningStatus status)
        {
            if ((status == AssetProcessor::AssetScanningStatus::Completed) || (status == AssetProcessor::AssetScanningStatus::Stopped))
            {
                AZ_Printf(AssetProcessor::ConsoleChannel, QCoreApplication::translate("Batch Mode", "Analyzing scanned files for changes...\n").toUtf8().constData());
                CheckForIdle();
            }
        });
}

void BatchApplicationManager::TryScanProductDependencies()
{
    if (!m_dependencyScanPattern.isEmpty())
    {
        m_assetProcessorManager->ScanForMissingProductDependencies(m_dependencyScanPattern, m_fileDependencyScanPattern, m_dependencyAddtionalScanFolders, m_dependencyScanMaxIteration);
        m_dependencyScanPattern.clear();
    }
}

void BatchApplicationManager::TryHandleFileRelocation()
{
    HandleFileRelocation();
}

bool BatchApplicationManager::InitApplicationServer()
{
    m_applicationServer = new BatchApplicationServer();
    return true;
}

