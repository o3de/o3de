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

namespace ScriptCanvas::Nodeables::Spawning
{
    //! A wrapper around Spawnable that can be used by Script Canvas
    struct SpawnableAsset final
    {
        AZ_RTTI(SpawnableAsset, "{A96A5037-AD0D-43B6-9948-ED63438C4A52}");
        static void Reflect(AZ::ReflectContext* context);

        SpawnableAsset() = default;
        SpawnableAsset(const SpawnableAsset& rhs);
        SpawnableAsset& operator=(const SpawnableAsset& rhs);

        void OnSpawnAssetChanged();

        AZ::Data::Asset<AzFramework::Spawnable> m_asset;
    };
}
