/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StdAfx.h"

#include <Asset/BlastAssetHandler.h>

#include <Asset/BlastAsset.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/GenericStreams.h>

namespace Blast
{
    BlastAssetHandler::~BlastAssetHandler()
    {
        Unregister();
    }

    AZ::Data::AssetPtr BlastAssetHandler::CreateAsset(
        const AZ::Data::AssetId& id, [[maybe_unused]] const AZ::Data::AssetType& type)
    {
        if (!CanHandleAsset(id) || type != GetAssetType())
        {
            AZ_Error("Blast", false, "Invalid asset type! BlastAssetHandler only handle 'BlastAsset'");
            return nullptr;
        }

        return aznew BlastAsset;
    }

    AZ::Data::AssetHandler::LoadResult BlastAssetHandler::LoadAssetData(
        const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
        [[maybe_unused]] const AZ::Data::AssetFilterCB& assetLoadFilterCB)
    {
        if (!stream || asset.GetType() != AZ::AzTypeInfo<BlastAsset>::Uuid())
        {
            AZ_Error(
                "Blast", asset.GetType() == AZ::AzTypeInfo<BlastAsset>::Uuid(),
                "Invalid asset type! We only handle 'BlastAsset'");
            return LoadResult::Error;
        }

        BlastAsset* data = asset.GetAs<BlastAsset>();

        const size_t sizeBytes = static_cast<size_t>(stream->GetLength());
        AZStd::vector<char> buffer;
        buffer.resize_no_construct(sizeBytes);
        stream->Read(sizeBytes, buffer.data());

        return data->LoadFromBuffer(buffer.data(), sizeBytes) ? LoadResult::LoadComplete : LoadResult::Error;
    }

    void BlastAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
    {
        delete ptr;
    }

    void BlastAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
    {
        assetTypes.push_back(AZ::AzTypeInfo<BlastAsset>::Uuid());
    }

    void BlastAssetHandler::Register()
    {
        AZ_Assert(AZ::Data::AssetManager::IsReady(), "Asset manager isn't ready!");
        AZ::Data::AssetManager::Instance().RegisterHandler(this, AZ::AzTypeInfo<BlastAsset>::Uuid());

        AZ::AssetTypeInfoBus::Handler::BusConnect(AZ::AzTypeInfo<BlastAsset>::Uuid());
    }

    void BlastAssetHandler::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(AZ::AzTypeInfo<BlastAsset>::Uuid());

        if (AZ::Data::AssetManager::IsReady())
        {
            AZ::Data::AssetManager::Instance().UnregisterHandler(this);
        }
    }

    AZ::Data::AssetType BlastAssetHandler::GetAssetType() const
    {
        return AZ::AzTypeInfo<BlastAsset>::Uuid();
    }

    const char* BlastAssetHandler::GetAssetTypeDisplayName() const
    {
        return "Blast Asset";
    }

    const char* BlastAssetHandler::GetGroup() const
    {
        return "Blast";
    }

    const char* BlastAssetHandler::GetBrowserIcon() const
    {
        return "Icons/Components/Box.png";
    }

    void BlastAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
    {
        extensions.push_back("blast");
    }
} // namespace Blast
