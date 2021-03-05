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
