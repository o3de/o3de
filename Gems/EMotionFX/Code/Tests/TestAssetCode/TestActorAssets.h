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
