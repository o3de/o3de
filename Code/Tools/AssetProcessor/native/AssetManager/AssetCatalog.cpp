/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "native/AssetManager/AssetCatalog.h"

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/string/wildcard.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/FileTag/FileTagBus.h>
#include <AzFramework/FileTag/FileTag.h>
#include <AzToolsFramework/API/AssetDatabaseBus.h>

#include <QElapsedTimer>
#include "PathDependencyManager.h"
#include <utilities/UuidManager.h>

namespace AssetProcessor
{
    AssetCatalog::AssetCatalog(QObject* parent, AssetProcessor::PlatformConfiguration* platformConfiguration)
        : QObject(parent)
        , m_platformConfig(platformConfiguration)
        , m_registryBuiltOnce(false)
        , m_registriesMutex(QMutex::Recursive)
    {

        for (const AssetBuilderSDK::PlatformInfo& info : m_platformConfig->GetEnabledPlatforms())
        {
            if (info.m_identifier == AssetBuilderSDK::CommonPlatformName)
            {
                // Currently the Common platform is not supported as a product asset platform
                continue;
            }

            m_platforms.push_back(QString::fromUtf8(info.m_identifier.c_str()));
        }

        [[maybe_unused]] bool computedCacheRoot = AssetUtilities::ComputeProjectCacheRoot(m_cacheRoot);
        AZ_Assert(computedCacheRoot, "Could not compute cache root for AssetCatalog");

        // save 30mb for this.  Really large projects do get this big (and bigger)
        // if you don't do this, things get fragmented very fast.
        m_saveBuffer.reserve(1024 * 1024 * 30);

        AssetUtilities::ComputeProjectPath();

        if (!ConnectToDatabase())
        {
            AZ_Error("AssetCatalog", false, "Failed to connect to sqlite database");
        }

        AssetRegistryRequestBus::Handler::BusConnect();
        AzToolsFramework::AssetSystemRequestBus::Handler::BusConnect();
        AzToolsFramework::ToolsAssetSystemBus::Handler::BusConnect();
        AZ::Data::AssetCatalogRequestBus::Handler::BusConnect();
    }

    AssetCatalog::~AssetCatalog()
    {
        AzToolsFramework::ToolsAssetSystemBus::Handler::BusDisconnect();
        AzToolsFramework::AssetSystemRequestBus::Handler::BusDisconnect();
        AssetRegistryRequestBus::Handler::BusDisconnect();
        AZ::Data::AssetCatalogRequestBus::Handler::BusDisconnect();
        SaveRegistry_Impl();
    }

    void AssetCatalog::OnAssetMessage(AzFramework::AssetSystem::AssetNotificationMessage message)
    {
        using namespace AzFramework::AssetSystem;
        if (message.m_type == AssetNotificationMessage::AssetChanged)
        {
            //get the full product path to determine file size
            AZ::Data::AssetInfo assetInfo;
            assetInfo.m_assetId = message.m_assetId;
            assetInfo.m_assetType = message.m_assetType;
            assetInfo.m_relativePath = message.m_data.c_str();
            assetInfo.m_sizeBytes = message.m_sizeBytes;
            QString assetPlatform{ QString::fromUtf8(message.m_platform.c_str()) };

            AZ_Assert(assetInfo.m_assetId.IsValid(), "AssetID is not valid!!!");
            AZ_Assert(!assetInfo.m_relativePath.empty(), "Product path is empty");
            AZ_Assert(!assetPlatform.isEmpty(), "Product platform is empty");

            m_catalogIsDirty = true;
            {
                QMutexLocker locker(&m_registriesMutex);
                m_registries[message.m_platform.c_str()].RegisterAsset(assetInfo.m_assetId, assetInfo);
                m_registries[assetPlatform].SetAssetDependencies(message.m_assetId, message.m_dependencies);

                using namespace AzFramework::FileTag;

                // We are checking preload Dependency only for runtime assets
                AZStd::vector<AZStd::string> excludedTagsList = { FileTags[static_cast<unsigned int>(FileTagsIndex::EditorOnly)] };

                bool editorOnlyAsset = false;
                QueryFileTagsEventBus::EventResult(editorOnlyAsset, FileTagType::Exclude,
                    &QueryFileTagsEventBus::Events::Match, message.m_data.c_str(), excludedTagsList);

                if (!editorOnlyAsset)
                {
                    for (auto& productDependency : message.m_dependencies)
                    {
                        auto loadBehavior = AZ::Data::ProductDependencyInfo::LoadBehaviorFromFlags(productDependency.m_flags);
                        if (loadBehavior == AZ::Data::AssetLoadBehavior::PreLoad)
                        {
                            m_preloadAssetList.emplace_back(AZStd::make_pair(message.m_assetId, message.m_platform.c_str()));
                            break;
                        }
                    }
                }
            }

            if (m_registryBuiltOnce)
            {
                Q_EMIT SendAssetMessage(message);
            }
        }
        else if (message.m_type == AssetNotificationMessage::AssetRemoved)
        {
            QMutexLocker locker(&m_registriesMutex);

            QString assetPlatform{ QString::fromUtf8(message.m_platform.c_str()) };
            AZ_Assert(!assetPlatform.isEmpty(), "Product platform is empty");

            auto found = m_registries[assetPlatform].m_assetIdToInfo.find(message.m_assetId);

            if (found != m_registries[assetPlatform].m_assetIdToInfo.end())
            {
                m_catalogIsDirty = true;

                m_registries[assetPlatform].UnregisterAsset(message.m_assetId);

                if (m_registryBuiltOnce)
                {
                    Q_EMIT SendAssetMessage(message);
                }
            }
        }
    }

    bool AssetCatalog::CheckValidatedAssets(AZ::Data::AssetId assetId, const QString& platform)
    {
        auto found = m_cachedNoPreloadDependenyAssetList.equal_range(assetId);

        for (auto platformIter = found.first; platformIter != found.second; ++platformIter)
        {
            if (platformIter->second == platform)
            {
                // we have already verified this asset for this run and it does not have any preload dependency for the specified platform, therefore we can safely skip it
                return false;
            }
        }

        return true;

    }

    void AssetCatalog::ValidatePreLoadDependency()
    {
        if (m_currentlyValidatingPreloadDependency)
        {
            return;
        }
        m_currentlyValidatingPreloadDependency = true;

        for (auto iter = m_preloadAssetList.begin(); iter != m_preloadAssetList.end(); iter++)
        {
            if (!CheckValidatedAssets(iter->first, iter->second))
            {
                continue;
            }

            AZStd::stack<AZStd::pair<AZ::Data::AssetId, AZ::Data::AssetId>> assetStack;
            AZStd::vector<AZ::Data::AssetId> currentAssetTree; // this is used to determine the hierarchy of asset loads.
            AZStd::unordered_set<AZ::Data::AssetId> currentVisitedAssetsTree;
            AZStd::unordered_set<AZ::Data::AssetId> allVisitedAssets;

            assetStack.push(AZStd::make_pair(iter->first, AZ::Data::AssetId()));

            bool cyclicDependencyFound = false;

            AZStd::lock_guard<AZStd::mutex> lock(m_databaseMutex);
            while (!assetStack.empty())
            {
                AZ::Data::AssetId assetId = assetStack.top().first;
                AZ::Data::AssetId parentAssetId = assetStack.top().second;
                assetStack.pop();
                allVisitedAssets.insert(assetId);

                while (currentAssetTree.size() && parentAssetId != currentAssetTree.back())
                {
                    currentVisitedAssetsTree.erase(currentAssetTree.back());
                    currentAssetTree.pop_back();
                };

                currentVisitedAssetsTree.insert(assetId);
                currentAssetTree.emplace_back(assetId);

                m_db->QueryProductDependencyBySourceGuidSubId(assetId.m_guid, assetId.m_subId, iter->second.toUtf8().constData(), [&](const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry& entry)
                    {
                        auto loadBehavior = AZ::Data::ProductDependencyInfo::LoadBehaviorFromFlags(entry.m_dependencyFlags);
                        if (loadBehavior == AZ::Data::AssetLoadBehavior::PreLoad)
                        {
                            AZ::Data::AssetId dependentAssetId(entry.m_dependencySourceGuid, entry.m_dependencySubID);
                            if (currentVisitedAssetsTree.find(dependentAssetId) == currentVisitedAssetsTree.end())
                            {
                                if (!CheckValidatedAssets(dependentAssetId, iter->second))
                                {
                                    // we have already verified that this asset does not have any preload dependency
                                    return true;
                                }

                                assetStack.push(AZStd::make_pair(dependentAssetId, assetId));
                            }
                            else
                            {
                                cyclicDependencyFound = true;

                                AZStd::string cyclicPreloadDependencyTreeString;
                                for (const auto& assetIdEntry : currentAssetTree)
                                {
                                    AzToolsFramework::AssetDatabase::ProductDatabaseEntry productDatabaseEntry;
                                    m_db->GetProductBySourceGuidSubId(assetIdEntry.m_guid, assetIdEntry.m_subId, productDatabaseEntry);
                                    cyclicPreloadDependencyTreeString = cyclicPreloadDependencyTreeString + AZStd::string::format("%s ->", productDatabaseEntry.m_productName.c_str());
                                };

                                AzToolsFramework::AssetDatabase::ProductDatabaseEntry productDatabaseEntry;
                                m_db->GetProductBySourceGuidSubId(dependentAssetId.m_guid, dependentAssetId.m_subId, productDatabaseEntry);

                                cyclicPreloadDependencyTreeString = cyclicPreloadDependencyTreeString + AZStd::string::format(" %s ", productDatabaseEntry.m_productName.c_str());

                                AzToolsFramework::AssetDatabase::ProductDatabaseEntry productDatabaseRootEntry;
                                m_db->GetProductBySourceGuidSubId(iter->first.m_guid, iter->first.m_subId, productDatabaseRootEntry);

                                AZ_Error(AssetProcessor::ConsoleChannel, false, "Preload circular dependency detected while processing asset (%s).\n Preload hierarchy is %s . Adjust your product dependencies for assets in this chain to break this loop.",
                                    productDatabaseRootEntry.m_productName.c_str(), cyclicPreloadDependencyTreeString.c_str());

                                return  false;

                            }
                        }

                        return true;
                    });


                if (cyclicDependencyFound)
                {
                    currentVisitedAssetsTree.clear();
                    currentAssetTree.clear();
                    AZStd::stack<AZStd::pair<AZ::Data::AssetId, AZ::Data::AssetId>> emptyAssetStack;
                    assetStack.swap(emptyAssetStack);
                }
            };

            if (!cyclicDependencyFound)
            {
                for (const auto& assetId : allVisitedAssets)
                {
                    m_cachedNoPreloadDependenyAssetList.emplace(AZStd::make_pair(assetId, iter->second)); // assetid, platform
                }
            }
        }

        m_preloadAssetList.clear();
        m_cachedNoPreloadDependenyAssetList.clear();
        m_currentlyValidatingPreloadDependency = false;
    }

    void AssetCatalog::SaveRegistry_Impl()
    {
        bool allCatalogsSaved = true;
        // note that its safe not to save the catalog if the catalog is not dirty
        // because the engine will be accepting updates as long as the update has a higher or equal
        // number to the saveId, not just equal.
        if (m_catalogIsDirty)
        {
            m_catalogIsDirty = false;
            // Reflect registry for serialization.
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            AZ_Assert(serializeContext, "Unable to retrieve serialize context.");
            if (nullptr == serializeContext->FindClassData(AZ::AzTypeInfo<AzFramework::AssetRegistry>::Uuid()))
            {
                AzFramework::AssetRegistry::ReflectSerialize(serializeContext);
            }

            // save out a catalog for each platform
            for (const QString& platform : m_platforms)
            {
                // Serialize out the catalog to a memory buffer, and then dump that memory buffer to stream.
                QElapsedTimer timer;
                timer.start();
                m_saveBuffer.clear();
                // allow this to grow by up to 20mb at a time so as not to fragment.
                // we re-use the save buffer each time to further reduce memory load.
                AZ::IO::ByteContainerStream<AZStd::vector<char>> catalogFileStream(&m_saveBuffer, 1024 * 1024 * 20);

                // these 3 lines are what writes the entire registry to the memory stream
                AZ::ObjectStream* objStream = AZ::ObjectStream::Create(&catalogFileStream, *serializeContext, AZ::ObjectStream::ST_BINARY);
                {
                    QMutexLocker locker(&m_registriesMutex);
                    objStream->WriteClass(&m_registries[platform]);
                }
                objStream->Finalize();

                // now write the memory stream out to the temp folder
                QString workSpace;
                if (!AssetUtilities::CreateTempWorkspace(workSpace))
                {
                    AZ_Warning(AssetProcessor::ConsoleChannel, false, "Failed to create a temp workspace for catalog writing\n");
                }
                else
                {
                    auto settingsRegistry = AZ::SettingsRegistry::Get();
                    AZ::SettingsRegistryInterface::FixedValueString cacheRootFolder;
                    settingsRegistry->Get(cacheRootFolder, AZ::SettingsRegistryMergeUtils::FilePathKey_CacheProjectRootFolder);

                    QString tempRegistryFile = QString("%1/%2").arg(workSpace).arg("assetcatalog.xml.tmp");
                    QString platformCacheDir = QString("%1/%2").arg(cacheRootFolder.c_str()).arg(platform);
                    QString actualRegistryFile = QString("%1/%2").arg(platformCacheDir).arg("assetcatalog.xml");

                    AZ_TracePrintf(AssetProcessor::DebugChannel, "Creating asset catalog: %s --> %s\n", tempRegistryFile.toUtf8().constData(), actualRegistryFile.toUtf8().constData());
                    AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
                    if (AZ::IO::FileIOBase::GetInstance()->Open(tempRegistryFile.toUtf8().data(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary, fileHandle))
                    {
                        AZ::IO::FileIOBase::GetInstance()->Write(fileHandle, m_saveBuffer.data(), m_saveBuffer.size());
                        AZ::IO::FileIOBase::GetInstance()->Close(fileHandle);

                        // Make sure that the destination folder of the registry file exists
                        QDir registryDir(platformCacheDir);
                        if (!registryDir.exists())
                        {
                            QString absPath = registryDir.absolutePath();
                            [[maybe_unused]] AZ::IO::Result makeDirResult = AZ::IO::FileIOBase::GetInstance()->CreatePath(absPath.toUtf8().constData());
                            AZ_Warning(AssetProcessor::ConsoleChannel, makeDirResult, "Failed create folder %s", platformCacheDir.toUtf8().constData());
                        }

                        // if we succeeded in doing this, then use "rename" to move the file over the previous copy.
                        bool moved = AssetUtilities::MoveFileWithTimeout(tempRegistryFile, actualRegistryFile, 3);
                        allCatalogsSaved = allCatalogsSaved && moved;

                        // warn if it failed
                        AZ_Warning(AssetProcessor::ConsoleChannel, moved, "Failed to move %s to %s", tempRegistryFile.toUtf8().constData(), actualRegistryFile.toUtf8().constData());

                        if (moved)
                        {
                            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Saved %s catalog containing %u assets in %fs\n", platform.toUtf8().constData(), m_registries[platform].m_assetIdToInfo.size(), timer.elapsed() / 1000.0f);
                        }
                    }
                    else
                    {
                        AZ_Warning(AssetProcessor::ConsoleChannel, false, "Failed to create catalog file %s", tempRegistryFile.toUtf8().constData());
                        allCatalogsSaved = false;
                    }

                    AZ::IO::FileIOBase::GetInstance()->DestroyPath(workSpace.toUtf8().data());
                }
            }
        }

        {
            // scoped to minimize the duration of this mutex lock
            QMutexLocker locker(&m_savingRegistryMutex);
            m_currentlySavingCatalog = false;
            RegistrySaveComplete(m_currentRegistrySaveVersion, allCatalogsSaved);
            AssetRegistryNotificationBus::Broadcast(&AssetRegistryNotifications::OnRegistrySaveComplete, m_currentRegistrySaveVersion, allCatalogsSaved);
        }
    }

    AzFramework::AssetSystem::GetUnresolvedDependencyCountsResponse AssetCatalog::HandleGetUnresolvedDependencyCountsRequest(MessageData<AzFramework::AssetSystem::GetUnresolvedDependencyCountsRequest> messageData)
    {
        AzFramework::AssetSystem::GetUnresolvedDependencyCountsResponse response;

        {
            QMutexLocker locker(&m_registriesMutex);

            const auto& productDependencies = m_registries[messageData.m_platform].GetAssetDependencies(messageData.m_message->m_assetId);

            for (const AZ::Data::ProductDependency& productDependency : productDependencies)
            {
                if (m_registries[messageData.m_platform].m_assetIdToInfo.find(productDependency.m_assetId)
                    == m_registries[messageData.m_platform].m_assetIdToInfo.end())
                {
                    ++response.m_unresolvedAssetIdReferences;
                }
            }
        }

        {
            AZStd::lock_guard<AZStd::mutex> lock(m_databaseMutex);

            m_db->QueryProductDependencyBySourceGuidSubId(messageData.m_message->m_assetId.m_guid, messageData.m_message->m_assetId.m_subId, messageData.m_platform.toUtf8().constData(), [&response](const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry& entry)
                {
                    if (!entry.m_unresolvedPath.empty() && entry.m_unresolvedPath.find('*') == entry.m_unresolvedPath.npos
                        && !entry.m_unresolvedPath.starts_with(ExcludedDependenciesSymbol))
                    {
                        ++response.m_unresolvedPathReferences;
                    }

                    return true;
                });
        }

        return response;
    }

    void AssetCatalog::HandleSaveAssetCatalogRequest(MessageData<AzFramework::AssetSystem::SaveAssetCatalogRequest> messageData)
    {
        int registrySaveVersion = SaveRegistry();
        m_queuedSaveCatalogRequest.insert(registrySaveVersion, messageData.m_key);
    }

    void AssetCatalog::RegistrySaveComplete(int assetCatalogVersion, bool allCatalogsSaved)
    {
        for (auto iter = m_queuedSaveCatalogRequest.begin(); iter != m_queuedSaveCatalogRequest.end();)
        {
            if (iter.key() <= assetCatalogVersion)
            {
                AssetProcessor::NetworkRequestID& requestId = iter.value();
                AzFramework::AssetSystem::SaveAssetCatalogResponse saveCatalogResponse;
                saveCatalogResponse.m_saved = allCatalogsSaved;
                AssetProcessor::ConnectionBus::Event(requestId.first, &AssetProcessor::ConnectionBus::Events::SendResponse, requestId.second, saveCatalogResponse);
                iter = m_queuedSaveCatalogRequest.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
    }

    int AssetCatalog::SaveRegistry()
    {
        QMutexLocker locker(&m_savingRegistryMutex);

        if (!m_currentlySavingCatalog)
        {
            m_currentlySavingCatalog = true;
            QMetaObject::invokeMethod(this, "SaveRegistry_Impl", Qt::QueuedConnection);
            return ++m_currentRegistrySaveVersion;
        }

        return m_currentRegistrySaveVersion;
    }

    void AssetCatalog::BuildRegistry()
    {
        m_catalogIsDirty = true;
        m_registryBuiltOnce = true;

        {
            AZStd::lock_guard<AZStd::mutex> lock(m_databaseMutex);
            QMutexLocker locker(&m_registriesMutex);

            for (QString platform : m_platforms)
            {
                auto inserted = m_registries.insert(platform, AzFramework::AssetRegistry());
                AzFramework::AssetRegistry& currentRegistry = inserted.value();
                // list of source entries in the database that need to have their UUID updated
                AZStd::vector<AzToolsFramework::AssetDatabase::SourceDatabaseEntry> sourceEntriesToUpdate;

                QElapsedTimer timer;
                timer.start();
                auto databaseQueryCallback = [&](AzToolsFramework::AssetDatabase::CombinedDatabaseEntry& combined)
                {
                    SourceAssetReference sourceAsset(combined.m_scanFolderPK, combined.m_scanFolder.c_str(), combined.m_sourceName.c_str());
                    AZ::Data::AssetId assetId;

                    auto* fileStateInterface = AZ::Interface<IFileStateRequests>::Get();

                    if (!fileStateInterface)
                    {
                        AZ_Assert(false, "Programmer Error - IFileStateRequests interface is not available");
                        return false;
                    }

                    const bool fileExists = fileStateInterface->Exists(sourceAsset.AbsolutePath().c_str());

                    // Only try to update for files which actually exist
                    if (fileExists)
                    {
                        auto canonicalUuid = AssetUtilities::GetSourceUuid(sourceAsset);

                        if (!canonicalUuid)
                        {
                            AZ_Error("AssetCatalog", false, "%s", canonicalUuid.GetError().c_str());
                            return true;
                        }

                        assetId = AZ::Data::AssetId(canonicalUuid.GetValue(), combined.m_subID);

                        if (canonicalUuid.GetValue() != combined.m_sourceGuid)
                        {
                            // Canonical UUID does not match stored UUID, this entry needs to be updated
                            sourceEntriesToUpdate.emplace_back(
                                combined.m_sourceID,
                                combined.m_scanFolderID,
                                combined.m_sourceName.c_str(),
                                canonicalUuid.GetValue(), // Updated UUID
                                combined.m_analysisFingerprint.c_str());
                        }
                    }
                    else
                    {
                        assetId = AZ::Data::AssetId(combined.m_sourceGuid, combined.m_subID);
                    }

                    // relative file path is gotten by removing the platform and game from the product name
                    AZStd::string_view relativeProductPath = AssetUtilities::StripAssetPlatformNoCopy(combined.m_productName);
                    QString fullProductPath = m_cacheRoot.absoluteFilePath(combined.m_productName.c_str());

                    AZ::u64 productFileSize = 0;
                    AZ::IO::FileIOBase::GetInstance()->Size(fullProductPath.toUtf8().constData(), productFileSize);

                    AZ::Data::AssetInfo info;
                    info.m_assetType = combined.m_assetType;
                    info.m_relativePath = relativeProductPath;
                    info.m_assetId = assetId;
                    info.m_sizeBytes = productFileSize;

                    // also register it at the legacy id(s) if its different:
                    currentRegistry.RegisterAsset(assetId, info);

                    return true; // see them all
                };

                m_db->QueryCombined(
                    databaseQueryCallback,
                    AZ::Uuid::CreateNull(),
                    nullptr,
                    platform.toUtf8().constData(),
                    AzToolsFramework::AssetSystem::JobStatus::Any,
                    true); /*we still need legacy IDs - hardly anyone else does*/

                auto* uuidInterface = AZ::Interface<IUuidRequests>::Get();
                AZ_Assert(uuidInterface, "Programmer Error - IUuidRequests is not available.");

                AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer productDependenciesToUpdate;

                m_db->QueryProductDependenciesTable(
                    [this, &platform, uuidInterface, &productDependenciesToUpdate](AZ::Data::AssetId& assetId, AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry& entry)
                    {
                        if (AzFramework::StringFunc::Equal(entry.m_platform.c_str(), platform.toUtf8().data()))
                        {
                            // Attempt to update the dependency UUID to the canonical UUID if possible
                            if (auto canonicalUuid = uuidInterface->GetCanonicalUuid(entry.m_dependencySourceGuid); canonicalUuid && canonicalUuid.value() != entry.m_dependencySourceGuid)
                            {
                                entry.m_dependencySourceGuid = canonicalUuid.value();
                                productDependenciesToUpdate.emplace_back(entry);
                            }

                            m_registries[platform].RegisterAssetDependency(
                                assetId,
                                AZ::Data::ProductDependency{ AZ::Data::AssetId(entry.m_dependencySourceGuid, entry.m_dependencySubID),
                                                             entry.m_dependencyFlags });
                        }

                        return true;
                    });

                AzToolsFramework::AssetDatabase::SourceFileDependencyEntryContainer sourceDependenciesToUpdate;
                m_db->QuerySourceDependencies(
                    [&sourceDependenciesToUpdate, &uuidInterface](AzToolsFramework::AssetDatabase::SourceFileDependencyEntry& entry)
                    {
                        bool update = false;

                        // Check if the sourceGuid needs to be updated
                        if (auto canonicalUuid = uuidInterface->GetCanonicalUuid(entry.m_sourceGuid);
                            canonicalUuid && canonicalUuid.value() != entry.m_sourceGuid)
                        {
                            if (canonicalUuid.value() != entry.m_sourceGuid)
                            {
                                update = true;
                                entry.m_sourceGuid = canonicalUuid.value();
                            }
                        }

                        // Check if the dependency uses a UUID and if it needs to be updated
                        if (entry.m_dependsOnSource.IsUuid())
                        {
                            if (auto canonicalUuid = uuidInterface->GetCanonicalUuid(entry.m_dependsOnSource.GetUuid());
                                canonicalUuid && canonicalUuid != entry.m_dependsOnSource.GetUuid())
                            {
                                update = true;
                                entry.m_dependsOnSource = AzToolsFramework::AssetDatabase::PathOrUuid(canonicalUuid.value());
                            }
                        }

                        if (update)
                        {
                            sourceDependenciesToUpdate.emplace_back(entry);
                        }

                        return true; // Iterate all entries
                    });

                // Update any old source UUIDs
                for (auto& sourceDatabaseEntry : sourceEntriesToUpdate)
                {
                    m_db->SetSource(sourceDatabaseEntry);
                }

                // Update any old product dependencies
                for (auto& productDependencyEntry : productDependenciesToUpdate)
                {
                    m_db->SetProductDependency(productDependencyEntry);
                }

                // Update any old source dependencies
                if (!sourceDependenciesToUpdate.empty())
                {
                    m_db->RemoveSourceFileDependencies(sourceDependenciesToUpdate);
                    m_db->SetSourceFileDependencies(sourceDependenciesToUpdate);
                }

                AZ_TracePrintf(
                    "Catalog",
                    "Read %u assets from database for %s in %fs\n",
                    currentRegistry.m_assetIdToInfo.size(),
                    platform.toUtf8().constData(),
                    timer.elapsed() / 1000.0f);
            }
        }

        Q_EMIT CatalogLoaded();
    }

    void AssetCatalog::OnDependencyResolved(const AZ::Data::AssetId& assetId, const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry& entry)
    {
        QString platform(entry.m_platform.c_str());
        if (!m_platforms.contains(platform))
        {
            return;
        }

        AzFramework::AssetSystem::AssetNotificationMessage message;
        message.m_type = AzFramework::AssetSystem::AssetNotificationMessage::NotificationType::AssetChanged;

        // Get the existing data from registry.
        AZ::Data::AssetInfo assetInfo = GetAssetInfoById(assetId);
        message.m_data = assetInfo.m_relativePath;
        message.m_sizeBytes = assetInfo.m_sizeBytes;
        message.m_assetId = assetId;
        message.m_assetType = assetInfo.m_assetType;
        message.m_platform = entry.m_platform.c_str();

        // Get legacyIds from registry to put in message.
        AZStd::unordered_map<AZ::Data::AssetId, AZ::Data::AssetId> legacyIds;

        // Add the new dependency entry and get the list of all dependencies for the message.
        AZ::Data::ProductDependency newDependency{ AZ::Data::AssetId(entry.m_dependencySourceGuid, entry.m_dependencySubID), entry.m_dependencyFlags };
        {
            QMutexLocker locker(&m_registriesMutex);
            m_registries[platform].RegisterAssetDependency(assetId, newDependency);
            message.m_dependencies = AZStd::move(m_registries[platform].GetAssetDependencies(assetId));
        }

        if (m_registryBuiltOnce)
        {
            Q_EMIT SendAssetMessage(message);
        }

        m_catalogIsDirty = true;
    }

    void AssetCatalog::OnConnect(unsigned int connectionId, QStringList platforms)
    {
        // Send out a message for each asset to make sure the connected tools are aware of the existence of all previously built assets
        // since the assetcatalog might not have been written out to disk previously.
        for (QString platform : platforms)
        {
            QMutexLocker locker(&m_registriesMutex);
            auto itr = m_registries.find(platform);

            if (itr == m_registries.end())
            {
                continue;
            }

            const auto& currentRegistry = *itr;

            AzFramework::AssetSystem::BulkAssetNotificationMessage bulkMessage;

            bulkMessage.m_messages.reserve(currentRegistry.m_assetIdToInfo.size());
            bulkMessage.m_type = AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged;

            for (const auto& assetInfo : currentRegistry.m_assetIdToInfo)
            {
                AzFramework::AssetSystem::AssetNotificationMessage message(
                    assetInfo.second.m_relativePath.c_str(),
                    AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged,
                    assetInfo.second.m_assetType,
                    platform.toUtf8().constData());

                message.m_assetId = assetInfo.second.m_assetId;
                message.m_sizeBytes = assetInfo.second.m_sizeBytes;
                message.m_dependencies = AZStd::move(currentRegistry.GetAssetDependencies(assetInfo.second.m_assetId));


                bulkMessage.m_messages.push_back(AZStd::move(message));
            }

            AssetProcessor::ConnectionBus::Event(connectionId, &AssetProcessor::ConnectionBus::Events::Send, 0, bulkMessage);
        }
    }

    void AssetCatalog::OnSourceQueued(AZ::Uuid sourceUuid, AZStd::unordered_set<AZ::Uuid> legacyUuids, const SourceAssetReference& sourceAsset)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_sourceUUIDToSourceNameMapMutex);

        m_sourceUUIDToSourceAssetMap.insert({ sourceUuid, sourceAsset });

        //adding legacy source uuid as well
        for (const auto legacyUuid : legacyUuids)
        {
        m_sourceUUIDToSourceAssetMap.insert({ legacyUuid, sourceAsset });
        }

        m_sourceAssetToSourceUUIDMap.insert({ sourceAsset, sourceUuid });
    }

    void AssetCatalog::OnSourceFinished(AZ::Uuid sourceUuid, AZStd::unordered_set<AZ::Uuid> legacyUuids)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_sourceUUIDToSourceNameMapMutex);

        auto found = m_sourceUUIDToSourceAssetMap.find(sourceUuid);
        if (found != m_sourceUUIDToSourceAssetMap.end())
        {
            m_sourceAssetToSourceUUIDMap.erase(found->second);
        }

        m_sourceUUIDToSourceAssetMap.erase(sourceUuid);

        for (const auto& legacyUuid : legacyUuids)
        {
        m_sourceUUIDToSourceAssetMap.erase(legacyUuid);
    }
    }

    //////////////////////////////////////////////////////////////////////////

    bool AssetCatalog::GetRelativeProductPathFromFullSourceOrProductPath(const AZStd::string& fullSourceOrProductPath, AZStd::string& relativeProductPath)
    {
        ProcessGetRelativeProductPathFromFullSourceOrProductPathRequest(fullSourceOrProductPath, relativeProductPath);

        if (!relativeProductPath.length())
        {
            // if we are here it means we have failed to determine the assetId we will send back the original path
            AZ_TracePrintf(AssetProcessor::DebugChannel, "GetRelativeProductPath no result, returning original %s...\n", fullSourceOrProductPath.c_str());
            relativeProductPath = fullSourceOrProductPath;
            return false;
        }

        return true;
    }

    bool AssetCatalog::GenerateRelativeSourcePath(
        const AZStd::string& sourcePath, AZStd::string& relativePath, AZStd::string& rootFolder)
    {
        QString normalizedSourcePath = AssetUtilities::NormalizeFilePath(sourcePath.c_str());
        QDir inputPath(normalizedSourcePath);
        QString scanFolder;
        QString relativeName;

        bool validResult = false;

        AZ_TracePrintf(AssetProcessor::DebugChannel, "ProcessGenerateRelativeSourcePathRequest: %s...\n", sourcePath.c_str());

        if (sourcePath.empty())
        {
            // For an empty input path, do nothing, we'll return an empty, invalid result.
            // (We check fullPath instead of inputPath, because an empty fullPath actually produces "." for inputPath)
        }
        else if (inputPath.isAbsolute())
        {
            // For an absolute path, try to convert it to a relative path, based on the existing scan folders.
            // To get the inputPath, we use absolutePath() instead of path() so that any . or .. entries get collapsed.
            validResult = m_platformConfig->ConvertToRelativePath(inputPath.absolutePath(), relativeName, scanFolder);
        }
        else if (inputPath.isRelative())
        {
            // For a relative path, concatenate it with each scan folder, and see if a valid relative path emerges.
            int scanFolders = m_platformConfig->GetScanFolderCount();
            for (int scanIdx = 0; scanIdx < scanFolders; scanIdx++)
            {
                auto& scanInfo = m_platformConfig->GetScanFolderAt(scanIdx);
                QDir possibleRoot(scanInfo.ScanPath());
                QDir possibleAbsolutePath = possibleRoot.filePath(normalizedSourcePath);
                // To get the inputPath, we use absolutePath() instead of path() so that any . or .. entries get collapsed.
                if (m_platformConfig->ConvertToRelativePath(possibleAbsolutePath.absolutePath(), relativeName, scanFolder))
                {
                    validResult = true;
                    break;
                }
            }
        }

        // The input has produced a valid relative path.  However, the path might match multiple nested scan folders,
        // so look to see if a higher-priority folder has a better match.
        if (validResult)
        {
            QString overridingFile = m_platformConfig->GetOverridingFile(relativeName, scanFolder);

            if (!overridingFile.isEmpty())
            {
                overridingFile = AssetUtilities::NormalizeFilePath(overridingFile);
                validResult = m_platformConfig->ConvertToRelativePath(overridingFile, relativeName, scanFolder);
            }
        }

        if (!validResult)
        {
            // if we are here it means we have failed to determine the relativePath, so we will send back the original path
            AZ_TracePrintf(AssetProcessor::DebugChannel,
                "GenerateRelativeSourcePath found no valid result, returning original path: %s...\n", sourcePath.c_str());

            rootFolder.clear();
            relativePath.clear();
            relativePath = sourcePath;
            return false;
        }

        relativePath = relativeName.toUtf8().data();
        rootFolder = scanFolder.toUtf8().data();

        AZ_Assert(!relativePath.empty(), "ConvertToRelativePath returned true, but relativePath is empty");

        return true;
    }

    bool AssetCatalog::GetFullSourcePathFromRelativeProductPath(const AZStd::string& relPath, AZStd::string& fullSourcePath)
    {
        ProcessGetFullSourcePathFromRelativeProductPathRequest(relPath, fullSourcePath);

        if (!fullSourcePath.length())
        {
            // if we are here it means that we failed to determine the full source path from the relative path and we will send back the original path
            AZ_TracePrintf(AssetProcessor::DebugChannel, "GetFullSourcePath no result, returning original %s...\n", relPath.c_str());
            fullSourcePath = relPath;
            return false;
        }

        return true;
    }

    bool AssetCatalog::GetAssetInfoById(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType, const AZStd::string& platformName, AZ::Data::AssetInfo& assetInfo, AZStd::string& rootFilePath)
    {
        assetInfo.m_assetId.SetInvalid();
        assetInfo.m_relativePath.clear();
        assetInfo.m_assetType = AZ::Data::s_invalidAssetType;
        assetInfo.m_sizeBytes = 0;

        // If the assetType wasn't provided, try to guess it
        if (assetType.IsNull())
        {
            SourceAssetReference sourceAsset;
            bool result = GetAssetInfoByIdOnly(assetId, platformName, assetInfo, sourceAsset);
            rootFilePath = sourceAsset.ScanFolderPath().c_str();
            return result;
        }

        bool isSourceType;

        {
            AZStd::lock_guard<AZStd::mutex> lock(m_sourceAssetTypesMutex);
            isSourceType = m_sourceAssetTypes.find(assetType) != m_sourceAssetTypes.end();
        }

        // If the assetType is registered as a source type, look up the source info
        if (isSourceType)
        {
            SourceAssetReference sourceAsset;

            if (GetSourceFileInfoFromAssetId(assetId, sourceAsset))
            {
                assetInfo.m_assetId = assetId;
                assetInfo.m_assetType = assetType;
                assetInfo.m_relativePath = sourceAsset.RelativePath().c_str();
                AZ::IO::FileIOBase::GetInstance()->Size(sourceAsset.AbsolutePath().c_str(), assetInfo.m_sizeBytes);
                rootFilePath = sourceAsset.ScanFolderPath().c_str();

                return true;
            }

            return false;
        }

        // Return the product file info
        rootFilePath.clear(); // products don't have root file paths.
        assetInfo = GetProductAssetInfo(platformName.c_str(), assetId);

        return !assetInfo.m_relativePath.empty();
    }

    QString AssetCatalog::GetDefaultAssetPlatform()
    {
        // get the first available platform, preferring the host platform.
        if (m_platforms.contains(AzToolsFramework::AssetSystem::GetHostAssetPlatform()))
        {
            return QString::fromUtf8(AzToolsFramework::AssetSystem::GetHostAssetPlatform());
        }

        // the GetHostAssetPlatform() "pc" or "osx" is not actually enabled for this compilation (maybe "server" or similar is in a build job).
        // in that case, we'll use the first we find!
        return m_platforms[0];
    }


    AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> AssetCatalog::GetDirectProductDependencies(
        const AZ::Data::AssetId& id)
    {
        QString platform = GetDefaultAssetPlatform();

        QMutexLocker locker(&m_registriesMutex);

        auto itr = m_registries[platform].m_assetDependencies.find(id);

        if (itr == m_registries[platform].m_assetDependencies.end())
        {
            return AZ::Failure<AZStd::string>("Failed to find asset in dependency map");
        }

        return AZ::Success(itr->second);
    }

    AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> AssetCatalog::GetAllProductDependencies(const AZ::Data::AssetId& id)
    {
        return GetAllProductDependenciesFilter(id, {}, {});
    }

    AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> AssetCatalog::GetAllProductDependenciesFilter(
        const AZ::Data::AssetId& id,
        const AZStd::unordered_set<AZ::Data::AssetId>& exclusionList,
        const AZStd::vector<AZStd::string>& wildcardPatternExclusionList)
    {
        AZStd::vector<AZ::Data::ProductDependency> dependencyList;
        AZStd::unordered_set<AZ::Data::AssetId> assetSet;
        AZ::Data::PreloadAssetListType preloadList;
        if (exclusionList.find(id) != exclusionList.end())
        {
            return AZ::Success(AZStd::move(dependencyList));
        }

        for (const AZStd::string& wildcardPattern : wildcardPatternExclusionList)
        {
            if (DoesAssetIdMatchWildcardPattern(id, wildcardPattern))
            {
                return AZ::Success(AZStd::move(dependencyList));
            }
        }

        AddAssetDependencies(id, assetSet, dependencyList, exclusionList, wildcardPatternExclusionList, preloadList);

        // dependencyList will be appended to while looping, so use a traditional loop
        for (size_t i = 0; i < dependencyList.size(); ++i)
        {
            AddAssetDependencies(dependencyList[i].m_assetId, assetSet, dependencyList, exclusionList, wildcardPatternExclusionList, preloadList);
        }

        return AZ::Success(AZStd::move(dependencyList));
    }

    AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> AssetCatalog::GetLoadBehaviorProductDependencies(
        const AZ::Data::AssetId& id, AZStd::unordered_set<AZ::Data::AssetId>& noloadSet,
        AZ::Data::PreloadAssetListType& preloadAssetList)
    {
        AZStd::vector<AZ::Data::ProductDependency> dependencyList;
        AZStd::vector<AZ::Data::ProductDependency> returnList;
        AZStd::unordered_set<AZ::Data::AssetId> assetSet;

        AddAssetDependencies(id, assetSet, dependencyList, {}, {}, preloadAssetList);

        // dependencyList will be appended to while looping, so use a traditional loop
        for (size_t i = 0; i < dependencyList.size(); ++i)
        {
            if (AZ::Data::ProductDependencyInfo::LoadBehaviorFromFlags(dependencyList[i].m_flags) == AZ::Data::AssetLoadBehavior::NoLoad)
            {
                noloadSet.insert(dependencyList[i].m_assetId);
                assetSet.erase(dependencyList[i].m_assetId);
            }
            else
            {
                returnList.push_back(dependencyList[i]);
                AddAssetDependencies(dependencyList[i].m_assetId, assetSet, dependencyList, {}, {}, preloadAssetList);
            }
        }

        return AZ::Success(AZStd::move(returnList));
    }

    bool AssetCatalog::DoesAssetIdMatchWildcardPattern(const AZ::Data::AssetId& assetId, const AZStd::string& wildcardPattern)
    {
        if (wildcardPattern.empty())
        {
            // pattern is empty, there is nothing to match
            return false;
        }

        AZStd::string relativePath = GetAssetPathById(assetId);
        if (relativePath.empty())
        {
            // assetId did not resolve to a relative path, cannot be matched
            return false;
        }

        return AZStd::wildcard_match(wildcardPattern, relativePath);
    }

    void AssetCatalog::AddAssetDependencies(
        const AZ::Data::AssetId& searchAssetId,
        AZStd::unordered_set<AZ::Data::AssetId>& assetSet,
        AZStd::vector<AZ::Data::ProductDependency>& dependencyList,
        const AZStd::unordered_set<AZ::Data::AssetId>& exclusionList,
        const AZStd::vector<AZStd::string>& wildcardPatternExclusionList,
        AZ::Data::PreloadAssetListType& preloadAssetList)
    {
        using namespace AZ::Data;

        QString platform = GetDefaultAssetPlatform();

        QMutexLocker locker(&m_registriesMutex);

        auto itr = m_registries[platform].m_assetDependencies.find(searchAssetId);

        if (itr != m_registries[platform].m_assetDependencies.end())
        {
            AZStd::vector<ProductDependency>& assetDependencyList = itr->second;

            for (const ProductDependency& dependency : assetDependencyList)
            {
                if (!dependency.m_assetId.IsValid())
                {
                    continue;
                }

                if (exclusionList.find(dependency.m_assetId) != exclusionList.end())
                {
                    continue;
                }

                bool isWildcardMatch = false;
                for (const AZStd::string& wildcardPattern : wildcardPatternExclusionList)
                {
                    isWildcardMatch = DoesAssetIdMatchWildcardPattern(dependency.m_assetId, wildcardPattern);
                    if (isWildcardMatch)
                    {
                        break;
                    }
                }
                if (isWildcardMatch)
                {
                    continue;
                }

                auto loadBehavior = AZ::Data::ProductDependencyInfo::LoadBehaviorFromFlags(dependency.m_flags);
                if (loadBehavior == AZ::Data::AssetLoadBehavior::PreLoad)
                {
                    preloadAssetList[searchAssetId].insert(dependency.m_assetId);
                }

                // Only proceed if this ID is valid and we haven't encountered this assetId before.
                // Invalid IDs usually come from unmet path product dependencies.
                if (assetSet.find(dependency.m_assetId) == assetSet.end())
                {
                    assetSet.insert(dependency.m_assetId); // add to the set of already-encountered assets
                    dependencyList.push_back(dependency); // put it in the flat list of dependencies we've found
                }
            }
        }
    }

    bool AssetCatalog::GetSourceInfoBySourcePath(const char* sourcePath, AZ::Data::AssetInfo& assetInfo, AZStd::string& watchFolder)
    {
        if (!sourcePath || strlen(sourcePath) <= 0)
        {
            assetInfo.m_assetId.SetInvalid();
            return false;
        }

        SourceAssetReference sourceAsset;

        if (!AzFramework::StringFunc::Path::IsRelative(sourcePath))
        {
            QString scanFolder;
            QString relPath;

            // Call ConvertToRelativePath first to verify the sourcePath exists in a scanfolder
            if (m_platformConfig->ConvertToRelativePath(sourcePath, relPath, scanFolder))
            {
                sourceAsset = SourceAssetReference(scanFolder, relPath);
            }
        }
        else
        {
            // relative paths get the first matching asset, and then they get the usual call.
            QString absolutePath = m_platformConfig->FindFirstMatchingFile(QString::fromUtf8(sourcePath));
            if (!absolutePath.isEmpty())
            {
                sourceAsset = SourceAssetReference(absolutePath);
            }
        }

        if (!sourceAsset)
        {
            assetInfo.m_assetId.SetInvalid();
            return false;
        }

        // now that we have a database path, we can at least return something.
        // but source info also includes UUID, which we need to hit the database for (or the in-memory map).

        // Check the database first for the UUID now that we have the "database name" (which includes output prefix)

        {
            AZStd::lock_guard<AZStd::mutex> lock(m_databaseMutex);
            AzToolsFramework::AssetDatabase::SourceDatabaseEntry returnedSource;

            if (m_db->GetSourceBySourceNameScanFolderId(sourceAsset.RelativePath().c_str(), sourceAsset.ScanFolderId(), returnedSource))
            {
                const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry = returnedSource;

                AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry scanEntry;
                if (m_db->GetScanFolderByScanFolderID(entry.m_scanFolderPK, scanEntry))
                {
                    watchFolder = scanEntry.m_scanFolder;
                    // since we are returning the UUID of a source file, as opposed to the full assetId of a product file produced by that source file,
                    // the subId part of the assetId will always be set to zero.
                    assetInfo.m_assetId = AZ::Data::AssetId(entry.m_sourceGuid, 0);

                    assetInfo.m_relativePath = entry.m_sourceName;
                    AZStd::string absolutePath;
                    AzFramework::StringFunc::Path::Join(scanEntry.m_scanFolder.c_str(), assetInfo.m_relativePath.c_str(), absolutePath);
                    AZ::IO::FileIOBase::GetInstance()->Size(absolutePath.c_str(), assetInfo.m_sizeBytes);
                    assetInfo.m_assetType = AZ::Uuid::CreateNull(); // most source files don't have a type!

                    // Go through the list of source assets and see if this asset's file path matches any of the filters
                    for (const auto& pair : m_sourceAssetTypeFilters)
                    {
                        if (AZStd::wildcard_match(pair.first, assetInfo.m_relativePath))
                        {
                            assetInfo.m_assetType = pair.second;
                            break;
                        }
                    }

                    return true;
                }
            }
        }

        watchFolder = sourceAsset.ScanFolderPath().c_str();

        // Source file isn't in the database yet, see if its in the job queue
        if (GetQueuedAssetInfoByRelativeSourceName(sourceAsset, assetInfo))
        {
            return true;
        }

        // Source file isn't in the job queue yet, source UUID needs to be created
        return GetUncachedSourceInfoFromDatabaseNameAndWatchFolder(sourceAsset, assetInfo);
    }

    bool AssetCatalog::GetSourceInfoBySourceUUID(const AZ::Uuid& sourceUuid, AZ::Data::AssetInfo& assetInfo, AZStd::string& watchFolder)
    {
        AZ::Data::AssetId partialId(sourceUuid, 0);
        SourceAssetReference sourceAsset;

        if (GetSourceFileInfoFromAssetId(partialId, sourceAsset))
        {
            watchFolder = sourceAsset.ScanFolderPath().c_str();

            assetInfo.m_assetId = partialId;
            assetInfo.m_assetType = AZ::Uuid::CreateNull(); // most source files don't have a type!
            assetInfo.m_relativePath = sourceAsset.RelativePath().c_str();
            AZ::IO::FileIOBase::GetInstance()->Size(sourceAsset.AbsolutePath().c_str(), assetInfo.m_sizeBytes);

            // if the type has registered with a typeid, then supply it here
            AZStd::lock_guard<AZStd::mutex> lock(m_sourceAssetTypesMutex);

            // Go through the list of source assets and see if this asset's file path matches any of the filters
            // if it does, we know what type it is (if not, the above call to CreateNull ensures it is null).
            for (const auto& pair : m_sourceAssetTypeFilters)
            {
                if (AZStd::wildcard_match(pair.first, sourceAsset.RelativePath().c_str()))
                {
                    assetInfo.m_assetType = pair.second;
                    break;
                }
            }

            return true;
        }
        // failed!
        return false;
    }

    bool AssetCatalog::GetAssetsProducedBySourceUUID(const AZ::Uuid& sourceUuid, AZStd::vector<AZ::Data::AssetInfo>& productsAssetInfo)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_databaseMutex);

        AzToolsFramework::AssetDatabase::SourceDatabaseEntry entry;

        if (m_db->GetSourceBySourceGuid(sourceUuid, entry))
        {
            AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer products;

            if (m_db->GetProductsBySourceID(entry.m_sourceID, products))
            {
                for (const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& product : products)
                {
                    AZ::Data::AssetInfo assetInfo;
                    assetInfo.m_assetId = AZ::Data::AssetId(sourceUuid, product.m_subID);
                    assetInfo.m_assetType = product.m_assetType;
                    productsAssetInfo.emplace_back(assetInfo);
                }
            }

            return true;
        }

        return false;
    }

    bool AssetCatalog::ClearFingerprintForAsset(const AZStd::string& sourcePath)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_databaseMutex);

        SourceAssetReference sourceAsset;

        if (QFileInfo(sourcePath.c_str()).isAbsolute())
        {
            sourceAsset = SourceAssetReference(sourcePath.c_str());
        }
        else
        {
            QString absolutePath = m_platformConfig->FindFirstMatchingFile(sourcePath.c_str());

            if (absolutePath.isEmpty())
            {
                return false;
            }

            sourceAsset = SourceAssetReference(absolutePath.toUtf8().constData());
        }

        if(!m_db->UpdateFileHashByFileNameAndScanFolderId(sourceAsset.RelativePath().c_str(), sourceAsset.ScanFolderId(), 0))
        {
            return false;
        }

        AzToolsFramework::AssetDatabase::SourceDatabaseEntry source;
        if (!m_db->GetSourceBySourceNameScanFolderId(sourceAsset.RelativePath().c_str(), sourceAsset.ScanFolderId(), source))
        {
            return false;
        }

        // if setting the file hash failed, still try to clear the job fingerprints.
        return m_db->SetJobFingerprintsBySourceID(source.m_sourceID, 0);
    }

    bool AssetCatalog::GetScanFolders(AZStd::vector<AZStd::string>& scanFolders)
    {
        int scanFolderCount = m_platformConfig->GetScanFolderCount();
        for (int i = 0; i < scanFolderCount; ++i)
        {
            scanFolders.push_back(m_platformConfig->GetScanFolderAt(i).ScanPath().toUtf8().constData());
        }
        return true;
    }

    bool AssetCatalog::GetAssetSafeFolders(AZStd::vector<AZStd::string>& assetSafeFolders)
    {
        int scanFolderCount = m_platformConfig->GetScanFolderCount();
        for (int scanFolderIndex = 0; scanFolderIndex < scanFolderCount; ++scanFolderIndex)
        {
            AssetProcessor::ScanFolderInfo& scanFolder = m_platformConfig->GetScanFolderAt(scanFolderIndex);
            if (scanFolder.CanSaveNewAssets())
            {
                assetSafeFolders.push_back(scanFolder.ScanPath().toUtf8().constData());
            }
        }
        return true;
    }

    bool AssetCatalog::IsAssetPlatformEnabled(const char* platform)
    {
        const AZStd::vector<AssetBuilderSDK::PlatformInfo>& enabledPlatforms = m_platformConfig->GetEnabledPlatforms();
        for (const AssetBuilderSDK::PlatformInfo& platformInfo : enabledPlatforms)
        {
            if (platformInfo.m_identifier == platform)
            {
                return true;
            }
        }
        return false;
    }

    int AssetCatalog::GetPendingAssetsForPlatform(const char* /*platform*/)
    {
        AZ_Assert(false, "Call to unsupported Asset Processor function GetPendingAssetsForPlatform on AssetCatalog");
        return -1;
    }

    AZStd::string AssetCatalog::GetAssetPathById(const AZ::Data::AssetId& id)
    {
        return GetAssetInfoById(id).m_relativePath;

    }

    AZ::Data::AssetId AssetCatalog::GetAssetIdByPath(const char* path, const AZ::Data::AssetType& typeToRegister, bool autoRegisterIfNotFound)
    {
        AZ_UNUSED(autoRegisterIfNotFound);
        AZ_Assert(autoRegisterIfNotFound == false, "Auto registration is invalid during asset processing.");
        AZ_UNUSED(typeToRegister);
        AZ_Assert(typeToRegister == AZ::Data::s_invalidAssetType, "Can not register types during asset processing.");
        AZStd::string relProductPath;
        GetRelativeProductPathFromFullSourceOrProductPath(path, relProductPath);
        QString tempPlatformName = GetDefaultAssetPlatform();

        AZ::Data::AssetId assetId;
        {
            QMutexLocker locker(&m_registriesMutex);
            assetId = m_registries[tempPlatformName].GetAssetIdByPath(relProductPath.c_str());
        }
        return assetId;
    }

    AZ::Data::AssetInfo AssetCatalog::GetAssetInfoById(const AZ::Data::AssetId& id)
    {
        AZ::Data::AssetType assetType;
        AZ::Data::AssetInfo assetInfo;
        AZStd::string rootFilePath;
        GetAssetInfoById(id, assetType, "", assetInfo, rootFilePath);
        return assetInfo;
    }

    bool ConvertDatabaseProductPathToProductFilename(AZStd::string_view dbPath, QString& productFileName)
    {
        // Always strip the leading directory from the product path
        // The leading directory can be either an asset platform path or a subfolder
        AZ::StringFunc::TokenizeNext(dbPath, AZ_CORRECT_AND_WRONG_FILESYSTEM_SEPARATOR);
        if (!dbPath.empty())
        {
            productFileName = QString::fromUtf8(dbPath.data(), aznumeric_cast<int>(dbPath.size()));
            return true;
        }
        return false;
    }

    void AssetCatalog::ProcessGetRelativeProductPathFromFullSourceOrProductPathRequest(const AZStd::string& fullPath, AZStd::string& relativeProductPath)
    {
        QString sourceOrProductPath = fullPath.c_str();
        QString normalizedSourceOrProductPath = AssetUtilities::NormalizeFilePath(sourceOrProductPath);

        QString productFileName;
        bool resultCode = false;
        QDir inputPath(normalizedSourceOrProductPath);

        AZ_TracePrintf(AssetProcessor::DebugChannel, "ProcessGetRelativeProductPath: %s...\n", sourceOrProductPath.toUtf8().constData());

        if (inputPath.isRelative())
        {
            //if the path coming in is already a relative path,we just send it back
            productFileName = sourceOrProductPath;
            resultCode = true;
        }
        else
        {
            QDir cacheRoot;
            AssetUtilities::ComputeProjectCacheRoot(cacheRoot);
            QString normalizedCacheRoot = AssetUtilities::NormalizeFilePath(cacheRoot.path());

            if (AssetUtilities::IsInCacheFolder(normalizedSourceOrProductPath.toUtf8().constData(), cacheRoot.absolutePath().toUtf8().constData()))
            {
                // The path send by the game/editor contains the cache root so we try to find the asset id
                // from the asset database
                normalizedSourceOrProductPath.remove(0, normalizedCacheRoot.length() + 1); // adding 1 for the native separator

                // If we are here it means that the asset database does not have any knowledge about this file,
                // most probably because AP has not processed the file yet
                // In this case we will try to compute the asset id from the product path
                // Now after removing the cache root,normalizedInputAssetPath can either be $Platform/$Game/xxx/yyy or something like $Platform/zzz
                // and the corresponding assetId have to be either xxx/yyy or zzz

                resultCode = ConvertDatabaseProductPathToProductFilename(normalizedSourceOrProductPath.toUtf8().data(), productFileName);
            }
            else
            {
                // If we are here it means its a source file, first see whether there is any overriding file and then try to find products
                QString scanFolder;
                QString relativeName;
                if (m_platformConfig->ConvertToRelativePath(normalizedSourceOrProductPath, relativeName, scanFolder))
                {
                    QString overridingFile = m_platformConfig->GetOverridingFile(relativeName, scanFolder);

                    if (overridingFile.isEmpty())
                    {
                        // no overriding file found
                        overridingFile = normalizedSourceOrProductPath;
                    }
                    else
                    {
                        overridingFile = AssetUtilities::NormalizeFilePath(overridingFile);
                    }

                    const auto* scanFolderInfo = m_platformConfig->GetScanFolderForFile(overridingFile);

                    if (scanFolderInfo && m_platformConfig->ConvertToRelativePath(overridingFile, scanFolderInfo, relativeName))
                    {
                        AZStd::lock_guard<AZStd::mutex> lock(m_databaseMutex);
                        AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer products;

                        if (m_db->GetProductsBySourceNameScanFolderID(relativeName, scanFolderInfo->ScanFolderID(), products))
                        {
                            resultCode = ConvertDatabaseProductPathToProductFilename(products[0].m_productName, productFileName);
                        }
                        else
                        {
                            productFileName = relativeName;
                            resultCode = true;
                        }
                    }
                }
            }
        }

        if (!resultCode)
        {
            productFileName = "";
        }

        relativeProductPath = productFileName.toUtf8().data();
    }

    void AssetCatalog::ProcessGetFullSourcePathFromRelativeProductPathRequest(const AZStd::string& relPath, AZStd::string& fullSourcePath)
    {
        QString assetPath = relPath.c_str();
        QString normalizedAssetPath = AssetUtilities::NormalizeFilePath(assetPath);
        int resultCode = 0;
        QString fullAssetPath;

        if (normalizedAssetPath.isEmpty())
        {
            fullSourcePath = "";
            return;
        }

        QDir inputPath(normalizedAssetPath);

        if (inputPath.isAbsolute())
        {
            QDir cacheRoot;
            AssetUtilities::ComputeProjectCacheRoot(cacheRoot);
            QString normalizedCacheRoot = AssetUtilities::NormalizeFilePath(cacheRoot.path());

            if (!AssetUtilities::IsInCacheFolder(normalizedAssetPath.toUtf8().constData(), cacheRoot.absolutePath().toUtf8().constData()))
            {
                // Attempt to convert to relative path
                QString dummy, convertedRelPath;
                if (m_platformConfig->ConvertToRelativePath(assetPath, convertedRelPath, dummy))
                {
                    // then find the first matching file to get correct casing
                    fullAssetPath = m_platformConfig->FindFirstMatchingFile(convertedRelPath);
                }

                if (fullAssetPath.isEmpty())
                {
                    // if we couldn't find it, just return the passed in path
                    fullAssetPath = assetPath;
                }

                resultCode = 1;
            }
            else
            {
                // The path send by the game/editor contains the cache root ,try to find the productName from it
                normalizedAssetPath.remove(0, normalizedCacheRoot.length() + 1); // adding 1 for the native separator
            }
        }

        if (!resultCode)
        {
            //remove aliases if present
            normalizedAssetPath = AssetUtilities::NormalizeAndRemoveAlias(normalizedAssetPath);

            if (!normalizedAssetPath.isEmpty()) // this happens if it comes in as just for example "@products@/"
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_databaseMutex);

                //We should have the asset now, we can now find the full asset path
                // we have to check each platform individually until we get a hit.
                const auto& platforms = m_platformConfig->GetEnabledPlatforms();
                QString productName;
                for (const AssetBuilderSDK::PlatformInfo& platformInfo : platforms)
                {
                    if (platformInfo.m_identifier == AssetBuilderSDK::CommonPlatformName)
                    {
                        // Common platform is not supported for product assets currently
                        continue;
                    }

                    QString platformName = QString::fromUtf8(platformInfo.m_identifier.c_str());
                    productName = AssetUtilities::GuessProductNameInDatabase(normalizedAssetPath, platformName, m_db.get());
                    if (!productName.isEmpty())
                    {
                        break;
                    }
                }

                if (!productName.isEmpty())
                {
                    //Now find the input name for the path,if we are here this should always return true since we were able to find the productName before
                    AzToolsFramework::AssetDatabase::SourceDatabaseEntryContainer sources;
                    if (m_db->GetSourcesByProductName(productName, sources))
                    {
                        //Once we have found the inputname we will try finding the full path
                        fullAssetPath = m_platformConfig->FindFirstMatchingFile(sources[0].m_sourceName.c_str());
                        if (!fullAssetPath.isEmpty())
                        {
                            resultCode = 1;
                        }
                    }
                }
                else
                {
                    // if we are not able to guess the product name than maybe the asset path is an input name
                    fullAssetPath = m_platformConfig->FindFirstMatchingFile(normalizedAssetPath);
                    if (!fullAssetPath.isEmpty())
                    {
                        resultCode = 1;
                    }
                }
            }
        }

        if (!resultCode)
        {
            fullSourcePath = "";
        }
        else
        {
            fullSourcePath = fullAssetPath.toUtf8().data();
        }
    }

    //////////////////////////////////////////////////////////////////////////

    void AssetCatalog::RegisterSourceAssetType(const AZ::Data::AssetType& assetType, const char* assetFileFilter)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_sourceAssetTypesMutex);
        m_sourceAssetTypes.insert(assetType);
        AZStd::vector<AZStd::string> tokens;
        AZStd::string semicolonSeperated(assetFileFilter);
        AZStd::tokenize(semicolonSeperated, AZStd::string(";"), tokens);

        for (const auto& pattern : tokens)
        {
            m_sourceAssetTypeFilters[pattern] = assetType;
        }
    }

    void AssetCatalog::UnregisterSourceAssetType(const AZ::Data::AssetType& /*assetType*/)
    {
        // For now, this does nothing, because it would just needlessly complicate things for no gain.
        // Unregister is only called when a builder is shut down, which really is only supposed to happen when AssetCatalog is being shutdown
        // Without a way of tracking how many builders have registered the same assetType and being able to perfectly keep track of every builder shutdown, even in the event of a crash,
        // the map would either be cleared prematurely or never get cleared at all
    }

    //////////////////////////////////////////////////////////////////////////

    bool AssetCatalog::GetSourceFileInfoFromAssetId(const AZ::Data::AssetId &assetId, SourceAssetReference& sourceAsset)
    {
        // Try checking the UuidManager, it keeps track of legacy UUIDs
        auto* uuidInterface = AZ::Interface<AssetProcessor::IUuidRequests>::Get();
        AZ_Assert(uuidInterface, "Programmer Error - IUuidRequests interface is not available.");

        if (auto result = uuidInterface->FindHighestPriorityFileByUuid(assetId.m_guid); result)
        {
            sourceAsset = SourceAssetReference(result.value());
            return true;
        }

        // Check the database next
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_databaseMutex);
            AzToolsFramework::AssetDatabase::SourceDatabaseEntry entry;

            if (m_db->GetSourceBySourceGuid(assetId.m_guid, entry))
            {
                AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry scanEntry;
                if (m_db->GetScanFolderByScanFolderID(entry.m_scanFolderPK, scanEntry))
                {
                    sourceAsset = SourceAssetReference(scanEntry.m_scanFolder.c_str(), entry.m_sourceName.c_str());

                    return true;
                }
            }
        }

        // Source file isn't in the database yet, see if its in the job queue
        return GetQueuedAssetInfoById(assetId.m_guid, sourceAsset);
    }

    AZ::Data::AssetInfo AssetCatalog::GetProductAssetInfo(const char* platformName, const AZ::Data::AssetId& assetId)
    {
        // this more or less follows the same algorithm that the game uses to look up products.
        using namespace AZ::Data;
        using namespace AzFramework;

        if ((!assetId.IsValid()) || (m_platforms.isEmpty()))
        {
            return AssetInfo();
        }

        // in case no platform name has been given, we are prepared to compute one.
        QString tempPlatformName;

        // if no platform specified, we'll use the current platform.
        if ((!platformName) || (platformName[0] == 0))
        {
            tempPlatformName = GetDefaultAssetPlatform();
        }
        else
        {
            tempPlatformName = QString::fromUtf8(platformName);
        }

        // note that m_platforms is not mutated at all during runtime, so we ignore it in the lock
        if (!m_platforms.contains(tempPlatformName))
        {
            return AssetInfo();
        }

        QMutexLocker locker(&m_registriesMutex);

        const AssetRegistry& registryToUse = m_registries[tempPlatformName];

        auto foundIter = registryToUse.m_assetIdToInfo.find(assetId);
        if (foundIter != registryToUse.m_assetIdToInfo.end())
        {
            return foundIter->second;
        }

        return AssetInfo(); // not found!
    }

    bool AssetCatalog::GetAssetInfoByIdOnly(const AZ::Data::AssetId& id, const AZStd::string& platformName, AZ::Data::AssetInfo& assetInfo, SourceAssetReference& sourceAsset)
    {
        if (GetSourceFileInfoFromAssetId(id, sourceAsset))
        {
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_sourceAssetTypesMutex);

                // Go through the list of source assets and see if this asset's file path matches any of the filters
                for (const auto& pair : m_sourceAssetTypeFilters)
                {
                    if (AZStd::wildcard_match(pair.first, sourceAsset.AbsolutePath().c_str()))
                    {
                        assetInfo.m_assetId = id;
                        assetInfo.m_assetType = pair.second;
                        assetInfo.m_relativePath = sourceAsset.RelativePath().c_str();
                        AZ::IO::FileIOBase::GetInstance()->Size(sourceAsset.AbsolutePath().c_str(), assetInfo.m_sizeBytes);

                        return true;
                    }
                }
            }

            // If we get to here, we're going to assume it's a product type
            sourceAsset = {};
            assetInfo = GetProductAssetInfo(platformName.c_str(), id);

            return !assetInfo.m_relativePath.empty();
        }

        // Asset isn't in the DB or in the APM queue, we don't know what this asset ID is
        return false;
    }

    bool AssetCatalog::GetQueuedAssetInfoById(const AZ::Uuid& guid, SourceAssetReference& sourceAsset)
    {
        if (!guid.IsNull())
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_sourceUUIDToSourceNameMapMutex);

            auto foundSource = m_sourceUUIDToSourceAssetMap.find(guid);
            if (foundSource != m_sourceUUIDToSourceAssetMap.end())
            {
                sourceAsset = foundSource->second;

                return true;
            }

            AZ_TracePrintf(AssetProcessor::DebugChannel, "GetQueuedAssetInfoById: AssetCatalog unable to find the requested source asset having uuid (%s).\n", guid.ToString<AZStd::string>().c_str());
        }

        return false;
    }


    bool AssetCatalog::GetQueuedAssetInfoByRelativeSourceName(const SourceAssetReference& sourceAsset, AZ::Data::AssetInfo& assetInfo)
    {
        if (sourceAsset)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_sourceUUIDToSourceNameMapMutex);

            auto foundSourceUUID = m_sourceAssetToSourceUUIDMap.find(sourceAsset);
            if (foundSourceUUID != m_sourceAssetToSourceUUIDMap.end())
            {
                assetInfo.m_relativePath = sourceAsset.RelativePath().c_str();
                assetInfo.m_assetId = foundSourceUUID->second;
                AZ::IO::FileIOBase::GetInstance()->Size(sourceAsset.AbsolutePath().c_str(), assetInfo.m_sizeBytes);
                assetInfo.m_assetType = AZ::Uuid::CreateNull(); // most source files don't have a type!

                // Go through the list of source assets and see if this asset's file path matches any of the filters
                for (const auto& pair : m_sourceAssetTypeFilters)
                {
                    if (AZStd::wildcard_match(pair.first, assetInfo.m_relativePath))
                    {
                        assetInfo.m_assetType = pair.second;
                        break;
                    }
                }

                return true;
            }
        }

        assetInfo.m_assetId.SetInvalid();

        return false;
    }

    bool AssetCatalog::GetUncachedSourceInfoFromDatabaseNameAndWatchFolder(const SourceAssetReference& sourceAsset, AZ::Data::AssetInfo& assetInfo)
    {
        // Make sure the source file exists first
        AZ::IO::FileIOBase* io = AZ::IO::FileIOBase::GetInstance();
        AZ_Assert(io, "Expected a FileIO Interface");
        if (!io->Exists(sourceAsset.AbsolutePath().c_str()))
        {
            return false;
        }

        auto sourceUUID = AssetUtilities::GetSourceUuid(sourceAsset);

        if (!sourceUUID)
        {
            return false;
        }

        AZ::Data::AssetId sourceAssetId(sourceUUID.GetValue(), 0);

        assetInfo.m_assetId = sourceAssetId;
        assetInfo.m_relativePath = sourceAsset.RelativePath().c_str();
        io->Size(sourceAsset.AbsolutePath().c_str(), assetInfo.m_sizeBytes);
        assetInfo.m_assetType = AZ::Uuid::CreateNull();

        // Go through the list of source assets and see if this asset's file path matches any of the filters
        for (const auto& pair : m_sourceAssetTypeFilters)
        {
            if (AZStd::wildcard_match(pair.first, assetInfo.m_relativePath))
            {
                assetInfo.m_assetType = pair.second;
                break;
            }
        }

        return true;
    }

    bool AssetCatalog::ConnectToDatabase()
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_databaseMutex);

        if (!m_db)
        {
            AZStd::string databaseLocation;
            AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Broadcast(&AzToolsFramework::AssetDatabase::AssetDatabaseRequests::GetAssetDatabaseLocation, databaseLocation);

            if (!databaseLocation.empty())
            {
                m_db = AZStd::make_unique<AssetProcessor::AssetDatabaseConnection>();
                m_db->OpenDatabase();

                return true;
            }

            return false;
        }

        return true;
    }

    void AssetCatalog::AsyncAssetCatalogStatusRequest()
    {
        if (m_catalogIsDirty)
        {
            Q_EMIT AsyncAssetCatalogStatusResponse(AssetCatalogStatus::RequiresSaving);
        }
        else
        {
            Q_EMIT AsyncAssetCatalogStatusResponse(AssetCatalogStatus::UpToDate);
        }
    }

}//namespace AssetProcessor


