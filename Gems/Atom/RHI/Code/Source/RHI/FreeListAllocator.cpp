/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/FreeListAllocator.h>
#include <Atom/RHI.Reflect/Bits.h>

namespace AZ::RHI
{
    bool FreeListAllocator::IsGarbageReady(Garbage& garbage) const
    {
        return (m_garbageCollectCycle - garbage.m_garbageCollectCycle) >= m_descriptor.m_garbageCollectLatency;
    }

    size_t FreeListAllocator::GetAllocatedByteCount() const
    {
        return m_byteCountTotal;
    }

    size_t FreeListAllocator::GetAllocationCount() const
    {
        return m_allocations.size();
    }

    const FreeListAllocator::Descriptor& FreeListAllocator::GetDescriptor() const
    {
        return m_descriptor;
    }

    const FreeListAllocator::Node& FreeListAllocator::GetNode(NodeHandle handle) const
    {
        return m_nodes[handle.GetIndex()];
    }

    FreeListAllocator::Node& FreeListAllocator::GetNode(NodeHandle handle)
    {
        return m_nodes[handle.GetIndex()];
    }

    FreeListAllocator::NodeHandle FreeListAllocator::GetFirstFreeHandle() const
    {
        return GetNode(m_headHandle).m_nextFree;
    }

    FreeListAllocator::NodeHandle FreeListAllocator::CreateNode()
    {
        if (m_nodeFreeList.size())
        {
            NodeHandle handle = m_nodeFreeList.back();
            m_nodeFreeList.pop_back();
            return handle;
        }
        else
        {
            m_nodes.emplace_back();
            return NodeHandle(m_nodes.size() - 1);
        }
    }

    FreeListAllocator::NodeHandle FreeListAllocator::InsertNode(NodeHandle handlePrevious, size_t address, size_t size)
    {
        NodeHandle handle = CreateNode();

        Node& nodeCurrent = GetNode(handle);
        nodeCurrent.m_addressOffset = address;
        nodeCurrent.m_size = size;

        Node& nodePrevious = GetNode(handlePrevious);
        nodeCurrent.m_nextFree = nodePrevious.m_nextFree;
        nodePrevious.m_nextFree = handle;

        return handle;
    }

    void FreeListAllocator::RemoveNode(NodeHandle handlePrevious, NodeHandle handleCurrent)
    {
        Node& nodePrevious = GetNode(handlePrevious);
        Node& nodeCurrent = GetNode(handleCurrent);
        nodePrevious.m_nextFree = nodeCurrent.m_nextFree;
        nodeCurrent.m_nextFree = NodeHandle{};
        ReleaseNode(handleCurrent);
    }

    void FreeListAllocator::ReleaseNode(NodeHandle handle)
    {
        m_nodeFreeList.push_back(handle);
    }

    void FreeListAllocator::Init(const Descriptor& descriptor)
    {
        Shutdown();

        m_descriptor = descriptor;

        // Create a sentinel node.
        m_headHandle = CreateNode();

        // Create the first free node, which is the size of the heap.
        InsertNode(m_headHandle, 0, m_descriptor.m_capacityInBytes);
    }

    void FreeListAllocator::Shutdown()
    {
        while (m_garbage.size())
        {
            m_garbage.pop();
        }
        m_garbageCollectCycle = 0;
        m_byteCountTotal = 0;

        m_nodeFreeList.clear();
        m_nodes.clear();
        m_allocations.clear();
    }

    VirtualAddress FreeListAllocator::Allocate(size_t byteCount, size_t byteAlignment)
    {
        if (byteCount == 0)
        {
            return VirtualAddress::CreateNull();
        }

        byteAlignment = AZStd::max<size_t>(byteAlignment, m_descriptor.m_alignmentInBytes);

        FindNodeRequest request;
        request.m_requestedSize = byteCount;
        request.m_requestedAlignment = byteAlignment;

        FindNodeResponse response;
        FindNode(request, response);

        if (response.m_handleFound.IsNull())
        {
            return VirtualAddress::CreateNull();
        }

        Node& nodeFound = GetNode(response.m_handleFound);
        const size_t byteCountAligned = nodeFound.m_size - response.m_leftoverSize;
        const size_t byteCountAlignmentPad = byteCountAligned - byteCount;
        const size_t byteAddressOffsetAligned = nodeFound.m_addressOffset + byteCountAlignmentPad;

        m_allocations.emplace(byteAddressOffsetAligned, Allocation{ byteCount, byteCountAlignmentPad });

        // If we have space leftover from the free node, split it.
        if (response.m_leftoverSize)
        {
            InsertNode(response.m_handleFound, nodeFound.m_addressOffset + byteCountAligned, response.m_leftoverSize);
        }

        // Remove the found node.
        RemoveNode(response.m_handlePrevious, response.m_handleFound);

        m_byteCountTotal += byteCountAligned;
        AZ_Assert(m_byteCountTotal <= m_descriptor.m_capacityInBytes, "Allocator overflow. Allocated more memory than allowed.");

        return VirtualAddress::CreateFromOffset(m_descriptor.m_addressBase.m_ptr + byteAddressOffsetAligned);
    }

    void FreeListAllocator::DeAllocate(VirtualAddress address)
    {
        if (address.IsNull())
        {
            return;
        }

        const size_t addressOffset = address.m_ptr - m_descriptor.m_addressBase.m_ptr;
        if (addressOffset >= m_descriptor.m_capacityInBytes)
        {
            AZ_Assert(false, "offset is not valid for this allocator");
            return;
        }

        m_garbage.push(Garbage{ addressOffset, m_garbageCollectCycle });
    }

    void FreeListAllocator::GarbageCollect()
    {
        while (m_garbage.size() && IsGarbageReady(m_garbage.front()))
        {
            GarbageCollectInternal(m_garbage.front());
            m_garbage.pop();
        }
        m_garbageCollectCycle++;
    }

    void FreeListAllocator::GarbageCollectForce()
    {
        while (m_garbage.size())
        {
            GarbageCollectInternal(m_garbage.front());
            m_garbage.pop();
        }
    }

    void FreeListAllocator::GarbageCollectInternal(const Garbage& garbage)
    {
        auto it = m_allocations.find(garbage.m_addressOffset);
        if (it == m_allocations.end())
        {
            AZ_Assert(false, "address not valid for allocator");
            return;
        }

        const Allocation& allocation = it->second;
        FreeInternal(garbage.m_addressOffset, allocation);

        const size_t byteCountAligned = allocation.m_byteCount + allocation.m_byteCountAlignmentPad;

        AZ_Assert(m_byteCountTotal >= byteCountAligned, "Allocator underflow. Freeing more memory than allowed.");
        m_byteCountTotal -= byteCountAligned;

        m_allocations.erase(it);
    }

    void FreeListAllocator::FreeInternal(size_t address, const Allocation& allocation)
    {
        const size_t blockAddress = address - allocation.m_byteCountAlignmentPad;
        const size_t blockSize = allocation.m_byteCount + allocation.m_byteCountAlignmentPad;

        NodeHandle handlePrevious = m_headHandle;
        NodeHandle handleNext;
        NodeHandle handle = GetFirstFreeHandle();
        while (handle.IsValid())
        {
            Node& node = GetNode(handle);

            // Find the first free node that has an address greater than the address
            // we are freeing.
            if (blockAddress < node.m_addressOffset)
            {
                handleNext = handle;
                break;
            }

            handlePrevious = handle;
            handle = node.m_nextFree;
        }

        // Compact previous node with current node if they line up.
        bool previousMerged = false;

        if (handlePrevious != m_headHandle)
        {
            Node& nodePrevious = GetNode(handlePrevious);
            if (nodePrevious.m_addressOffset + nodePrevious.m_size == blockAddress)
            {
                nodePrevious.m_size += blockSize;
                previousMerged = true;
            }
        }

        if (!previousMerged)
        {
            handlePrevious = InsertNode(handlePrevious, blockAddress, blockSize);
        }

        // Compact current node with next node if they line up.
        if (handleNext.IsValid())
        {
            Node& nodePrevious = GetNode(handlePrevious);
            Node& nodeNext = GetNode(handleNext);
            AZ_Assert(nodePrevious.m_addressOffset < nodeNext.m_addressOffset, "Allocations not sorted");
            if (nodePrevious.m_addressOffset + nodePrevious.m_size == nodeNext.m_addressOffset)
            {
                nodePrevious.m_size += nodeNext.m_size;
                RemoveNode(handlePrevious, handleNext);
            }
        }
    }

    size_t FreeListAllocator::GetRequiredSize(
        size_t requestedSize,
        size_t requestedAlignment,
        size_t addressOffset)
    {
        uint64_t addressFull = m_descriptor.m_addressBase.m_ptr + addressOffset;
        uint64_t alignedAddress = AlignUpNPOT<uint64_t>(addressFull, requestedAlignment);
        size_t alignmentPad = (size_t)(alignedAddress - addressFull);
        return requestedSize + alignmentPad;
    }

    void FreeListAllocator::FindNode(const FindNodeRequest& request, FindNodeResponse& response)
    {
        switch (m_descriptor.m_policy)
        {
        case FreeListAllocatorPolicy::FirstFit:
            FindNodeFirst(request, response);
            return;
        case FreeListAllocatorPolicy::BestFit:
            FindNodeBest(request, response);
            return;
        }
    }

    void FreeListAllocator::FindNodeFirst(const FindNodeRequest& request, FindNodeResponse& response)
    {
        response.m_handlePrevious = m_headHandle;
        NodeHandle handle = GetFirstFreeHandle();
        while (handle.IsValid())
        {
            Node& node = GetNode(handle);

            const size_t requiredSize =
                GetRequiredSize(request.m_requestedSize, request.m_requestedAlignment, node.m_addressOffset);

            if (requiredSize <= node.m_size)
            {
                response.m_handleFound = handle;
                response.m_leftoverSize = node.m_size - requiredSize;
                return;
            }

            response.m_handlePrevious = handle;
            handle = node.m_nextFree;
        }
    }

    void FreeListAllocator::FindNodeBest(const FindNodeRequest& request, FindNodeResponse& response)
    {
        size_t leftoverSizeMin = static_cast<size_t>(-1);

        NodeHandle handlePrevious = m_headHandle;
        NodeHandle handle = GetFirstFreeHandle();
        while (handle.IsValid())
        {
            Node& node = GetNode(handle);

            const size_t requiredSize =
                GetRequiredSize(request.m_requestedSize, request.m_requestedAlignment, node.m_addressOffset);

            if (requiredSize <= node.m_size)
            {
                const size_t leftoverSize = node.m_size - requiredSize;
                if (leftoverSize < leftoverSizeMin)
                {
                    response.m_handleFound = handle;
                    response.m_handlePrevious = handlePrevious;
                    response.m_leftoverSize = leftoverSize;
                    leftoverSizeMin = leftoverSize;
                }
            }

            handlePrevious = handle;
            handle = node.m_nextFree;
        }
    }

    float FreeListAllocator::ComputeFragmentation() const
    {
        if (m_byteCountTotal == 0 || m_byteCountTotal == m_descriptor.m_capacityInBytes ||
            m_descriptor.m_capacityInBytes == static_cast<size_t>(-1))
        {
            return 0.f;
        }

        size_t maxFreeBlockSize = 0;
        NodeHandle handle = GetFirstFreeHandle();
        while (handle.IsValid())
        {
            const Node& node = GetNode(handle);
            if (node.m_size > maxFreeBlockSize)
            {
                maxFreeBlockSize = node.m_size;
            }
            handle = node.m_nextFree;
        }

        return 1.f - static_cast<float>(maxFreeBlockSize) / (m_descriptor.m_capacityInBytes - m_byteCountTotal);
    }

    void FreeListAllocator::Clone(RHI::Allocator* newAllocator)
    {
        FreeListAllocator* newFreeListAllocator = static_cast<FreeListAllocator*>(newAllocator);
        newFreeListAllocator->m_headHandle = m_headHandle;
        newFreeListAllocator->m_nodeFreeList = m_nodeFreeList;
        newFreeListAllocator->m_nodes = m_nodes;
        newFreeListAllocator->m_allocations = m_allocations;
        newFreeListAllocator->m_garbage = m_garbage;
        newFreeListAllocator->m_garbageCollectCycle = m_garbageCollectCycle;
        newFreeListAllocator->m_byteCountTotal = m_byteCountTotal;
    }
}
