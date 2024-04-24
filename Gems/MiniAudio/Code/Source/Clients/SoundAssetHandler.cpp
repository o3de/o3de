/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <Clients/MiniAudioPlaybackComponent.h>
#include <Clients/SoundAssetHandler.h>
#include <MiniAudio/SoundAsset.h>

namespace MiniAudio
{
    SoundAssetHandler::SoundAssetHandler()
    {
        Register();
    }

    SoundAssetHandler::~SoundAssetHandler()
    {
        Unregister();
    }

    void SoundAssetHandler::Register()
    {
        const bool assetManagerReady = AZ::Data::AssetManager::IsReady();
        AZ_Error("SoundAssetHandler", assetManagerReady, "Asset manager isn't ready.");
        if (assetManagerReady)
        {
            AZ::Data::AssetManager::Instance().RegisterHandler(this, AZ::AzTypeInfo<SoundAsset>::Uuid());
        }

        AZ::AssetTypeInfoBus::Handler::BusConnect(AZ::AzTypeInfo<SoundAsset>::Uuid());
    }

    void SoundAssetHandler::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect();

        if (AZ::Data::AssetManager::IsReady())
        {
            AZ::Data::AssetManager::Instance().UnregisterHandler(this);
        }
    }

    // AZ::AssetTypeInfoBus
    AZ::Data::AssetType SoundAssetHandler::GetAssetType() const
    {
        return AZ::AzTypeInfo<SoundAsset>::Uuid();
    }

    void SoundAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
    {
        extensions.push_back(SoundAsset::FileExtension);
    }

    const char* SoundAssetHandler::GetAssetTypeDisplayName() const
    {
        return "Sound Asset (MiniAudio Gem)";
    }

    const char* SoundAssetHandler::GetBrowserIcon() const
    {
        return "Icons/Components/ColliderMesh.svg";
    }

    const char* SoundAssetHandler::GetGroup() const
    {
        return "Sound";
    }

    // Disable spawning of physics asset entities on drag and drop
    AZ::Uuid SoundAssetHandler::GetComponentTypeId() const
    {
        // NOTE: This doesn't do anything when CanCreateComponent returns false
        return AZ::Uuid(EditorMiniAudioPlaybackComponentTypeId);
    }

    bool SoundAssetHandler::CanCreateComponent([[maybe_unused]] const AZ::Data::AssetId& assetId) const
    {
        return false;
    }

    // AZ::Data::AssetHandler
    AZ::Data::AssetPtr SoundAssetHandler::CreateAsset([[maybe_unused]] const AZ::Data::AssetId& id, const AZ::Data::AssetType& type)
    {
        if (type == AZ::AzTypeInfo<SoundAsset>::Uuid())
        {
            return aznew SoundAsset();
        }

        AZ_Error("SoundAssetHandler", false, "This handler deals only with SoundAsset type.");
        return nullptr;
    }

    AZ::Data::AssetHandler::LoadResult SoundAssetHandler::LoadAssetData(
        const AZ::Data::Asset<AZ::Data::AssetData>& asset,
        AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
        [[maybe_unused]] const AZ::Data::AssetFilterCB& assetLoadFilterCB)
    {
        const bool result = AZ::Utils::LoadObjectFromStreamInPlace<SoundAsset>(*stream, *asset.GetAs<SoundAsset>());
        if (result == false)
        {
            AZ_Error(__FUNCTION__, false, "Failed to load asset");
            return AssetHandler::LoadResult::Error;
        }

        return AssetHandler::LoadResult::LoadComplete;
    }

    void SoundAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
    {
        delete ptr;
    }

    void SoundAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
    {
        assetTypes.push_back(AZ::AzTypeInfo<SoundAsset>::Uuid());
    }
} // namespace MiniAudio
