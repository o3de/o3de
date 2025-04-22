/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/std/parallel/condition_variable.h>
#include <AZTestShared/Utils/Utils.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace Data;

    struct LoadAssetDataSynchronizer
    {
        AZStd::mutex m_conditionMutex;
        AZStd::condition_variable m_condition;

        AZStd::atomic_int m_numBlocking{ 0 };
        bool m_readyToLoad{ false };
    };

    struct AssetDefinition
    {
        AssetDefinition* AddNoLoad(const Uuid& id);
        AssetDefinition* AddPreload(const Uuid& id);
        AssetDefinition* AddQueueLoad(const Uuid& id);

        AssetId m_assetId;
        AssetType m_type;
        AZ::IO::Path m_fileName;

        // Additional delay to add to loading on top of any global catalog delay
        size_t m_loadDelay{ 0 };

        // If set, LoadAssetData will block until the condition is triggered.
        LoadAssetDataSynchronizer* m_loadSynchronizer{ nullptr };

        // If true, queries for GetAssetTypeById will return invalid data
        bool m_noAssetData{ false };

        // If true, no handler will be registered for this asset
        bool m_noHandler{ false };

        AZStd::vector<ProductDependency> m_noLoadDependencies;
        AZStd::vector<ProductDependency> m_preloadDependencies;
        AZStd::vector<ProductDependency> m_queueLoadDependencies;
    };

    struct DataDrivenHandlerAndCatalog
        : AssetHandler
        , AssetCatalog
        , AssetCatalogRequestBus::Handler
    {
        AZ_CLASS_ALLOCATOR(DataDrivenHandlerAndCatalog, AZ::SystemAllocator);

        DataDrivenHandlerAndCatalog();
        ~DataDrivenHandlerAndCatalog() override;

        AssetPtr CreateAsset([[maybe_unused]] const AssetId& id, const AssetType& type) override;
        LoadResult LoadAssetData(const Asset<AssetData>& asset, AZStd::shared_ptr<AssetDataStream> stream,
                                 const AssetFilterCB& assetLoadFilterCB) override;
        bool SaveAssetData(const Asset<AssetData>& asset, IO::GenericStream* stream) override;
        void DestroyAsset(AssetPtr ptr) override;
        void GetHandledAssetTypes(AZStd::vector<AssetType>& assetTypes) override;
        void GetDefaultAssetLoadPriority(AssetType type, AZ::IO::IStreamerTypes::Deadline& defaultDeadline,
            AZ::IO::IStreamerTypes::Priority& defaultPriority) const override;

        AssetStreamInfo GetStreamInfoForLoad(const AssetId& id, const AssetType& /*type*/) override;
        AssetStreamInfo GetStreamInfoForSave(const AssetId& id, const AssetType& /*type*/) override;
        Outcome<AZStd::vector<ProductDependency>, AZStd::string> GetAllProductDependencies(const AssetId& assetId) override;
        Outcome<AZStd::vector<ProductDependency>, AZStd::string> GetLoadBehaviorProductDependencies(const AssetId& assetId,
            AZStd::unordered_set<AssetId>& noloadSet, PreloadAssetListType& preloadList) override;

        AssetInfo GetAssetInfoById(const AssetId& assetId) override;

        const char* GetStreamName(const AssetId& id);

        const AssetDefinition* FindByType(const Uuid& type);
        const AssetDefinition* FindById(const AssetId& id);
        void AddDependenciesHelper(const AZStd::vector<ProductDependency>& list, AZStd::vector<ProductDependency>& listOutput);
        void AddDependencies(const AssetDefinition* def, AZStd::vector<ProductDependency>& listOutput);
        void AddPreloads(const AssetDefinition* def, PreloadAssetListType& preloadList);
        void AddNoLoads(const AssetDefinition* def, AZStd::unordered_set<AssetId>& noloadSet);

        void SetArtificialDelayMilliseconds(size_t createDelay, size_t loadDelay);

        template<typename T>
        AssetDefinition* AddAsset(const Uuid& assetUuid, const AZ::IO::Path& fileName, size_t loadDelay = 0, bool noAssetData = false,
            bool noHandler = false, LoadAssetDataSynchronizer* loadSynchronizer = nullptr)
        {
            m_assetDefinitions.push_back(AssetDefinition{
                AssetId(assetUuid, 0), azrtti_typeid<T>(), GetTestFolderPath() / fileName, loadDelay, loadSynchronizer, noAssetData, noHandler });

            return &m_assetDefinitions.back();
        }

        AZStd::atomic_int m_numCreations = 0;
        AZStd::atomic_int m_numDestructions = 0;
        AZStd::atomic_int m_numLoads = 0;
        AZStd::atomic_bool m_failLoad{ false };
        SerializeContext* m_context{ nullptr };

        size_t m_createDelay = 0;
        size_t m_loadDelay = 0;

        AZStd::chrono::milliseconds m_defaultDeadline = AZStd::chrono::milliseconds(250);
        AZ::IO::IStreamerTypes::Priority m_defaultPriority = AZ::IO::IStreamerTypes::s_priorityMedium;

        AZStd::vector<AssetDefinition> m_assetDefinitions;
    };
}
