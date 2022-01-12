/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/sort.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzFramework/Spawnable/SpawnableAssetHandler.h>
#include <AzFramework/Spawnable/SpawnableAssetBus.h>

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
            ResolveEntityAliases(spawnable, asset, stream->GetStreamingDeadline(), stream->GetStreamingPriority(), assetLoadFilterCB);
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
        return "Icons/Components/Viewport/EntityInSlice.png";
    }

    void SpawnableAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
    {
        extensions.push_back(Spawnable::FileExtension);
    }

    uint32_t SpawnableAssetHandler::BuildSubId(AZStd::string_view id)
    {
        AZ::Uuid subIdHash = AZ::Uuid::CreateData(id.data(), id.size());
        return azlossy_caster(subIdHash.GetHash());
    }

    void SpawnableAssetHandler::ResolveEntityAliases(
        Spawnable* spawnable,
        [[maybe_unused]] const AZ::Data::Asset<AZ::Data::AssetData>& asset,
        AZStd::chrono::milliseconds streamingDeadline,
        AZ::IO::IStreamerTypes::Priority streamingPriority,
        const AZ::Data::AssetFilterCB& assetLoadFilterCB)
    {
        Spawnable::EntityAliasVisitor aliases = spawnable->TryGetAliases();
        AZ_Assert(aliases.IsValid(), "Newly created Spawnable '%s' was already locked.", asset.GetHint().c_str());
        if (aliases.HasAliases())
        {
            AZ_Assert(
                AZStd::is_sorted(
                    aliases.begin(), aliases.end(),
                    [](const Spawnable::EntityAlias& lhs, const Spawnable::EntityAlias& rhs)
                    {
                        return lhs.HasLowerIndex(rhs);
                    }),
                "Spawnable '%s' has an unsorted entity alias list.", asset.GetHint().c_str());

            SpawnableAssetEventsBus::Broadcast(
                &SpawnableAssetEvents::OnResolveAliases, aliases, spawnable->GetMetaData(), spawnable->GetEntities());

            // The aliases will only be optimized if OnResolveAliases has made any changes.
            aliases.Optimize();
            aliases.ListSpawnablesRequiringLoad(
                [&assetLoadFilterCB, streamingDeadline, streamingPriority](AZ::Data::Asset<Spawnable>& assetPendingLoad)
                {
                    AZ::Data::AssetLoadParameters loadInfo;
                    loadInfo.m_assetLoadFilterCB = assetLoadFilterCB;
                    loadInfo.m_deadline = streamingDeadline;
                    loadInfo.m_priority = streamingPriority;
                    assetPendingLoad.QueueLoad(loadInfo);
                });
        }
    }
} // namespace AzFramework
