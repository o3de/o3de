/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "particle/core/ParticleDataPool.h"
#include <cstring>

namespace SimuCore::ParticleCore {
    constexpr uint32_t ALIGNMENT = 16;

    template <typename T>
    constexpr T AlignUp(const T& value, size_t alignment)
    {
        return static_cast<T>((static_cast<size_t>(value) + (alignment - 1)) & ~(alignment - 1));
    }

    uint32_t ParticleDataSet::Alloc()
    {
        if (!freeList.empty()) {
            auto ret = freeList.back();
            freeList.pop_back();
            return ret;
        }
        uint32_t ret = static_cast<uint32_t>(rawData.size());
        rawData.resize(rawData.size() + stride);
        return ret;
    }

    void ParticleDataSet::Free(uint32_t index)
    {
        if (index >= rawData.size()) {
            return;
        }

        freeList.emplace_back(index);
    }

    uint8_t* ParticleDataSet::At(uint32_t index)
    {
        if (index >= rawData.size()) {
            return nullptr;
        }
        return &rawData[index];
    }

    uint32_t ParticleDataSet::ActiveSize() const
    {
        return static_cast<uint32_t>(rawData.size() - freeList.size() * stride);
    }

    void ParticleDataSet::SetData(const uint8_t* src, uint32_t size)
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

    void ParticleDataPool::EmplaceData(uint32_t stride, const uint8_t* data, uint32_t dataSize)
    {
        uint32_t alignSize = AlignUp(stride, ALIGNMENT);
        DataMap::const_iterator iter = sets.emplace(alignSize, new ParticleDataSet(alignSize)).first;
        iter->second->SetData(data, dataSize);
    }

    uint32_t ParticleDataPool::Alloc(uint32_t size)
    {
        uint32_t alignSize = AlignUp(size, ALIGNMENT);
        auto iter = sets.find(alignSize);
        if (iter == sets.end()) {
            iter = sets.emplace(alignSize, new ParticleDataSet(alignSize)).first;
        }

        return iter->second->Alloc();
    }

    void ParticleDataPool::Free(uint32_t size, uint32_t index)
    {
        uint32_t alignSize = AlignUp(size, ALIGNMENT);
        DataMap::const_iterator iter = sets.find(alignSize);
        if (iter != sets.end()) {
            iter->second->Free(index);
        }
    }

    uint32_t ParticleDataPool::AlignSize(uint32_t size)
    {
        return AlignUp(size, ALIGNMENT);
    }

    uint8_t* ParticleDataPool::Data(uint32_t size, uint32_t index)
    {
        uint32_t alignSize = AlignUp(size, ALIGNMENT);
        DataMap::const_iterator iter = sets.find(alignSize);
        if (iter == sets.end()) {
            return nullptr;
        }
        return iter->second->At(index);
    }

    const uint8_t* ParticleDataPool::Data(uint32_t size, uint32_t index) const
    {
        uint32_t alignSize = AlignUp(size, ALIGNMENT);
        auto iter = sets.find(alignSize);
        if (iter == sets.end()) {
            return nullptr;
        }
        return iter->second->At(index);
    }
}
