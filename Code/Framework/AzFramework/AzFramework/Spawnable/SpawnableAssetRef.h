/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzFramework/Spawnable/Spawnable.h>

namespace AzFramework
{
    //! A wrapper around Spawnable asset that can be used by Script Canvas and Lua
    struct SpawnableAssetRef final
    {
        AZ_RTTI(SpawnableAssetRef, "{A96A5037-AD0D-43B6-9948-ED63438C4A52}");
        static void Reflect(AZ::ReflectContext* context);

        SpawnableAssetRef() = default;
        SpawnableAssetRef(const SpawnableAssetRef& rhs);
        SpawnableAssetRef& operator=(const SpawnableAssetRef& rhs);

        void OnSpawnAssetChanged();

        AZ::Data::Asset<Spawnable> m_asset;
    };
}
