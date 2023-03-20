/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/string.h>
#include <AzCore/std/sort.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzFramework/Spawnable/SpawnableAssetBus.h>

namespace AzFramework::SpawnableAssetUtils
{
    void ResolveEntityAliases(
        Spawnable* spawnable,
        [[maybe_unused]] const AZStd::string& spawnableName,
        AZStd::chrono::milliseconds streamingDeadline,
        AZ::IO::IStreamerTypes::Priority streamingPriority,
        const AZ::Data::AssetFilterCB& assetLoadFilterCB)
    {
        Spawnable::EntityAliasVisitor aliases = spawnable->TryGetAliases();
        AZ_Assert(aliases.IsValid(), "Newly created Spawnable '%s' was already locked.", spawnableName.c_str());
        if (aliases.HasAliases())
        {
            AZ_Assert(
                AZStd::is_sorted(
                    aliases.begin(), aliases.end(),
                    [](const Spawnable::EntityAlias& lhs, const Spawnable::EntityAlias& rhs)
                    {
                        return lhs.HasLowerIndex(rhs);
                    }),
                "Spawnable '%s' has an unsorted entity alias list.", spawnableName.c_str());

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
} // AzFramework::SpawnableAssetUtils
