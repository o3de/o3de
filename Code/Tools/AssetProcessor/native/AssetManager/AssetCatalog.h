/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QStringList>
#include <QHash>
#include <QDir>
#include "native/AssetDatabase/AssetDatabase.h"
#include "native/assetprocessor.h"
#include "native/utilities/AssetUtilEBusHelper.h"
#include "native/utilities/PlatformConfiguration.h"
#include <AzFramework/Asset/AssetRegistry.h>
#include <QMutex>
#include <QMultiMap>
#include <AzCore/IO/SystemFile.h>
#include <AzToolsFramework/ToolsComponents/ToolsAssetCatalogBus.h>
#endif

#include "AssetRequestHandler.h"

namespace AzFramework
{
    class AssetRegistry;
    namespace AssetSystem
    {
        class AssetNotificationMessage;
    }
}

namespace AssetProcessor
{
    class AssetDatabaseConnection;

    class AssetCatalog 
        : public QObject
        , private AssetRegistryRequestBus::Handler
        , private AzToolsFramework::AssetSystemRequestBus::Handler
        , private AzToolsFramework::ToolsAssetSystemBus::Handler
        , private AZ::Data::AssetCatalogRequestBus::Handler
    {
        using NetworkRequestID = AssetProcessor::NetworkRequestID;
        using BaseAssetProcessorMessage = AzFramework::AssetSystem::BaseAssetProcessorMessage;
        Q_OBJECT;
    
    public:
        AssetCatalog(QObject* parent, AssetProcessor::PlatformConfiguration* platformConfiguration);
        virtual ~AssetCatalog();

    Q_SIGNALS:
        // outgoing message to the network
        void SendAssetMessage(AzFramework::AssetSystem::AssetNotificationMessage message);
        void AsyncAssetCatalogStatusResponse(AssetCatalogStatus status);

    public Q_SLOTS:
        // incoming message from the AP
        void OnAssetMessage(AzFramework::AssetSystem::AssetNotificationMessage message);
        void OnDependencyResolved(const AZ::Data::AssetId& assetId, const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry& entry);

        void SaveRegistry_Impl();
        virtual AzFramework::AssetSystem::GetUnresolvedDependencyCountsResponse HandleGetUnresolvedDependencyCountsRequest(MessageData<AzFramework::AssetSystem::GetUnresolvedDependencyCountsRequest> messageData);
        virtual void HandleSaveAssetCatalogRequest(MessageData<AzFramework::AssetSystem::SaveAssetCatalogRequest> messageData);
        void BuildRegistry();
        void OnSourceQueued(AZ::Uuid sourceUuid, AZ::Uuid legacyUuid, QString rootPath, QString relativeFilePath);
        void OnSourceFinished(AZ::Uuid sourceUuid, AZ::Uuid legacyUuid);
        void AsyncAssetCatalogStatusRequest();
        
    protected:

        //////////////////////////////////////////////////////////////////////////
        // AssetRegistryRequestBus::Handler overrides
        int SaveRegistry() override;
        void ValidatePreLoadDependency() override;
        //////////////////////////////////////////////////////////////////////////

        void RegistrySaveComplete(int assetCatalogVersion, bool allCatalogsSaved);

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::AssetSystem::AssetSystemRequestBus::Handler overrides
        bool GetRelativeProductPathFromFullSourceOrProductPath(const AZStd::string& fullPath, AZStd::string& relativeProductPath) override;

        //! Given a partial or full source file path, respond with its relative path and the watch folder it is relative to.
        //! The input source path does not need to exist, so this can be used for new files that haven't been saved yet.
        bool GenerateRelativeSourcePath(
            const AZStd::string& sourcePath, AZStd::string& relativePath, AZStd::string& watchFolder) override;

        bool GetFullSourcePathFromRelativeProductPath(const AZStd::string& relPath, AZStd::string& fullSourcePath) override;
        bool GetAssetInfoById(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType, const AZStd::string& platformName, AZ::Data::AssetInfo& assetInfo, AZStd::string& rootFilePath) override;
        bool GetSourceInfoBySourcePath(const char* sourcePath, AZ::Data::AssetInfo& assetInfo, AZStd::string& watchFolder) override;
        bool GetSourceInfoBySourceUUID(const AZ::Uuid& sourceUuid, AZ::Data::AssetInfo& assetInfo, AZStd::string& watchFolder) override;
        bool GetScanFolders(AZStd::vector<AZStd::string>& scanFolders) override;
        bool GetAssetSafeFolders(AZStd::vector<AZStd::string>& assetSafeFolders) override;
        bool IsAssetPlatformEnabled(const char* platform) override;
        int GetPendingAssetsForPlatform(const char* platform) override;
        bool GetAssetsProducedBySourceUUID(const AZ::Uuid& sourceUuid, AZStd::vector<AZ::Data::AssetInfo>& productsAssetInfo) override;
        ////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////
        // AssetCatalogRequestBus overrides
        AZStd::string GetAssetPathById(const AZ::Data::AssetId& id) override;
        AZ::Data::AssetId GetAssetIdByPath(const char* path, const AZ::Data::AssetType& typeToRegister, bool autoRegisterIfNotFound) override;
        AZ::Data::AssetInfo GetAssetInfoById(const AZ::Data::AssetId& id) override;
        AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> GetDirectProductDependencies(const AZ::Data::AssetId& id) override;
        AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> GetAllProductDependencies(const AZ::Data::AssetId& id) override;
        AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> GetLoadBehaviorProductDependencies(
            const AZ::Data::AssetId& id, AZStd::unordered_set<AZ::Data::AssetId>& noloadSet,
            AZ::Data::PreloadAssetListType& preloadAssetList) override;
        ////////////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::ToolsAssetSystemBus::Handler
        void RegisterSourceAssetType(const AZ::Data::AssetType& assetType, const char* assetFileFilter) override;
        void UnregisterSourceAssetType(const AZ::Data::AssetType& assetType) override;
        //////////////////////////////////////////////////////////////////////////

        //! given some absolute path, please respond with its relative product path.  For now, this will be a
        //! string like 'textures/blah.tif' (we don't care about extensions), but eventually, this will
        //! be an actual asset UUID.
        void ProcessGetRelativeProductPathFromFullSourceOrProductPathRequest(const AZStd::string& fullPath, AZStd::string& relativeProductPath);

        //! This function helps in determining the full product path of an relative product path.
        //! In the future we will be sending an asset UUID to this function to request for full path.
        void ProcessGetFullSourcePathFromRelativeProductPathRequest(const AZStd::string& relPath, AZStd::string& fullSourcePath);

        //! Gets the source file info for an Asset by checking the DB first and the APM queue second
        bool GetSourceFileInfoFromAssetId(const AZ::Data::AssetId &assetId, AZStd::string& watchFolder, AZStd::string& relativePath);
        
        //! Gets the product AssetInfo based on a platform and assetId.  If you specify a null or empty platform the current or first available will be used.
        AZ::Data::AssetInfo GetProductAssetInfo(const char* platformName, const AZ::Data::AssetId& id);
        
        //! GetAssetInfo that tries to figure out if the asset is a product or source so it can return info about the product or source respectively
        bool GetAssetInfoByIdOnly(const AZ::Data::AssetId& id, const AZStd::string& platformName, AZ::Data::AssetInfo& assetInfo, AZStd::string& rootFilePath);
        
        //! Checks in the currently-in-queue assets list for info on an asset (by source Id)
        bool GetQueuedAssetInfoById(const AZ::Uuid& guid, AZStd::string& watchFolder, AZStd::string& relativePath);

        //! Checks in the currently-in-queue assets list for info on an asset (by source name)
        bool GetQueuedAssetInfoByRelativeSourceName(const char* sourceName, AZ::Data::AssetInfo& assetInfo, AZStd::string& watchFolder);

        //! Gets the source info for a source that is not in the DB or APM queue
        bool GetUncachedSourceInfoFromDatabaseNameAndWatchFolder(const char* sourceDatabasePath, const char* watchFolder, AZ::Data::AssetInfo& assetInfo);

        bool ConnectToDatabase();

        bool CheckValidatedAssets(AZ::Data::AssetId assetId, const QString& platform);

        //! For lookups that don't provide a specific platform, provide a default platform to use.
        QString GetDefaultAssetPlatform();

        AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> GetAllProductDependenciesFilter(
            const AZ::Data::AssetId& id,
            const AZStd::unordered_set<AZ::Data::AssetId>& exclusionList,
            const AZStd::vector<AZStd::string>& wildcardPatternExclusionList) override;

        bool DoesAssetIdMatchWildcardPattern(const AZ::Data::AssetId& assetId, const AZStd::string& wildcardPattern) override;

        void AddAssetDependencies(
            const AZ::Data::AssetId& searchAssetId,
            AZStd::unordered_set<AZ::Data::AssetId>& assetSet,
            AZStd::vector<AZ::Data::ProductDependency>& dependencyList,
            const AZStd::unordered_set<AZ::Data::AssetId>& exclusionList,
            const AZStd::vector<AZStd::string>& wildcardPatternExclusionList,
            AZ::Data::PreloadAssetListType& preloadAssetList);

        //! List of AssetTypes that should return info for the source instead of the product
        AZStd::unordered_set<AZ::Data::AssetType> m_sourceAssetTypes;
        AZStd::unordered_map<AZStd::string, AZ::Data::AssetType> m_sourceAssetTypeFilters;
        AZStd::mutex m_sourceAssetTypesMutex;

        //! Used to protect access to the database connection, only one thread can use it at a time
        AZStd::mutex m_databaseMutex;

        struct SourceInfo
        {
            QString m_watchFolder;
            QString m_sourceName;
        };

        AZStd::mutex m_sourceUUIDToSourceNameMapMutex;
        using SourceUUIDToSourceNameMap = AZStd::unordered_map<AZ::Uuid, SourceInfo>;
        using SourceNameToSourceUuidMap = AZStd::unordered_map<AZStd::string, AZ::Uuid>;

        SourceUUIDToSourceNameMap m_sourceUUIDToSourceNameMap; // map of uuids to source file names for assets that are currently in the processing queue
        SourceNameToSourceUuidMap m_sourceNameToSourceUUIDMap;

        QMutex m_registriesMutex;
        QHash<QString, AzFramework::AssetRegistry> m_registries; // per platform.
        AssetProcessor::PlatformConfiguration* m_platformConfig;
        QStringList m_platforms;
        AZStd::unique_ptr<AssetDatabaseConnection> m_db;
        QDir m_cacheRoot;

        bool m_registryBuiltOnce;
        bool m_catalogIsDirty = true;
        bool m_currentlySavingCatalog = false;
        bool m_currentlyValidatingPreloadDependency = false;
        int m_currentRegistrySaveVersion = 0;
        QMutex m_savingRegistryMutex;
        QMultiMap<int, AssetProcessor::NetworkRequestID> m_queuedSaveCatalogRequest;
        AZStd::vector<AZStd::pair<AZ::Data::AssetId, QString>> m_preloadAssetList;
        AZStd::unordered_multimap<AZ::Data::AssetId, QString> m_cachedNoPreloadDependenyAssetList;

        AZStd::vector<char> m_saveBuffer; // so that we don't realloc all the time

        QDir m_cacheRootDir;
    };
}
