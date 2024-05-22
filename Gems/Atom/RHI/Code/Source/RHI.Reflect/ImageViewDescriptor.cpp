/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/ImageViewDescriptor.h>
#include <Atom/RHI.Reflect/Interval.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ::RHI
{
    ImageViewDescriptor::ImageViewDescriptor(
        Format overrideFormat)
        : m_overrideFormat{ overrideFormat }
    {}

    void ImageViewDescriptor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<ImageViewDescriptor>()
                ->Version(1)
                ->Field("MipSliceMin", &ImageViewDescriptor::m_mipSliceMin)
                ->Field("MipSliceMax", &ImageViewDescriptor::m_mipSliceMax)
                ->Field("ArraySliceMin", &ImageViewDescriptor::m_arraySliceMin)
                ->Field("ArraySliceMax", &ImageViewDescriptor::m_arraySliceMax)
                ->Field("OverrideFormat", &ImageViewDescriptor::m_overrideFormat)
                ->Field("OverrideBindFlags", &ImageViewDescriptor::m_overrideBindFlags)
                ->Field("IsCubemap", &ImageViewDescriptor::m_isCubemap)
                ->Field("AspectFlags", &ImageViewDescriptor::m_aspectFlags)
                ->Field("IsArray", &ImageViewDescriptor::m_isArray)
                ;
        }
    }

    ImageViewDescriptor ImageViewDescriptor::Create(
        Format format,
        uint16_t mipSliceMin,
        uint16_t mipSliceMax)
    {
        ImageViewDescriptor descriptor;
        descriptor.m_overrideFormat = format;
        descriptor.m_mipSliceMin = mipSliceMin;
        descriptor.m_mipSliceMax = mipSliceMax;
        return descriptor;
    }

    ImageViewDescriptor ImageViewDescriptor::Create(
        Format format,
        uint16_t mipSliceMin,
        uint16_t mipSliceMax,
        uint16_t arraySliceMin,
        uint16_t arraySliceMax)
    {
        ImageViewDescriptor descriptor;
        descriptor.m_overrideFormat = format;
        descriptor.m_mipSliceMin = mipSliceMin;
        descriptor.m_mipSliceMax = mipSliceMax;
        descriptor.m_arraySliceMin = arraySliceMin;
        descriptor.m_arraySliceMax = arraySliceMax;
        return descriptor;
    }

    ImageViewDescriptor ImageViewDescriptor::CreateCubemap()
    {
        ImageViewDescriptor descriptor;
        descriptor.m_isCubemap = true;
        return descriptor;
    }

    ImageViewDescriptor ImageViewDescriptor::CreateCubemap(
        Format format,
        uint16_t mipSliceMin,
        uint16_t mipSliceMax)
    {
        ImageViewDescriptor descriptor = Create(format, mipSliceMin, mipSliceMax);
        descriptor.m_isCubemap = true;
        return descriptor;
    }

    ImageViewDescriptor ImageViewDescriptor::CreateCubemap(
        Format format,
        uint16_t mipSliceMin,
        uint16_t mipSliceMax,
        uint16_t cubeSliceMin,
        uint16_t cubeSliceMax)
    {
        const uint16_t CubeFaceCount = 6;
        const uint16_t arraySliceMin = cubeSliceMin * CubeFaceCount;
        const uint16_t arraySliceMax = cubeSliceMax * CubeFaceCount;

        ImageViewDescriptor descriptor;
        descriptor.m_overrideFormat = format;
        descriptor.m_mipSliceMin = mipSliceMin;
        descriptor.m_mipSliceMax = mipSliceMax;
        descriptor.m_arraySliceMin = arraySliceMin;
        descriptor.m_arraySliceMax = arraySliceMax;
        descriptor.m_isCubemap = true;
        return descriptor;
    }

    ImageViewDescriptor ImageViewDescriptor::CreateCubemapFace(
        Format format,
        uint16_t mipSliceMin,
        uint16_t mipSliceMax,
        uint16_t faceSlice)
    {
        ImageViewDescriptor descriptor;
        descriptor.m_overrideFormat = format;
        descriptor.m_mipSliceMin = mipSliceMin;
        descriptor.m_mipSliceMax = mipSliceMax;
        descriptor.m_arraySliceMin = faceSlice;
        descriptor.m_arraySliceMax = faceSlice;
        descriptor.m_isCubemap = true;
        return descriptor;
    }

    ImageViewDescriptor ImageViewDescriptor::Create3D(
        Format overrideFormat,
        uint16_t mipSliceMin,
        uint16_t mipSliceMax,
        uint16_t depthSliceMin,
        uint16_t depthSliceMax)
    {
        ImageViewDescriptor descriptor;
        descriptor.m_overrideFormat = overrideFormat;
        descriptor.m_mipSliceMin = mipSliceMin;
        descriptor.m_mipSliceMax = mipSliceMax;
        descriptor.m_depthSliceMin = depthSliceMin;
        descriptor.m_depthSliceMax = depthSliceMax;
        return descriptor;
    }

    HashValue64 ImageViewDescriptor::GetHash(HashValue64 seed) const
    {
        return TypeHash64(*this, seed);
    }
        
    bool ImageViewDescriptor::IsSameSubResource(const ImageViewDescriptor& other) const
    {
        return
            m_mipSliceMin == other.m_mipSliceMin &&
            m_mipSliceMax == other.m_mipSliceMax &&
            m_arraySliceMin == other.m_arraySliceMin &&
            m_arraySliceMax == other.m_arraySliceMax &&
            m_depthSliceMin == other.m_depthSliceMin &&
            m_depthSliceMax == other.m_depthSliceMax &&
            m_aspectFlags == other.m_aspectFlags;
    }

    bool ImageViewDescriptor::OverlapsSubResource(const ImageViewDescriptor& other) const
    {
        return CheckBitsAny(m_aspectFlags, other.m_aspectFlags) &&
            Interval(m_arraySliceMin, m_arraySliceMax).Overlaps(Interval(other.m_arraySliceMin, other.m_arraySliceMax)) &&
            Interval(m_mipSliceMin, m_mipSliceMax).Overlaps(Interval(other.m_mipSliceMin, other.m_mipSliceMax));
        ;
    }
    
    bool ImageViewDescriptor::operator==(const ImageViewDescriptor& other) const
    {
        return
            IsSameSubResource(other) &&
            m_overrideFormat == other.m_overrideFormat &&
            m_overrideBindFlags == other.m_overrideBindFlags &&
            m_isCubemap == other.m_isCubemap &&
            m_isArray == other.m_isArray;
            
    }

    bool ImageViewDescriptor::operator!=(const ImageViewDescriptor& other) const
    {
        return !operator==(other);
    }
}
