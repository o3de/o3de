/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Debug/AssetTrackingTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AssetMemoryAnalyzer
{
    class AnalyzerImpl;

    namespace Data
    {
        enum class AllocationCategories
        {
            HEAP,
            VRAM,

            COUNT
        };

        constexpr int ALLOCATION_CATEGORY_COUNT = (int)AllocationCategories::COUNT;

        // A location in code
        struct CodePoint
        {
            const char* m_file;
            int m_line;
            AllocationCategories m_category;
        };

        // Meta-information to attach to an individual allocation
        struct AllocationData
        {
            union CategoryInfo
            {
                struct
                {
                    AZ::IAllocator* m_allocator;
                }
                m_heapInfo;
            };

            CodePoint* m_codePoint;
            CategoryInfo m_categoryInfo;
        };

        // Information about a point in code where allocations occur
        struct AllocationPoint
        {
            struct AllocationInfo
            {
                // Size in bytes
                uint32_t m_size;
            };

            using AllocationInfos = AZStd::vector<AllocationInfo, AZ::Debug::AZStdAssetTrackingAllocator>;

            // The point in code where allocations occur
            const CodePoint* m_codePoint;

            // Total memory allocated through this code point (will be the sum of m_allocations)
            uint32_t m_totalAllocatedMemory = 0;

            // Individual allocations that occurred through this code point
            AllocationInfos m_allocations;
        };

        // Summary information about a group of allocations
        struct Summary
        {
            // Total bytes allocated in the group
            uint32_t m_allocatedMemory = 0;

            // Total number of separate allocations in the group
            uint32_t m_allocationCount = 0;
        };

        // Information about an asset
        struct AssetData
        {
            struct CodePointInfo
            {
                AllocationPoint::AllocationInfos m_allocations;
                uint32_t m_totalBytes = 0;
                AllocationCategories m_category;
            };

            uint32_t m_totalAllocations[ALLOCATION_CATEGORY_COUNT];
            uint32_t m_totalBytes[ALLOCATION_CATEGORY_COUNT];
            AZStd::unordered_map<CodePoint*, CodePointInfo, AZStd::hash<CodePoint*>, AZStd::equal_to<CodePoint*>, AZ::Debug::AZStdAssetTrackingAllocator> m_codePointsToAllocations;
        };

        // Information about a specific asset.
        struct AssetInfo
        {
            // Identifier for the asset.
            const char* m_id = nullptr;

            // Total allocations/bytes for this asset, including allocations for any child assets.
            Summary m_totalSummary[ALLOCATION_CATEGORY_COUNT];

            // Total allocations/bytes for this asset alone, excluding allocations for child assets.
            Summary m_localSummary[ALLOCATION_CATEGORY_COUNT];

            // Child assets (i.e. assets that enter into scope while this asset is already in scope)
            AZStd::vector<AssetInfo, AZ::Debug::AZStdAssetTrackingAllocator> m_childAssets;

            // Points in code at which this asset has made allocations
            AZStd::vector<AllocationPoint, AZ::Debug::AZStdAssetTrackingAllocator> m_allocationPoints;
        };

        typedef AZStd::vector<AllocationPoint, AZ::Debug::AZStdAssetTrackingAllocator> AllocationPoints;
    }

    // Analysis of all loaded assets at a moment in time
    class FrameAnalysis
    {
    public:
        AZ_TYPE_INFO(FrameAnalysis, "{6B7287A6-EE5E-4A9D-B219-586DAD865537}");
        AZ_CLASS_ALLOCATOR(FrameAnalysis, AZ::Debug::AssetTrackingAllocator, 0);

        const Data::AssetInfo& GetRootAsset() const
        {
            return m_rootAsset;
        }

        const Data::AllocationPoints& GetAllocationPoints() const
        {
            return m_allocationPoints;
        }

    private:
        Data::AssetInfo m_rootAsset;
        Data::AllocationPoints m_allocationPoints;

        friend AnalyzerImpl;
    };

    class Analyzer
    {
    public:
        AZ_TYPE_INFO(Analyzer, "{00FB30E2-706C-41E6-9BDD-F52A40CF3366}");
        AZ_CLASS_ALLOCATOR(Analyzer, AZ::Debug::AssetTrackingAllocator, 0);

        Analyzer();
        ~Analyzer();

        AZStd::shared_ptr<FrameAnalysis> GetAnalysis();

    private:
        AZStd::unique_ptr<AnalyzerImpl> m_impl;
    };
}
