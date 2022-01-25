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
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/ActorManager.h>
#include <Tests/TestAssetCode/ActorFactory.h>

namespace EMotionFX
{
    class TestActorAssets
    {
    public:
        static AZStd::string FileToBase64(const char* filePath);
        static AZ::Data::Asset<Integration::ActorAsset> GetAssetFromActor(const AZ::Data::AssetId& assetId, AZStd::unique_ptr<Actor>&& actor);

        template<class ActorT, typename... Args>
        static AZ::Data::Asset<Integration::ActorAsset> CreateActorAssetAndRegister(const AZ::Data::AssetId& assetId, Args&&... args)
        {
            AZStd::unique_ptr<ActorT> actor = ActorFactory::CreateAndInit<ActorT>(AZStd::forward<Args>(args)...);
            AZ::Data::Asset<Integration::ActorAsset> actorAsset = TestActorAssets::GetAssetFromActor(assetId, AZStd::move(actor));
            GetEMotionFX().GetActorManager()->RegisterActor(actorAsset);
            return AZStd::move(actorAsset);
        }
    };
} // namespace EMotionFX
