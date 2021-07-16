/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Asset/AssetCommon.h>

namespace LmbrCentral
{
    class LensFlareAsset
        : public AZ::Data::AssetData
    {
        friend class LensFlareAssetHandler;

    public:
        AZ_RTTI(LensFlareAsset, "{CF44D1F0-F178-4A3D-A9E6-D44721F50C20}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(LensFlareAsset, AZ::SystemAllocator, 0);

        const AZStd::vector<AZStd::string>& GetPaths() { return m_flarePaths; }

    private:
        void AddPath(AZStd::string&& newPath) { m_flarePaths.emplace_back(std::move(newPath)); }

        AZStd::vector<AZStd::string> m_flarePaths;
    };
} // namespace LmbrCentral
