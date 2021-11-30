/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Handle.h>
#include <Atom/RHI/Allocator.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/queue.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AZ
{
    namespace RHI
    {
        enum class FreeListAllocatorPolicy
        {
            /// Finds the first block that fits the allocation request. Useful
            /// if you want to keep the watermark on the heap low.
            FirstFit = 0,

            /// Finds the best block in the entire heap.
            BestFit
        };

        class FreeListAllocator final
            : public Allocator
        {
        public:
            AZ_CLASS_ALLOCATOR(FreeListAllocator, AZ::SystemAllocator, 0);

            struct Descriptor : public Allocator::Descriptor
            {
                /// The allocation policy to use when allocating blocks.
                FreeListAllocatorPolicy m_policy = FreeListAllocatorPolicy::BestFit;
            };

            FreeListAllocator() = default;

            void Init(const Descriptor& descriptor);

            //////////////////////////////////////////////////////////////////////////
            // Allocator
            void Shutdown() override;
            VirtualAddress Allocate(size_t byteCount, size_t byteAlignment) override;
            void DeAllocate(VirtualAddress allocation) override;
            void GarbageCollect() override;
            void GarbageCollectForce() override;
            size_t GetAllocationCount() const override;
            size_t GetAllocatedByteCount() const override;
            const Descriptor& GetDescriptor() const override;
            void Clone(RHI::Allocator* newAllocator)  override;
            //////////////////////////////////////////////////////////////////////////

        private:
            Descriptor m_descriptor;

            struct Garbage
            {
                size_t m_addressOffset;
                size_t m_garbageCollectCycle;
            };

            bool IsGarbageReady(Garbage& garbage) const;

            void GarbageCollectInternal(const Garbage& garbage);

            using NodeHandle = Handle<size_t>;

            struct Node
            {
                bool IsValid() const
                {
                    return m_size != 0;
                }

                NodeHandle m_nextFree;
                size_t m_addressOffset = 0;
                size_t m_size = 0;
            };

            NodeHandle CreateNode();
            void ReleaseNode(NodeHandle handle);

            NodeHandle InsertNode(NodeHandle handlePrevious, size_t address, size_t size);
            void RemoveNode(NodeHandle handlePrevious, NodeHandle handleCurrent);

            struct FindNodeRequest
            {
                size_t m_requestedSize = 0;
                size_t m_requestedAlignment = 0;
            };

            struct FindNodeResponse
            {
                size_t m_leftoverSize = 0;
                NodeHandle m_handlePrevious;
                NodeHandle m_handleFound;
            };

            void FindNode(const FindNodeRequest& request, FindNodeResponse& response);
            void FindNodeBest(const FindNodeRequest& request, FindNodeResponse& response);
            void FindNodeFirst(const FindNodeRequest& request, FindNodeResponse& response);

            size_t GetRequiredSize(
                size_t requestedSize,
                size_t requestedAlignment,
                size_t addressOffset);

            NodeHandle GetFirstFreeHandle();
            Node& GetNode(NodeHandle handle);

            struct Allocation
            {
                size_t m_byteCount;
                size_t m_byteCountAlignmentPad;
            };

            void FreeInternal(size_t address, const Allocation& allocation);

            NodeHandle m_headHandle;
            AZStd::vector<NodeHandle> m_nodeFreeList;
            AZStd::vector<Node> m_nodes;
            AZStd::unordered_map<size_t, Allocation> m_allocations;
            AZStd::queue<Garbage> m_garbage;
            size_t m_garbageCollectCycle = 0;
            size_t m_byteCountTotal = 0;
        };
    }
}
