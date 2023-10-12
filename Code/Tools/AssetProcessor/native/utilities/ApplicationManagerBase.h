/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include <native/FileWatcher/FileWatcherBase.h>
#include <native/utilities/ApplicationManager.h>
#include <native/utilities/AssetBuilderInfo.h>
#include <native/utilities/BuilderManager.h>
#include <native/utilities/UuidManager.h>
#include <QtGui/qwindowdefs.h>
#endif

namespace AzToolsFramework
{
    class ProcessWatcher;
    class Ticker;
}

namespace AssetProcessor
{
    class AssetCatalog;
    class AssetProcessorManager;
    class AssetRequestHandler;
    class AssetScanner;
    class AssetServerHandler;
    class BuilderConfigurationManager;
    class BuilderManager;
    class ExternalModuleAssetBuilderInfo;
    class FileProcessor;
    class FileStateBase;
    class FileStateCache;
    class InternalAssetBuilderInfo;
    class PlatformConfiguration;
    class RCController;
    class SettingsRegistryBuilder;
}

class ApplicationServer;
class ConnectionManager;
class ControlRequestHandler;

class ApplicationManagerBase
    : public ApplicationManager
    , public AssetBuilderSDK::AssetBuilderBus::Handler
    , public AssetProcessor::AssetBuilderInfoBus::Handler
    , public AssetProcessor::AssetBuilderRegistrationBus::Handler
    , public AZ::Debug::TraceMessageBus::Handler
    , protected AzToolsFramework::AssetDatabase::AssetDatabaseRequests::Bus::Handler
    , public AZ::Interface<AssetProcessor::IDiskSpaceInfo>::Registrar
    , protected AzToolsFramework::SourceControlNotificationBus::Handler
    , public AssetProcessor::MessageInfoBus::Handler
{
    Q_OBJECT
public:
    ApplicationManagerBase(int* argc, char*** argv, QObject* parent = nullptr);
    ApplicationManagerBase(int* argc, char*** argv, AZ::ComponentApplicationSettings componentAppSettings);
    ApplicationManagerBase(int* argc, char*** argv, QObject* parent, AZ::ComponentApplicationSettings componentAppSettings);
    virtual ~ApplicationManagerBase();
    ApplicationManager::BeforeRunStatus BeforeRun() override;
    void Destroy() override;
    bool Run() override;
    void HandleFileRelocation() const;
    bool Activate() override;
    bool PostActivate() override;
    void Reflect() override;

    AssetProcessor::PlatformConfiguration* GetPlatformConfiguration() const;

    AssetProcessor::AssetProcessorManager* GetAssetProcessorManager() const;

    AssetProcessor::AssetScanner* GetAssetScanner() const;

    AssetProcessor::RCController* GetRCController() const;

    ConnectionManager* GetConnectionManager() const;
    ApplicationServer* GetApplicationServer() const;

    int ProcessedAssetCount() const;
    int FailedAssetsCount() const;
    void ResetProcessedAssetCount();
    void ResetFailedAssetCount();

    //! AssetBuilderSDK::AssetBuilderBus Interface
    void RegisterBuilderInformation(const AssetBuilderSDK::AssetBuilderDesc& builderDesc) override;
    void RegisterComponentDescriptor(AZ::ComponentDescriptor* descriptor) override;
    void BuilderLog(const AZ::Uuid& builderId, const char* message, ...) override;
    void BuilderLogV(const AZ::Uuid& builderId, const char* message, va_list list) override;
    bool FindBuilderInformation(const AZ::Uuid& builderGuid, AssetBuilderSDK::AssetBuilderDesc& descriptionOut) override;

    //! AssetBuilderSDK::InternalAssetBuilderBus Interface
    void UnRegisterBuilderDescriptor(const AZ::Uuid& builderId) override;

    //! AssetProcessor::AssetBuilderInfoBus Interface
    void GetMatchingBuildersInfo(const AZStd::string& assetPath, AssetProcessor::BuilderInfoList& builderInfoList) override;
    void GetAllBuildersInfo(AssetProcessor::BuilderInfoList& builderInfoList) override;

    //! TraceMessageBus Interface
    bool OnError(const char* window, const char* message) override;

    //! IDiskSpaceInfo Interface
    bool CheckSufficientDiskSpace(qint64 requiredSpace, bool shutdownIfInsufficient) override;

    //! AzFramework::SourceControlNotificationBus::Handler
    void ConnectivityStateChanged(const AzToolsFramework::SourceControlState newState) override;

    //! MessageInfoBus::Handler
    void OnBuilderRegistrationFailure() override;

    void RemoveOldTempFolders();

    void Rescan();

    void FastScan();

    bool IsAssetProcessorManagerIdle() const override;
    bool CheckFullIdle();

    // Used to track AP command line checks, so the help can be easily printed.
    struct APCommandLineSwitch
    {
        APCommandLineSwitch(AZStd::vector<APCommandLineSwitch>& commandLineInfo, const char* switchTitle, const char* helpText)
            : m_switch(switchTitle)
            , m_helpText(helpText)
        {
            commandLineInfo.push_back(*this);
        }
        const char* m_switch;
        const char* m_helpText;
    };

    virtual WId GetWindowId() const;

Q_SIGNALS:
    void CheckAssetProcessorManagerIdleState();
    void ConnectionStatusMsg(QString message);
    void SourceControlReady();
    void OnBuildersRegistered();
    void AssetProcesserManagerIdleStateChange(bool isIdle);
    void FullIdle(bool isIdle);
public Q_SLOTS:
    void OnAssetProcessorManagerIdleState(bool isIdle);

protected:
    virtual void InitAssetProcessorManager(AZStd::vector<APCommandLineSwitch>& commandLineInfo);//Deletion of assetProcessor Manager will be handled by the ThreadController
    virtual void InitAssetCatalog(); // Deletion of AssetCatalog will be handled when the ThreadController is deleted by the base ApplicationManager
    virtual void ConnectAssetCatalog();
    virtual void InitRCController();
    virtual void DestroyRCController();
    virtual void InitAssetScanner();
    virtual void DestroyAssetScanner();
    virtual bool InitPlatformConfiguration();
    virtual void DestroyPlatformConfiguration();
    virtual void InitFileMonitor(AZStd::unique_ptr<FileWatcherBase> fileWatcher);
    virtual void DestroyFileMonitor();
    virtual bool InitBuilderConfiguration();
    virtual void InitControlRequestHandler();
    virtual void DestroyControlRequestHandler();
    virtual bool InitApplicationServer() = 0;
    void DestroyApplicationServer();
    virtual void InitConnectionManager();
    void DestroyConnectionManager();
    void InitAssetRequestHandler(AssetProcessor::AssetRequestHandler* assetRequestHandler);
    virtual void InitFileStateCache();
    virtual void InitUuidManager();
    void CreateQtApplication() override;

    bool InitializeInternalBuilders();
    void InitBuilderManager();
    void ShutdownBuilderManager();
    bool InitAssetDatabase(bool ignoreFutureAssetDBVersionError);
    void ShutDownAssetDatabase();
    void InitAssetServerHandler();
    void DestroyAssetServerHandler();
    void InitFileProcessor();
    void ShutDownFileProcessor();
    virtual void InitSourceControl() = 0;
    void InitInputThread();
    void InputThread();

    void HandleCommandLineHelp(AZStd::vector<APCommandLineSwitch>& commandLineInfo);

    // Give an opportunity to derived classes to make connections before the application server starts listening
    virtual void MakeActivationConnections() {}
    virtual bool GetShouldExitOnIdle() const = 0;
    virtual void TryScanProductDependencies() {}
    virtual void TryHandleFileRelocation() {}
    // IMPLEMENTATION OF -------------- AzToolsFramework::AssetDatabase::AssetDatabaseRequests::Bus::Listener
    bool GetAssetDatabaseLocation(AZStd::string& location) override;
    // ------------------------------------------------------------

    AssetProcessor::AssetCatalog* GetAssetCatalog() const { return m_assetCatalog; }

    bool CheckReprocessFileList();

    ApplicationServer* m_applicationServer = nullptr;
    ConnectionManager* m_connectionManager = nullptr;

    // keep track of the critical loading point where we are loading other dlls so the error messages can be better.
    bool m_isCurrentlyLoadingGems = false;

public Q_SLOTS:
    void OnActiveJobsCountChanged(unsigned int count);
protected Q_SLOTS:
    void CheckForIdle();

protected:
    int m_processedAssetCount = 0;
    int m_warningCount = 0;
    int m_errorCount = 0;
    int m_remainingAssetsToFinalize = 0;
    AZStd::set<AZStd::string> m_failedAssets;
    bool m_AssetProcessorManagerIdleState = false;
    bool m_sourceControlReady = false;
    bool m_fullIdle = false;

    AZStd::unique_ptr<FileWatcherBase> m_fileWatcher;
    AssetProcessor::PlatformConfiguration* m_platformConfiguration = nullptr;
    AssetProcessor::AssetProcessorManager* m_assetProcessorManager = nullptr;
    AssetProcessor::AssetCatalog* m_assetCatalog = nullptr;
    AssetProcessor::AssetScanner* m_assetScanner = nullptr;
    AssetProcessor::RCController* m_rcController = nullptr;
    AssetProcessor::AssetRequestHandler* m_assetRequestHandler = nullptr;
    AssetProcessor::BuilderManager* m_builderManager = nullptr;
    AssetProcessor::AssetServerHandler* m_assetServerHandler = nullptr;
    ControlRequestHandler* m_controlRequestHandler = nullptr;

    AZStd::unique_ptr<AssetProcessor::FileStateBase> m_fileStateCache;
    AZStd::unique_ptr<AssetProcessor::FileProcessor> m_fileProcessor;
    AZStd::unique_ptr<AssetProcessor::BuilderConfigurationManager> m_builderConfig;
    AZStd::unique_ptr<AssetProcessor::UuidManager> m_uuidManager;

    // The internal builders
    AZStd::shared_ptr<AssetProcessor::InternalRecognizerBasedBuilder> m_internalBuilder;
    AZStd::shared_ptr<AssetProcessor::SettingsRegistryBuilder> m_settingsRegistryBuilder;

    bool m_builderRegistrationComplete = false;

    // Builder description map based on the builder id
    AZStd::unordered_map<AZ::Uuid, AssetBuilderSDK::AssetBuilderDesc> m_builderDescMap;

    // Lookup for builder ids based on the name.  The builder name must be unique
    AZStd::unordered_map<AZStd::string, AZ::Uuid> m_builderNameToId;

    // Builder pattern matchers to used to locate the builder descriptors that match a pattern
    AZStd::list<AssetUtilities::BuilderFilePatternMatcher> m_matcherBuilderPatterns;

    // Collection of all the external module builders
    AZStd::list<AssetProcessor::ExternalModuleAssetBuilderInfo*>    m_externalAssetBuilders;

    AssetProcessor::ExternalModuleAssetBuilderInfo* m_currentExternalAssetBuilder = nullptr;

    QAtomicInt m_connectionsAwaitingAssetCatalogSave = 0;
    int m_remainingAPMJobs = 0;
    bool m_assetProcessorManagerIsReady = false;

    unsigned int m_highestConnId = 0;
    AzToolsFramework::Ticker* m_ticker = nullptr; // for ticking the tickbus.

    QList<QMetaObject::Connection> m_connectionsToRemoveOnShutdown;
    QString m_dependencyScanPattern;
    QString m_fileDependencyScanPattern;
    QString m_reprocessFileList;
    QStringList m_filesToReprocess;
    AZStd::vector<AZStd::string> m_dependencyAddtionalScanFolders;
    int m_dependencyScanMaxIteration = AssetProcessor::MissingDependencyScanner::DefaultMaxScanIteration; // The maximum number of times to recurse when scanning a file for missing dependencies.
};
