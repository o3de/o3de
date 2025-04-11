/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/BufferDescriptor.h>
#include <Atom/RHI.Reflect/Format.h>

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::RHI
{
    //! Buffer views describe how to interpret a region of memory in a buffer.
    struct BufferViewDescriptor
    {
        AZ_TYPE_INFO(BufferViewDescriptor, "{AC5C4601-1824-434F-B070-B4A48DBDB437}");
        AZ_CLASS_ALLOCATOR(BufferViewDescriptor, SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);

        BufferViewDescriptor() = default;
        bool operator==(const BufferViewDescriptor& other) const;
            
        //! Creates a structured buffer view. Structured buffers are defined by an array of non-fundamental
        //! types, or custom data structures. The exact format of the data structure is defined elsewhere
        //! (e.g. in the shader).
        static BufferViewDescriptor CreateStructured(
            uint32_t elementOffset,
            uint32_t elementCount,
            uint32_t elementSize);

        //! Creates a raw (unsigned 32bit integral) buffer view. This can be used to describe
        //! constant buffers or simple append / consume buffers.
        static BufferViewDescriptor CreateRaw(
            uint32_t byteOffset,
            uint32_t byteCount);

        //! Creates a buffer with a fundamental type. This is similar to a structured buffer except that
        //! the type is fundamental, and thus can be described by a format.
        static BufferViewDescriptor CreateTyped(
            uint32_t elementOffset,
            uint32_t elementCount,
            Format elementFormat);

        /**
            * Creates a ray tracing TLAS buffer view. This is a specialized ray tracing buffer with a fixed
            * element size and format.
            */
        static BufferViewDescriptor CreateRayTracingTLAS(
            uint32_t totalByteCount);

        HashValue64 GetHash(HashValue64 seed = HashValue64{ 0 }) const;

        //! Check if it overlaps with a subresource described by another BufferViewDescriptor
        bool OverlapsSubResource(const BufferViewDescriptor& other) const;

        //! Number of elements from the start of the buffer to offset.
        uint32_t m_elementOffset = 0;

        //! The number of elements in the view.
        uint32_t m_elementCount = 0;

        //! The size in bytes of each element.
        uint32_t m_elementSize = 0;

        //! The format of each element. Should be Unknown for structured buffers, or R32 for raw buffers.
        Format m_elementFormat = Format::Unknown;

        //! The bind flags used by this view. Should be compatible with the bind flags of the underlying buffer.
        BufferBindFlags m_overrideBindFlags = BufferBindFlags::None;

        //BIG HACK - To be removed later.
        //[GFX_TODO][ATOM-5668] - Skinning: fix output buffer dependencies
        //Currently there is no way track multiple skinning buffer resources tied to the same id and hence
        //this is a temp solution to disable buffer validation for the Skinning pass. Please remove this Hack afer a proper solution is in place.
        bool m_ignoreFrameAttachmentValidation = false;

        // manual alignment padding
        char m_pad0 = 0, m_pad1 = 0, m_pad2 = 0;
    };
}
