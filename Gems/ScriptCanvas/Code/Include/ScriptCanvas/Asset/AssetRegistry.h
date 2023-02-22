/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Asset/AssetManager.h>

#include <ScriptCanvas/Asset/AssetRegistryBus.h>
#include <ScriptCanvas/Asset/AssetDescription.h>

namespace ScriptCanvas
{
    class AssetRegistry
        : AssetRegistryRequestBus::MultiHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(AssetRegistry, AZ::SystemAllocator);

        AssetRegistry();
        ~AssetRegistry();

        template <typename AssetType, typename HandlerType, typename AssetDescriptionType>
        void Register()
        {
            AZ::Data::AssetType assetType(azrtti_typeid<AssetType>());
            if (AZ::Data::AssetManager::Instance().GetHandler(assetType))
            {
                return; // Asset Type already handled
            }

            m_assetHandlers[assetType] = AZStd::make_unique<HandlerType>();
            AZ::Data::AssetManager::Instance().RegisterHandler(m_assetHandlers[assetType].get(), assetType);

            AssetDescriptionType assetDescription;
            m_assetDescription[assetType] = assetDescription;

            //AZ_TracePrintf("Asset Registry", "AssetRegistry registering: %s (extension: %s)\n", assetType.ToString<AZStd::string>().c_str(), assetDescription.GetExtensionImpl());

            // Use AssetCatalog service to register ScriptCanvas asset type and extension
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddAssetType, assetType);
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::EnableCatalogForAsset, assetType);
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddExtension, assetDescription.GetExtensionImpl());

            if (m_assetHandlerFileFilter.find(assetType) == m_assetHandlerFileFilter.end()
                && assetDescription.GetIsEditableTypeImpl())
            {
                m_assetHandlerFileFilter[assetType] = assetDescription.GetFileFilterImpl();
            }

            AssetRegistryRequestBus::MultiHandler::BusConnect(assetType);
        }

        void Unregister();

        template <typename AssetType>
        AZ::Data::AssetHandler* GetAssetHandler()
        {
            AZ::Data::AssetType assetType(azrtti_typeid<AssetType>());
            return GetAssetHandler(assetType);
        }

        // AssetRegistryRequestBus
        AZ::Data::AssetHandler* GetAssetHandler() override;
        AssetDescription* GetAssetDescription(AZ::Data::AssetType assetType) override;
        AZStd::vector<AZStd::string> GetAssetHandlerFileFilters() override;        

    private:

        AZ::Data::AssetHandler* GetAssetHandler(const AZ::Data::AssetType& assetType);

        AssetRegistry(const AssetRegistry&) = delete;

        AZStd::unordered_map<AZ::Data::AssetType, AZStd::unique_ptr<AZ::Data::AssetHandler>> m_assetHandlers;
        AZStd::unordered_map<AZ::Data::AssetType, AssetDescription> m_assetDescription;
        AZStd::unordered_map<AZ::Data::AssetType, AZStd::string> m_assetHandlerFileFilter;
    };
}
