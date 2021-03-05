
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

#include <AzFramework/Asset/AssetSeedList.h>

namespace AzFramework
{
    SeedInfo::SeedInfo(AZ::Data::AssetId assetId, PlatformFlags platformFlags, const AZStd::string& assetRelativePath, const AZStd::string& seedListFilePath)
        : m_assetId(assetId)
        , m_platformFlags(platformFlags)
        , m_assetRelativePath(assetRelativePath)
        , m_seedListFilePath(seedListFilePath)
    {
    }

    void SeedInfo::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SeedInfo>()
                ->Version(2)
                ->Field("assetId", &SeedInfo::m_assetId)
                ->Field("platformFlags", &SeedInfo::m_platformFlags)
                ->Field("pathHint", &SeedInfo::m_assetRelativePath);
        }

    }

    void AssetSeedListReflector::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AssetSeedListReflector>()
                ->Version(1)
                ->Field("assetList", &AssetSeedListReflector::m_fileInfoList);
        }
    }
} // namespace AzFramework
