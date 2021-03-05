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

#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/containers/vector.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include <Tests/TestAssetCode/TestActorAssets.h>

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
