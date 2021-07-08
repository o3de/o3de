/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <Integration/Assets/MotionSetAsset.h>

namespace EMotionFX
{
    class MotionSet;

    class MotionSetAssetFactory
    {
    public:
        static AZ::Data::Asset<EMotionFX::Integration::MotionSetAsset> Create(const AZ::Data::AssetId id, MotionSet* motionSet)
        {
            auto asset = AZ::Data::AssetManager::Instance().CreateAsset<EMotionFX::Integration::MotionSetAsset>(id);
            asset->SetData(motionSet);
            return asset;
        }
    };
} // namespace EMotionFX
