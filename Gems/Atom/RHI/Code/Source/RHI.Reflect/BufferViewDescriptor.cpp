/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/BufferViewDescriptor.h>
#include <Atom/RHI.Reflect/Interval.h>

#include <AzCore/Utils/TypeHash.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ::RHI
{
    void BufferViewDescriptor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<BufferViewDescriptor>()
                ->Version(1)
                ->Field("m_elementOffset", &BufferViewDescriptor::m_elementOffset)
                ->Field("m_elementCount", &BufferViewDescriptor::m_elementCount)
                ->Field("m_elementSize", &BufferViewDescriptor::m_elementSize)
                ->Field("m_elementFormat", &BufferViewDescriptor::m_elementFormat)
                ->Field("m_ignoreFrameAttachmentValidation", &BufferViewDescriptor::m_ignoreFrameAttachmentValidation)
                ;
        }
    }

    AZ::HashValue64 BufferViewDescriptor::GetHash(AZ::HashValue64 seed) const
    {
        return TypeHash64(*this, seed);
    }

    BufferViewDescriptor BufferViewDescriptor::CreateStructured(
        uint32_t elementOffset,
        uint32_t elementCount,
        uint32_t elementSize)
    {
        BufferViewDescriptor descriptor;
        descriptor.m_elementOffset = elementOffset;
        descriptor.m_elementCount = elementCount;
        descriptor.m_elementSize = elementSize;
        descriptor.m_elementFormat = Format::Unknown;
        return descriptor;
    }

    BufferViewDescriptor BufferViewDescriptor::CreateRaw(
        uint32_t byteOffset,
        uint32_t byteCount)
    {
        BufferViewDescriptor descriptor;
        descriptor.m_elementOffset = byteOffset >> 2;
        descriptor.m_elementCount = byteCount >> 2;
        descriptor.m_elementSize = 4;
        descriptor.m_elementFormat = Format::R32_UINT;
        return descriptor;
    }

    BufferViewDescriptor BufferViewDescriptor::CreateTyped(
        uint32_t elementOffset,
        uint32_t elementCount,
        Format elementFormat)
    {
        BufferViewDescriptor descriptor;
        descriptor.m_elementOffset = elementOffset;
        descriptor.m_elementCount = elementCount;
        descriptor.m_elementSize = GetFormatSize(elementFormat);
        descriptor.m_elementFormat = elementFormat;
        return descriptor;
    }

    BufferViewDescriptor BufferViewDescriptor::CreateRayTracingTLAS(
        uint32_t totalByteCount)
    {
        // TLAS format is a raw buffer with a float4 element size
        BufferViewDescriptor descriptor;
        descriptor.m_elementOffset = 0;
        descriptor.m_elementCount = totalByteCount / 16;
        descriptor.m_elementSize = 16;
        descriptor.m_elementFormat = Format::R32_UINT;
        return descriptor;
    }
    
    bool BufferViewDescriptor::operator==(const BufferViewDescriptor& other) const
    {
        return
            m_elementOffset == other.m_elementOffset &&
            m_elementCount == other.m_elementCount &&
            m_elementSize == other.m_elementSize &&
            m_elementFormat == other.m_elementFormat &&
            m_overrideBindFlags == other.m_overrideBindFlags &&
            m_ignoreFrameAttachmentValidation == other.m_ignoreFrameAttachmentValidation;
    }

    bool BufferViewDescriptor::OverlapsSubResource(const BufferViewDescriptor& other) const
    {
        uint32_t begin = m_elementOffset * m_elementSize;
        uint32_t end = begin + m_elementCount * m_elementSize - 1;
        uint32_t otherBegin = other.m_elementOffset * other.m_elementSize;
        uint32_t otherEnd = otherBegin + other.m_elementCount * other.m_elementSize - 1;
        return Interval(begin, end).Overlaps(Interval(otherBegin, otherEnd));
    }
}
