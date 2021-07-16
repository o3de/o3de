/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <Integration/Assets/ActorAsset.h>

namespace EMotionFX
{
    class TestActorAssets
    {
    public:
        static AZStd::string FileToBase64(const char* filePath);
        static AZ::Data::Asset<Integration::ActorAsset> GetAssetFromActor(const AZ::Data::AssetId& assetId, AZStd::unique_ptr<Actor>&& actor);
    };
} // namespace EMotionFX
