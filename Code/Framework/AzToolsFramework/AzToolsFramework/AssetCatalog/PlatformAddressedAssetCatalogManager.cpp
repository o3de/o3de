/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/AssetCatalog/PlatformAddressedAssetCatalogManager.h>
#include <AzCore/Interface/Interface.h>
#include <AzFramework/Asset/NetworkAssetNotification_private.h>

namespace AzToolsFramework
{
    PlatformAddressedAssetCatalogManager::PlatformAddressedAssetCatalogManager()
    {
        LoadCatalogs();
        AzFramework::AssetSystem::NetworkAssetUpdateInterface* notificationInterface = AZ::Interface<AzFramework::AssetSystem::NetworkAssetUpdateInterface>::Get();
        // If there is an AssetCatalog around it gets the updates
        if (!notificationInterface)
        {
            AZ::Interface<AzFramework::AssetSystem::NetworkAssetUpdateInterface>::Register(this);
        }
    }

    PlatformAddressedAssetCatalogManager::PlatformAddressedAssetCatalogManager(AzFramework::PlatformId platformId)
    {
        LoadSingleCatalog(platformId);
        // If there is an AssetCatalog around it gets the updates
        AzFramework::AssetSystem::NetworkAssetUpdateInterface* notificationInterface = AZ::Interface<AzFramework::AssetSystem::NetworkAssetUpdateInterface>::Get();
        if (!notificationInterface)
        {
            AZ::Interface<AzFramework::AssetSystem::NetworkAssetUpdateInterface>::Register(this);
        }
    }

    PlatformAddressedAssetCatalogManager::~PlatformAddressedAssetCatalogManager()
    {
        AzFramework::AssetSystem::NetworkAssetUpdateInterface* notificationInterface = AZ::Interface<AzFramework::AssetSystem::NetworkAssetUpdateInterface>::Get();
        if (notificationInterface == this)
        {
            AZ::Interface<AzFramework::AssetSystem::NetworkAssetUpdateInterface>::Unregister(this);
        }
    }

    void PlatformAddressedAssetCatalogManager::LoadCatalogs()
    {
        m_assetCatalogList.clear();
        for (int platformNum = AzFramework::PlatformId::PC; platformNum < AzFramework::PlatformId::NumPlatformIds; ++platformNum)
        {
            if (PlatformAddressedAssetCatalog::CatalogExists(static_cast<AzFramework::PlatformId>(platformNum)))
            {
                m_assetCatalogList.emplace_back(AZStd::make_unique<PlatformAddressedAssetCatalog>(static_cast<AzFramework::PlatformId>(platformNum)));
            }
        }
    }

    void PlatformAddressedAssetCatalogManager::LoadSingleCatalog(AzFramework::PlatformId platformId)
    {
        m_assetCatalogList.clear();
        if (platformId != AzFramework::PlatformId::Invalid)
        {
            m_assetCatalogList.emplace_back(AZStd::make_unique<PlatformAddressedAssetCatalog>(platformId));
        }
    }

    void PlatformAddressedAssetCatalogManager::TakeSingleCatalog(AZStd::unique_ptr<AzToolsFramework::PlatformAddressedAssetCatalog>&& newCatalog)
    {
        m_assetCatalogList.emplace_back(AZStd::move(newCatalog));
    }

    AZStd::vector<AzFramework::PlatformId> PlatformAddressedAssetCatalogManager::GetEnabledPlatforms()
    {
        AZStd::vector<AzFramework::PlatformId> returnVec;
        for (int platformNum = AzFramework::PlatformId::PC; platformNum < AzFramework::PlatformId::NumPlatformIds; ++platformNum)
        {
            if (PlatformAddressedAssetCatalog::CatalogExists(static_cast<AzFramework::PlatformId>(platformNum)))
            {
                returnVec.push_back(static_cast<AzFramework::PlatformId>(platformNum));
            }
        }
        return returnVec;
    }

    AZStd::string PlatformAddressedAssetCatalogManager::GetEnabledPlatformsString()
    {
        auto enabledVec = GetEnabledPlatforms();
        AzFramework::PlatformFlags platformFlags = AzFramework::PlatformFlags::Platform_NONE;
        for (auto thisPlatform : enabledVec)
        {
            platformFlags |= AzFramework::PlatformHelper::GetPlatformFlagFromPlatformIndex(thisPlatform);
        }
        return AZStd::string_view(AzFramework::PlatformHelper::GetCommaSeparatedPlatformList(platformFlags));
    }

    void PlatformAddressedAssetCatalogManager::AssetChanged(const AZStd::vector<AzFramework::AssetSystem::AssetNotificationMessage>& messages, bool isCatalogInitialize)
    {
        if(messages.empty())
        {
            return;
        }

        // All messages are required to be for the same platform, so just check the first
        int platformId = AzFramework::PlatformHelper::GetPlatformIndexFromName(messages[0].m_platform.c_str());
        for (const auto& thisCatalog : m_assetCatalogList)
        {
            if (thisCatalog->GetPlatformId() == platformId)
            {
                thisCatalog->AssetChanged(messages, isCatalogInitialize);
            }
        }
    }

    void PlatformAddressedAssetCatalogManager::AssetRemoved(const AZStd::vector<AzFramework::AssetSystem::AssetNotificationMessage>& messages)
    {
        if (messages.empty())
        {
            return;
        }

        // All messages are required to be for the same platform, so just check the first
        int platformId = AzFramework::PlatformHelper::GetPlatformIndexFromName(messages[0].m_platform.c_str());
        for (const auto& thisCatalog : m_assetCatalogList)
        {
            if (thisCatalog->GetPlatformId() == platformId)
            {
                thisCatalog->AssetRemoved(messages);
            }
        }
    }

    AZStd::string PlatformAddressedAssetCatalogManager::GetSupportedPlatforms()
    {
        return GetEnabledPlatformsString();
    }
} // namespace AzToolsFramework
