
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzFramework/Platform/PlatformDefaults.h>

namespace AzFramework
{
    struct SeedInfo
    {
        AZ_TYPE_INFO(SeedInfo, "{FACC3682-2ACA-4AA4-B85A-07AD276D18A0}");

        SeedInfo() = default;
        SeedInfo(AZ::Data::AssetId assetId, PlatformFlags platformFlags, const AZStd::string& path, const AZStd::string& seedListFilePath = AZStd::string());

        static void Reflect(AZ::ReflectContext* context);

        AZ::Data::AssetId m_assetId;
        PlatformFlags m_platformFlags;
        AZStd::string m_assetRelativePath;
        AZStd::string m_seedListFilePath;
    };

    using AssetSeedList = AZStd::vector<SeedInfo>;
    class AssetSeedListReflector
    {
    public:

        AZ_TYPE_INFO(AssetSeedListReflector, "{26E389E4-087B-4C79-883F-7216181189BF}");
        AZ_CLASS_ALLOCATOR(AssetSeedListReflector, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* context);

        AssetSeedList m_fileInfoList;
    };
} // namespace AzFramework
