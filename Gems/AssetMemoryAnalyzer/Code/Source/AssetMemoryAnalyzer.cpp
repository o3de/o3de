/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AssetMemoryAnalyzer.h"

#include <AzCore/Memory/MemoryDrillerBus.h>
#include <AzCore/Debug/AssetTrackingTypesImpl.h>
#include <AzCore/Debug/AssetTracking.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/make_shared.h>

///////////////////////////////////////////////////////////////////////////////
// CodePoint hash-table support
///////////////////////////////////////////////////////////////////////////////

template<>
struct AZStd::hash<AssetMemoryAnalyzer::Data::CodePoint>
{
    size_t operator()(const AssetMemoryAnalyzer::Data::CodePoint& codePoint) const
    {
        size_t seed = 0;
        AZStd::hash_combine(seed, codePoint.m_file);
        AZStd::hash_combine(seed, codePoint.m_line);

        return seed;
    }
};

namespace AssetMemoryAnalyzer
{
    namespace Data
    {
        inline bool operator==(const CodePoint& lhs, const CodePoint& rhs)
        {
            return lhs.m_file == rhs.m_file &&
                lhs.m_line == rhs.m_line;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// AnalyzerImpl class
///////////////////////////////////////////////////////////////////////////////

namespace AssetMemoryAnalyzer
{
    class AnalyzerImpl :
        public AZ::Debug::MemoryDrillerBus::Handler
    {
    public:
        AZ_TYPE_INFO(AnalyzerImpl, "{E460E4DE-2160-4171-A4B6-3C2DB6692C32}");
        AZ_CLASS_ALLOCATOR(AnalyzerImpl, AZ::Debug::AssetTrackingAllocator, 0);

        AnalyzerImpl();
        ~AnalyzerImpl();

        // MemoryDrillerBus
        void RegisterAllocator(AZ::IAllocator* allocator) override;
        void UnregisterAllocator(AZ::IAllocator* allocator) override;
        void DumpAllAllocations() override;
        void RegisterAllocation(AZ::IAllocator* allocator, void* address, size_t byteSize, size_t alignment, const char* name, const char* fileName, int lineNum, unsigned int stackSuppressCount) override;
        void UnregisterAllocation(AZ::IAllocator* allocator, void* address, size_t byteSize, size_t alignment, AZ::Debug::AllocationInfo* info) override;
        void ReallocateAllocation(AZ::IAllocator* allocator, void* prevAddress, void* newAddress, size_t newByteSize, size_t newAlignment) override;
        void ResizeAllocation(AZ::IAllocator* allocator, void* address, size_t newSize) override;

        AZStd::shared_ptr<FrameAnalysis> GetAnalysis();

    private:
        void RegisterAllocationCommon(void* address, size_t byteSize, const char* fileName, int lineNum, Data::AllocationData::CategoryInfo categoryInfo, Data::AllocationCategories category);
        void UnregisterAllocationCommon(void* address);

        using AssetTree = AZ::Debug::AssetTree<Data::AssetData>;
        using AssetTreeNode = typename AssetTree::NodeType;
        using AllocationTable = AZ::Debug::AllocationTable<Data::AllocationData>;
        using CodePoints = AZStd::unordered_set<Data::CodePoint, AZStd::hash<Data::CodePoint>, AZStd::equal_to<Data::CodePoint>, AZ::Debug::AZStdAssetTrackingAllocator>;
        using mutex_type = AZStd::mutex;
        using lock_type = AZStd::lock_guard<mutex_type>;

        mutex_type m_mutex;
        CodePoints m_codePoints;
        AssetTree m_assetTree;
        AllocationTable m_allocationTable;
        AZ::Debug::AssetTracking m_assetTracking;
        bool m_captureUncategorizedAllocations = false;
        bool m_performingAnalysis = false;
    };


    ///////////////////////////////////////////////////////////////////////////////
    // AnalyzerImpl functions
    ///////////////////////////////////////////////////////////////////////////////

    AnalyzerImpl::AnalyzerImpl() : 
        m_allocationTable(m_mutex),
        m_assetTracking(&m_assetTree, &m_allocationTable)
    {
        AZ::Debug::MemoryDrillerBus::Handler::BusConnect();
    }

    AnalyzerImpl::~AnalyzerImpl()
    {
        AZ::Debug::MemoryDrillerBus::Handler::BusDisconnect();
    }

    void AnalyzerImpl::RegisterAllocator(AZ::IAllocator* allocator)
    {
        AZ_UNUSED(allocator);
    }

    void AnalyzerImpl::UnregisterAllocator(AZ::IAllocator* allocator)
    {
        AZ_UNUSED(allocator);
    }

    void AnalyzerImpl::DumpAllAllocations()
    {
    }

    void AnalyzerImpl::RegisterAllocation(AZ::IAllocator* allocator, void* address, size_t byteSize, size_t alignment, const char* name, const char* fileName, int lineNum, unsigned int stackSuppressCount)
    {
        AZ_UNUSED(name);
        AZ_UNUSED(alignment);
        AZ_UNUSED(stackSuppressCount);

        Data::AllocationData::CategoryInfo categoryInfo;
        categoryInfo.m_heapInfo.m_allocator = allocator;
        RegisterAllocationCommon(address, byteSize, fileName, lineNum, categoryInfo, Data::AllocationCategories::HEAP);
    }

    void AnalyzerImpl::UnregisterAllocation(AZ::IAllocator* allocator, void* address, size_t byteSize, size_t alignment, AZ::Debug::AllocationInfo* info)
    {
        AZ_UNUSED(allocator);
        AZ_UNUSED(byteSize);
        AZ_UNUSED(alignment);
        AZ_UNUSED(info);

        UnregisterAllocationCommon(address);
    }

    void AnalyzerImpl::ReallocateAllocation(AZ::IAllocator* allocator, void* prevAddress, void* newAddress, size_t newByteSize, size_t newAlignment)
    {
        AZ_UNUSED(allocator);
        AZ_UNUSED(newAlignment);

        if (m_performingAnalysis)
        {
            return;
        }
        
        m_allocationTable.ReallocateAllocation(prevAddress, newAddress, newByteSize);
    }

    void AnalyzerImpl::ResizeAllocation(AZ::IAllocator* allocator, void* address, size_t newSize)
    {
        AZ_UNUSED(allocator);

        if (m_performingAnalysis)
        {
            return;
        }

        m_allocationTable.ResizeAllocation(address, newSize);
    }

    void AnalyzerImpl::RegisterAllocationCommon(void* address, size_t byteSize, const char* fileName, int lineNum, Data::AllocationData::CategoryInfo categoryInfo, Data::AllocationCategories category)
    {
        if (m_performingAnalysis)
        {
            return;
        }

        AZ::Debug::AssetTreeNodeBase* activeAsset = m_assetTracking.GetCurrentThreadAsset();

        if (!activeAsset)
        {
            if (m_captureUncategorizedAllocations)
            {
                activeAsset = &m_assetTree.GetRoot();
            }
            else
            {
                return;
            }
        }

        {
            // Store a record for this allocation, at this code-point
            lock_type lock(m_mutex);
            auto insertResult = m_codePoints.emplace(Data::CodePoint{ fileName ? fileName : "<unknown>", lineNum, category });
            Data::CodePoint* cp = &*insertResult.first;
            m_allocationTable.Get().emplace(address, AllocationTable::RecordType{ activeAsset, (uint32_t)byteSize, Data::AllocationData{ cp, categoryInfo } });
            static_cast<typename AssetTree::NodeType*>(activeAsset)->m_data.m_totalAllocations[(int)category]++;
        }
    }

    void AnalyzerImpl::UnregisterAllocationCommon(void* address)
    {
        if (m_performingAnalysis)
        {
            return;
        }

        {
            // Delete the record of this allocation if it exists
            lock_type lock(m_mutex);
            auto& table = m_allocationTable.Get();
            auto itr = table.find(address);

            if (itr != table.end())
            {
                static_cast<typename AssetTree::NodeType*>(itr->second.m_asset)->m_data.m_totalAllocations[(int)itr->second.m_data.m_codePoint->m_category]--;
                table.erase(address);
            }
        }
    }

    AZStd::shared_ptr<FrameAnalysis> AnalyzerImpl::GetAnalysis()
    {
        using namespace Data;

        lock_type lock(m_mutex);
        m_performingAnalysis = true;  // Prevent recursive allocations from disrupting our work

        auto result = AZStd::allocate_shared<FrameAnalysis>(AZ::Debug::AZStdAssetTrackingAllocator());
        FrameAnalysis* analysis = result.get();

        // Walk through all allocations and record their individual contributions to the analysisData for their owning asset
        for (auto& allocationInfo : m_allocationTable.Get())
        {
            auto assetData = &static_cast<typename AssetTree::NodeType*>(allocationInfo.second.m_asset)->m_data;
            auto category = allocationInfo.second.m_data.m_codePoint->m_category;

            // Update total bytes for this asset
            assetData->m_totalBytes[(int)category] += allocationInfo.second.m_size;

            // Locate or create a recording of this code point within the analysis for this asset
            auto codePointItr = assetData->m_codePointsToAllocations.find(allocationInfo.second.m_data.m_codePoint);

            if (codePointItr == assetData->m_codePointsToAllocations.end())
            {
                codePointItr = assetData->m_codePointsToAllocations.emplace(allocationInfo.second.m_data.m_codePoint, AssetData::CodePointInfo()).first;
                codePointItr->second.m_category = category;
            }

            // Update the code point within the analysis for this asset with information about this allocation
            codePointItr->second.m_allocations.emplace_back(AllocationPoint::AllocationInfo{ allocationInfo.second.m_size });
            codePointItr->second.m_totalBytes += allocationInfo.second.m_size;
        }

        // Declare function to recurse through the asset tree, converting the analysisData of every node into matching information in the public API (AssetMemory:: namespace)
        AZStd::function<void(AssetInfo*, AssetTreeNode*, int)> recurse;
        recurse = [&recurse](AssetInfo* outAsset, AssetTreeNode* inAsset, int depth)
        {
            outAsset->m_id = inAsset->m_primaryinfo ? inAsset->m_primaryinfo->m_id->m_id.c_str() : nullptr;

            // For every code point in this asset node, record its allocations
            for (auto& codePointInfo : inAsset->m_data.m_codePointsToAllocations)
            {
                outAsset->m_allocationPoints.emplace_back(AllocationPoint());
                auto allocationPoint = &outAsset->m_allocationPoints.back();
                allocationPoint->m_codePoint = codePointInfo.first;
                allocationPoint->m_allocations.swap(codePointInfo.second.m_allocations);
                allocationPoint->m_totalAllocatedMemory = codePointInfo.second.m_totalBytes;

                // Add these allocations to our total count of allocations for this asset
                int categoryIndex = (int)codePointInfo.first->m_category;
                outAsset->m_localSummary[categoryIndex].m_allocationCount += (uint32_t)allocationPoint->m_allocations.size();

                // Reserve memory for the next frame, as the number of allocations are unlikely to change much over time
                codePointInfo.second.m_allocations.reserve(allocationPoint->m_allocations.size());
                codePointInfo.second.m_totalBytes = 0;  // Reset for next frame
            }

            // Initialize the local and total summary
            for (int categoryIndex = 0; categoryIndex < ALLOCATION_CATEGORY_COUNT; categoryIndex++)
            {
                outAsset->m_localSummary[categoryIndex].m_allocatedMemory = inAsset->m_data.m_totalBytes[categoryIndex];
                outAsset->m_totalSummary[categoryIndex] = outAsset->m_localSummary[categoryIndex];
            }

            // Recurse over child assets
            outAsset->m_childAssets.resize(inAsset->m_children.size());
            size_t childIdx = 0;

            for (auto& inChildItr : inAsset->m_children)
            {
                auto outChild = &outAsset->m_childAssets[childIdx++];
                recurse(outChild, &inChildItr.second, depth + 1);

                // Have child assets contribute to the total summary
                for (int categoryIndex = 0; categoryIndex < ALLOCATION_CATEGORY_COUNT; categoryIndex++)
                {
                    outAsset->m_totalSummary[categoryIndex].m_allocatedMemory += outChild->m_totalSummary[categoryIndex].m_allocatedMemory;
                    outAsset->m_totalSummary[categoryIndex].m_allocationCount += outChild->m_totalSummary[categoryIndex].m_allocationCount;
                }
            }

            // Clear analysis data out for the next frame
            AZStd::for_each(inAsset->m_data.m_totalBytes, inAsset->m_data.m_totalBytes + ALLOCATION_CATEGORY_COUNT, [](uint32_t& x) { x = 0; });
        };

        recurse(&analysis->m_rootAsset, static_cast<typename AssetTree::NodeType*>(&m_assetTree.GetRoot()), 0);

        m_performingAnalysis = false;

        return result;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Analyzer functions
    ///////////////////////////////////////////////////////////////////////////////

    Analyzer::Analyzer() : m_impl(aznew AnalyzerImpl)
    {
    }

    Analyzer::~Analyzer()
    {
    }

    AZStd::shared_ptr<FrameAnalysis> Analyzer::GetAnalysis()
    {
        return m_impl->GetAnalysis();
    }
}
