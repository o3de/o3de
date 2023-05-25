/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Asset/AssetRegistry.h>
#include <AzCore/Asset/AssetManager.h>

#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Asset/RuntimeAssetHandler.h>
#include <AzCore/std/smart_ptr/make_shared.h>

DECLARE_EBUS_INSTANTIATION(ScriptCanvas::AssetRegistryRequests);

namespace ScriptCanvas
{
    void AssetRegistry::Unregister()
    {
        for (auto& handler : m_assetHandlers)
        {
            AZ::Data::AssetManager::Instance().UnregisterHandler(handler.second.get());
        }
    }

    AZ::Data::AssetHandler* AssetRegistry::GetAssetHandler()
    {
        const AssetRegistryRequestBus::BusIdType assetType = *AssetRegistryRequestBus::GetCurrentBusId();
        return GetAssetHandler(assetType);
    }

    AssetDescription* AssetRegistry::GetAssetDescription(AZ::Data::AssetType assetType)
    {
        if (m_assetDescription.find(assetType) != m_assetDescription.end())
        {
            return &m_assetDescription[assetType];
        }

        return nullptr;
    }

    AZStd::vector<AZStd::string> AssetRegistry::GetAssetHandlerFileFilters()
    {
        AZStd::vector<AZStd::string> filters;
        for (auto filter : m_assetHandlerFileFilter)
        {
            filters.push_back(filter.second);
        }
        return filters;
    }

    AZ::Data::AssetHandler* AssetRegistry::GetAssetHandler(const AZ::Data::AssetType& assetType)
    {
        if (m_assetHandlers.find(assetType) != m_assetHandlers.end())
        {
            return m_assetHandlers[assetType].get();
        }

        return nullptr;
    }

    AssetRegistry::AssetRegistry()
    {
    }

    AssetRegistry::~AssetRegistry()
    {
        AssetRegistryRequestBus::MultiHandler::BusDisconnect();
    }

}
