/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/BufferDescriptor.h>
#include <Atom/RHI.Reflect/BufferViewDescriptor.h>
#include <Atom/RHI/interval_map.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/sort.h>

namespace AZ::RHI
{
    //! Describes a range of a buffer.
    struct BufferSubresourceRange
    {
        uint64_t m_byteOffset = 0;
        uint64_t m_byteSize = 0;

        BufferSubresourceRange() = default;

        BufferSubresourceRange(uint64_t offset, uint64_t size)
            : m_byteOffset(offset)
            , m_byteSize(size)
        {}

        explicit BufferSubresourceRange(const BufferDescriptor& descriptor)
            : m_byteOffset(0)
            , m_byteSize(descriptor.m_byteCount)
        {}

        explicit BufferSubresourceRange(const BufferViewDescriptor& descriptor)
            : m_byteOffset(static_cast<uint64_t>(descriptor.m_elementOffset) * descriptor.m_elementSize)
            , m_byteSize(static_cast<uint64_t>(descriptor.m_elementCount) * descriptor.m_elementSize)
        {}

        bool operator==(const BufferSubresourceRange& other) const
        {
            return
                m_byteOffset == other.m_byteOffset &&
                m_byteSize == other.m_byteSize;
        }
    };

    //! Utility class to track a property value over multiple ranges of a buffer.
    //! This class will keep track of the values that each buffer range has and
    //! will merge continuous ranges that have the same property value.
    //! For example, this could be use to keep track of queue ownership over a
    //! buffer range.
    template<class T>
    class BufferProperty
    {
    public:
        //! Describes the property value of one range of the buffer.
        struct PropertyRange
        {
            BufferSubresourceRange m_range;
            T m_property;

            bool operator==(const PropertyRange& other) const
            {
                return other.m_range == m_range &&
                    other.m_property == m_property;
            }
        };

        //! Initialize the buffer property.
        void Init(const BufferDescriptor& descriptor);

        //! Returns if has been initialized.
        bool IsInitialized() const;

        //! Sets a new value over a buffer range.
        void Set(const BufferSubresourceRange& range, const T& property);

        //! Returns a list with all the property values over a buffer range.
        AZStd::vector<PropertyRange> Get(const BufferSubresourceRange& range) const;

        //! Removes all property values that were previously set.
        void Reset();

    private:

        // Interval map used to keep track of the property values.
        interval_map<uint64_t, T> m_intervalMap;

        // Descriptor of the buffer.
        BufferDescriptor m_bufferDescriptor;

        // If BufferProperty has been initialized.
        bool m_initialized = false;
    };

    template<class T>
    void BufferProperty<T>::Init(const BufferDescriptor& descriptor)
    {
        Reset();
        m_bufferDescriptor = descriptor;
        m_initialized = true;
    }

    template<class T>
    bool BufferProperty<T>::IsInitialized() const
    {
        return m_initialized;
    }

    template<class T>
    void BufferProperty<T>::Set(const BufferSubresourceRange& range, const T& property)
    {
        AZ_Assert(m_initialized, "BufferProperty has not being initialized");
        m_intervalMap.assign(
            range.m_byteOffset,
            AZStd::min(range.m_byteOffset + range.m_byteSize, static_cast<uint64_t>(m_bufferDescriptor.m_byteCount)),
            property);
    }

    template<class T>
    AZStd::vector<typename BufferProperty<T>::PropertyRange> BufferProperty<T>::Get(const BufferSubresourceRange& range) const
    {
        AZ_Assert(m_initialized, "BufferProperty has not being initialized");
        AZStd::vector<PropertyRange> intervals;
        auto overlap = m_intervalMap.overlap(
            range.m_byteOffset,
            AZStd::min(range.m_byteOffset + range.m_byteSize, static_cast<uint64_t>(m_bufferDescriptor.m_byteCount)));

        uint64_t requestedRangeEnd = range.m_byteOffset + range.m_byteSize;
        for (auto it = overlap.first; it != overlap.second; ++it)
        {
            intervals.emplace_back();
            PropertyRange& propertyRange = intervals.back();
            uint64_t beginRange = AZStd::max(range.m_byteOffset, it.interval_begin());
            uint64_t endRange = AZStd::min(requestedRangeEnd, it.interval_end());
            propertyRange.m_range = BufferSubresourceRange(beginRange, aznumeric_caster(endRange - beginRange));
            propertyRange.m_property = it.value();
        }
        return intervals;
    }

    template<class T>
    void BufferProperty<T>::Reset()
    {
        m_intervalMap.clear();
    }
}
