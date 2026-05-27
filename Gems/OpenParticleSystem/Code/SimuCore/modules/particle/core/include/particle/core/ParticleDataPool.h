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

namespace SimuCore::ParticleCore
{
    //! A class allocator which always aligns to at least 16 byte alignment
    class ParticleAlignedAllocator
    {
    public:
        using value_type = void;
        using pointer = void*;
        using size_type = AZStd::size_t;
        using difference_type = AZStd::ptrdiff_t;
        using align_type = AZStd::size_t;

        AZ_FORCE_INLINE ParticleAlignedAllocator() = default;
        AZ_FORCE_INLINE ParticleAlignedAllocator(const char*) {};
        AZ_FORCE_INLINE ParticleAlignedAllocator(const ParticleAlignedAllocator& rhs) = default;
        AZ_FORCE_INLINE ParticleAlignedAllocator([[maybe_unused]] const ParticleAlignedAllocator& rhs, [[maybe_unused]] const char* name)
        {
        }
        AZ_FORCE_INLINE ParticleAlignedAllocator& operator=(const ParticleAlignedAllocator& rhs) = default;

        pointer allocate(size_type byteSize, size_type alignment);
        void deallocate(pointer ptr, size_type byteSize, size_type alignment);
        pointer reallocate(pointer ptr, size_type newSize, align_type alignment);
    };

    AZ_FORCE_INLINE bool operator==(const ParticleAlignedAllocator& a, const ParticleAlignedAllocator& b)
    {
        (void)a;
        (void)b;
        return true;
    }

    AZ_FORCE_INLINE bool operator!=(const ParticleAlignedAllocator& a, const ParticleAlignedAllocator& b)
    {
        (void)a;
        (void)b;
        return false;
    }

    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME(ParticleAlignedAllocator, "{08DEB93F-D7A1-4494-B772-008C185D7E20}", "ParticleAlignedAllocator");
    
    using SIMDAlignedBuffer = AZStd::vector<AZ::u8, ParticleAlignedAllocator>;

    class ParticleDataSet
    {
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
        // the rawData buffer is organized as a vector of pooled data, but that pooled data contains objects at fixed stride offsets,
        // and those objects might themselves be SIMD aligned structures.  The code internally aligns everything inside the buffer, but we must make sure
        // the buffer itself is aligned so that all the internal alignments are valid.
        SIMDAlignedBuffer rawData;
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
