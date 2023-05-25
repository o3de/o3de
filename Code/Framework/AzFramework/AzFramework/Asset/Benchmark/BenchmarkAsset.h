/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AzFramework
{
    constexpr const char s_benchmarkAssetExtension[] = "benchmark";

    //! BenchmarkAsset is a representative placeholder asset used for asset load benchmarks.
    //! BenchmarkAsset is generated from a BenchmarkSettingsAsset asset.  It is designed to
    //! provide a variety of asset loading scenarios to benchmark by using different sizes
    //! and combinations of dependent asset hierarchies.
    class BenchmarkAsset
        : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(BenchmarkAsset, "{FEDD2FFE-C8E6-4627-9B88-C3A6E9BA8A98}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(BenchmarkAsset, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* context);

        uint64_t m_bufferSize;
        AZStd::vector<uint8_t> m_buffer;
        AZStd::vector<AZ::Data::Asset<BenchmarkAsset>> m_assetReferences;
    };
}// namespace AzFramework
