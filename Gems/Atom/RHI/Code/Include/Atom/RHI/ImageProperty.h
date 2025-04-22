/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ImageDescriptor.h>
#include <Atom/RHI.Reflect/ImageSubresource.h>
#include <Atom/RHI/interval_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/sort.h>

namespace AZ::RHI
{
    //! Utility class to track a property value of the subresources of an image.
    //! This class will keep track of the values that each subresource has and
    //! will merge continuous ranges that have the same property value. This is
    //! compatible with the way graphic APIs handle image properties.
    //! One possible use could be to keep track of layout states of an image to properly
    //! add barriers between image state transitions.
    template<class T>
    class ImageProperty
    {
    public:
        //! Describes the property value of one image subresource range.
        struct PropertyRange
        {
            ImageSubresourceRange m_range;
            T m_property;

            bool operator==(const PropertyRange& other) const
            {
                return other.m_range == m_range &&
                    other.m_property == m_property;
            }
        };

        //! Initialize the ImageProperty with the descriptor of the image.
        void Init(const ImageDescriptor& descriptor);

        //! Returns if has been initialized.
        bool IsInitialized() const;

        //! Sets a new property value for an image subresource range.
        void Set(const ImageSubresourceRange& range, const T& property);

        //! Returns a list with all the property values over an image subresource range.
        //! Continuous ranges that have the same property will be group together.
        AZStd::vector<PropertyRange> Get(const ImageSubresourceRange& range) const;

        //! Removes all property values that were previously set.
        void Reset();

    private:
        // Utility functions to transform to/from an image subresource index to/from an integer index
        // where the integer index is the position of the subresource if we layout the subresources in order by mip level and image aspect.
        // e.g Layout for an image with 4 mip map levels and 3 array layers and 2 image aspects.
        //
        //            arrays
        //       _______________
        //   m   | 0 | 1  | 2  |
        //   i   | 3 | 4  | 5  |         Image Aspect 0
        //   p   | 6 | 7  | 8  |  
        //   s   | 9 | 10 | 11 |
        //       _________________
        //   m   | 12 | 13  | 14 |
        //   i   | 15 | 16  | 17 |       Image Aspect 1
        //   p   | 18 | 19  | 20 |
        //   s   | 21 | 22  | 23 |
        //
        // All these subresources of the image are layout as a linear array in the interval_map:
        //
        //       [0-1-2-3-4-5-6-7-8-9-10-11-12-13-14-15-16-17-18-19-20-21-22-23]
        //
        // An interval can represent multiple image aspects, or mip map levels. For example, the interval
        // [6, 18) includes the Image Aspect 0, mip levels 2 and 3, and also Image Aspect 1 mip levels 0 and 1.

        uint32_t ConvertSubresourceToIndex(ImageAspect aspect, uint16_t mipSlice, uint16_t arraySlice) const;
        ImageSubresource ConvertIndexToSubresource(uint32_t index) const;
        Interval GetAspectInterval(ImageAspect aspect) const;

        /// Interval map used for tracking the property values across subresources.
        interval_map<uint32_t, T> m_intervalMap;

        /// Image Descriptor.
        ImageDescriptor m_imageDescriptor;

        /// If ImageProperty has been initialized.
        bool m_initialized = false;
    };

    /// Returns the image aspect for the least significant bit set in "flags".
    inline ImageAspect GetMinAspect(ImageAspectFlags flags)
    {
        int flagsMask = aznumeric_caster(flags);
        return static_cast<ImageAspect>(static_cast<uint32_t>(log2(flagsMask & -flagsMask)));
    }

    /// Returns the image aspect for the most significant bit set in "flags".
    inline ImageAspect GetMaxAspect(ImageAspectFlags flags)
    {
        uint32_t flagsMask = aznumeric_caster(flags);
        return static_cast<ImageAspect>(static_cast<uint32_t>(log2(flagsMask)));
    }

    /// Returns if the image aspects present in "aspectFlags" are consecutive.
    /// (i.e. there's no disabled bits between the first and last enabled bit).
    inline bool IsContinousRange(ImageAspectFlags aspectFlags)
    {
        ImageAspect minAspect = GetMinAspect(aspectFlags);
        ImageAspect maxAspect = GetMaxAspect(aspectFlags);
        for (uint32_t i = aznumeric_caster(minAspect); i < static_cast<uint32_t>(maxAspect); ++i)
        {
            if (!CheckBitsAll(aspectFlags, static_cast<ImageAspectFlags>(AZ_BIT(i))))
            {
                return false;
            }
        }

        return true;
    }

    template<class T>
    void ImageProperty<T>::Init(const ImageDescriptor& descriptor)
    {
        Reset();
        m_imageDescriptor = descriptor;
        m_initialized = true;
    }

    template<class T>
    bool ImageProperty<T>::IsInitialized() const
    {
        return m_initialized;
    }

    template<class T>
    void ImageProperty<T>::Set(const ImageSubresourceRange& range, const T& property)
    {
        AZ_Assert(m_initialized, "ImageProperty has not being initialized");
        // Filter ranges and aspect image flags to what the image support.
        ImageSubresourceRange subResourceRange = range;
        subResourceRange.m_mipSliceMax = AZStd::min(static_cast<uint16_t>(m_imageDescriptor.m_mipLevels - 1), subResourceRange.m_mipSliceMax);
        subResourceRange.m_arraySliceMax = AZStd::min(static_cast<uint16_t>(m_imageDescriptor.m_arraySize - 1), subResourceRange.m_arraySliceMax);
        subResourceRange.m_aspectFlags = FilterBits(GetImageAspectFlags(m_imageDescriptor.m_format), subResourceRange.m_aspectFlags);
        const uint32_t mipSize = subResourceRange.m_mipSliceMax - subResourceRange.m_mipSliceMin + 1;
        const uint32_t arraySize = subResourceRange.m_arraySliceMax - subResourceRange.m_arraySliceMin + 1;            

        // Check if the ImageSubresourceRange is continuous so we can just make one call for one interval.
        if (IsContinousRange(subResourceRange.m_aspectFlags) &&
            m_imageDescriptor.m_mipLevels == mipSize &&
            m_imageDescriptor.m_arraySize == arraySize)
        {
            m_intervalMap.assign(
                ConvertSubresourceToIndex(GetMinAspect(subResourceRange.m_aspectFlags), subResourceRange.m_mipSliceMin, subResourceRange.m_arraySliceMin),
                ConvertSubresourceToIndex(GetMaxAspect(subResourceRange.m_aspectFlags), subResourceRange.m_mipSliceMax, subResourceRange.m_arraySliceMax) + 1,
                property);
        }
        else
        {
            // The range is not continuous so we have to go by ImageAspect.
            for (uint32_t i = 0; i < ImageAspectCount; ++i)
            {
                if (!CheckBitsAll(subResourceRange.m_aspectFlags, static_cast<ImageAspectFlags>(AZ_BIT(i))))
                {
                    continue;
                }

                // Check if we can make one call for all miplevels of the image aspect.
                ImageAspect aspect = static_cast<ImageAspect>(i);
                if (m_imageDescriptor.m_arraySize == arraySize)
                {
                    m_intervalMap.assign(
                        ConvertSubresourceToIndex(aspect, subResourceRange.m_mipSliceMin, subResourceRange.m_arraySliceMin),
                        ConvertSubresourceToIndex(aspect, subResourceRange.m_mipSliceMax, subResourceRange.m_arraySliceMax) + 1,
                        property);
                }
                else
                {
                    // Insert intervals by mip level.
                    for (uint16_t mipLevel = subResourceRange.m_mipSliceMin; mipLevel <= subResourceRange.m_mipSliceMax; ++mipLevel)
                    {
                        m_intervalMap.assign(
                            ConvertSubresourceToIndex(aspect, mipLevel, subResourceRange.m_arraySliceMin),
                            ConvertSubresourceToIndex(aspect, mipLevel, subResourceRange.m_arraySliceMax) + 1,
                            property);
                    }
                }
            }
        }            
    }

    template<class T>
    AZStd::vector<typename ImageProperty<T>::PropertyRange> ImageProperty<T>::Get(const ImageSubresourceRange& range) const
    {
        AZ_Assert(m_initialized, "ImageProperty has not being initialized");
        // Filter ranges and aspect image flags to what the image support.
        AZStd::vector<PropertyRange> intervals;
        ImageSubresourceRange subResourceRange = range;
        subResourceRange.m_mipSliceMax = AZStd::min(static_cast<uint16_t>(m_imageDescriptor.m_mipLevels - 1), subResourceRange.m_mipSliceMax);
        subResourceRange.m_arraySliceMax = AZStd::min(static_cast<uint16_t>(m_imageDescriptor.m_arraySize - 1), subResourceRange.m_arraySliceMax);
        subResourceRange.m_aspectFlags = FilterBits(GetImageAspectFlags(m_imageDescriptor.m_format), subResourceRange.m_aspectFlags);
        const uint32_t arraySize = subResourceRange.m_arraySliceMax - subResourceRange.m_arraySliceMin + 1;
        const uint32_t mipSize = subResourceRange.m_mipSliceMax - subResourceRange.m_mipSliceMin + 1;
        const uint32_t subresourcesPerAspect = m_imageDescriptor.m_mipLevels * m_imageDescriptor.m_arraySize;

        auto getIntervals = [&](uint32_t beginIndex, uint32_t endIndex)
        {
            auto overlapInterval = m_intervalMap.overlap(beginIndex, endIndex);
            for (auto it = overlapInterval.first; it != overlapInterval.second; ++it)
            {
                // We may need to split the interval into multiple subresource ranges if it includes
                // multiple image aspects but different subresource count per image aspect.
                // For example, if the interval contains from mip 0 to 5 of aspect 'Plane1' and mip 0 to 3
                // of 'Plane2' (which is a continuous range) we need to split into 2 ranges:
                // Range 1 = 'Plane1' mip 0-5
                // Range 2 = 'Plane2' mip 0-3.
                uint32_t minIndex = AZStd::max(beginIndex, it.interval_begin());
                uint32_t maxIndex = AZStd::min(endIndex, it.interval_end());
                // Traverse the interval by image aspect level.
                for (uint32_t index = minIndex; index < maxIndex;)
                {
                    uint32_t aspectIndex = index / subresourcesPerAspect;
                    Interval aspectInterval = GetAspectInterval(static_cast<ImageAspect>(aspectIndex));
                    uint32_t aspectEndIndex = AZStd::min(aspectInterval.m_max, static_cast<uint32_t>(maxIndex - 1));
                    intervals.emplace_back();
                    PropertyRange& propRange = intervals.back();
                    auto min = ConvertIndexToSubresource(index);
                    auto max = ConvertIndexToSubresource(aspectEndIndex);
                    propRange.m_range.m_mipSliceMin = AZStd::max(min.m_mipSlice, subResourceRange.m_mipSliceMin);
                    propRange.m_range.m_mipSliceMax = AZStd::min(max.m_mipSlice, subResourceRange.m_mipSliceMax);
                    propRange.m_range.m_arraySliceMin = AZStd::max(min.m_arraySlice, subResourceRange.m_arraySliceMin);
                    propRange.m_range.m_arraySliceMax = AZStd::min(max.m_arraySlice, subResourceRange.m_arraySliceMax);
                    propRange.m_range.m_aspectFlags = static_cast<ImageAspectFlags>(AZ_BIT(aspectIndex));
                    propRange.m_property = static_cast<const T>(it.value());
                    index = aspectEndIndex + 1;
                }
            }
        };

        // Check if the ImageSubresourceRange is continuous so we can just make one call for one interval.
        if (IsContinousRange(subResourceRange.m_aspectFlags) &&
            m_imageDescriptor.m_mipLevels == mipSize &&
            m_imageDescriptor.m_arraySize == arraySize)
        {
            getIntervals(
                ConvertSubresourceToIndex(GetMinAspect(subResourceRange.m_aspectFlags), subResourceRange.m_mipSliceMin, subResourceRange.m_arraySliceMin),
                ConvertSubresourceToIndex(GetMaxAspect(subResourceRange.m_aspectFlags), subResourceRange.m_mipSliceMax, subResourceRange.m_arraySliceMax) + 1);
        }
        else
        {
            // Traverse one image aspect at a time.
            for (uint32_t i = 0; i < ImageAspectCount; ++i)
            {
                if (!CheckBitsAll(subResourceRange.m_aspectFlags, static_cast<ImageAspectFlags>(AZ_BIT(i))))
                {
                    continue;
                }

                // Check if we can make one call for all miplevels of the image aspect.
                ImageAspect aspect = static_cast<ImageAspect>(i);
                if (m_imageDescriptor.m_arraySize == arraySize)
                {
                    getIntervals(
                        ConvertSubresourceToIndex(aspect, subResourceRange.m_mipSliceMin, subResourceRange.m_arraySliceMin),
                        ConvertSubresourceToIndex(aspect, subResourceRange.m_mipSliceMax, subResourceRange.m_arraySliceMax) + 1);
                }
                else
                {
                    // Traverse one mip level at a time.
                    for (uint16_t mipLevel = subResourceRange.m_mipSliceMin; mipLevel <= subResourceRange.m_mipSliceMax; ++mipLevel)
                    {
                        getIntervals(
                            ConvertSubresourceToIndex(aspect, mipLevel, subResourceRange.m_arraySliceMin),
                            ConvertSubresourceToIndex(aspect, mipLevel, subResourceRange.m_arraySliceMax) + 1);
                    }                        
                }
            }
        }

        if (intervals.size() > 1)
        {
            // Merge Image Aspects for same intervals per aspect with same property values.
            auto sortFunc = [this](const PropertyRange& lhs, const PropertyRange& rhs)
            {
                uint32_t lhsIndex = ConvertSubresourceToIndex(static_cast<ImageAspect>(0), lhs.m_range.m_mipSliceMin, lhs.m_range.m_arraySliceMin);
                uint32_t rhsIndex = ConvertSubresourceToIndex(static_cast<ImageAspect>(0), rhs.m_range.m_mipSliceMin, rhs.m_range.m_arraySliceMin);
                return lhsIndex < rhsIndex;
            };
            AZStd::sort(intervals.begin(), intervals.end(), sortFunc);
            for (uint32_t i = 0; i < intervals.size() - 1;)
            {
                auto& current = intervals[i];
                const auto& next = intervals[i + 1u];
                if (current.m_range.m_mipSliceMin == next.m_range.m_mipSliceMin &&
                    current.m_range.m_mipSliceMax == next.m_range.m_mipSliceMax &&
                    current.m_range.m_arraySliceMin == next.m_range.m_arraySliceMin &&
                    current.m_range.m_arraySliceMax == next.m_range.m_arraySliceMax &&
                    current.m_property == next.m_property)
                {
                    current.m_range.m_aspectFlags |= next.m_range.m_aspectFlags;
                    intervals.erase(intervals.begin() + i + 1);
                }
                else
                {
                    ++i;
                }
            }
        }
        return intervals;
    }

    template<class T>
    void ImageProperty<T>::Reset()
    {
        m_intervalMap.clear();
    }

    template<class T>
    uint32_t ImageProperty<T>::ConvertSubresourceToIndex(ImageAspect aspect, uint16_t mipSlice, uint16_t arraySlice) const
    {
        return (static_cast<uint32_t>(aspect) * m_imageDescriptor.m_arraySize * m_imageDescriptor.m_mipLevels) + mipSlice * m_imageDescriptor.m_arraySize + arraySlice;
    }

    template<class T>
    ImageSubresource ImageProperty<T>::ConvertIndexToSubresource(uint32_t index) const
    {
        const uint32_t subresourcesPerAspect = m_imageDescriptor.m_mipLevels * m_imageDescriptor.m_arraySize;
        return ImageSubresource(
            static_cast<uint16_t>((index % subresourcesPerAspect) / m_imageDescriptor.m_arraySize),
            static_cast<uint16_t>((index % subresourcesPerAspect) % m_imageDescriptor.m_arraySize),
            static_cast<ImageAspect>(index/ subresourcesPerAspect));
    }

    template<class T>
    Interval ImageProperty<T>::GetAspectInterval(ImageAspect aspect) const
    {
        const uint32_t aspectIndex = aznumeric_caster(aspect);
        const uint32_t subresourcesPerAspect = m_imageDescriptor.m_mipLevels * m_imageDescriptor.m_arraySize;
        const uint32_t beginIndex = aspectIndex * subresourcesPerAspect;
        return Interval(beginIndex, beginIndex + subresourcesPerAspect - 1);
    }
}
