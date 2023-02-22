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
#include <AzCore/Serialization/ObjectStream.h>

namespace AzFramework
{
    constexpr const char s_benchmarkSettingsAssetExtension[] = "benchmarksettings";

    //! This provides a compact means of locally generating a large volume of test assets used for load benchmarks.
    //! The generated assets can be used for benchmarking asset loads in a variety of situations.
    //!
    //! To use:
    //! In the LY Editor, go to the Asset Editor and create a new "Benchmark Settings Asset" asset.  Set the following
    //! settings as appropriate for your test:
    //!    - "Asset Buffer Size":            The number of bytes to save in the buffer for the top-level asset.
    //!    - "Dependent Asset Buffer Size":  The number of bytes to save in the buffer for every asset below the top-level one.
    //!    - "Dependency Depth":             The depth of the generated asset dependency tree.
    //!    - "Assets Per Dependency":        The number of assets to generate per asset in the dependency tree.
    //!    - "Asset Storage Type":           Whether to save the asset as JSON, XML, or BINARY.
    //!                                      NOTE:  With text-based formats, every byte in the asset buffer takes up 2 bytes in
    //!                                      the final output.
    //!
    //! Example tests and their associated settings:
    //!    - Load 1 100 MB asset:
    //!        Asset Buffer Size:              100 MB
    //!        Dependent Asset Buffer Size:    0
    //!        Dependency Depth:               0
    //!        Assets Per Dependency:          0
    //!
    //!    - Load 1 10 MB asset that directly depends on 100 1 MB assets:
    //!        Asset Buffer Size:              10 MB
    //!        Dependent Asset Buffer Size:    1 MB
    //!        Dependency Depth:               1
    //!        Assets Per Dependency:          100
    //!
    //!    - Load 1 10 MB asset that depends on 5 1 MB assets, each of which also depends on 5 1 MB assets:
    //!        Asset Buffer Size:              10 MB
    //!        Dependent Asset Buffer Size:    1 MB
    //!        Dependency Depth:               2
    //!        Assets Per Dependency:          5
    class BenchmarkSettingsAsset
        : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(BenchmarkSettingsAsset, "{D570D0DD-CE8D-4DF3-BC3E-77DB92D72626}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(BenchmarkSettingsAsset, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* context);

        uint64_t m_primaryAssetByteSize = 1024;
        uint64_t m_dependentAssetByteSize = 0;
        uint32_t m_numAssetsPerDependency = 0;
        uint32_t m_dependencyDepth = 0;

        AZ::DataStream::StreamType m_assetStorageType = AZ::DataStream::StreamType::ST_BINARY;
    };
}// namespace AzFramework
