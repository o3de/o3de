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

#pragma once

#include <AzCore/Debug/AssetTrackingTypes.h>

#include <AzCore/std/containers/map.h>

namespace AZ
{
    namespace Debug
    {
        // A node in the current asset state tree.
        // Each thread maintains a stack of currently in-scope assets. As this stack changes the asset tree forms.
        // The same asset may appear in multiple places in the tree, e.g. if asset A is a common asset loaded by both asset B and asset C, the tree may look like:
        //    Root -> B -> A
        //       \--> C -> A
        template<typename AssetDataT>
        class AssetTreeNode : public AssetTreeNodeBase
        {
        public:
            AssetTreeNode(const AssetMasterInfo* masterInfo = nullptr, AssetTreeNode* parent = nullptr) :
                m_masterInfo(masterInfo),
                m_parent(parent)
            {
            }

            const AssetMasterInfo* GetAssetMasterInfo() const override
            {
                return m_masterInfo;
            }

            AssetTreeNodeBase* FindOrAddChild(const AssetTrackingId& id, const AssetMasterInfo* info) override
            {
                AssetTreeNodeBase* result = nullptr;
                auto childItr = m_children.find(id);

                if (childItr != m_children.end())
                {
                    result = &childItr->second;
                }
                else
                {
                    auto childResult = m_children.emplace(id, AssetTreeNode(info, this));
                    result = &childResult.first->second;
                }

                return result;
            }


            using AssetMap = AssetTrackingMap<AssetTrackingId, AssetTreeNode>;

            const AssetMasterInfo* m_masterInfo;
            AssetTreeNode* m_parent;
            AssetMap m_children;
            AssetDataT m_data;
        };

        template<typename AssetDataT>
        class AssetTree : public AssetTreeBase
        {
        public:
            AssetTreeNodeBase& GetRoot() override
            {
                return m_rootAssets;
            }

            using NodeType = AssetTreeNode<AssetDataT>;

            NodeType m_rootAssets;
        };


        template<typename AllocationDataT>
        struct AllocationRecord
        {
            AssetTreeNodeBase* m_asset;
            uint32_t m_size;
            AllocationDataT m_data;
        };


        template<typename AllocationDataT>
        class AllocationTable : public AssetAllocationTableBase
        {
        public:
            using RecordType = AllocationRecord<AllocationDataT>;
            using AllocationReverseMap = AZStd::map<void*, RecordType, AZStd::greater<void*>, AZStdAssetTrackingAllocator>;
            using mutex_type = AZStd::mutex;
            using lock_type = AZStd::lock_guard<mutex_type>;

            AllocationTable(mutex_type& mutex) : m_mutex(mutex)
            {
            }

            AssetTreeNodeBase* FindAllocation(void* ptr) const override
            {
                // Note that ptr is not guaranteed to have an exact entry in the map. For instance, ptr may point to a member of the original object that was allocated, or 
                // ptr may be a different "this" pointer in the case of multiple inheritance.
                //
                // To solve this, we use lower_bound() and check to see if ptr falls in the range of the nearest allocation. Our map uses AZStd::greater instead of 
                // AZStd::less as its sorting function, and thus sorts largest-to-smallest instead of smallest-to-largest. This causes lower_bound() to return the first 
                // iterator that is not greater than otherAllocation, i.e. less than or equal to ptr.
                lock_type lock(m_mutex);
                auto itr = m_allocationTable.lower_bound(ptr);
                AssetTreeNodeBase* result = nullptr;

                if (itr != m_allocationTable.end())
                {
                    // Check if otherAllocation is within the size range of the allocation we found
                    if (reinterpret_cast<uintptr_t>(ptr) <= reinterpret_cast<uintptr_t>(itr->first) + itr->second.m_size)
                    {
                        result = itr->second.m_asset;
                    }
                }

                return result;
            }

            void ReallocateAllocation(void* prevAddress, void* newAddress, size_t newByteSize)
            {
                lock_type lock(m_mutex);
                auto itr = m_allocationTable.find(prevAddress);

                if (itr != m_allocationTable.end())
                {
                    RecordType newAllocation = itr->second;
                    newAllocation.m_size = (uint32_t)newByteSize;

                    m_allocationTable.erase(itr);
                    m_allocationTable.emplace(newAddress, AZStd::move(newAllocation));
                }
            }

            void ResizeAllocation(void* address, size_t newSize)
            {
                // Resize an existing allocation if we can find it
                lock_type lock(m_mutex);
                auto itr = m_allocationTable.find(address);

                if (itr != m_allocationTable.end())
                {
                    itr->second.m_size = (uint32_t)newSize;
                }
            }

            AllocationReverseMap& Get()
            {
                return m_allocationTable;
            }

            const AllocationReverseMap& Get() const
            {
                return m_allocationTable;
            }

        private:
            AllocationReverseMap m_allocationTable;
            mutex_type& m_mutex;
        };
    }
}
