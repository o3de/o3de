/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "ApplicationManagerBase.h"

#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/sort.h>

#include <native/utilities/BuilderConfigurationManager.h>
#include <native/resourcecompiler/rccontroller.h>
#include <native/AssetManager/assetScanner.h>
#include <native/AssetManager/FileStateCache.h>
#include <native/AssetManager/ControlRequestHandler.h>
#include <native/connection/connectionManager.h>
#include <native/utilities/ByteArrayStream.h>
#include <native/AssetManager/AssetRequestHandler.h>
#include <native/FileProcessor/FileProcessor.h>
#include <native/utilities/ApplicationServer.h>
#include <native/utilities/AssetServerHandler.h>
#include <native/InternalBuilders/SettingsRegistryBuilder.h>
#include <AzToolsFramework/Application/Ticker.h>
#include <AzToolsFramework/ToolsFileUtils/ToolsFileUtils.h>
#include <AssetBuilder/AssetBuilderStatic.h>

#include <iostream>

#include <QCoreApplication>

#include <QElapsedTimer>

//! CreateJobs will wait up to 2 minutes before timing out
//! This shouldn't need to be so high but very large slices can take a while to process currently
//! This should be reduced down to something more reasonable after slice jobs are sped up
static const int s_MaximumCreateJobsTimeSeconds = 60 * 2;

//! ProcessJobs will wait up to 1 hour before timing out
static const int s_MaximumProcessJobsTimeSeconds = 60 * 60;

//! Reserve extra disk space when doing disk space checks to leave a little room for logging, database operations, etc
static const qint64 s_ReservedDiskSpaceInBytes = 256 * 1024;

//! Maximum number of temp folders allowed
static const int s_MaximumTempFolders = 10000;

ApplicationManagerBase::ApplicationManagerBase(int* argc, char*** argv, QObject* parent)
    : ApplicationManager(argc, argv, parent)
{
    qRegisterMetaType<AZ::u32>("AZ::u32");
    qRegisterMetaType<AZ::Uuid>("AZ::Uuid");
}


ApplicationManagerBase::~ApplicationManagerBase()
{
    AzToolsFramework::SourceControlNotificationBus::Handler::BusDisconnect();
    AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
    AssetProcessor::AssetBuilderRegistrationBus::Handler::BusDisconnect();
    AssetBuilderSDK::AssetBuilderBus::Handler::BusDisconnect();
    AssetProcessor::AssetBuilderInfoBus::Handler::BusDisconnect();

    if (m_settingsRegistryBuilder)
    {
        m_settingsRegistryBuilder->Uninitialize();
    }

    if (m_internalBuilder)
    {
        m_internalBuilder->UnInitialize();
    }

    for (AssetProcessor::ExternalModuleAssetBuilderInfo* externalAssetBuilderInfo : this->m_externalAssetBuilders)
    {
        externalAssetBuilderInfo->UnInitialize();
        delete externalAssetBuilderInfo;
    }

    Destroy();
}

AssetProcessor::RCController* ApplicationManagerBase::GetRCController() const
{
    return m_rcController;
}

int ApplicationManagerBase::ProcessedAssetCount() const
{
    return m_processedAssetCount;
}
int ApplicationManagerBase::FailedAssetsCount() const
{
    return static_cast<int>(m_failedAssets.size());
}

void ApplicationManagerBase::ResetProcessedAssetCount()
{
    m_processedAssetCount = 0;
}

void ApplicationManagerBase::ResetFailedAssetCount()
{
    m_failedAssets = AZStd::set<AZStd::string>{};
}


AssetProcessor::AssetScanner* ApplicationManagerBase::GetAssetScanner() const
{
    return m_assetScanner;
}

AssetProcessor::AssetProcessorManager* ApplicationManagerBase::GetAssetProcessorManager() const
{
    return m_assetProcessorManager;
}

AssetProcessor::PlatformConfiguration* ApplicationManagerBase::GetPlatformConfiguration() const
{
    return m_platformConfiguration;
}

ConnectionManager* ApplicationManagerBase::GetConnectionManager() const
{
    return m_connectionManager;
}

ApplicationServer* ApplicationManagerBase::GetApplicationServer() const
{
    return m_applicationServer;
}

void ApplicationManagerBase::InitAssetProcessorManager(AZStd::vector<ApplicationManagerBase::APCommandLineSwitch>& commandLineInfo)
{
    AssetProcessor::ThreadController<AssetProcessor::AssetProcessorManager>* assetProcessorHelper = new AssetProcessor::ThreadController<AssetProcessor::AssetProcessorManager>();

    addRunningThread(assetProcessorHelper);
    m_assetProcessorManager = assetProcessorHelper->initialize([this, &assetProcessorHelper]()
    {
        return new AssetProcessor::AssetProcessorManager(m_platformConfiguration, assetProcessorHelper);
    });
    QObject::connect(this, &ApplicationManagerBase::OnBuildersRegistered, m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::OnBuildersRegistered, Qt::QueuedConnection);

    connect(this, &ApplicationManagerBase::SourceControlReady, [this]()
    {
        m_sourceControlReady = true;
    });

    const AzFramework::CommandLine* commandLine = nullptr;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(commandLine, &AzFramework::ApplicationRequests::GetCommandLine);

    const APCommandLineSwitch Command_waitOnLaunch(commandLineInfo, "waitOnLaunch", "Briefly pauses Asset Processor during initializiation. Useful if you want to attach a debugger.");
    const APCommandLineSwitch Command_zeroAnalysisMode(commandLineInfo, "zeroAnalysisMode", "Enables using file modification time when examining source assets for processing.");
    const APCommandLineSwitch Command_enableQueryLogging(commandLineInfo, "enableQueryLogging", "Enables logging database queries.");
    const APCommandLineSwitch Command_dependencyScanPattern(commandLineInfo, "dependencyScanPattern", "Scans assets that match the given pattern for missing product dependencies.");
    const APCommandLineSwitch Command_dsp(commandLineInfo, "dsp", Command_dependencyScanPattern.m_helpText);
    const APCommandLineSwitch Command_fileDependencyScanPattern(commandLineInfo, "fileDependencyScanPattern", "Used with dependencyScanPattern to farther filter the scan.");
    const APCommandLineSwitch Command_fdsp(commandLineInfo, "fdsp", Command_fileDependencyScanPattern.m_helpText);
    const APCommandLineSwitch Command_additionalScanFolders(commandLineInfo, "additionalScanFolders", "Used with dependencyScanPattern to farther filter the scan.");
    const APCommandLineSwitch Command_dependencyScanMaxIteration(commandLineInfo, "dependencyScanMaxIteration", "Used to limit the number of recursive searches per line when running dependencyScanPattern.");
    const APCommandLineSwitch Command_warningLevel(commandLineInfo, "warningLevel", "Configure the error and warning reporting level for AssetProcessor. Pass in 1 for fatal errors, 2 for fatal errors and warnings.");
    const APCommandLineSwitch Command_acceptInput(commandLineInfo, "acceptInput", "Enable external control messaging via the ControlRequestHandler, used with automated tests.");
    const APCommandLineSwitch Command_debugOutput(commandLineInfo, "debugOutput", "When enabled, builders that support it will output debug information as product assets. Used primarily with scene files.");
    const APCommandLineSwitch Command_truncatefingerprint(commandLineInfo, "truncatefingerprint", "Truncates the fingerprint used for processed assets. Useful if you plan to compress product assets to share on another machine because some compression formats like zip will truncate file mod timestamps.");
    const APCommandLineSwitch Command_reprocessFileList(commandLineInfo, "reprocessFileList", "Reprocesses files in the passed in newline separated text file.");

    if (commandLine->HasSwitch(Command_waitOnLaunch.m_switch))
    {
        // Useful for attaching the debugger, this forces a short pause.
        AZStd::this_thread::sleep_for(AZStd::chrono::seconds(20));
    }

    if (commandLine->HasSwitch(Command_zeroAnalysisMode.m_switch))
    {
        m_assetProcessorManager->SetEnableModtimeSkippingFeature(true);
    }

    if (commandLine->HasSwitch(Command_enableQueryLogging.m_switch))
    {
        m_assetProcessorManager->SetQueryLogging(true);
    }

    if (commandLine->HasSwitch(Command_dependencyScanPattern.m_switch))
    {
        m_dependencyScanPattern = commandLine->GetSwitchValue(Command_dependencyScanPattern.m_switch, 0).c_str();
    }
    else if (commandLine->HasSwitch(Command_dsp.m_switch))
    {
        m_dependencyScanPattern = commandLine->GetSwitchValue(Command_dsp.m_switch, 0).c_str();
    }

    if (commandLine->HasSwitch(Command_reprocessFileList.m_switch))
    {
        m_reprocessFileList = commandLine->GetSwitchValue(Command_reprocessFileList.m_switch, 0).c_str();
    }

    m_fileDependencyScanPattern = "*";

    if (commandLine->HasSwitch(Command_fileDependencyScanPattern.m_switch))
    {
        m_fileDependencyScanPattern = commandLine->GetSwitchValue(Command_fileDependencyScanPattern.m_switch, 0).c_str();
    }
    else if (commandLine->HasSwitch(Command_fdsp.m_switch))
    {
        m_fileDependencyScanPattern = commandLine->GetSwitchValue(Command_fdsp.m_switch, 0).c_str();
    }

    if (commandLine->HasSwitch(Command_additionalScanFolders.m_switch))
    {
        for (size_t idx = 0; idx < commandLine->GetNumSwitchValues(Command_additionalScanFolders.m_switch); idx++)
        {
            AZStd::string value = commandLine->GetSwitchValue(Command_additionalScanFolders.m_switch, idx);
            m_dependencyAddtionalScanFolders.emplace_back(AZStd::move(value));
        }
    }

    if (commandLine->HasSwitch(Command_dependencyScanMaxIteration.m_switch))
    {
        AZStd::string maxIterationAsString = commandLine->GetSwitchValue(Command_dependencyScanMaxIteration.m_switch, 0);
        m_dependencyScanMaxIteration = AZStd::stoi(maxIterationAsString);
    }

    if (commandLine->HasSwitch(Command_warningLevel.m_switch))
    {
        using namespace AssetProcessor;
        const AZStd::string& levelString = commandLine->GetSwitchValue(Command_warningLevel.m_switch, 0);
        WarningLevel warningLevel = WarningLevel::Default;

        switch(AZStd::stoi(levelString))
        {
            case 1:
                warningLevel = WarningLevel::FatalErrors;
                break;
            case 2:
                warningLevel = WarningLevel::FatalErrorsAndWarnings;
                break;
        }
        AssetProcessor::JobDiagnosticRequestBus::Broadcast(&AssetProcessor::JobDiagnosticRequestBus::Events::SetWarningLevel, warningLevel);
    }
    if (commandLine->HasSwitch(Command_acceptInput.m_switch))
    {
        InitControlRequestHandler();
    }

    if (commandLine->HasSwitch(Command_debugOutput.m_switch))
    {
        m_assetProcessorManager->SetBuilderDebugFlag(true);
    }

    if (commandLine->HasSwitch(Command_truncatefingerprint.m_switch))
    {
        // Zip archive format uses 2 second precision truncated
        const int ArchivePrecision = 2000;
        int precision = ArchivePrecision;

        if (commandLine->GetNumSwitchValues(Command_truncatefingerprint.m_switch) > 0)
        {
            precision = AZStd::stoi(commandLine->GetSwitchValue(Command_truncatefingerprint.m_switch, 0));

            if(precision < 1)
            {
                precision = 1;
            }
        }

        AssetUtilities::SetTruncateFingerprintTimestamp(precision);
    }
}

void ApplicationManagerBase::HandleCommandLineHelp(AZStd::vector<ApplicationManagerBase::APCommandLineSwitch>& commandLineInfo)
{
    const AzFramework::CommandLine* commandLine = nullptr;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(commandLine, &AzFramework::ApplicationRequests::GetCommandLine);
    if (!commandLine)
    {
        AZ_TracePrintf(
            "AssetProcessor",
            "Asset Processor Command Line information not available, help cannot be printed. This is an application initialization problem "
            "and should be resolved in code.\n");
        return;
    }
    const APCommandLineSwitch Command_help(commandLineInfo, "help", "Displays this message.");
    const APCommandLineSwitch Command_h(commandLineInfo, "h", Command_help.m_helpText);

    // The regset command line flag is checked elsewhere, but handled here to make the help text complete.
    const APCommandLineSwitch Command_regset(commandLineInfo, "regset", "Set the given registry key to the given value.");

    if (commandLine->HasSwitch(Command_help.m_switch) || commandLine->HasSwitch(Command_h.m_switch))
    {
        // Other O3DE tools have a more full featured system for registering command flags
        // that includes help output, but right now the AssetProcessor just checks strings
        // via HasSwitch. This means this help output has to be updated manually.
        AZ_TracePrintf("AssetProcessor", "Asset Processor Command Line Flags:\n");
        for ([[maybe_unused]] const auto& command : commandLineInfo)
        {
            AZ_TracePrintf("AssetProcessor", "\t%s : %s\n", command.m_switch, command.m_helpText);
        }
    }
}

void ApplicationManagerBase::Rescan()
{
    m_assetProcessorManager->SetEnableModtimeSkippingFeature(false);
    GetAssetScanner()->StartScan();
}

void ApplicationManagerBase::InitAssetCatalog()
{
    using namespace AssetProcessor;
    ThreadController<AssetCatalog>* assetCatalogHelper = new ThreadController<AssetCatalog>();

    addRunningThread(assetCatalogHelper);
    m_assetCatalog = assetCatalogHelper->initialize([this, &assetCatalogHelper]()
            {
                AssetProcessor::AssetCatalog* catalog = new AssetCatalog(assetCatalogHelper, m_platformConfiguration);

                // Using a direct connection so we know the catalog has been updated before continuing on with code might depend on the asset being in the catalog
                connect(m_assetProcessorManager, &AssetProcessorManager::AssetMessage, catalog, &AssetCatalog::OnAssetMessage, Qt::DirectConnection);
                connect(m_assetProcessorManager, &AssetProcessorManager::SourceQueued, catalog, &AssetCatalog::OnSourceQueued);
                connect(m_assetProcessorManager, &AssetProcessorManager::SourceFinished, catalog, &AssetCatalog::OnSourceFinished);
                connect(m_assetProcessorManager, &AssetProcessorManager::PathDependencyResolved, catalog, &AssetCatalog::OnDependencyResolved);

                return catalog;
            });

    // schedule the asset catalog to build its registry in its own thread:
    QMetaObject::invokeMethod(m_assetCatalog, "BuildRegistry", Qt::QueuedConnection);
}

void ApplicationManagerBase::InitRCController()
{
    m_rcController = new AssetProcessor::RCController(m_platformConfiguration->GetMinJobs(), m_platformConfiguration->GetMaxJobs());

    QObject::connect(m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::AssetToProcess, m_rcController, &AssetProcessor::RCController::JobSubmitted);
    QObject::connect(m_rcController, &AssetProcessor::RCController::FileCompiled, m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::AssetProcessed, Qt::UniqueConnection);
    QObject::connect(m_rcController, &AssetProcessor::RCController::FileFailed, m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::AssetFailed);
    QObject::connect(m_rcController, &AssetProcessor::RCController::FileCancelled, m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::AssetCancelled);
    QObject::connect(m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::EscalateJobs, m_rcController, &AssetProcessor::RCController::EscalateJobs);
    QObject::connect(m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::SourceDeleted, m_rcController, &AssetProcessor::RCController::RemoveJobsBySource);
    QObject::connect(m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::JobComplete, m_rcController, &AssetProcessor::RCController::OnJobComplete);
    QObject::connect(m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::AddedToCatalog, m_rcController, &AssetProcessor::RCController::OnAddedToCatalog);
}

void ApplicationManagerBase::DestroyRCController()
{
    if (m_rcController)
    {
        delete m_rcController;
        m_rcController = nullptr;
    }
}

void ApplicationManagerBase::InitAssetScanner()
{
    using namespace AssetProcessor;
    m_assetScanner = new AssetScanner(m_platformConfiguration);

    // asset processor manager
    QObject::connect(m_assetScanner, &AssetScanner::AssetScanningStatusChanged, m_assetProcessorManager, &AssetProcessorManager::OnAssetScannerStatusChange);
    QObject::connect(m_assetScanner, &AssetScanner::FilesFound,                 m_assetProcessorManager, &AssetProcessorManager::AssessFilesFromScanner);
    QObject::connect(m_assetScanner, &AssetScanner::FoldersFound,                 m_assetProcessorManager, &AssetProcessorManager::RecordFoldersFromScanner);

    QObject::connect(m_assetScanner, &AssetScanner::FilesFound, [this](QSet<AssetFileInfo> files) { m_fileStateCache->AddInfoSet(files); });
    QObject::connect(m_assetScanner, &AssetScanner::FoldersFound, [this](QSet<AssetFileInfo> files) { m_fileStateCache->AddInfoSet(files); });
    QObject::connect(m_assetScanner, &AssetScanner::ExcludedFound, [this](QSet<AssetFileInfo> files) { m_fileStateCache->AddInfoSet(files); });

    // file table
    QObject::connect(m_assetScanner, &AssetScanner::AssetScanningStatusChanged, m_fileProcessor.get(), &FileProcessor::OnAssetScannerStatusChange);
    QObject::connect(m_assetScanner, &AssetScanner::FilesFound,                 m_fileProcessor.get(), &FileProcessor::AssessFilesFromScanner);
    QObject::connect(m_assetScanner, &AssetScanner::FoldersFound,               m_fileProcessor.get(), &FileProcessor::AssessFoldersFromScanner);

}

void ApplicationManagerBase::DestroyAssetScanner()
{
    if (m_assetScanner)
    {
        delete m_assetScanner;
        m_assetScanner = nullptr;
    }
}

bool ApplicationManagerBase::InitPlatformConfiguration()
{
    m_platformConfiguration = new AssetProcessor::PlatformConfiguration();
    QDir assetRoot;
    AssetUtilities::ComputeAssetRoot(assetRoot);
    return m_platformConfiguration->InitializeFromConfigFiles(GetSystemRoot().absolutePath(), assetRoot.absolutePath(), GetProjectPath());
}

bool ApplicationManagerBase::InitBuilderConfiguration()
{
    m_builderConfig = AZStd::make_unique<AssetProcessor::BuilderConfigurationManager>();
    QString configFile = QDir(GetProjectPath()).absoluteFilePath(AssetProcessor::BuilderConfigFile);

    if (!QFile::exists(configFile))
    {
        AZ_TracePrintf("AssetProcessor", "No builder configuration file found at %s - skipping\n", configFile.toUtf8().data());
        return false;
    }

    if (!m_builderConfig->LoadConfiguration(configFile.toStdString().c_str()))
    {
        AZ_Error("AssetProcessor", false, "Failed to Initialize from %s - check the log files in the logs/ subfolder for more information.", configFile.toUtf8().data());
        return false;
    }
    return true;
}

void ApplicationManagerBase::DestroyPlatformConfiguration()
{
    if (m_platformConfiguration)
    {
        delete m_platformConfiguration;
        m_platformConfiguration = nullptr;
    }
}

void ApplicationManagerBase::InitFileMonitor(AZStd::unique_ptr<FileWatcher> fileWatcher)
{
    m_fileWatcher = AZStd::move(fileWatcher);

    for (int folderIdx = 0; folderIdx < m_platformConfiguration->GetScanFolderCount(); ++folderIdx)
    {
        const AssetProcessor::ScanFolderInfo& info = m_platformConfiguration->GetScanFolderAt(folderIdx);
        m_fileWatcher->AddFolderWatch(info.ScanPath(), info.RecurseSubFolders());
    }

    QDir cacheRoot;
    if (AssetUtilities::ComputeProjectCacheRoot(cacheRoot))
    {
        m_fileWatcher->AddFolderWatch(cacheRoot.absolutePath(), true);
    }

    if (m_platformConfiguration->GetScanFolderCount() || !cacheRoot.path().isEmpty())
    {
        const auto cachePath = QDir::toNativeSeparators(cacheRoot.absolutePath());

        // For the handlers below, we need to make sure to use invokeMethod on any QObjects so Qt can queue
        // the callback to run on the QObject's thread if needed.  The APM methods for example are not thread-safe.

        const auto OnFileAdded = [this, cachePath](QString path)
        {
            const bool isCacheRoot = AssetUtilities::IsInCacheFolder(path.toUtf8().constData(), cachePath.toUtf8().constData());
            if (!isCacheRoot)
            {
                [[maybe_unused]] bool result = QMetaObject::invokeMethod(m_assetProcessorManager, [this, path]()
                {
                    m_assetProcessorManager->AssessAddedFile(path);
                }, Qt::QueuedConnection);
                AZ_Assert(result, "Failed to invoke m_assetProcessorManager::AssessAddedFile");

                result = QMetaObject::invokeMethod(m_fileProcessor.get(), [this, path]()
                {
                    m_fileProcessor->AssessAddedFile(path);
                }, Qt::QueuedConnection);
                AZ_Assert(result, "Failed to invoke m_fileProcessor::AssessAddedFile");

                auto cache = AZ::Interface<AssetProcessor::ExcludedFolderCacheInterface>::Get();

                if(cache)
                {
                    cache->FileAdded(path);
                }
                else
                {
                    AZ_Error("AssetProcessor", false, "ExcludedFolderCacheInterface not found");
                }
            }

            m_fileStateCache->AddFile(path);
        };

        const auto OnFileModified = [this, cachePath](QString path)
        {
            const bool isCacheRoot = AssetUtilities::IsInCacheFolder(path.toUtf8().constData(), cachePath.toUtf8().constData());
            if (!isCacheRoot)
            {
                m_fileStateCache->UpdateFile(path);
            }

            [[maybe_unused]] bool result = QMetaObject::invokeMethod(
                m_assetProcessorManager,
                [this, path]
                {
                    m_assetProcessorManager->AssessModifiedFile(path);
                },
                Qt::QueuedConnection);

            AZ_Assert(result, "Failed to invoke m_assetProcessorManager::AssessModifiedFile");
        };

        const auto OnFileRemoved = [this, cachePath](QString path)
        {
            [[maybe_unused]] bool result = false;
            const bool isCacheRoot = AssetUtilities::IsInCacheFolder(path.toUtf8().constData(), cachePath.toUtf8().constData());
            if (!isCacheRoot)
            {
                result = QMetaObject::invokeMethod(m_fileProcessor.get(), [this, path]()
                {
                    m_fileProcessor->AssessDeletedFile(path);
                }, Qt::QueuedConnection);
                AZ_Assert(result, "Failed to invoke m_fileProcessor::AssessDeletedFile");
            }

            result = QMetaObject::invokeMethod(m_assetProcessorManager, [this, path]()
            {
                m_assetProcessorManager->AssessDeletedFile(path);
            }, Qt::QueuedConnection);
            AZ_Assert(result, "Failed to invoke m_assetProcessorManager::AssessDeletedFile");

            m_fileStateCache->RemoveFile(path);
        };

        connect(m_fileWatcher.get(), &FileWatcher::fileAdded, OnFileAdded);
        connect(m_fileWatcher.get(), &FileWatcher::fileModified, OnFileModified);
        connect(m_fileWatcher.get(), &FileWatcher::fileRemoved, OnFileRemoved);
    }
}

void ApplicationManagerBase::DestroyFileMonitor()
{
    if(m_fileWatcher)
    {
        m_fileWatcher->ClearFolderWatches();
        m_fileWatcher = nullptr;
    }
}

void ApplicationManagerBase::DestroyApplicationServer()
{
    if (m_applicationServer)
    {
        delete m_applicationServer;
        m_applicationServer = nullptr;
    }
}

void ApplicationManagerBase::DestroyControlRequestHandler()
{
    if (m_controlRequestHandler)
    {
        delete m_controlRequestHandler;
        m_controlRequestHandler = nullptr;
    }
}

void ApplicationManagerBase::InitControlRequestHandler()
{
    m_controlRequestHandler = new ControlRequestHandler(this);
}

void ApplicationManagerBase::InitConnectionManager()
{
    using namespace AzFramework::AssetSystem;
    using namespace AzToolsFramework::AssetSystem;

    m_connectionManager = new ConnectionManager();

    QObject* connectionAndChangeMessagesThreadContext = this;

    // AssetProcessor Manager related stuff
    auto forwardMessageFunction = [](AzFramework::AssetSystem::AssetNotificationMessage message)
        {
            EBUS_EVENT(AssetProcessor::ConnectionBus, SendPerPlatform, 0, message, QString::fromUtf8(message.m_platform.c_str()));
        };

    [[maybe_unused]] bool result = QObject::connect(GetAssetCatalog(), &AssetProcessor::AssetCatalog::SendAssetMessage, connectionAndChangeMessagesThreadContext, forwardMessageFunction, Qt::QueuedConnection);
    AZ_Assert(result, "Failed to connect to AssetCatalog signal");

    result = QObject::connect(m_connectionManager, &ConnectionManager::ConnectionReady, GetAssetCatalog(), &AssetProcessor::AssetCatalog::OnConnect, Qt::QueuedConnection);
    AZ_Assert(result, "Failed to connect to AssetCatalog signal");

    //Application manager related stuff

    // The AssetCatalog has to be rebuilt on connection, so we force the incoming connection messages to be serialized as they connect to the ApplicationManagerBase
    result = QObject::connect(m_applicationServer, &ApplicationServer::newIncomingConnection, m_connectionManager, &ConnectionManager::NewConnection, Qt::QueuedConnection);

    AZ_Assert(result, "Failed to connect to ApplicationServer signal");

    //RcController related stuff
    result = QObject::connect(GetRCController(), &AssetProcessor::RCController::JobStatusChanged, GetAssetProcessorManager(), &AssetProcessor::AssetProcessorManager::OnJobStatusChanged);
    AZ_Assert(result, "Failed to connect to RCController signal");

    result = QObject::connect(GetRCController(), &AssetProcessor::RCController::JobStarted, this,
            [](QString inputFile, QString platform)
            {
                QString msg = QCoreApplication::translate("O3DE Asset Processor", "Processing %1 (%2)...\n", "%1 is the name of the file, and %2 is the platform to process it for").arg(inputFile, platform);
                AZ_Printf(AssetProcessor::ConsoleChannel, "%s", msg.toUtf8().constData());
                AssetNotificationMessage message(inputFile.toUtf8().constData(), AssetNotificationMessage::JobStarted, AZ::Data::s_invalidAssetType, platform.toUtf8().constData());
                EBUS_EVENT(AssetProcessor::ConnectionBus, SendPerPlatform, 0, message, platform);
            }
            );
    AZ_Assert(result, "Failed to connect to RCController signal");

    result = QObject::connect(GetRCController(), &AssetProcessor::RCController::FileCompiled, this,
            [](AssetProcessor::JobEntry entry, AssetBuilderSDK::ProcessJobResponse /*response*/)
            {
                AssetNotificationMessage message(entry.m_sourceAssetReference.RelativePath().c_str(), AssetNotificationMessage::JobCompleted, AZ::Data::s_invalidAssetType, entry.m_platformInfo.m_identifier.c_str());
                EBUS_EVENT(AssetProcessor::ConnectionBus, SendPerPlatform, 0, message, QString::fromUtf8(entry.m_platformInfo.m_identifier.c_str()));
            }
            );
    AZ_Assert(result, "Failed to connect to RCController signal");

    result = QObject::connect(GetRCController(), &AssetProcessor::RCController::FileFailed, this,
            [](AssetProcessor::JobEntry entry)
            {
                AssetNotificationMessage message(entry.m_sourceAssetReference.RelativePath().c_str(), AssetNotificationMessage::JobFailed, AZ::Data::s_invalidAssetType, entry.m_platformInfo.m_identifier.c_str());
                EBUS_EVENT(AssetProcessor::ConnectionBus, SendPerPlatform, 0, message, QString::fromUtf8(entry.m_platformInfo.m_identifier.c_str()));
            }
            );
    AZ_Assert(result, "Failed to connect to RCController signal");

    result = QObject::connect(GetRCController(), &AssetProcessor::RCController::JobsInQueuePerPlatform, this,
            [](QString platform, int count)
            {
                AssetNotificationMessage message(QByteArray::number(count).constData(), AssetNotificationMessage::JobCount, AZ::Data::s_invalidAssetType, platform.toUtf8().constData());
                EBUS_EVENT(AssetProcessor::ConnectionBus, SendPerPlatform, 0, message, platform);
            }
            );
    AZ_Assert(result, "Failed to connect to RCController signal");

    m_connectionManager->RegisterService(RequestPing::MessageType,
        AZStd::bind([](unsigned int connId, unsigned int /*type*/, unsigned int serial, QByteArray /*payload*/)
            {
                ResponsePing responsePing;
                EBUS_EVENT_ID(connId, AssetProcessor::ConnectionBus, SendResponse, serial, responsePing);
            }, AZStd::placeholders::_1, AZStd::placeholders::_2, AZStd::placeholders::_3, AZStd::placeholders::_4)
        );

    m_connectionManager->RegisterService(
        AssetBuilder::BuilderRegistrationRequest::MessageType,
        [this](unsigned int /*connId*/, unsigned int /*type*/, unsigned int /*serial*/, QByteArray payload, QString)
    {
        AssetBuilder::BuilderRegistrationRequest registrationRequest;

        if (m_builderRegistrationComplete)
        {
            return;
        }

        m_builderRegistrationComplete = true;

        if (AssetProcessor::UnpackMessage(payload, registrationRequest))
        {
            for (const auto& builder : registrationRequest.m_builders)
            {
                AssetBuilderSDK::AssetBuilderDesc desc;
                desc.m_name = builder.m_name;
                desc.m_patterns = builder.m_patterns;
                desc.m_version = builder.m_version;
                desc.m_analysisFingerprint = builder.m_analysisFingerprint;
                desc.m_flags = builder.m_flags;
                desc.m_busId = builder.m_busId;
                desc.m_flagsByJobKey = builder.m_flagsByJobKey;
                desc.m_productsToKeepOnFailure = builder.m_productsToKeepOnFailure;

                // Builders registered this way are always external builders
                desc.m_builderType = AssetBuilderSDK::AssetBuilderDesc::AssetBuilderType::External;

                RegisterBuilderInformation(desc);
            }

            QTimer::singleShot(
                0, this,
                [this]()
                {
                    if (!PostActivate())
                    {
                        QuitRequested();
                    }
                });
        }
    });

    //You can get Asset Processor Current State
    using AzFramework::AssetSystem::RequestAssetProcessorStatus;
    auto GetState = [this](unsigned int connId, unsigned int, unsigned int serial, QByteArray payload, QString)
        {
            RequestAssetProcessorStatus requestAssetProcessorMessage;

            if (AssetProcessor::UnpackMessage(payload, requestAssetProcessorMessage))
            {
                bool status = false;
                //check whether the scan is complete,the asset processor manager initial processing is complete and
                //the number of copy jobs are zero

                int numberOfPendingJobs = GetRCController()->NumberOfPendingCriticalJobsPerPlatform(requestAssetProcessorMessage.m_platform.c_str());
                status = (GetAssetScanner()->status() == AssetProcessor::AssetScanningStatus::Completed)
                    && m_assetProcessorManagerIsReady
                    && (!numberOfPendingJobs);

                ResponseAssetProcessorStatus responseAssetProcessorMessage;
                responseAssetProcessorMessage.m_isAssetProcessorReady = status;
                responseAssetProcessorMessage.m_numberOfPendingJobs = numberOfPendingJobs + m_remainingAPMJobs;
                if (responseAssetProcessorMessage.m_numberOfPendingJobs && m_highestConnId < connId)
                {
                    // We will just emit this status message once per connId
                    Q_EMIT ConnectionStatusMsg(QString(" Critical assets need to be processed for %1 platform. Editor/Game will launch once they are processed.").arg(requestAssetProcessorMessage.m_platform.c_str()));
                    m_highestConnId = connId;
                }
                EBUS_EVENT_ID(connId, AssetProcessor::ConnectionBus, SendResponse, serial, responseAssetProcessorMessage);
            }
        };
    // connect the network messages to the Request handler:
    m_connectionManager->RegisterService(RequestAssetProcessorStatus::MessageType, GetState);

    // ability to see if an asset platform is enabled or not
    using AzToolsFramework::AssetSystem::AssetProcessorPlatformStatusRequest;
    m_connectionManager->RegisterService(AssetProcessorPlatformStatusRequest::MessageType,
        [](unsigned int connId, unsigned int, unsigned int serial, QByteArray payload, QString)
        {
            AssetProcessorPlatformStatusResponse responseMessage;

            AssetProcessorPlatformStatusRequest requestMessage;
            if (AssetProcessor::UnpackMessage(payload, requestMessage))
            {
                AzToolsFramework::AssetSystemRequestBus::BroadcastResult(responseMessage.m_isPlatformEnabled,
                        &AzToolsFramework::AssetSystemRequestBus::Events::IsAssetPlatformEnabled, requestMessage.m_platform.c_str());
            }

            AssetProcessor::ConnectionBus::Event(connId,
                &AssetProcessor::ConnectionBus::Events::SendResponse, serial, responseMessage);
        });


    // check the total number of assets remaining for a specified platform
    using AzToolsFramework::AssetSystem::AssetProcessorPendingPlatformAssetsRequest;
    m_connectionManager->RegisterService(AssetProcessorPendingPlatformAssetsRequest::MessageType,
        [this](unsigned int connId, unsigned int, unsigned int serial, QByteArray payload, QString)
        {
            AssetProcessorPendingPlatformAssetsResponse responseMessage;

            AssetProcessorPendingPlatformAssetsRequest requestMessage;
            if (AssetProcessor::UnpackMessage(payload, requestMessage))
            {
                const char* platformIdentifier = requestMessage.m_platform.c_str();
                responseMessage.m_numberOfPendingJobs =
                    GetRCController()->NumberOfPendingJobsPerPlatform(platformIdentifier);
            }

            AssetProcessor::ConnectionBus::Event(connId,
                &AssetProcessor::ConnectionBus::Events::SendResponse, serial, responseMessage);
        });
}

void ApplicationManagerBase::DestroyConnectionManager()
{
    if (m_connectionManager)
    {
        delete m_connectionManager;
        m_connectionManager = nullptr;
    }
}

void ApplicationManagerBase::InitAssetRequestHandler(AssetProcessor::AssetRequestHandler* assetRequestHandler)
{
    using namespace AzFramework::AssetSystem;
    using namespace AzToolsFramework::AssetSystem;
    using namespace AzFramework::AssetSystem;
    using namespace AssetProcessor;

    m_assetRequestHandler = assetRequestHandler;

    auto router = AZ::Interface<IRequestRouter>::Get();

    if (router)
    {
        router->RegisterQueuedCallbackHandler(GetAssetProcessorManager(), &AssetProcessorManager::ProcessGetAssetJobsInfoRequest);
        router->RegisterQueuedCallbackHandler(GetAssetProcessorManager(), &AssetProcessorManager::ProcessGetAssetJobLogRequest);
        router->RegisterQueuedCallbackHandler(GetAssetProcessorManager(), &AssetProcessorManager::ProcessGetAbsoluteAssetDatabaseLocationRequest);
        router->RegisterQueuedCallbackHandler(GetAssetCatalog(), &AssetCatalog::HandleSaveAssetCatalogRequest);
        router->RegisterQueuedCallbackHandler(GetAssetCatalog(), &AssetCatalog::HandleGetUnresolvedDependencyCountsRequest);
    }

    // connect the "Does asset exist?" loop to each other:
    QObject::connect(m_assetRequestHandler, &AssetRequestHandler::RequestAssetExists, GetAssetProcessorManager(), &AssetProcessorManager::OnRequestAssetExists);
    QObject::connect(GetAssetProcessorManager(), &AssetProcessorManager::SendAssetExistsResponse, m_assetRequestHandler, &AssetRequestHandler::OnRequestAssetExistsResponse);

    QObject::connect(GetAssetProcessorManager(), &AssetProcessorManager::FenceFileDetected, m_assetRequestHandler, &AssetRequestHandler::OnFenceFileDetected);

    // connect the Asset Request Handler to RC:
    QObject::connect(m_assetRequestHandler, &AssetRequestHandler::RequestCompileGroup, GetRCController(), &RCController::OnRequestCompileGroup);
    QObject::connect(m_assetRequestHandler, &AssetRequestHandler::RequestEscalateAssetBySearchTerm, GetRCController(), &RCController::OnEscalateJobsBySearchTerm);
    QObject::connect(m_assetRequestHandler, &AssetRequestHandler::RequestEscalateAssetByUuid, GetRCController(), &RCController::OnEscalateJobsBySourceUUID);

    QObject::connect(GetRCController(), &RCController::CompileGroupCreated, m_assetRequestHandler, &AssetRequestHandler::OnCompileGroupCreated);
    QObject::connect(GetRCController(), &RCController::CompileGroupFinished, m_assetRequestHandler, &AssetRequestHandler::OnCompileGroupFinished);

    QObject::connect(GetAssetProcessorManager(), &AssetProcessor::AssetProcessorManager::NumRemainingJobsChanged, this, [this](int newNum)
        {
            if (!m_assetProcessorManagerIsReady)
            {
                if (m_remainingAPMJobs == newNum)
                {
                    return;
                }

                m_remainingAPMJobs = newNum;

                if (!m_remainingAPMJobs)
                {
                    m_assetProcessorManagerIsReady = true;
                }
            }

            AssetProcessor::AssetProcessorStatusEntry entry(AssetProcessor::AssetProcessorStatus::Analyzing_Jobs, newNum);
            Q_EMIT AssetProcessorStatusChanged(entry);
        });
}

void ApplicationManagerBase::InitFileStateCache()
{
    const AzFramework::CommandLine* commandLine = nullptr;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(commandLine, &AzFramework::ApplicationRequests::GetCommandLine);

    if (commandLine->HasSwitch("disableFileCache"))
    {
        m_fileStateCache = AZStd::make_unique<AssetProcessor::FileStatePassthrough>();
        return;
    }

    m_fileStateCache = AZStd::make_unique<AssetProcessor::FileStateCache>();
}

ApplicationManager::BeforeRunStatus ApplicationManagerBase::BeforeRun()
{
    ApplicationManager::BeforeRunStatus status = ApplicationManager::BeforeRun();

    if (status != ApplicationManager::BeforeRunStatus::Status_Success)
    {
        return status;
    }

    //Register all QMetatypes here
    qRegisterMetaType<AzFramework::AssetSystem::AssetStatus>("AzFramework::AssetSystem::AssetStatus");
    qRegisterMetaType<AzFramework::AssetSystem::AssetStatus>("AssetStatus");

    qRegisterMetaType<AssetProcessor::AssetScanningStatus>("AssetScanningStatus");

    qRegisterMetaType<AssetProcessor::NetworkRequestID>("NetworkRequestID");

    qRegisterMetaType<AssetProcessor::JobEntry>("JobEntry");
    qRegisterMetaType<AzToolsFramework::AssetSystem::JobInfo>("AzToolsFramework::AssetSystem::JobInfo");

    qRegisterMetaType<AssetBuilderSDK::ProcessJobResponse>("ProcessJobResponse");

    qRegisterMetaType<AzToolsFramework::AssetSystem::JobStatus>("AzToolsFramework::AssetSystem::JobStatus");
    qRegisterMetaType<AzToolsFramework::AssetSystem::JobStatus>("JobStatus");

    qRegisterMetaType<AssetProcessor::JobDetails>("JobDetails");
    qRegisterMetaType<AZ::Data::AssetId>("AZ::Data::AssetId");
    qRegisterMetaType<AZ::Data::AssetInfo>("AZ::Data::AssetInfo");

    qRegisterMetaType<AzToolsFramework::AssetSystem::AssetJobLogRequest>("AzToolsFramework::AssetSystem::AssetJobLogRequest");
    qRegisterMetaType<AzToolsFramework::AssetSystem::AssetJobLogRequest>("AssetJobLogRequest");

    qRegisterMetaType<AzToolsFramework::AssetSystem::AssetJobLogResponse>("AzToolsFramework::AssetSystem::AssetJobLogResponse");
    qRegisterMetaType<AzToolsFramework::AssetSystem::AssetJobLogResponse>("AssetJobLogResponse");

    qRegisterMetaType<AzFramework::AssetSystem::BaseAssetProcessorMessage*>("AzFramework::AssetSystem::BaseAssetProcessorMessage*");
    qRegisterMetaType<AzFramework::AssetSystem::BaseAssetProcessorMessage*>("BaseAssetProcessorMessage*");

    qRegisterMetaType<AssetProcessor::JobIdEscalationList>("AssetProcessor::JobIdEscalationList");
    qRegisterMetaType<AzFramework::AssetSystem::AssetNotificationMessage>("AzFramework::AssetSystem::AssetNotificationMessage");
    qRegisterMetaType<AzFramework::AssetSystem::AssetNotificationMessage>("AssetNotificationMessage");
    qRegisterMetaType<AZStd::string>("AZStd::string");

    qRegisterMetaType<AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry>("AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry");

    qRegisterMetaType<AssetProcessor::AssetCatalogStatus>("AssetCatalogStatus");
    qRegisterMetaType<AssetProcessor::AssetCatalogStatus>("AssetProcessor::AssetCatalogStatus");

    qRegisterMetaType<QSet<QString> >("QSet<QString>");
    qRegisterMetaType<QSet<AssetProcessor::AssetFileInfo>>("QSet<AssetFileInfo>");
    qRegisterMetaType<AssetProcessor::SourceAssetReference>("SourceAssetReference");

    AssetBuilderSDK::AssetBuilderBus::Handler::BusConnect();
    AssetProcessor::AssetBuilderRegistrationBus::Handler::BusConnect();
    AssetProcessor::AssetBuilderInfoBus::Handler::BusConnect();
    AZ::Debug::TraceMessageBus::Handler::BusConnect();
    AzToolsFramework::SourceControlNotificationBus::Handler::BusConnect();

    return ApplicationManager::BeforeRunStatus::Status_Success;
}

void ApplicationManagerBase::Destroy()
{
    delete m_ticker;
    m_ticker = nullptr;

    delete m_assetRequestHandler;
    m_assetRequestHandler = nullptr;

    ShutdownBuilderManager();
    ShutDownFileProcessor();

    DestroyControlRequestHandler();
    DestroyConnectionManager();
    DestroyAssetServerHandler();
    DestroyRCController();
    DestroyAssetScanner();
    DestroyFileMonitor();
    ShutDownAssetDatabase();
    DestroyPlatformConfiguration();
    DestroyApplicationServer();
}

bool ApplicationManagerBase::Run()
{
    bool showErrorMessageOnRegistryProblem = false;
    RegistryCheckInstructions registryCheckInstructions = CheckForRegistryProblems(nullptr, showErrorMessageOnRegistryProblem);
    if (registryCheckInstructions != RegistryCheckInstructions::Continue)
    {
        return false;
    }

    if (!Activate())
    {
        return false;
    }

    AZ_Printf(AssetProcessor::ConsoleChannel, "Asset Processor Batch Processing Started.\n");
    AZ_Printf(AssetProcessor::ConsoleChannel, "-----------------------------------------\n");
    QElapsedTimer allAssetsProcessingTimer;
    allAssetsProcessingTimer.start();
    m_duringStartup = false;
    qApp->exec();

    AZ_Printf(AssetProcessor::ConsoleChannel, "-----------------------------------------\n");
    AZ_Printf(AssetProcessor::ConsoleChannel, "Asset Processor Batch Processing complete\n");

    if (!m_failedAssets.empty())
    {
        AZ_Printf(AssetProcessor::ConsoleChannel, "---------------FAILED ASSETS-------------\n");

        for (const auto& failedAsset : m_failedAssets)
        {
            AZ_Printf(AssetProcessor::ConsoleChannel, "%s\n", failedAsset.c_str());
        }

        AZ_Printf(AssetProcessor::ConsoleChannel, "-----------------------------------------\n");
    }

    AZ_Printf(AssetProcessor::ConsoleChannel, "Number of Assets Successfully Processed: %d.\n", ProcessedAssetCount());
    AZ_Printf(AssetProcessor::ConsoleChannel, "Number of Assets Failed to Process: %d.\n", FailedAssetsCount());
    AZ_Printf(AssetProcessor::ConsoleChannel, "Number of Warnings Reported: %d.\n", m_warningCount);
    AZ_Printf(AssetProcessor::ConsoleChannel, "Number of Errors Reported: %d.\n", m_errorCount);
    AZ_Printf(AssetProcessor::ConsoleChannel, "Total Assets Processing Time: %fs\n", allAssetsProcessingTimer.elapsed() / 1000.0f);
    AZ_Printf(AssetProcessor::ConsoleChannel, "Asset Processor Batch Processing Completed.\n");

    RemoveOldTempFolders();
    Destroy();

    return FailedAssetsCount() == 0;
}

void ApplicationManagerBase::HandleFileRelocation() const
{
    static constexpr char Delimiter[] = "--------------------------- RELOCATION REPORT  ---------------------------\n";
    static constexpr char MoveCommand[] = "move";
    static constexpr char DeleteCommand[] = "delete";
    static constexpr char ConfirmCommand[] = "confirm";
    static constexpr char LeaveEmptyFoldersCommand[] = "leaveEmptyFolders";
    static constexpr char AllowBrokenDependenciesCommand[] = "allowBrokenDependencies";
    static constexpr char UpdateReferencesCommand[] = "updateReferences";
    static constexpr char ExcludeMetaDataFiles[] = "excludeMetaDataFiles";

    const AzFramework::CommandLine* commandLine = nullptr;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(commandLine, &AzFramework::ApplicationRequests::GetCommandLine);

    const bool allowBrokenDependencies = commandLine->HasSwitch(AllowBrokenDependenciesCommand);
    const bool previewOnly = !commandLine->HasSwitch(ConfirmCommand);
    const bool leaveEmptyFolders = commandLine->HasSwitch(LeaveEmptyFoldersCommand);
    const bool doMove = commandLine->HasSwitch(MoveCommand);
    const bool doDelete = commandLine->HasSwitch(DeleteCommand);
    const bool updateReferences = commandLine->HasSwitch(UpdateReferencesCommand);
    const bool excludeMetaDataFiles = commandLine->HasSwitch(ExcludeMetaDataFiles);

    if(doMove || doDelete)
    {
        int printCounter = 0;

        while(!m_sourceControlReady)
        {
            // We need to wait for source control to be ready before continuing

            if (printCounter % 10 == 0)
            {
                AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Waiting for Source Control connection\n");
            }

            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(100));
            AZ::TickBus::ExecuteQueuedEvents();

            ++printCounter;
        }
    }

    if(!doMove && updateReferences)
    {
        AZ_Error(AssetProcessor::ConsoleChannel, false, "Command --%s must be used with command --%s", UpdateReferencesCommand, MoveCommand);
        return;
    }

    // Print some errors to inform users that the move or delete command must be included
    if(!doMove && !doDelete)
    {
        AZ_Error(AssetProcessor::ConsoleChannel, previewOnly, "Command --%s must be used with command --%s or --%s", ConfirmCommand, MoveCommand, DeleteCommand);
        AZ_Error(AssetProcessor::ConsoleChannel, !leaveEmptyFolders, "Command --%s must be used with command --%s or --%s", LeaveEmptyFoldersCommand, MoveCommand, DeleteCommand);
        AZ_Error(AssetProcessor::ConsoleChannel, !allowBrokenDependencies, "Command --%s must be used with command --%s or --%s", AllowBrokenDependenciesCommand, MoveCommand, DeleteCommand);

        return;
    }

    if (doMove)
    {
        if (commandLine->GetNumSwitchValues(MoveCommand) != 2)
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, "Invalid format for move command.  Expected format is %s=<source>,<destination>", MoveCommand);
            return;
        }

        AZ_Printf(AssetProcessor::ConsoleChannel, Delimiter);

        auto source = commandLine->GetSwitchValue(MoveCommand, 0);
        auto destination = commandLine->GetSwitchValue(MoveCommand, 1);

        AZ_Printf(AssetProcessor::ConsoleChannel, "Move Source: %s, Destination: %s\n", source.c_str(), destination.c_str());

        if(!previewOnly)
        {
            AZ_Printf(AssetProcessor::ConsoleChannel, "Performing real file move\n");

            if (leaveEmptyFolders)
            {
                AZ_Printf(AssetProcessor::ConsoleChannel, "Leaving empty folders\n");
            }
            else
            {
                AZ_Printf(AssetProcessor::ConsoleChannel, "Deleting empty folders\n");
            }

            if(updateReferences)
            {
                AZ_Printf(AssetProcessor::ConsoleChannel, "Attempting to perform reference fix-up\n");
            }
        }
        else
        {
            AZ_Printf(AssetProcessor::ConsoleChannel, "SETTING: Preview file move.  Run again with --%s to actually make changes\n", ConfirmCommand);
        }

        auto* relocationInterface = AZ::Interface<AssetProcessor::ISourceFileRelocation>::Get();

        if(relocationInterface)
        {
            auto result = relocationInterface->Move(source, destination, previewOnly, allowBrokenDependencies, !leaveEmptyFolders, updateReferences, excludeMetaDataFiles);

            if (result.IsSuccess())
            {
                AssetProcessor::RelocationSuccess success = result.TakeValue();

                // The report can be too long for the AZ_Printf buffer, so split it into individual lines
                AZStd::string report = relocationInterface->BuildReport(success.m_relocationContainer, success.m_updateTasks, true, updateReferences);
                AZStd::vector<AZStd::string> lines;
                AzFramework::StringFunc::Tokenize(report.c_str(), lines, "\n");

                for (const AZStd::string& line : lines)
                {
                    AZ_Printf(AssetProcessor::ConsoleChannel, (line + "\n").c_str());
                }

                if (!previewOnly)
                {
                    AZ_Printf(AssetProcessor::ConsoleChannel, "MOVE COMPLETE\n");

                    AZ_Printf(AssetProcessor::ConsoleChannel, "TOTAL DEPENDENCIES FOUND: %d\n", success.m_updateTotalCount);
                    AZ_Printf(AssetProcessor::ConsoleChannel, "SUCCESSFULLY UPDATED: %d\n", success.m_updateSuccessCount);
                    AZ_Printf(AssetProcessor::ConsoleChannel, "FAILED TO UPDATE: %d\n", success.m_updateFailureCount);

                    AZ_Printf(AssetProcessor::ConsoleChannel, "TOTAL FILES: %d\n", success.m_moveTotalCount);
                    AZ_Printf(AssetProcessor::ConsoleChannel, "SUCCESS COUNT: %d\n", success.m_moveSuccessCount);
                    AZ_Printf(AssetProcessor::ConsoleChannel, "FAILURE COUNT: %d\n", success.m_moveFailureCount);
                }
            }
            else
            {
                AssetProcessor::MoveFailure failure = result.TakeError();

                AZ_Printf(AssetProcessor::ConsoleChannel, failure.m_reason.c_str());

                if(failure.m_dependencyFailure)
                {
                    AZ_Printf(AssetProcessor::ConsoleChannel, "To ignore and continue anyway, re-run this command with the --%s option OR re-run this command with the --%s option to attempt to fix-up references\n", AllowBrokenDependenciesCommand, UpdateReferencesCommand);
                }
            }
        }
        else
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, "Unable to retrieve ISourceFileRelocation interface");
            return;
        }

        AZ_Printf(AssetProcessor::ConsoleChannel, Delimiter);
    }
    else if(doDelete)
    {
        if(commandLine->GetNumSwitchValues(DeleteCommand) != 1)
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, "Invalid format for delete command.  Expected format is %s=<source>", DeleteCommand);
            return;
        }

        AZ_Printf(AssetProcessor::ConsoleChannel, Delimiter);

        auto source = commandLine->GetSwitchValue(DeleteCommand, 0);

        AZ_Printf(AssetProcessor::ConsoleChannel, "Delete Source: %s\n", source.c_str());

        if (!previewOnly)
        {
            AZ_Printf(AssetProcessor::ConsoleChannel, "Performing real file delete\n");

            if (leaveEmptyFolders)
            {
                AZ_Printf(AssetProcessor::ConsoleChannel, "Leaving empty folders\n");
            }
            else
            {
                AZ_Printf(AssetProcessor::ConsoleChannel, "Deleting empty folders\n");
            }
        }
        else
        {
            AZ_Printf(AssetProcessor::ConsoleChannel, "SETTING: Preview file delete.  Run again with --%s to actually make changes\n", ConfirmCommand);
        }

        auto* relocationInterface = AZ::Interface<AssetProcessor::ISourceFileRelocation>::Get();

        if (relocationInterface)
        {
            auto result = relocationInterface->Delete(source, previewOnly, allowBrokenDependencies, !leaveEmptyFolders, excludeMetaDataFiles);

            if (result.IsSuccess())
            {
                AssetProcessor::RelocationSuccess success = result.TakeValue();

                // The report can be too long for the AZ_Printf buffer, so split it into individual lines
                AZStd::string report = relocationInterface->BuildReport(success.m_relocationContainer, success.m_updateTasks, false, updateReferences);
                AZStd::vector<AZStd::string> lines;
                AzFramework::StringFunc::Tokenize(report.c_str(), lines, "\n");

                for (const AZStd::string& line : lines)
                {
                    AZ_Printf(AssetProcessor::ConsoleChannel, (line + "\n").c_str());
                }

                if (!previewOnly)
                {
                    AZ_Printf(AssetProcessor::ConsoleChannel, "DELETE COMPLETE\n");
                    AZ_Printf(AssetProcessor::ConsoleChannel, "TOTAL FILES: %d\n", success.m_moveTotalCount);
                    AZ_Printf(AssetProcessor::ConsoleChannel, "SUCCESS COUNT: %d\n", success.m_moveSuccessCount);
                    AZ_Printf(AssetProcessor::ConsoleChannel, "FAILURE COUNT: %d\n", success.m_moveFailureCount);
                }
            }
            else
            {
                AZ_Printf(AssetProcessor::ConsoleChannel, result.TakeError().c_str());
            }
        }
        else
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, "Unable to retrieve ISourceFileRelocation interface");
        }

        AZ_Printf(AssetProcessor::ConsoleChannel, Delimiter);
    }
}

bool ApplicationManagerBase::CheckFullIdle()
{
    bool isIdle = m_rcController->IsIdle() && m_AssetProcessorManagerIdleState;
    if (isIdle != m_fullIdle)
    {
        m_fullIdle = isIdle;
        Q_EMIT FullIdle(m_fullIdle);
    }
    return isIdle;
}


void ApplicationManagerBase::CheckForIdle()
{
    if (InitiatedShutdown())
    {
        return;
    }

    bool shouldExit = GetShouldExitOnIdle();

    if (shouldExit && m_connectionsToRemoveOnShutdown.empty())
    {
        // we've already entered this state once.  Ignore repeats.  this can happen if another sender of events
        // rapidly flicks between idle/not idle and sends many "I'm done!" messages which are all queued up.
        return;
    }

    if (CheckFullIdle())
    {
        if (CheckReprocessFileList())
        {
            return;
        }

        if (shouldExit)
        {
            // If everything else is done, and it was requested to scan for missing product dependencies, perform that scan now.
            TryScanProductDependencies();

            TryHandleFileRelocation();

            // since we are shutting down, we save the registry and then we quit.
            AZ_Printf(AssetProcessor::ConsoleChannel, "No assets remain in the build queue.  Saving the catalog, and then shutting down.\n");
            // stop accepting any further idle messages, as we will shut down - don't want this function to repeat!
            for (const QMetaObject::Connection& connection : m_connectionsToRemoveOnShutdown)
            {
                QObject::disconnect(connection);
            }
            m_connectionsToRemoveOnShutdown.clear();

            // Checking the status of the asset catalog here using qt's signal slot mechanism
            // to ensure that we do not have any pending events in the event loop that can make the catalog dirty again
            QObject::connect(m_assetCatalog, &AssetProcessor::AssetCatalog::AsyncAssetCatalogStatusResponse, this, [&](AssetProcessor::AssetCatalogStatus status)
                {
                    if (status == AssetProcessor::AssetCatalogStatus::RequiresSaving)
                    {
                        AssetProcessor::AssetRegistryRequestBus::Broadcast(&AssetProcessor::AssetRegistryRequests::SaveRegistry);
                    }

                    AssetProcessor::AssetRegistryRequestBus::Broadcast(&AssetProcessor::AssetRegistryRequests::ValidatePreLoadDependency);

                    QuitRequested();
                }, Qt::UniqueConnection);

            QMetaObject::invokeMethod(m_assetCatalog, "AsyncAssetCatalogStatusRequest", Qt::QueuedConnection);
        }
        else
        {
            // we save the registry when we become idle, but we stay running.
            AssetProcessor::AssetRegistryRequestBus::Broadcast(&AssetProcessor::AssetRegistryRequests::SaveRegistry);
            AssetProcessor::AssetRegistryRequestBus::Broadcast(&AssetProcessor::AssetRegistryRequests::ValidatePreLoadDependency);
        }
    }
}

void ApplicationManagerBase::InitBuilderManager()
{
    AZ_Assert(m_connectionManager != nullptr, "ConnectionManager must be started before the builder manager");
    m_builderManager = new AssetProcessor::BuilderManager(m_connectionManager);

    QObject::connect(m_connectionManager, &ConnectionManager::ConnectionDisconnected, this, [this](unsigned int connId)
        {
            m_builderManager->ConnectionLost(connId);
        });

}

void ApplicationManagerBase::ShutdownBuilderManager()
{
    if (m_builderManager)
    {
        delete m_builderManager;
        m_builderManager = nullptr;
    }
}

bool ApplicationManagerBase::InitAssetDatabase(bool ignoreFutureAssetDBVersionError)
{
    AzToolsFramework::AssetDatabase::AssetDatabaseRequests::Bus::Handler::BusConnect();

    // create or upgrade the asset database here, so that it is already good for the rest of the application and the rest
    // of the application does not have to worry about a failure to upgrade or create it.
    AssetProcessor::AssetDatabaseConnection database;
    if (!database.OpenDatabase(ignoreFutureAssetDBVersionError))
    {
        return false;
    }

    database.CloseDatabase();

    return true;
}

void ApplicationManagerBase::ShutDownAssetDatabase()
{
    AzToolsFramework::AssetDatabase::AssetDatabaseRequests::Bus::Handler::BusDisconnect();
}

void ApplicationManagerBase::InitFileProcessor()
{
    AssetProcessor::ThreadController<AssetProcessor::FileProcessor>* fileProcessorHelper = new AssetProcessor::ThreadController<AssetProcessor::FileProcessor>();

    addRunningThread(fileProcessorHelper);
    m_fileProcessor.reset(fileProcessorHelper->initialize([this]()
    {
        return new AssetProcessor::FileProcessor(m_platformConfiguration);
    }));
}

void ApplicationManagerBase::ShutDownFileProcessor()
{
    m_fileProcessor.reset();
}

void ApplicationManagerBase::InitAssetServerHandler()
{
    m_assetServerHandler = new AssetProcessor::AssetServerHandler();
    m_assetServerHandler->HandleRemoteConfiguration();
}

void ApplicationManagerBase::DestroyAssetServerHandler()
{
    delete m_assetServerHandler;
    m_assetServerHandler = nullptr;
}

// IMPLEMENTATION OF -------------- AzToolsFramework::AssetDatabase::AssetDatabaseRequests::Bus::Listener
bool ApplicationManagerBase::GetAssetDatabaseLocation(AZStd::string& location)
{
    QDir cacheRoot;
    if (!AssetUtilities::ComputeProjectCacheRoot(cacheRoot))
    {
        location = "assetdb.sqlite";
    }

    location = cacheRoot.absoluteFilePath("assetdb.sqlite").toUtf8().data();
    return true;
}

// ------------------------------------------------------------

bool ApplicationManagerBase::CheckReprocessFileList()
{
    if (m_reprocessFileList.isEmpty() && m_filesToReprocess.isEmpty())
    {
        return false;
    }

    if (!m_reprocessFileList.isEmpty())
    {
        QFile reprocessFile(m_reprocessFileList);
        m_reprocessFileList.clear();
        if (!reprocessFile.open(QIODevice::ReadOnly))
        {
            AZ_Error("AssetProcessor", false, "Unable to open reprocess file list with path %s.", reprocessFile.fileName().toUtf8().data());
            return false;
        }

        while (!reprocessFile.atEnd())
        {
            m_filesToReprocess.append(reprocessFile.readLine());
        }

        reprocessFile.close();

        if (m_filesToReprocess.empty())
        {
            AZ_Error(
                "AssetProcessor", false, "No files listed to reprocess in the file at path %s.", reprocessFile.fileName().toUtf8().data());
            return false;
        }

    }

    // Queue one at a time, and wait for idle.
    // This makes sure the files in the list are processed in the same order.
    // Otherwise, the order can shuffle based on Asset Processor state.
    m_assetProcessorManager->RequestReprocess(m_filesToReprocess.front());
    m_filesToReprocess.pop_front();

    return true;
}

bool ApplicationManagerBase::Activate()
{
    QDir projectCache;
    if (!AssetUtilities::ComputeProjectCacheRoot(projectCache))
    {
        AZ_Error("AssetProcessor", false, "Could not compute project cache root, please configure your project correctly to launch Asset Processor.");
        return false;
    }

    AZ_TracePrintf(AssetProcessor::ConsoleChannel,
        "AssetProcessor will process assets from project root %s.\n", AssetUtilities::ComputeProjectPath().toUtf8().data());

    // Shutdown if the disk has less than 128MB of free space
    if (!CheckSufficientDiskSpace(128 * 1024 * 1024, true))
    {
        // CheckSufficientDiskSpace reports an error if disk space is low.
        return false;
    }

    bool appInited = InitApplicationServer();
    if (!appInited)
    {
        AZ_Error(
            "AssetProcessor", false, "InitApplicationServer failed, something internal to Asset Processor has failed, please report this to support if you encounter this error.");
        return false;
    }

    const AzFramework::CommandLine* commandLine = nullptr;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(commandLine, &AzFramework::ApplicationRequests::GetCommandLine);

    AZStd::vector<APCommandLineSwitch> commandLineInfo;
    const APCommandLineSwitch Command_ignoreFutureDBError(commandLineInfo, "ignoreFutureAssetDatabaseVersionError", "When not set, if the Asset Processor encounters an Asset Database "
        "with a future version, it will emit an error and shut down. When set, instead it will print the error as a log and erase the Asset Database, then it will proceed to initialize. "
        "This is intended for use with automated builds, and shouldn't be used by individuals. If an individual finds they want to use this flag frequently, the team should "
        "examine their workflows to determine why some team members encounter issues with future versioned Asset Databases.");

    if (!InitAssetDatabase(commandLine->HasSwitch(Command_ignoreFutureDBError.m_switch)))
    {
        // AssetDatabaseConnection::OpenDatabase reports any errors it encounters.
        return false;
    }

    if (!ApplicationManager::Activate())
    {
        // ApplicationManager::Activate() reports any errors it encounters.
        return false;
    }

    if (!InitPlatformConfiguration())
    {
        AZ_Error("AssetProcessor", false, "Failed to Initialize from AssetProcessorPlatformConfig.setreg - check the log files in the logs/ subfolder for more information.");
        return false;
    }

    InitBuilderConfiguration();
    PopulateApplicationDependencies();

    InitAssetProcessorManager(commandLineInfo);
    HandleCommandLineHelp(commandLineInfo);
    AssetBuilderSDK::InitializeSerializationContext();
    AssetBuilderSDK::InitializeBehaviorContext();
    AssetBuilder::InitializeSerializationContext();

    InitFileStateCache();
    InitFileProcessor();

    InitAssetCatalog();
    InitFileMonitor(AZStd::make_unique<FileWatcher>());
    InitAssetScanner();
    InitAssetServerHandler();
    InitRCController();

    InitConnectionManager();
    InitAssetRequestHandler(new AssetProcessor::AssetRequestHandler());

    InitBuilderManager();

    InitSourceControl();

    //We must register all objects that need to be notified if we are shutting down before we install the ctrlhandler

    // inserting in the front so that the application server is notified first
    // and we stop listening for new incoming connections during shutdown
    RegisterObjectForQuit(m_applicationServer, true);
    RegisterObjectForQuit(m_fileProcessor.get());
    RegisterObjectForQuit(m_connectionManager);
    RegisterObjectForQuit(m_assetProcessorManager);
    RegisterObjectForQuit(m_rcController);

    m_connectionsToRemoveOnShutdown << QObject::connect(
        m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::AssetProcessorManagerIdleState,
        this, [this](bool state)
        {
            if (state)
            {
                QMetaObject::invokeMethod(m_rcController, "SetDispatchPaused", Qt::QueuedConnection, Q_ARG(bool, false));
            }
        });

    m_connectionsToRemoveOnShutdown << QObject::connect(
        m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::AssetProcessorManagerIdleState,
        this, &ApplicationManagerBase::OnAssetProcessorManagerIdleState);

    m_connectionsToRemoveOnShutdown << QObject::connect(
        m_rcController, &AssetProcessor::RCController::BecameIdle,
        this, [this]()
        {
            Q_EMIT CheckAssetProcessorManagerIdleState();
        });

    m_connectionsToRemoveOnShutdown << QObject::connect(
        this, &ApplicationManagerBase::CheckAssetProcessorManagerIdleState,
        m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::CheckAssetProcessorIdleState);

    MakeActivationConnections();

    // only after everyones had a chance to init messages, we start listening.
    if (m_applicationServer)
    {
        if (!m_applicationServer->startListening())
        {
            // startListening reports any errors it encounters.
            return false;
        }
    }

    AssetProcessor::AssetProcessorStatusEntry entry(AssetProcessor::AssetProcessorStatus::Initializing_Builders, 0, QString());
    Q_EMIT AssetProcessorStatusChanged(entry);

    // Start up a thread which will request a builder to start to handle the registration of gems/builders
    // Builder info will be sent back to the AP via the network connection, start up will wait for the info before continuing
    // See ApplicationManagerBase::InitConnectionManager BuilderRegistrationRequest for the resume point here
    // Waiting here is not possible because the message comes back as a network message, which requires the main thread to process it
    // Since execution has to continue, this also means the thread object will go out of scope, so it must be detached before exiting.
    AZStd::thread_desc desc;
    desc.m_name = "Builder Component Registration";
    AZStd::thread builderRegistrationThread(
        desc,
        []()
        {
            AssetProcessor::BuilderRef builder;
            AssetProcessor::BuilderManagerBus::BroadcastResult(builder, &AssetProcessor::BuilderManagerBus::Events::GetBuilder, AssetProcessor::BuilderPurpose::Registration);

            if (!builder)
            {
                AZ_Error("ApplicationManagerBase", false, "AssetBuilder process failed to start.  Builder registration cannot complete.  Shutting down.");
                AssetProcessor::MessageInfoBus::Broadcast(&AssetProcessor::MessageInfoBus::Events::OnBuilderRegistrationFailure);
            }
        });

    builderRegistrationThread.detach();

    return true;
}

bool ApplicationManagerBase::PostActivate()
{
    m_connectionManager->LoadConnections();

    InitializeInternalBuilders();

    Q_EMIT OnBuildersRegistered();

    // 25 milliseconds is above the 'while loop' thing that QT does on windows (where small time ticks will spin loop instead of sleep)
    m_ticker = new AzToolsFramework::Ticker(nullptr, 25.0f);
    m_ticker->Start();
    connect(m_ticker, &AzToolsFramework::Ticker::Tick, this, []()
        {
            AZ::SystemTickBus::ExecuteQueuedEvents();
            AZ::SystemTickBus::Broadcast(&AZ::SystemTickEvents::OnSystemTick);
        });

    // now that everything is up and running, we start scanning.  Before this, we don't want file events to start percolating through the
    // asset system.

    GetAssetScanner()->StartScan();

    return true;
}

void ApplicationManagerBase::CreateQtApplication()
{
    m_qApp = new QCoreApplication(*m_frameworkApp.GetArgC(), *m_frameworkApp.GetArgV());
}

bool ApplicationManagerBase::InitializeInternalBuilders()
{
    m_internalBuilder = AZStd::make_shared<AssetProcessor::InternalRecognizerBasedBuilder>();
    bool result = m_internalBuilder->Initialize(*this->m_platformConfiguration);

    m_settingsRegistryBuilder = AZStd::make_shared<AssetProcessor::SettingsRegistryBuilder>();
    result = m_settingsRegistryBuilder->Initialize() && result;

    return result;
}

static void HandleConditionalRetry(const AssetProcessor::BuilderRunJobOutcome& result, int retryCount, AssetProcessor::BuilderRef& builderRef, AssetProcessor::BuilderPurpose purpose)
{
    // If a lost connection occured or the process was terminated before a response can be read, and there is another retry to get the
    // response from a Builder, then handle the logic to log and sleep before attempting the retry of the job
    if ((result == AssetProcessor::BuilderRunJobOutcome::LostConnection ||
         result == AssetProcessor::BuilderRunJobOutcome::ProcessTerminated ) && (retryCount <= AssetProcessor::RetriesForJobLostConnection))
    {
        const int delay = 1 << (retryCount-1);

        // Check if we need a new builder, and if so, request a new one
        if (!builderRef->IsValid())
        {
            // If the connection was lost and the process handle is no longer valid, then we need to request a new builder to reprocess the job
            AZStd::string oldBuilderId = builderRef->GetUuid().ToString<AZStd::string>();
            builderRef.release();

            AssetProcessor::BuilderManagerBus::BroadcastResult(builderRef, &AssetProcessor::BuilderManagerBusTraits::GetBuilder, purpose);

            if (builderRef)
            {
                AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Lost connection to builder %s. Retrying with a new builder %s (Attempt %d with %d second delay)",
                                oldBuilderId.c_str(),
                                builderRef->GetUuid().ToString<AZStd::string>().c_str(),
                                retryCount+1,
                                delay);
            }
            else
            {
                AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Lost connection to builder %s and no further builders are available. Job will not retry.\n",
                               oldBuilderId.c_str());
                // if we failed to get a builder ref, it means we're probably
                // shutting down, in which case we do not want to do an exponential
                // backoff delay and need to return immediately.
                return;
            }
        }
        else
        {
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Lost connection to builder %s. Retrying (Attempt %d  with %d second delay)",
                            builderRef->GetUuid().ToString<AZStd::string>().c_str(),
                            retryCount+1,
                            delay);
        }
        AZStd::this_thread::sleep_for(AZStd::chrono::seconds(delay));
    }
}

void ApplicationManagerBase::RegisterBuilderInformation(const AssetBuilderSDK::AssetBuilderDesc& builderDesc)
{
    if (!builderDesc.IsExternalBuilder())
    {
        // Create Job Function validation
        AZ_Error(
            AssetProcessor::ConsoleChannel, builderDesc.m_createJobFunction,
            "Create Job Function (m_createJobFunction) for %s builder is empty.\n", builderDesc.m_name.c_str());

        // Process Job Function validation
        AZ_Error(
            AssetProcessor::ConsoleChannel, builderDesc.m_processJobFunction,
            "Process Job Function (m_processJobFunction) for %s builder is empty.\n", builderDesc.m_name.c_str());
    }

    // Bus ID validation
    AZ_Error(AssetProcessor::ConsoleChannel,
        !builderDesc.m_busId.IsNull(),
        "Bus ID for %s builder is empty.\n",
        builderDesc.m_name.c_str());

    AssetBuilderSDK::AssetBuilderDesc modifiedBuilderDesc = builderDesc;
    // Allow for overrides defined in a BuilderConfig.ini file to update our code defined default values
    AssetProcessor::BuilderConfigurationRequestBus::Broadcast(&AssetProcessor::BuilderConfigurationRequests::UpdateBuilderDescriptor, builderDesc.m_name, modifiedBuilderDesc);

    if (builderDesc.IsExternalBuilder())
    {
        // We're going to override the createJob function so we can run it externally in AssetBuilder, rather than having it run
        // inside the AP
        modifiedBuilderDesc.m_createJobFunction =
            [this](const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
        {
            AssetProcessor::BuilderRef builderRef;
            AssetProcessor::BuilderManagerBus::BroadcastResult(builderRef, &AssetProcessor::BuilderManagerBusTraits::GetBuilder, AssetProcessor::BuilderPurpose::CreateJobs);

            if (builderRef)
            {
                int retryCount = 0;
                AssetProcessor::BuilderRunJobOutcome result;

                if (this->InitiatedShutdown())
                {
                    return; // exit early if you're shutting down!
                }

                do
                {
                    retryCount++;
                    result = builderRef->RunJob<AssetBuilder::CreateJobsNetRequest, AssetBuilder::CreateJobsNetResponse>(
                        request, response, s_MaximumCreateJobsTimeSeconds, "create", "", nullptr);

                    HandleConditionalRetry(result, retryCount, builderRef, AssetProcessor::BuilderPurpose::CreateJobs);

                } while ((result == AssetProcessor::BuilderRunJobOutcome::LostConnection ||
                          result == AssetProcessor::BuilderRunJobOutcome::ProcessTerminated) &&
                          retryCount <= AssetProcessor::RetriesForJobLostConnection);
            }
            else
            {
                AZ_Error("AssetProcessor", false, "Failed to retrieve a valid builder to process job");
            }
        };

        const bool debugOutput = m_assetProcessorManager->GetBuilderDebugFlag();
        // Also override the processJob function to run externally
        modifiedBuilderDesc.m_processJobFunction =
            [this, debugOutput](const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
        {
            AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);

            AssetProcessor::BuilderRef builderRef;
            AssetProcessor::BuilderManagerBus::BroadcastResult(builderRef, &AssetProcessor::BuilderManagerBusTraits::GetBuilder, AssetProcessor::BuilderPurpose::ProcessJob);

            if (builderRef)
            {
                if (debugOutput)
                {
                    AssetProcessor::BuilderManagerBus::Broadcast(
                        &AssetProcessor::BuilderManagerBusTraits::AddAssetToBuilderProcessedList, builderRef->GetUuid(),
                        request.m_fullPath);
                }

                int retryCount = 0;
                AssetProcessor::BuilderRunJobOutcome result;

                do
                {
                    if (jobCancelListener.IsCancelled())
                    {
                        // do not attempt to continue to retry or spawn
                        // new builders during shut down.
                        break;
                    }

                    if (this->InitiatedShutdown())
                    {
                        return; // exit early if you're shutting down!
                    }

                    retryCount++;
                    result = builderRef->RunJob<AssetBuilder::ProcessJobNetRequest, AssetBuilder::ProcessJobNetResponse>(
                        request, response, s_MaximumProcessJobsTimeSeconds, "process", "", &jobCancelListener, request.m_tempDirPath);

                    HandleConditionalRetry(result, retryCount, builderRef, AssetProcessor::BuilderPurpose::ProcessJob);

                } while ((result == AssetProcessor::BuilderRunJobOutcome::LostConnection ||
                          result == AssetProcessor::BuilderRunJobOutcome::ProcessTerminated) &&
                          retryCount <= AssetProcessor::RetriesForJobLostConnection);
            }
            else
            {
                AZ_Error("AssetProcessor", false, "Failed to retrieve a valid builder to process job");
            }
        };
    }

    if (m_builderDescMap.find(modifiedBuilderDesc.m_busId) != m_builderDescMap.end())
    {
        AZ_Warning(AssetProcessor::DebugChannel, false, "Uuid for %s builder is already registered.\n", modifiedBuilderDesc.m_name.c_str());
        return;
    }
    if (m_builderNameToId.find(modifiedBuilderDesc.m_name) != m_builderNameToId.end())
    {
        AZ_Warning(AssetProcessor::DebugChannel, false, "Duplicate builder detected.  A builder named '%s' is already registered.\n", modifiedBuilderDesc.m_name.c_str());
        return;
    }

    AZStd::sort(modifiedBuilderDesc.m_patterns.begin(), modifiedBuilderDesc.m_patterns.end(),
        [](const AssetBuilderSDK::AssetBuilderPattern& first, const AssetBuilderSDK::AssetBuilderPattern& second)
    {
        return first.ToString() < second.ToString();
    });

    m_builderDescMap[modifiedBuilderDesc.m_busId] = modifiedBuilderDesc;
    m_builderNameToId[modifiedBuilderDesc.m_name] = modifiedBuilderDesc.m_busId;

    for (const AssetBuilderSDK::AssetBuilderPattern& pattern : modifiedBuilderDesc.m_patterns)
    {
        AssetUtilities::BuilderFilePatternMatcher patternMatcher(pattern, modifiedBuilderDesc.m_busId);
        m_matcherBuilderPatterns.push_back(patternMatcher);
    }
}

void ApplicationManagerBase::RegisterComponentDescriptor(AZ::ComponentDescriptor* descriptor)
{
    ApplicationManager::RegisterComponentDescriptor(descriptor);
    if (m_currentExternalAssetBuilder)
    {
        m_currentExternalAssetBuilder->RegisterComponentDesc(descriptor);
    }
    else
    {
        AZ_Warning(AssetProcessor::DebugChannel, false, "Component description can only be registered during component activation.\n");
    }
}

void ApplicationManagerBase::BuilderLog(const AZ::Uuid& builderId, const char* message, ...)
{
    va_list args;
    va_start(args, message);
    BuilderLogV(builderId, message, args);
    va_end(args);
}

void ApplicationManagerBase::BuilderLogV(const AZ::Uuid& builderId, const char* message, va_list list)
{
    AZStd::string builderName;

    if (m_builderDescMap.find(builderId) != m_builderDescMap.end())
    {
        char messageBuffer[1024];
        azvsnprintf(messageBuffer, 1024, message, list);
        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Builder name : %s Message : %s.\n", m_builderDescMap[builderId].m_name.c_str(), messageBuffer);
    }
    else
    {
        // asset processor does not know about this builder id
        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "AssetProcessor does not know about the builder id: %s. \n", builderId.ToString<AZStd::string>().c_str());
    }
}

bool ApplicationManagerBase::FindBuilderInformation(const AZ::Uuid& builderGuid, AssetBuilderSDK::AssetBuilderDesc& descriptionOut)
{
    auto iter = m_builderDescMap.find(builderGuid);
    if (iter != m_builderDescMap.end())
    {
        descriptionOut = iter->second;
        return true;
    }
    else
    {
        return false;
    }
}

void ApplicationManagerBase::UnRegisterBuilderDescriptor(const AZ::Uuid& builderId)
{
    if (m_builderDescMap.find(builderId) == m_builderDescMap.end())
    {
        AZ_Warning(AssetProcessor::DebugChannel, false, "Cannot unregister builder descriptor for Uuid %s, not currently registered.\n", builderId.ToString<AZStd::string>().c_str());
        return;
    }

    // Remove from the map
    AssetBuilderSDK::AssetBuilderDesc& descToUnregister = m_builderDescMap[builderId];
    AZStd::string descNameToUnregister = descToUnregister.m_name;
    descToUnregister.m_createJobFunction.clear();
    descToUnregister.m_processJobFunction.clear();
    m_builderDescMap.erase(builderId);
    m_builderNameToId.erase(descNameToUnregister);

    // Remove the matcher build pattern
    for (auto remover = this->m_matcherBuilderPatterns.begin();
         remover != this->m_matcherBuilderPatterns.end();
         remover++)
    {
        if (remover->GetBuilderDescID() == builderId)
        {
            auto deleteIter = remover;
            remover++;
            this->m_matcherBuilderPatterns.erase(deleteIter);
        }
    }
}

void ApplicationManagerBase::GetMatchingBuildersInfo(const AZStd::string& assetPath, AssetProcessor::BuilderInfoList& builderInfoList)
{
    AZStd::set<AZ::Uuid>  uniqueBuilderDescIDs;

    for (AssetUtilities::BuilderFilePatternMatcher& matcherPair : m_matcherBuilderPatterns)
    {
        if (uniqueBuilderDescIDs.find(matcherPair.GetBuilderDescID()) != uniqueBuilderDescIDs.end())
        {
            continue;
        }
        if (matcherPair.MatchesPath(assetPath))
        {
            const AssetBuilderSDK::AssetBuilderDesc& builderDesc = m_builderDescMap[matcherPair.GetBuilderDescID()];
            uniqueBuilderDescIDs.insert(matcherPair.GetBuilderDescID());
            builderInfoList.push_back(builderDesc);
        }
    }
}

void ApplicationManagerBase::GetAllBuildersInfo(AssetProcessor::BuilderInfoList& builderInfoList)
{
    for (const auto &builderPair : m_builderDescMap)
    {
        builderInfoList.push_back(builderPair.second);
    }
}

bool ApplicationManagerBase::OnError(const char* /*window*/, const char* /*message*/)
{
    // We don't need to print the message to stdout, the trace system will already do that

    return true;
}

bool ApplicationManagerBase::CheckSufficientDiskSpace(qint64 requiredSpace, bool shutdownIfInsufficient)
{
    QDir cacheDir;
    if (!AssetUtilities::ComputeProjectCacheRoot(cacheDir))
    {
        AZ_Error(
            "AssetProcessor",
            false,
            "Could not compute project cache root, please configure your project correctly to launch Asset Processor.");
        return false;
    }

    QString savePath = cacheDir.absolutePath();

    if (!QDir(savePath).exists())
    {
        // GetFreeDiskSpace will fail if the path does not exist
        QDir dir;
        dir.mkpath(savePath);
    }

    qint64 bytesFree = 0;
    [[maybe_unused]] bool result = AzToolsFramework::ToolsFileUtils::GetFreeDiskSpace(savePath, bytesFree);

    AZ_Assert(result, "Unable to determine the amount of free space on drive containing path (%s).", savePath.toUtf8().constData());

    if (bytesFree < requiredSpace + s_ReservedDiskSpaceInBytes)
    {
        if (shutdownIfInsufficient)
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, "There is insufficient disk space to continue running.  AssetProcessor will now exit");
            QMetaObject::invokeMethod(this, "QuitRequested", Qt::QueuedConnection);
        }

        return false;
    }

    return true;
}

void ApplicationManagerBase::RemoveOldTempFolders()
{
    QDir rootDir;
    if (!AssetUtilities::ComputeAssetRoot(rootDir))
    {
        return;
    }

    QString startFolder;
    if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
    {
        if (AZ::IO::Path userPath; settingsRegistry->Get(userPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectUserPath))
        {
            startFolder = QString::fromUtf8(userPath.c_str(), aznumeric_cast<int>(userPath.Native().size()));
        }
    }

    QDir root;
    if (!AssetUtilities::CreateTempRootFolder(startFolder, root))
    {
        return;
    }

    // We will remove old temp folders if either their modified time is older than the cutoff time or
    // if the total number of temp folders have exceeded the maximum number of temp folders.
    QFileInfoList entries = root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time); // sorting by modification time
    int folderCount = 0;
    bool removeFolder = false;
    QDateTime cutoffTime = QDateTime::currentDateTime().addDays(-7);
    for (const QFileInfo& entry : entries)
    {
        if (!entry.fileName().startsWith("JobTemp-"))
        {
            continue;
        }

        // Since we are sorting the folders list from latest to oldest, we will either be in a state where we have to delete all the remaining folders or not
        // because either we have reached the folder limit or reached the cutoff date limit.
        removeFolder = removeFolder || (folderCount++ >= s_MaximumTempFolders) ||
            (entry.lastModified() < cutoffTime);

        if (removeFolder)
        {
            QDir dir(entry.absoluteFilePath());
            dir.removeRecursively();
        }
    }
}

void ApplicationManagerBase::ConnectivityStateChanged(const AzToolsFramework::SourceControlState /*newState*/)
{
    Q_EMIT SourceControlReady();
}

void ApplicationManagerBase::OnBuilderRegistrationFailure()
{
    QMetaObject::invokeMethod(this, "QuitRequested");
}

void ApplicationManagerBase::OnAssetProcessorManagerIdleState(bool isIdle)
{
    // these can come in during shutdown.
    if (InitiatedShutdown())
    {
        return;
    }

    if (isIdle)
    {
        if (!m_AssetProcessorManagerIdleState)
        {
            // We want to again ask the APM for the idle state just incase it goes from idle to non idle in between
            Q_EMIT CheckAssetProcessorManagerIdleState();
        }
        else
        {
            CheckForIdle();
            return;
        }
    }
    if (isIdle != m_AssetProcessorManagerIdleState)
    {
        Q_EMIT AssetProcesserManagerIdleStateChange(isIdle);
    }
    m_AssetProcessorManagerIdleState = isIdle;
}

bool ApplicationManagerBase::IsAssetProcessorManagerIdle() const
{
    return m_AssetProcessorManagerIdleState;
}

void ApplicationManagerBase::OnActiveJobsCountChanged(unsigned int count)
{
    AssetProcessor::AssetProcessorStatusEntry entry(AssetProcessor::AssetProcessorStatus::Processing_Jobs, count);
    Q_EMIT AssetProcessorStatusChanged(entry);
}
