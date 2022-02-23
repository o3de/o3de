/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "CreateSpawnTicketNodeable.h"

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectContext.h>

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Asset/AssetCommon.h>

#include <AzFramework/Spawnable/Spawnable.h>

namespace ScriptCanvas::Nodeables::Spawning
{
    struct SpawnableData final
    {
        AZ_RTTI(SpawnableData, "{0BD201B8-3668-42BC-8D09-C4749AAE15D4}");
        static void Reflect(AZ::ReflectContext* context);

        SpawnableData();
        SpawnableData(const SpawnableData& rhs);
        SpawnableData& operator=(const SpawnableData& rhs);

        void OnSpawnAssetChanged();
        void UpdateTicket();

        AZ::Data::Asset<AzFramework::Spawnable> m_spawnableAsset;
        AZStd::vector<uint32_t> m_entityIndices;
        AZStd::shared_ptr<AzFramework::EntitySpawnTicket> m_ticket;

    };
}
