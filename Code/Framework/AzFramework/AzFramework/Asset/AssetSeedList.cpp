
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Asset/AssetSeedList.h>
#include <AzCore/Serialization/SerializeContext.h>

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
