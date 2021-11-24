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

#include <iostream>

#include <QCoreApplication>

#include <QElapsedTimer>

//! Amount of time to wait between checking the status of the AssetBuilder process
static const int s_MaximumSleepTimeMS = 10;

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
    AssetProcessor::DiskSpaceInfoBus::Handler::BusDisconnect();
    AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
    AssetProcessor::AssetBuilderRegistrationBus::Handler::BusDisconnect();
    AssetBuilderSDK::AssetBuilderBus::Handler::BusDisconnect();

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
    return m_failedAssetsCount;
}

void ApplicationManagerBase::ResetProcessedAssetCount()
{
    m_processedAssetCount = 0;
}

void ApplicationManagerBase::ResetFailedAssetCount()
{
    m_failedAssetsCount = 0;
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

void ApplicationManagerBase::InitAssetProcessorManager()
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

    struct APCommandLineSwitch
    {
        APCommandLineSwitch(const char* switchTitle, const char* helpText)
            : m_switch(switchTitle)
            , m_helpText(helpText)
        {

        }
        const char* m_switch;
        const char* m_helpText;
    };

    const APCommandLineSwitch Command_waitOnLaunch("waitOnLaunch", "Briefly pauses Asset Processor during initializiation. Useful if you want to attach a debugger.");
    const APCommandLineSwitch Command_zeroAnalysisMode("zeroAnalysisMode", "Enables using file modification time when examining source assets for processing.");
    const APCommandLineSwitch Command_enableQueryLogging("enableQueryLogging", "Enables logging database queries.");
    const APCommandLineSwitch Command_dependencyScanPattern("dependencyScanPattern", "Scans assets that match the given pattern for missing product dependencies.");
    const APCommandLineSwitch Command_dsp("dsp", Command_dependencyScanPattern.m_helpText);
    const APCommandLineSwitch Command_fileDependencyScanPattern("fileDependencyScanPattern", "Used with dependencyScanPattern to farther filter the scan.");
    const APCommandLineSwitch Command_fdsp("fdsp", Command_fileDependencyScanPattern.m_helpText);
    const APCommandLineSwitch Command_additionalScanFolders("additionalScanFolders", "Used with dependencyScanPattern to farther filter the scan.");
    const APCommandLineSwitch Command_dependencyScanMaxIteration("dependencyScanMaxIteration", "Used to limit the number of recursive searches per line when running dependencyScanPattern.");
    const APCommandLineSwitch Command_warningLevel("warningLevel", "Configure the error and warning reporting level for AssetProcessor. Pass in 1 for fatal errors, 2 for fatal errors and warnings.");
    const APCommandLineSwitch Command_acceptInput("acceptInput", "Enable external control messaging via the ControlRequestHandler, used with automated tests.");
    const APCommandLineSwitch Command_debugOutput("debugOutput", "When enabled, builders that support it will output debug information as product assets. Used primarily with scene files.");
    const APCommandLineSwitch Command_sortJobsByDBSourceName("sortJobsByDBSourceName", "When enabled, sorts pending jobs with equal priority and dependencies by database source name instead of job ID. Useful for automated tests to process assets in the same order each time.");
    const APCommandLineSwitch Command_truncatefingerprint("truncatefingerprint", "Truncates the fingerprint used for processed assets. Useful if you plan to compress product assets to share on another machine because some compression formats like zip will truncate file mod timestamps.");
    const APCommandLineSwitch Command_help("help", "Displays this message.");
    const APCommandLineSwitch Command_h("h", Command_help.m_helpText);

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

    if (commandLine->HasSwitch(Command_sortJobsByDBSourceName.m_switch))
    {
        m_sortJobsByDBSourceName = true;
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

    if (commandLine->HasSwitch(Command_help.m_switch) || commandLine->HasSwitch(Command_h.m_switch))
    {
        // Other O3DE tools have a more full featured system for registering command flags
        // that includes help output, but right now the AssetProcessor just checks strings
        // via HasSwitch. This means this help output has to be updated manually.
        AZ_TracePrintf("AssetProcessor", "Asset Processor Command Line Flags:\n");
        AZ_TracePrintf("AssetProcessor", "\t%s : %s\n", Command_waitOnLaunch.m_switch, Command_waitOnLaunch.m_helpText);
        AZ_TracePrintf("AssetProcessor", "\t%s : %s\n", Command_zeroAnalysisMode.m_switch, Command_zeroAnalysisMode.m_helpText);
        AZ_TracePrintf("AssetProcessor", "\t%s : %s\n", Command_enableQueryLogging.m_switch, Command_enableQueryLogging.m_helpText);
        AZ_TracePrintf("AssetProcessor", "\t%s : %s\n", Command_dependencyScanPattern.m_switch, Command_dependencyScanPattern.m_helpText);
        AZ_TracePrintf("AssetProcessor", "\t%s : %s\n", Command_dsp.m_switch, Command_dsp.m_helpText);
        AZ_TracePrintf("AssetProcessor", "\t%s : %s\n", Command_fileDependencyScanPattern.m_switch, Command_fileDependencyScanPattern.m_helpText);
        AZ_TracePrintf("AssetProcessor", "\t%s : %s\n", Command_fdsp.m_switch, Command_fdsp.m_helpText);
        AZ_TracePrintf("AssetProcessor", "\t%s : %s\n", Command_additionalScanFolders.m_switch, Command_additionalScanFolders.m_helpText);
        AZ_TracePrintf("AssetProcessor", "\t%s : %s\n", Command_dependencyScanMaxIteration.m_switch, Command_dependencyScanMaxIteration.m_helpText);
        AZ_TracePrintf("AssetProcessor", "\t%s : %s\n", Command_warningLevel.m_switch, Command_warningLevel.m_helpText);
        AZ_TracePrintf("AssetProcessor", "\t%s : %s\n", Command_acceptInput.m_switch, Command_acceptInput.m_helpText);
        AZ_TracePrintf("AssetProcessor", "\t%s : %s\n", Command_debugOutput.m_switch, Command_debugOutput.m_helpText);
        AZ_TracePrintf("AssetProcessor", "\t%s : %s\n", Command_sortJobsByDBSourceName.m_switch, Command_sortJobsByDBSourceName.m_helpText);
        AZ_TracePrintf("AssetProcessor", "\t%s : %s\n", Command_truncatefingerprint.m_switch, Command_truncatefingerprint.m_helpText);
        AZ_TracePrintf("AssetProcessor", "\t%s : %s\n", Command_help.m_switch, Command_help.m_helpText);
        AZ_TracePrintf("AssetProcessor", "\t%s : %s\n", Command_h.m_switch, Command_h.m_helpText);
        AZ_TracePrintf("AssetProcessor", "\tregset : set the given registry key to the given value.\n");
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

    if (m_sortJobsByDBSourceName)
    {
        m_rcController->SetQueueSortOnDBSourceName();
    }

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

void ApplicationManagerBase::InitFileMonitor()
{
    m_folderWatches.reserve(m_platformConfiguration->GetScanFolderCount());
    m_watchHandles.reserve(m_platformConfiguration->GetScanFolderCount());
    for (int folderIdx = 0; folderIdx < m_platformConfiguration->GetScanFolderCount(); ++folderIdx)
    {
        const AssetProcessor::ScanFolderInfo& info = m_platformConfiguration->GetScanFolderAt(folderIdx);

        FolderWatchCallbackEx* newFolderWatch = new FolderWatchCallbackEx(info.ScanPath(), "", info.RecurseSubFolders());
        // hook folder watcher to assess files on add/modify
        // relevant files will be sent to resource compiler
        QObject::connect(newFolderWatch, &FolderWatchCallbackEx::fileAdded,
            m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::AssessAddedFile);
        QObject::connect(newFolderWatch, &FolderWatchCallbackEx::fileModified,
            m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::AssessModifiedFile);
        QObject::connect(newFolderWatch, &FolderWatchCallbackEx::fileRemoved,
            m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::AssessDeletedFile);

        QObject::connect(newFolderWatch, &FolderWatchCallbackEx::fileAdded, [this](QString path) { m_fileStateCache->AddFile(path); });
        QObject::connect(newFolderWatch, &FolderWatchCallbackEx::fileModified, [this](QString path) { m_fileStateCache->UpdateFile(path); });
        QObject::connect(newFolderWatch, &FolderWatchCallbackEx::fileRemoved, [this](QString path) { m_fileStateCache->RemoveFile(path); });

        QObject::connect(newFolderWatch, &FolderWatchCallbackEx::fileAdded, [](QString path) { AZ::Interface<AssetProcessor::ExcludedFolderCacheInterface>::Get()->FileAdded(path); });

        QObject::connect(newFolderWatch, &FolderWatchCallbackEx::fileAdded,
            m_fileProcessor.get(), &AssetProcessor::FileProcessor::AssessAddedFile);
        QObject::connect(newFolderWatch, &FolderWatchCallbackEx::fileRemoved,
            m_fileProcessor.get(), &AssetProcessor::FileProcessor::AssessDeletedFile);

        m_folderWatches.push_back(AZStd::unique_ptr<FolderWatchCallbackEx>(newFolderWatch));
        m_watchHandles.push_back(m_fileWatcher.AddFolderWatch(newFolderWatch));
    }

    // also hookup monitoring for the cache (output directory)
    QDir cacheRoot;
    if (AssetUtilities::ComputeProjectCacheRoot(cacheRoot))
    {
        FolderWatchCallbackEx* newFolderWatch = new FolderWatchCallbackEx(cacheRoot.absolutePath(), "", true);

        QObject::connect(newFolderWatch, &FolderWatchCallbackEx::fileAdded, [this](QString path) { m_fileStateCache->AddFile(path); });
        QObject::connect(newFolderWatch, &FolderWatchCallbackEx::fileModified, [this](QString path) { m_fileStateCache->UpdateFile(path); });
        QObject::connect(newFolderWatch, &FolderWatchCallbackEx::fileRemoved, [this](QString path) { m_fileStateCache->RemoveFile(path); });

        // we only care about cache root deletions.
        QObject::connect(newFolderWatch, &FolderWatchCallbackEx::fileRemoved,
            m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::AssessDeletedFile);

        m_folderWatches.push_back(AZStd::unique_ptr<FolderWatchCallbackEx>(newFolderWatch));
        m_watchHandles.push_back(m_fileWatcher.AddFolderWatch(newFolderWatch));
    }
}

void ApplicationManagerBase::DestroyFileMonitor()
{
    for (int watchHandle : m_watchHandles)
    {
        m_fileWatcher.RemoveFolderWatch(watchHandle);
    }
    m_folderWatches.resize(0);
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

    bool result = QObject::connect(GetAssetCatalog(), &AssetProcessor::AssetCatalog::SendAssetMessage, connectionAndChangeMessagesThreadContext, forwardMessageFunction, Qt::QueuedConnection);
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
                AssetNotificationMessage message(entry.m_pathRelativeToWatchFolder.toUtf8().constData(), AssetNotificationMessage::JobCompleted, AZ::Data::s_invalidAssetType, entry.m_platformInfo.m_identifier.c_str());
                EBUS_EVENT(AssetProcessor::ConnectionBus, SendPerPlatform, 0, message, QString::fromUtf8(entry.m_platformInfo.m_identifier.c_str()));
            }
            );
    AZ_Assert(result, "Failed to connect to RCController signal");

    result = QObject::connect(GetRCController(), &AssetProcessor::RCController::FileFailed, this,
            [](AssetProcessor::JobEntry entry)
            {
                AssetNotificationMessage message(entry.m_pathRelativeToWatchFolder.toUtf8().constData(), AssetNotificationMessage::JobFailed, AZ::Data::s_invalidAssetType, entry.m_platformInfo.m_identifier.c_str());
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

    qRegisterMetaType<FileChangeInfo>("FileChangeInfo");

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

    AssetBuilderSDK::AssetBuilderBus::Handler::BusConnect();
    AssetProcessor::AssetBuilderRegistrationBus::Handler::BusConnect();
    AssetProcessor::AssetBuilderInfoBus::Handler::BusConnect();
    AZ::Debug::TraceMessageBus::Handler::BusConnect();
    AssetProcessor::DiskSpaceInfoBus::Handler::BusConnect();
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

    bool startedSuccessfully = true;

    if (!PostActivate())
    {
        QuitRequested();
        startedSuccessfully = false;
    }

    AZ_Printf(AssetProcessor::ConsoleChannel, "Asset Processor Batch Processing Started.\n");
    AZ_Printf(AssetProcessor::ConsoleChannel, "-----------------------------------------\n");
    QElapsedTimer allAssetsProcessingTimer;
    allAssetsProcessingTimer.start();
    m_duringStartup = false;
    qApp->exec();

    AZ_Printf(AssetProcessor::ConsoleChannel, "-----------------------------------------\n");
    AZ_Printf(AssetProcessor::ConsoleChannel, "Asset Processor Batch Processing complete\n");
    AZ_Printf(AssetProcessor::ConsoleChannel, "Number of Assets Successfully Processed: %d.\n", ProcessedAssetCount());
    AZ_Printf(AssetProcessor::ConsoleChannel, "Number of Assets Failed to Process: %d.\n", FailedAssetsCount());
    AZ_Printf(AssetProcessor::ConsoleChannel, "Number of Warnings Reported: %d.\n", m_warningCount);
    AZ_Printf(AssetProcessor::ConsoleChannel, "Number of Errors Reported: %d.\n", m_errorCount);
    AZ_Printf(AssetProcessor::ConsoleChannel, "Total Assets Processing Time: %fs\n", allAssetsProcessingTimer.elapsed() / 1000.0f);
    AZ_Printf(AssetProcessor::ConsoleChannel, "Asset Processor Batch Processing Completed.\n");

    RemoveOldTempFolders();
    Destroy();

    return (startedSuccessfully && FailedAssetsCount() == 0);
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

        auto* interface = AZ::Interface<AssetProcessor::ISourceFileRelocation>::Get();

        if(interface)
        {
            auto result = interface->Move(source, destination, previewOnly, allowBrokenDependencies, !leaveEmptyFolders, updateReferences, excludeMetaDataFiles);

            if (result.IsSuccess())
            {
                AssetProcessor::RelocationSuccess success = result.TakeValue();

                // The report can be too long for the AZ_Printf buffer, so split it into individual lines
                AZStd::string report = interface->BuildReport(success.m_relocationContainer, success.m_updateTasks, true, updateReferences);
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

        auto* interface = AZ::Interface<AssetProcessor::ISourceFileRelocation>::Get();

        if (interface)
        {
            auto result = interface->Delete(source, previewOnly, allowBrokenDependencies, !leaveEmptyFolders, excludeMetaDataFiles);

            if (result.IsSuccess())
            {
                AssetProcessor::RelocationSuccess success = result.TakeValue();

                // The report can be too long for the AZ_Printf buffer, so split it into individual lines
                AZStd::string report = interface->BuildReport(success.m_relocationContainer, success.m_updateTasks, false, updateReferences);
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

bool ApplicationManagerBase::InitAssetDatabase()
{
    AzToolsFramework::AssetDatabase::AssetDatabaseRequests::Bus::Handler::BusConnect();

    // create or upgrade the asset database here, so that it is already good for the rest of the application and the rest
    // of the application does not have to worry about a failure to upgrade or create it.
    AssetProcessor::AssetDatabaseConnection database;
    if (!database.OpenDatabase())
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
    // This will cache whether AP is running in server mode or not.
    // It is also important to invoke it here because incase the asset server address is invalid, the error message should get captured in the AP log.
    AssetUtilities::InServerMode();
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
    if (!CheckSufficientDiskSpace(projectCache.absolutePath(), 128 * 1024 * 1024, true))
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

    if (!InitAssetDatabase())
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

    m_isCurrentlyLoadingGems = true;
    if (!ActivateModules())
    {
        // ActivateModules reports any errors it encounters.
        m_isCurrentlyLoadingGems = false;
        return false;
    }

    m_isCurrentlyLoadingGems = false;
    PopulateApplicationDependencies();

    InitAssetProcessorManager();
    AssetBuilderSDK::InitializeSerializationContext();
    AssetBuilderSDK::InitializeBehaviorContext();
    
    InitFileStateCache();
    InitFileProcessor();

    InitAssetCatalog();
    InitFileMonitor();
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
    return true;
}

bool ApplicationManagerBase::PostActivate()
{
    m_connectionManager->LoadConnections();

    InitializeInternalBuilders();
    if (!InitializeExternalBuilders())
    {
        AZ_Error("AssetProcessor", false, "AssetProcessor is closing. Failed to initialize and load all the external builders. Please ensure that Builders_Temp directory is not read-only. Please see log for more information.\n");
        return false;
    }

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

bool ApplicationManagerBase::InitializeExternalBuilders()
{
    AssetProcessor::AssetProcessorStatusEntry entry(AssetProcessor::AssetProcessorStatus::Initializing_Builders);
    Q_EMIT AssetProcessorStatusChanged(entry);
    QCoreApplication::processEvents(QEventLoop::AllEvents);


    // Get the list of external build modules (full paths)
    QStringList fileList;
    GetExternalBuilderFileList(fileList);

    for (const QString& filePath : fileList)
    {
        if (QLibrary::isLibrary(filePath))
        {
            AssetProcessor::ExternalModuleAssetBuilderInfo* externalAssetBuilderInfo = new AssetProcessor::ExternalModuleAssetBuilderInfo(filePath);
            AssetProcessor::AssetBuilderType assetBuilderType = externalAssetBuilderInfo->Load();
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "AssetProcessor is loading library %s\n", filePath.toUtf8().data());
            if (assetBuilderType == AssetProcessor::AssetBuilderType::None)
            {
                AZ_Warning(AssetProcessor::DebugChannel, false, "Non-builder DLL was found in Builders directory %s, skipping. \n", filePath.toUtf8().data());
                delete externalAssetBuilderInfo;
                continue;
            }

            if (assetBuilderType == AssetProcessor::AssetBuilderType::Invalid)
            {
                AZ_Warning(AssetProcessor::DebugChannel, false, "AssetProcessor was not able to load the library: %s\n", filePath.toUtf8().data());
                delete externalAssetBuilderInfo;
                return false;
            }

            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Initializing and registering builder %s\n", externalAssetBuilderInfo->GetName().toUtf8().data());

            m_currentExternalAssetBuilder = externalAssetBuilderInfo;

            externalAssetBuilderInfo->Initialize();

            m_currentExternalAssetBuilder = nullptr;

            m_externalAssetBuilders.push_back(externalAssetBuilderInfo);
        }
    }

    // Also init external builders which may be inside of Gems
    AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
        &AzToolsFramework::ToolsApplicationRequests::CreateAndAddEntityFromComponentTags,
        AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }), "AssetBuilders Entity");

    return true;
}

bool ApplicationManagerBase::WaitForBuilderExit(AzFramework::ProcessWatcher* processWatcher, AssetBuilderSDK::JobCancelListener* jobCancelListener, AZ::u32 processTimeoutLimitInSeconds)
{
    AZ::u32 exitCode = 0;
    bool finishedOK = false;
    QElapsedTimer ticker;
    ProcessCommunicatorTracePrinter tracer(processWatcher->GetCommunicator(), "AssetBuilder");

    ticker.start();

    while (!finishedOK)
    {
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(s_MaximumSleepTimeMS));

        tracer.Pump();

        if (ticker.elapsed() > processTimeoutLimitInSeconds * 1000 || (jobCancelListener && jobCancelListener->IsCancelled()))
        {
            break;
        }

        if (!processWatcher->IsProcessRunning(&exitCode))
        {
            finishedOK = true; // we either cant wait for it, or it finished.
            break;
        }
    }

    tracer.Pump(); // empty whats left if possible.

    if (processWatcher->IsProcessRunning(&exitCode))
    {
        processWatcher->TerminateProcess(1);
    }

    if (exitCode != 0)
    {
        AZ_Error(AssetProcessor::ConsoleChannel, false, "AssetBuilder exited with error code %d", exitCode);
        return false;
    }
    else if (jobCancelListener && jobCancelListener->IsCancelled())
    {
        AZ_TracePrintf(AssetProcessor::DebugChannel, "AssetBuilder was terminated. There was a request to cancel the job.\n");
        return false;
    }
    else if (!finishedOK)
    {
        AZ_Error(AssetProcessor::ConsoleChannel, false, "AssetBuilder failed to terminate within %d seconds", processTimeoutLimitInSeconds);
        return false;
    }

    return true;
}

void ApplicationManagerBase::RegisterBuilderInformation(const AssetBuilderSDK::AssetBuilderDesc& builderDesc)
{
    // Create Job Function validation
    AZ_Error(AssetProcessor::ConsoleChannel,
        builderDesc.m_createJobFunction,
        "Create Job Function (m_createJobFunction) for %s builder is empty.\n",
        builderDesc.m_name.c_str());

    // Process Job Function validation
    AZ_Error(AssetProcessor::ConsoleChannel,
        builderDesc.m_processJobFunction,
        "Process Job Function (m_processJobFunction) for %s builder is empty.\n",
        builderDesc.m_name.c_str());

    // Bus ID validation
    AZ_Error(AssetProcessor::ConsoleChannel,
        !builderDesc.m_busId.IsNull(),
        "Bus ID for %s builder is empty.\n",
        builderDesc.m_name.c_str());

    // This is an external builder registering, we will want to track its builder desc since it can register multiple ones
    AZStd::string builderFilePath;
    if (m_currentExternalAssetBuilder)
    {
        m_currentExternalAssetBuilder->RegisterBuilderDesc(builderDesc.m_busId);
        builderFilePath = m_currentExternalAssetBuilder->GetModuleFullPath().toUtf8().data();
    }

    AssetBuilderSDK::AssetBuilderDesc modifiedBuilderDesc = builderDesc;
    // Allow for overrides defined in a BuilderConfig.ini file to update our code defined default values
    AssetProcessor::BuilderConfigurationRequestBus::Broadcast(&AssetProcessor::BuilderConfigurationRequests::UpdateBuilderDescriptor, builderDesc.m_name, modifiedBuilderDesc);

    if (builderDesc.IsExternalBuilder())
    {
        // We're going to override the createJob function so we can run it externally in AssetBuilder, rather than having it run inside the AP
        modifiedBuilderDesc.m_createJobFunction = [builderFilePath](const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
            {
                AssetProcessor::BuilderRef builderRef;
                AssetProcessor::BuilderManagerBus::BroadcastResult(builderRef, &AssetProcessor::BuilderManagerBusTraits::GetBuilder);

                if (builderRef)
                {
                    int retryCount = 0;
                    AssetProcessor::BuilderRunJobOutcome result;

                    do
                    {
                        retryCount++;
                        result = builderRef->RunJob<AssetBuilderSDK::CreateJobsNetRequest, AssetBuilderSDK::CreateJobsNetResponse>(request, response, s_MaximumCreateJobsTimeSeconds, "create", builderFilePath, nullptr);
                    } while (result == AssetProcessor::BuilderRunJobOutcome::LostConnection && retryCount <= AssetProcessor::RetriesForJobNetworkError);
                }
                else
                {
                    AZ_Error("AssetProcessor", false, "Failed to retrieve a valid builder to process job");
                }
            };

        // Also override the processJob function to run externally
        modifiedBuilderDesc.m_processJobFunction = [builderFilePath](const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
            {
                AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);

                AssetProcessor::BuilderRef builderRef;
                AssetProcessor::BuilderManagerBus::BroadcastResult(builderRef, &AssetProcessor::BuilderManagerBusTraits::GetBuilder);

                if (builderRef)
                {
                    int retryCount = 0;
                    AssetProcessor::BuilderRunJobOutcome result;

                    do
                    {
                        retryCount++;
                        result = builderRef->RunJob<AssetBuilderSDK::ProcessJobNetRequest, AssetBuilderSDK::ProcessJobNetResponse>(request, response, s_MaximumProcessJobsTimeSeconds, "process", builderFilePath, &jobCancelListener, request.m_tempDirPath);
                    } while (result == AssetProcessor::BuilderRunJobOutcome::LostConnection && retryCount <= AssetProcessor::RetriesForJobNetworkError);
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

bool ApplicationManagerBase::CheckSufficientDiskSpace(const QString& savePath, qint64 requiredSpace, bool shutdownIfInsufficient)
{
    if (!QDir(savePath).exists())
    {
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
