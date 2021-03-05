/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
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
        AZ_CLASS_ALLOCATOR(BenchmarkAsset, AZ::SystemAllocator, 0);
        static void Reflect(AZ::ReflectContext* context);

        uint64_t m_bufferSize;
        AZStd::vector<uint8_t> m_buffer;
        AZStd::vector<AZ::Data::Asset<BenchmarkAsset>> m_assetReferences;
    };
}// namespace AzFramework
