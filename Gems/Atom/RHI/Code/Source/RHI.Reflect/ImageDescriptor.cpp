/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/ImageDescriptor.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ::RHI
{
    ImageBindFlags GetImageBindFlags(ScopeAttachmentUsage usage, ScopeAttachmentAccess access)
    {
        switch (usage)
        {
        case ScopeAttachmentUsage::RenderTarget:
            return ImageBindFlags::Color;
        case ScopeAttachmentUsage::DepthStencil:
            return ImageBindFlags::DepthStencil;
        case ScopeAttachmentUsage::Shader:
            switch (access)
            {
            case ScopeAttachmentAccess::ReadWrite:
                return ImageBindFlags::ShaderReadWrite;
            case ScopeAttachmentAccess::Read:
                return ImageBindFlags::ShaderRead;
            case ScopeAttachmentAccess::Write:
                return ImageBindFlags::ShaderWrite;
            }
            break;
        case ScopeAttachmentUsage::Copy:
            switch (access)
            {
            case ScopeAttachmentAccess::Read:
                return ImageBindFlags::CopyRead;
            case ScopeAttachmentAccess::Write:
                return ImageBindFlags::CopyWrite;
            }
            break;
        case ScopeAttachmentUsage::Resolve:
            return ImageBindFlags::CopyWrite;
        case ScopeAttachmentUsage::ShadingRate:
            return ImageBindFlags::ShadingRate;
        case ScopeAttachmentUsage::Predication:
        case ScopeAttachmentUsage::Indirect:
        case ScopeAttachmentUsage::SubpassInput:
        case ScopeAttachmentUsage::InputAssembly:
        case ScopeAttachmentUsage::Uninitialized:
            break;
        default:
            break;
        }
        return ImageBindFlags::None;
    }

    void ImageDescriptor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<ImageDescriptor>()
                ->Version(2)
                ->Field("BindFlags", &ImageDescriptor::m_bindFlags)
                ->Field("Dimension", &ImageDescriptor::m_dimension)
                ->Field("Size", &ImageDescriptor::m_size)
                ->Field("ArraySize", &ImageDescriptor::m_arraySize)
                ->Field("MipLevels", &ImageDescriptor::m_mipLevels)
                ->Field("Format", &ImageDescriptor::m_format)
                ->Field("MultisampleState", &ImageDescriptor::m_multisampleState)
                ->Field("SharedQueueMask", &ImageDescriptor::m_sharedQueueMask)
                ->Field("IsCubemap", &ImageDescriptor::m_isCubemap)
                ;
        }
    }

    HashValue64 ImageDescriptor::GetHash(HashValue64 seed /* = 0 */) const
    {
        return TypeHash64(*this, seed);
    }

    ImageDescriptor ImageDescriptor::Create1D(
        ImageBindFlags bindFlags,
        uint32_t width,
        Format format)
    {
        ImageDescriptor descriptor;
        descriptor.m_bindFlags = bindFlags;
        descriptor.m_dimension = ImageDimension::Image1D;
        descriptor.m_size.m_width = width;
        descriptor.m_format = format;
        return descriptor;
    }

    ImageDescriptor ImageDescriptor::Create1DArray(
        ImageBindFlags bindFlags,
        uint32_t width,
        uint16_t arraySize,
        Format format)
    {
        ImageDescriptor descriptor;
        descriptor.m_bindFlags = bindFlags;
        descriptor.m_dimension = ImageDimension::Image1D;
        descriptor.m_size.m_width = width;
        descriptor.m_arraySize = arraySize;
        descriptor.m_format = format;
        return descriptor;
    }

    ImageDescriptor ImageDescriptor::Create2D(
        ImageBindFlags bindFlags,
        uint32_t width,
        uint32_t height,
        Format format)
    {
        ImageDescriptor descriptor;
        descriptor.m_bindFlags = bindFlags;
        descriptor.m_size.m_width = width;
        descriptor.m_size.m_height = height;
        descriptor.m_format = format;
        return descriptor;
    }

    ImageDescriptor ImageDescriptor::Create2DArray(
        ImageBindFlags bindFlags,
        uint32_t width,
        uint32_t height,
        uint16_t arraySize,
        Format format)
    {
        ImageDescriptor descriptor;
        descriptor.m_bindFlags = bindFlags;
        descriptor.m_size.m_width = width;
        descriptor.m_size.m_height = height;
        descriptor.m_arraySize = arraySize;
        descriptor.m_format = format;
        return descriptor;
    }

    ImageDescriptor ImageDescriptor::CreateCubemap(
        ImageBindFlags bindFlags,
        uint32_t width,
        Format format)
    {
        ImageDescriptor descriptor;
        descriptor.m_bindFlags = bindFlags;
        descriptor.m_size.m_width = width;
        descriptor.m_size.m_height = width;
        descriptor.m_arraySize = NumCubeMapSlices;
        descriptor.m_format = format;
        descriptor.m_isCubemap = true;
        return descriptor;
    }

    ImageDescriptor ImageDescriptor::CreateCubemapArray(
        ImageBindFlags bindFlags,
        uint32_t width,
        uint16_t arraySize,
        Format format)
    {
        ImageDescriptor descriptor;
        descriptor.m_bindFlags = bindFlags;
        descriptor.m_size.m_width = width;
        descriptor.m_size.m_height = width;
        descriptor.m_arraySize = NumCubeMapSlices * arraySize;
        descriptor.m_format = format;
        descriptor.m_isCubemap = true;
        return descriptor;
    }

    ImageDescriptor ImageDescriptor::Create3D(
        ImageBindFlags bindFlags,
        uint32_t width,
        uint32_t height,
        uint32_t depth,
        Format format)
    {
        ImageDescriptor descriptor;
        descriptor.m_bindFlags = bindFlags;
        descriptor.m_dimension = ImageDimension::Image3D;
        descriptor.m_size.m_width = width;
        descriptor.m_size.m_height = height;
        descriptor.m_size.m_depth = depth;
        descriptor.m_format = format;
        return descriptor;
    }
}
