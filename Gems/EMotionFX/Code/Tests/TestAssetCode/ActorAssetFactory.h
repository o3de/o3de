/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Asset/AssetCommon.h>
#include <Integration/Assets/ActorAsset.h>

namespace EMotionFX
{
    class Actor;

    class ActorAssetFactory
    {
    public:
        static AZ::Data::Asset<EMotionFX::Integration::ActorAsset> Create(const AZ::Data::AssetId id, AZStd::unique_ptr<Actor> actor)
        {
            auto asset = AZ::Data::AssetManager::Instance().CreateAsset<EMotionFX::Integration::ActorAsset>(id);
            asset->SetData(AZStd::move(actor));
            return asset;
        }
    };
} // namespace EMotionFX
