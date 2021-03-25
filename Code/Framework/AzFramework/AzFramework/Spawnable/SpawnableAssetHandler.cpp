/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/string/string.h>
#include <AzFramework/Spawnable/SpawnableAssetHandler.h>
#include <AzFramework/Spawnable/Spawnable.h>

namespace AzFramework
{
    SpawnableAssetHandler::SpawnableAssetHandler()
    {
        AZ::AssetTypeInfoBus::MultiHandler::BusConnect(AZ::AzTypeInfo<Spawnable>::Uuid());
    }

    SpawnableAssetHandler::~SpawnableAssetHandler()
    {
        AZ::AssetTypeInfoBus::MultiHandler::BusDisconnect();
    }

    AZ::Data::AssetPtr SpawnableAssetHandler::CreateAsset(const AZ::Data::AssetId& id, [[maybe_unused]] const AZ::Data::AssetType& type)
    {
        AZ_Assert(type == AZ::AzTypeInfo<Spawnable>::Uuid(),
            "Asset handler for Spawnable was given a type that's not a Spawnable: %s", type.ToString<AZStd::string>().c_str());
        return aznew Spawnable(id);
    }

    void SpawnableAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
    {
        delete ptr;
    }

    void SpawnableAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
    {
        assetTypes.push_back(AZ::AzTypeInfo<Spawnable>::Uuid());
    }

    auto SpawnableAssetHandler::LoadAssetData(
        const AZ::Data::Asset<AZ::Data::AssetData>& asset,
        AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
        const AZ::Data::AssetFilterCB& assetLoadFilterCB) -> LoadResult
    {
        Spawnable* spawnable = asset.GetAs<Spawnable>();
        AZ_Assert(spawnable, "Loaded asset data handed to the SpawnableAssetHandler didn't contain a Spawanble.");

        AZ::ObjectStream::FilterDescriptor filter(assetLoadFilterCB);
        if (AZ::Utils::LoadObjectFromStreamInPlace(*stream, *spawnable, nullptr /*SerializeContext*/, filter))
        {
            return AZ::Data::AssetHandler::LoadResult::LoadComplete;
        }
        else
        {
            AZ_Error("Spawnable", false, "Failed to deserialize asset %s.", asset->GetId().ToString<AZStd::string>().c_str());
            return AZ::Data::AssetHandler::LoadResult::Error;
        }
    }

    AZ::Data::AssetType SpawnableAssetHandler::GetAssetType() const
    {
        return AZ::AzTypeInfo<Spawnable>::Uuid();
    }

    const char* SpawnableAssetHandler::GetAssetTypeDisplayName() const
    {
        return "Spawnable";
    }

    const char* SpawnableAssetHandler::GetGroup() const
    {
        return "Prefab";
    }

    const char* SpawnableAssetHandler::GetBrowserIcon() const
    {
        return "Editor/Icons/Components/Viewport/EntityInSlice.png";
    }

    void SpawnableAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
    {
        extensions.push_back(Spawnable::FileExtension);
    }
} // namespace AzFramework
