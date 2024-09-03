/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/Conversions.h>

namespace AZ::WebGPU
{
    const char* ToString(wgpu::BackendType backend)
    {
        switch (backend)
        {
        case wgpu::BackendType::D3D12: return "D3D12";
        case wgpu::BackendType::Metal: return "Metal";
        case wgpu::BackendType::Vulkan: return "Vulkan";
        case wgpu::BackendType::WebGPU: return "WebGPU";
        case wgpu::BackendType::Null: return "Null";
        case wgpu::BackendType::D3D11: return "D3D11";
        case wgpu::BackendType::OpenGL: return "OpenGL";
        case wgpu::BackendType::OpenGLES: return "OpenGLES";
        case wgpu::BackendType::Undefined: return "Undefined";
        default:
            AZ_Assert(false, "Invalid backend type %d", backend);
            return "";
        }
    }

    const char* ToString(wgpu::DeviceLostReason reason)
    {
        switch (reason)
        {
        case wgpu::DeviceLostReason::Unknown:return "Unknown";
        case wgpu::DeviceLostReason::Destroyed: return "Destroyed";
        case wgpu::DeviceLostReason::InstanceDropped: return "InstanceDropped";
        case wgpu::DeviceLostReason::FailedCreation: return "FailedCreation";
        default:
            AZ_Assert(false, "Invalid device lost reason %d", reason);
            return "";
        }
    }

    const char* ToString(wgpu::ErrorType type)
    {
        switch (type)
        {
        case wgpu::ErrorType::Validation: return "Validation";
        case wgpu::ErrorType::OutOfMemory: return "Out of memory";
        case wgpu::ErrorType::Unknown: return "Unknown";
        case wgpu::ErrorType::DeviceLost: return "Device lost";
        default:
            AZ_Assert(false, "Invalid error type %d", type);
            return "";
        }
    }

    const char* ToString(wgpu::Status status)
    {
        switch (status)
        {
        case wgpu::Status::Error: return "Error";
        case wgpu::Status::Success: return "Success";
        default:
            AZ_Assert(false, "Invalid status type %d", status);
            return "";
        }
    }

    // definition of the macro "RHIWGPU_EXPAND_FOR_FORMATS" containing formats' names and aspect flags.
    #include "Formats.inl"

    wgpu::TextureFormat ConvertFormat(RHI::Format format, [[maybe_unused]] bool raiseAsserts)
    {
#define RHIWGPU_RHI_TO_WGPU(_FormatID, _WGPUFormat, _Color, _Depth, _Stencil)                                                              \
    case RHI::Format::_FormatID:                                                                                                           \
        return wgpu::TextureFormat::_WGPUFormat;

        switch (format)
        {
        case RHI::Format::Unknown:
            return wgpu::TextureFormat::Undefined;

            RHIWGPU_EXPAND_FOR_FORMATS(RHIWGPU_RHI_TO_WGPU)

        default:
            AZ_Assert(!raiseAsserts, "unhandled conversion in ConvertFormat");
            return wgpu::TextureFormat::Undefined;
        }

#undef RHIWGPU_RHI_TO_WGPU
    }

    RHI::Format ConvertFormat(wgpu::TextureFormat format)
    {
#define RHIWGPU_WGPU_TO_RHI(_FormatID, _WGPUFormat, _Color, _Depth, _Stencil)                                                                \
    case wgpu::TextureFormat::_WGPUFormat:                                                                                                   \
        return RHI::Format::_FormatID;

        switch (format)
        {
        case wgpu::TextureFormat::Undefined:
            return RHI::Format::Unknown;

            RHIWGPU_EXPAND_FOR_FORMATS(RHIWGPU_WGPU_TO_RHI)

        default:
            AZ_Assert(false, "unhandled conversion in ConvertFormat");
            return RHI::Format::Unknown;
        }

#undef RHIWGPU_WGPU_TO_RHI
    }
#undef RHIWGPU_EXPAND_FOR_FORMATS

    wgpu::TextureDimension ConvertImageDimension(RHI::ImageDimension dimension)
    {
        switch (dimension)
        {
        case RHI::ImageDimension::Image1D: return wgpu::TextureDimension::e1D;
        case RHI::ImageDimension::Image2D: return wgpu::TextureDimension::e2D;
        case RHI::ImageDimension::Image3D: return wgpu::TextureDimension::e3D;
        default:
            AZ_Assert(false, "Invalid RHI::ImageDimension %d", dimension);
            return wgpu::TextureDimension::Undefined;
        }
    }

    wgpu::Extent3D ConvertImageSize(const RHI::Size& size)
    {
        return wgpu::Extent3D{ size.m_width, size.m_height, size.m_depth};
    }

    wgpu::TextureUsage ConvertImageBinding(RHI::ImageBindFlags flags)
    {
        wgpu::TextureUsage usage = wgpu::TextureUsage::None;
        if (RHI::CheckBitsAll(flags, RHI::ImageBindFlags::ShaderRead))
        {
            usage |= wgpu::TextureUsage::TextureBinding;
        }
        if (RHI::CheckBitsAll(flags, RHI::ImageBindFlags::ShaderWrite))
        {
            usage |= wgpu::TextureUsage::StorageBinding;
        }
        if (RHI::CheckBitsAny(flags, RHI::ImageBindFlags::Color | RHI::ImageBindFlags::DepthStencil))
        {
            usage |= wgpu::TextureUsage::RenderAttachment;
        }
        if (RHI::CheckBitsAll(flags, RHI::ImageBindFlags::Depth))
        {
            usage |= wgpu::TextureUsage::RenderAttachment;
        }
        if (RHI::CheckBitsAll(flags, RHI::ImageBindFlags::CopyRead))
        {
            usage |= wgpu::TextureUsage::CopySrc;
        }
        if (RHI::CheckBitsAll(flags, RHI::ImageBindFlags::CopyWrite))
        {
            usage |= wgpu::TextureUsage::CopyDst;
        }
        return usage;
    }

     wgpu::TextureAspect ConvertImageAspect(RHI::ImageAspect imageAspect)
     {
        switch (imageAspect)
        {
        case RHI::ImageAspect::Color:
            return wgpu::TextureAspect::All;
        case RHI::ImageAspect::Depth:
            return wgpu::TextureAspect::DepthOnly;
        case RHI::ImageAspect::Stencil:
            return wgpu::TextureAspect::StencilOnly;
        default:
            AZ_Assert(false, "Invalid image aspect %d", imageAspect);
            return wgpu::TextureAspect::Undefined;
        }
    }

    wgpu::TextureAspect ConvertImageAspectFlags(RHI::ImageAspectFlags flags)
    {
        wgpu::TextureAspect wgpuFlags = wgpu::TextureAspect::Undefined;
        for (uint32_t i = 0; i < RHI::ImageAspectCount; ++i)
        {
            if (!RHI::CheckBitsAll(flags, static_cast<RHI::ImageAspectFlags>(AZ_BIT(i))))
            {
                continue;
            }

            if (wgpuFlags != wgpu::TextureAspect::Undefined)
            {
                return wgpu::TextureAspect::All;
            }

            wgpuFlags = ConvertImageAspect(static_cast<RHI::ImageAspect>(i));
        }

        return wgpuFlags;
    }
}
