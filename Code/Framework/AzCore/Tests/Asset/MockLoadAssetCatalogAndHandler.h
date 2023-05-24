/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetManager.h>
#include <Tests/Asset/TestAssetTypes.h>

namespace UnitTest
{
    /**
     * MockLoadAssetCatalogAndHandler is a mock AssetCatalog / AssetHandler that will take a set of Asset IDs
     * and pretend to load them as valid assets of the given type.  This is useful for unit tests that need to test functionality
     * like setting asset IDs or loading assets that may trigger a dependent asset load as a side effect, but the actual asset
     * loaded is irrelevant to the test.
     * To use:  Simply create an instance of this in the unit test, and pass the set of asset IDs to mock out into the constructor.
     */
    class MockLoadAssetCatalogAndHandler
        : public AZ::Data::AssetCatalog
        , public AZ::Data::AssetCatalogRequestBus::Handler
        , public AZ::Data::AssetHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(MockLoadAssetCatalogAndHandler, AZ::SystemAllocator);

        /**
         * Create the mock AssetCatalog / AssetHandler
         * @param ids The set of asset IDs to fake load. Ex: {id1, id2, ...}
         * @param assetType (optional) The asset type to register as and use.  Defaults to EmptyAsset.
         * @param createAsset (optional) A lambda function for constructing a new asset of this type.  Defaults to no-op.
         * @param destroyAsset (optional) A lambda function for destroying the constructed asset.  Defaults to no-op.
         */
        MockLoadAssetCatalogAndHandler(
            AZStd::unordered_set<AZ::Data::AssetId> ids
            , AZ::Data::AssetType assetType = azrtti_typeid<UnitTest::EmptyAsset>()
            , AZ::Data::AssetPtr(*createAsset)() = []() { return AZ::Data::AssetPtr(nullptr); }
            , void(*destroyAsset)(AZ::Data::AssetPtr asset) = [](AZ::Data::AssetPtr) {})
            : m_ids(AZStd::move(ids))
            , m_assetType(assetType)
            , m_createAsset(createAsset)
            , m_destroyAsset(destroyAsset)
        {
            AZ::Data::AssetManager::Instance().RegisterHandler(this, m_assetType);
            AZ::Data::AssetManager::Instance().RegisterCatalog(this, m_assetType);
            AZ::Data::AssetCatalogRequestBus::Handler::BusConnect();
        }

        ~MockLoadAssetCatalogAndHandler()
        {
            AZ::Data::AssetCatalogRequestBus::Handler::BusDisconnect();
            AZ::Data::AssetManager::Instance().UnregisterCatalog(this);
            AZ::Data::AssetManager::Instance().UnregisterHandler(this);
        }

        //////////////////////////////////////////////////////////////////////////
        // AssetCatalogRequestBus
        AZ::Data::AssetInfo GetAssetInfoById(const AZ::Data::AssetId& id) override
        {
            AZ::Data::AssetInfo result;
            if (m_ids.contains(id))
            {
                result.m_assetType = m_assetType;
                result.m_assetId = id;
            }
            return result;
        }

        //////////////////////////////////////////////////////////////////////////
        // AssetCatalog
        AZ::Data::AssetStreamInfo GetStreamInfoForLoad([[maybe_unused]] const AZ::Data::AssetId& id,
            [[maybe_unused]] const AZ::Data::AssetType& type) override
        {
            AZ::Data::AssetStreamInfo info;
            return info;
        }

        //////////////////////////////////////////////////////////////////////////
        // AssetHandler
        AZ::Data::AssetPtr CreateAsset([[maybe_unused]] const AZ::Data::AssetId& id,
            [[maybe_unused]] const AZ::Data::AssetType& type) override
        {
            return m_createAsset();
        }

        void DestroyAsset(AZ::Data::AssetPtr ptr) override
        {
            m_destroyAsset(ptr);
        }

        void GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes) override
        {
            assetTypes.emplace_back(m_assetType);
        }

        LoadResult LoadAssetData([[maybe_unused]] const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            [[maybe_unused]] AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            [[maybe_unused]] const AZ::Data::AssetFilterCB& assetLoadFilterCB) override
        {
            return LoadResult::LoadComplete;
        }
    protected:
        AZ::Data::AssetPtr(*m_createAsset)();
        void(*m_destroyAsset)(AZ::Data::AssetPtr asset);
        AZ::Data::AssetType m_assetType;
        AZStd::unordered_set<AZ::Data::AssetId> m_ids;
    };
}

