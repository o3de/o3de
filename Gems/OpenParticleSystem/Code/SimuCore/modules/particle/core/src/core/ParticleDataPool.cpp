/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include "particle/core/ParticleDataPool.h"
#include <cstring>

namespace SimuCore::ParticleCore
{
    constexpr AZ::u32 ALIGNMENT = 16;

        ParticleAlignedAllocator::pointer ParticleAlignedAllocator::allocate(
        ParticleAlignedAllocator::size_type byteSize, ParticleAlignedAllocator::size_type alignment)
        {
            if (alignment < 16)
            {
                alignment = 16;
            }
            return AZ::AllocatorInstance<AZ::SystemAllocator>::Get().allocate(byteSize, alignment);
        }
        void ParticleAlignedAllocator::deallocate(
            ParticleAlignedAllocator::pointer ptr, ParticleAlignedAllocator::size_type byteSize, ParticleAlignedAllocator::size_type alignment)
        {
            if (alignment < 16)
            {
                alignment = 16;
            }
            AZ::AllocatorInstance<AZ::SystemAllocator>::Get().deallocate(ptr, byteSize, alignment);
        }
        ParticleAlignedAllocator::pointer reallocate(
            ParticleAlignedAllocator::pointer ptr, ParticleAlignedAllocator::size_type newSize, ParticleAlignedAllocator::align_type newAlignment)
        {
            if (newAlignment < 16)
            {
                newAlignment = 16;
            }
            return AZ::AllocatorInstance<AZ::SystemAllocator>::Get().reallocate(ptr, newSize, newAlignment);
        }

    template <typename T>
    constexpr T AlignUp(const T& value, size_t alignment)
    {
        return static_cast<T>((static_cast<size_t>(value) + (alignment - 1)) & ~(alignment - 1));
    }

    AZ::u32 ParticleDataSet::Alloc()
    {
        if (!freeList.empty()) {
            auto ret = freeList.back();
            freeList.pop_back();
            return ret;
        }
        AZ::u32 ret = static_cast<AZ::u32>(rawData.size());
        rawData.resize(rawData.size() + stride);
        return ret;
    }

    void ParticleDataSet::Free(AZ::u32 index)
    {
        if (index >= rawData.size()) {
            return;
        }

        freeList.emplace_back(index);
    }

    AZ::u8* ParticleDataSet::At(AZ::u32 index)
    {
        if (index >= rawData.size()) {
            return nullptr;
        }
        return &rawData[index];
    }

    AZ::u32 ParticleDataSet::ActiveSize() const
    {
        return static_cast<AZ::u32>(rawData.size() - freeList.size() * stride);
    }

    void ParticleDataSet::SetData(const AZ::u8* src, AZ::u32 size)
    {
        freeList.clear();
        rawData.clear();

        rawData.resize(size);
        (void)memcpy(rawData.data(), src, size);
    }

    ParticleDataPool::~ParticleDataPool()
    {
        Reset();
    }

    void ParticleDataPool::Clone(ParticleDataPool& pool) const
    {
        for (const auto& set : sets) {
            pool.sets.emplace(set.first, new ParticleDataSet(*set.second));
        }
    }

    void ParticleDataPool::Reset()
    {
        for (const auto& set : sets) {
            delete set.second;
        }
        sets.clear();
    }

    void ParticleDataPool::EmplaceData(AZ::u32 stride, const AZ::u8* data, AZ::u32 dataSize)
    {
        AZ::u32 alignSize = AlignUp(stride, ALIGNMENT);
        DataMap::const_iterator iter = sets.emplace(alignSize, new ParticleDataSet(alignSize)).first;
        iter->second->SetData(data, dataSize);
    }

    AZ::u32 ParticleDataPool::Alloc(AZ::u32 size)
    {
        AZ::u32 alignSize = AlignUp(size, ALIGNMENT);
        auto iter = sets.find(alignSize);
        if (iter == sets.end()) {
            iter = sets.emplace(alignSize, new ParticleDataSet(alignSize)).first;
        }

        return iter->second->Alloc();
    }

    void ParticleDataPool::Free(AZ::u32 size, AZ::u32 index)
    {
        AZ::u32 alignSize = AlignUp(size, ALIGNMENT);
        DataMap::const_iterator iter = sets.find(alignSize);
        if (iter != sets.end()) {
            iter->second->Free(index);
        }
    }

    AZ::u32 ParticleDataPool::AlignSize(AZ::u32 size)
    {
        return AlignUp(size, ALIGNMENT);
    }

    AZ::u8* ParticleDataPool::Data(AZ::u32 size, AZ::u32 index)
    {
        AZ::u32 alignSize = AlignUp(size, ALIGNMENT);
        DataMap::const_iterator iter = sets.find(alignSize);
        if (iter == sets.end()) {
            return nullptr;
        }
        return iter->second->At(index);
    }

    const AZ::u8* ParticleDataPool::Data(AZ::u32 size, AZ::u32 index) const
    {
        AZ::u32 alignSize = AlignUp(size, ALIGNMENT);
        auto iter = sets.find(alignSize);
        if (iter == sets.end()) {
            return nullptr;
        }
        return iter->second->At(index);
    }
}
