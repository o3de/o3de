/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Asset/AssetCommon.h>
#include <Integration/Assets/AnimGraphAsset.h>

namespace EMotionFX
{
    class AnimGraph;

    class AnimGraphAssetFactory
    {
    public:
        static AZ::Data::Asset<EMotionFX::Integration::AnimGraphAsset> Create(const AZ::Data::AssetId id, AZStd::unique_ptr<AnimGraph> animGraph)
        {
            auto asset = AZ::Data::AssetManager::Instance().CreateAsset<Integration::AnimGraphAsset>(id);
            asset->SetData(animGraph.release());
            return asset;
        }
    };
} // namespace EMotionFX
