/*
* Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/AnimGraph.h>
#include <Tests/TestAssetCode/AnimGraphAssetFactory.h>

AZ::Data::Asset<EMotionFX::Integration::AnimGraphAsset> EMotionFX::AnimGraphAssetFactory::Create(
    const AZ::Data::AssetId id, AZStd::unique_ptr<AnimGraph> animGraph)
{
    auto asset = AZ::Data::AssetManager::Instance().CreateAsset<Integration::AnimGraphAsset>(id);
    asset->SetData(animGraph.release());
    return asset;
}
