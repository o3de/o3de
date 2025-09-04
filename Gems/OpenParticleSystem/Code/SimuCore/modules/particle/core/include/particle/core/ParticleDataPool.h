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
#include <AzCore/std/containers/vector.h>
#include "particle/core/Particle.h"

namespace SimuCore::ParticleCore {
    class ParticleDataSet {
    public:
        explicit ParticleDataSet(AZ::u32 dataSize)
            : stride(dataSize)
        {
        }

        ~ParticleDataSet() = default;

        AZ::u32 Alloc();

        void Free(AZ::u32 index);

        AZ::u8* At(AZ::u32 index);

        AZ::u32 ActiveSize() const;

        void SetData(const AZ::u8* src, AZ::u32 size);

    protected:
        AZ::u32 stride;
        AZStd::vector<AZ::u8> rawData;
        AZStd::vector<AZ::u32> freeList;
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

        void EmplaceData(AZ::u32 stride, const AZ::u8* data, AZ::u32 dataSize);

        template<typename T>
        AZ::u32 AllocT(const T& t)
        {
            AZ::u32 rst = Alloc(sizeof(T));
            AZ::u8* data = Data(sizeof(T), rst);
            if (data != nullptr) {
                new (data) T(t);
            }
            return rst;
        }

        AZ::u32 Alloc(AZ::u32 size);

        void Free(AZ::u32 size, AZ::u32 index);

        AZ::u8* Data(AZ::u32 size, AZ::u32 index);

        static AZ::u32 AlignSize(AZ::u32 size);

        template<typename T>
        T* Data(AZ::u32 index)
        {
            return reinterpret_cast<T*>(Data(sizeof(T), index));
        }

        const AZ::u8* Data(AZ::u32 size, AZ::u32 index) const;

        template<typename T>
        const T* Data(AZ::u32 index) const
        {
            return reinterpret_cast<T*>(Data(sizeof(T), index));
        }

        using DataMap = std::map<AZ::u32, ParticleDataSet*>;
        const DataMap& GetDataSets() const
        {
            return sets;
        }

    private:
        DataMap sets;
    };
}
