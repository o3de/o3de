/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <map>
#include <cstdint>
#include <vector>
#include "particle/core/Particle.h"

namespace SimuCore::ParticleCore {
    class ParticleDataSet {
    public:
        explicit ParticleDataSet(uint32_t dataSize)
            : stride(dataSize)
        {
        }

        ~ParticleDataSet() = default;

        uint32_t Alloc();

        void Free(uint32_t index);

        uint8_t* At(uint32_t index);

        uint32_t ActiveSize() const;

        void SetData(const uint8_t* src, uint32_t size);

    protected:
        uint32_t stride;
        std::vector<uint8_t> rawData;
        std::vector<uint32_t> freeList;
    };

    class ParticleDataPool {
    public:
        ParticleDataPool() = default;
        ~ParticleDataPool();

        ParticleDataPool(ParticleDataPool&& pool) = delete;
        ParticleDataPool& operator=(ParticleDataPool&& pool) = delete;

        ParticleDataPool(const ParticleDataPool& pool) = delete;
        ParticleDataPool& operator=(const ParticleDataPool& pool) = delete;

        void Clone(ParticleDataPool& pool) const;

        void Reset();

        void EmplaceData(uint32_t stride, const uint8_t* data, uint32_t dataSize);

        template<typename T>
        uint32_t AllocT(const T& t)
        {
            uint32_t rst = Alloc(sizeof(T));
            uint8_t* data = Data(sizeof(T), rst);
            if (data != nullptr) {
                new (data) T(t);
            }
            return rst;
        }

        uint32_t Alloc(uint32_t size);

        void Free(uint32_t size, uint32_t index);

        uint8_t* Data(uint32_t size, uint32_t index);

        static uint32_t AlignSize(uint32_t size);

        template<typename T>
        T* Data(uint32_t index)
        {
            return reinterpret_cast<T*>(Data(sizeof(T), index));
        }

        const uint8_t* Data(uint32_t size, uint32_t index) const;

        template<typename T>
        const T* Data(uint32_t index) const
        {
            return reinterpret_cast<T*>(Data(sizeof(T), index));
        }

        using DataMap = std::map<uint32_t, ParticleDataSet*>;
        const DataMap& GetDataSets() const
        {
            return sets;
        }

    private:
        DataMap sets;
    };
}
