/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/containers/vector.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include <Tests/TestAssetCode/TestActorAssets.h>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <Tests/TestAssetCode/JackActor.h>

namespace EMotionFX
{
    AZStd::string TestActorAssets::FileToBase64(const char* filePath)
    {
        AZ::IO::SystemFile systemFile;
        if (systemFile.Open(filePath, AZ::IO::SystemFile::SF_OPEN_READ_ONLY))
        {
            const size_t sizeInBytes = systemFile.Length();
            AZStd::vector<AZ::u8> dataToEncode;
            dataToEncode.resize(sizeInBytes);
            systemFile.Read(sizeInBytes, dataToEncode.begin());

            return AzFramework::StringFunc::Base64::Encode(dataToEncode.begin(), sizeInBytes);
        }

        return AZStd::string();
    }

    AZ::Data::Asset<Integration::ActorAsset> TestActorAssets::GetAssetFromActor(const AZ::Data::AssetId& assetId, AZStd::unique_ptr<Actor>&& actor)
    {
        AZ::Data::Asset<Integration::ActorAsset> actorAsset = AZ::Data::AssetManager::Instance().CreateAsset<Integration::ActorAsset>(assetId);
        actorAsset.GetAs<Integration::ActorAsset>()->SetData(AZStd::move(actor));
        return actorAsset;
    }
} // namespace EMotionFX
