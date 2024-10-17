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
    #include "TextureFormats.inl"

    wgpu::TextureFormat ConvertImageFormat(RHI::Format format, [[maybe_unused]] bool raiseAsserts)
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
            AZ_Assert(!raiseAsserts, "unhandled conversion in ConvertImageFormat");
            return wgpu::TextureFormat::Undefined;
        }

#undef RHIWGPU_RHI_TO_WGPU
    }

    RHI::Format ConvertImageFormat(wgpu::TextureFormat format)
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
            AZ_Assert(false, "unhandled conversion in ConvertImageFormat");
            return RHI::Format::Unknown;
        }

#undef RHIWGPU_WGPU_TO_RHI
    }
#undef RHIWGPU_EXPAND_FOR_FORMATS

    // definition of the macro "RHIWGPU_EXPAND_FOR_FORMATS" containing formats' names.
    #include "VertexFormats.inl"

    wgpu::VertexFormat ConvertVertexFormat(RHI::Format format)
    {
#define RHIWGPU_RHI_TO_WGPU(_FormatID, _WGPUFormat)                                                              \
    case RHI::Format::_FormatID:                                                                                 \
        return wgpu::VertexFormat::_WGPUFormat;

        switch (format)
        {
        case RHI::Format::Unknown:
            return wgpu::VertexFormat::Uint8x2;

            RHIWGPU_EXPAND_FOR_FORMATS(RHIWGPU_RHI_TO_WGPU)

        default:
            AZ_Assert(false, "unhandled conversion in ConvertVertexFormat");
            return wgpu::VertexFormat::Uint8x2;
        }
#undef RHIWGPU_RHI_TO_WGPU
    }

    RHI::Format ConvertVertexFormat(wgpu::VertexFormat format)
    {
#define RHIWGPU_WGPU_TO_RHI(_FormatID, _WGPUFormat)                                                              \
    case wgpu::VertexFormat::_WGPUFormat:                                                                        \
        return RHI::Format::_FormatID;

        switch (format)
        {
            RHIWGPU_EXPAND_FOR_FORMATS(RHIWGPU_WGPU_TO_RHI)

        default:
            AZ_Assert(false, "unhandled conversion in ConvertVertexFormat");
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

    wgpu::PrimitiveTopology ConvertPrimitiveTopology(RHI::PrimitiveTopology topology)
    {
        switch (topology)
        {
        case RHI::PrimitiveTopology::Undefined:
            return wgpu ::PrimitiveTopology::Undefined;
        case RHI::PrimitiveTopology::PointList:
            return wgpu ::PrimitiveTopology::PointList;
        case RHI::PrimitiveTopology::LineList:
            return wgpu ::PrimitiveTopology::LineList;
        case RHI::PrimitiveTopology::LineStrip:
            return wgpu ::PrimitiveTopology::LineStrip;
        case RHI::PrimitiveTopology::TriangleList:
            return wgpu ::PrimitiveTopology::TriangleList;
        case RHI::PrimitiveTopology::TriangleStrip:
            return wgpu ::PrimitiveTopology::TriangleStrip;
        default:
            AZ_Assert(false, "Unsuported primitive topology %d", topology);
            return wgpu ::PrimitiveTopology::Undefined;
        }
    }

    wgpu::CullMode ConvertCullMode(RHI::CullMode cullmode)
    {
        switch (cullmode)
        {
        case RHI::CullMode::None: return wgpu::CullMode::None;
        case RHI::CullMode::Front: return wgpu::CullMode::Front;
        case RHI::CullMode::Back: return wgpu::CullMode::Back;
        case RHI::CullMode::Invalid: return wgpu::CullMode::Undefined;
        default:
            AZ_Assert(false, "Unsuported cull mode %d", cullmode);
            return wgpu ::CullMode::Undefined;
        }
    }

    wgpu::CompareFunction ConvertCompareFunction(RHI::ComparisonFunc func)
    {
        switch (func)
        {
        case RHI::ComparisonFunc::Never: return wgpu::CompareFunction::Never;
        case RHI::ComparisonFunc::Less: return wgpu::CompareFunction::Less;
        case RHI::ComparisonFunc::Equal: return wgpu::CompareFunction::Equal;
        case RHI::ComparisonFunc::LessEqual: return wgpu::CompareFunction::LessEqual;
        case RHI::ComparisonFunc::Greater: return wgpu::CompareFunction::Greater;
        case RHI::ComparisonFunc::NotEqual: return wgpu::CompareFunction::NotEqual;
        case RHI::ComparisonFunc::GreaterEqual: return wgpu::CompareFunction::GreaterEqual;
        case RHI::ComparisonFunc::Always: return wgpu::CompareFunction::Always;
        case RHI::ComparisonFunc::Invalid: return wgpu::CompareFunction::Undefined;
        default:
            AZ_Assert(false, "Unsuported ComparisonFunc %d", func);
            return wgpu::CompareFunction::Undefined;
        }
    }

    wgpu::StencilOperation ConvertStencilOp(RHI::StencilOp op)
    {
        switch (op)
        {
        case RHI::StencilOp::Keep: return wgpu::StencilOperation::Keep;
        case RHI::StencilOp::Zero: return wgpu::StencilOperation::Zero;
        case RHI::StencilOp::Replace: return wgpu::StencilOperation::Replace;
        case RHI::StencilOp::IncrementSaturate: return wgpu::StencilOperation::IncrementClamp;
        case RHI::StencilOp::DecrementSaturate: return wgpu::StencilOperation::DecrementClamp;
        case RHI::StencilOp::Invert: return wgpu::StencilOperation::Invert;
        case RHI::StencilOp::Increment: return wgpu::StencilOperation::IncrementWrap;
        case RHI::StencilOp::Decrement: return wgpu::StencilOperation::DecrementWrap;
        case RHI::StencilOp::Invalid: return wgpu::StencilOperation::Undefined;
        default:
            AZ_Assert(false, "Unsuported StencilOp %d", op);
            return wgpu::StencilOperation::Undefined;
        }
    }

    wgpu::VertexStepMode ConvertVertexStep(RHI::StreamStepFunction step)
    {
        switch (step)
        {
        case RHI::StreamStepFunction::PerVertex: return wgpu::VertexStepMode::Vertex;
        case RHI::StreamStepFunction::PerInstance: return wgpu::VertexStepMode::Instance;
        default:
            AZ_Assert(false, "Unsuported StreamStepFunction %d", step);
            return wgpu::VertexStepMode::Undefined;
        }
    }

    wgpu::ColorWriteMask ConvertWriteMask(uint8_t writeMask)
    {
        wgpu::ColorWriteMask colorMask = wgpu::ColorWriteMask::None;
        if (RHI::CheckBitsAll(writeMask, static_cast<uint8_t>(RHI::WriteChannelMask::ColorWriteMaskAll)))
        {
            return wgpu::ColorWriteMask::All;
        }

        if (RHI::CheckBitsAny(writeMask, static_cast<uint8_t>(RHI::WriteChannelMask::ColorWriteMaskRed)))
        {
            colorMask |= wgpu::ColorWriteMask::Red;
        }
        if (RHI::CheckBitsAny(writeMask, static_cast<uint8_t>(RHI::WriteChannelMask::ColorWriteMaskGreen)))
        {
            colorMask |= wgpu::ColorWriteMask::Green;
        }
        if (RHI::CheckBitsAny(writeMask, static_cast<uint8_t>(RHI::WriteChannelMask::ColorWriteMaskBlue)))
        {
            colorMask |= wgpu::ColorWriteMask::Blue;
        }
        if (RHI::CheckBitsAny(writeMask, static_cast<uint8_t>(RHI::WriteChannelMask::ColorWriteMaskAlpha)))
        {
            colorMask |= wgpu::ColorWriteMask::Alpha;
        }
        return colorMask;
    }

    wgpu::BlendOperation ConvertBlendOp(RHI::BlendOp op)
    {
        switch (op)
        {
        case RHI::BlendOp::Add: return wgpu::BlendOperation::Add;
        case RHI::BlendOp::Subtract: return wgpu::BlendOperation::Subtract;
        case RHI::BlendOp::SubtractReverse: return wgpu::BlendOperation::ReverseSubtract;
        case RHI::BlendOp::Minimum: return wgpu::BlendOperation::Min;
        case RHI::BlendOp::Maximum: return wgpu::BlendOperation::Max;
        case RHI::BlendOp::Invalid: return wgpu::BlendOperation::Undefined;
        default:
            AZ_Assert(false, "Unsuported BlendOp %d", op);
            return wgpu::BlendOperation::Undefined;
        }
    }

    wgpu::BlendFactor ConvertBlendFactor(RHI::BlendFactor factor)
    {
        switch (factor)
        {
        case RHI::BlendFactor::Zero: return wgpu::BlendFactor::Zero;
        case RHI::BlendFactor::One: return wgpu::BlendFactor::One;
        case RHI::BlendFactor::ColorSource: return wgpu::BlendFactor::Src;
        case RHI::BlendFactor::ColorSourceInverse: return wgpu::BlendFactor::OneMinusSrc;
        case RHI::BlendFactor::AlphaSource: return wgpu::BlendFactor::SrcAlpha;
        case RHI::BlendFactor::AlphaSourceInverse: return wgpu::BlendFactor::OneMinusSrcAlpha;
        case RHI::BlendFactor::AlphaDest: return wgpu::BlendFactor::DstAlpha;
        case RHI::BlendFactor::AlphaDestInverse: return wgpu::BlendFactor::OneMinusDstAlpha;
        case RHI::BlendFactor::ColorDest: return wgpu::BlendFactor::Dst;
        case RHI::BlendFactor::ColorDestInverse: return wgpu::BlendFactor::OneMinusDst;
        case RHI::BlendFactor::AlphaSourceSaturate: return wgpu::BlendFactor::SrcAlphaSaturated;
        case RHI::BlendFactor::Factor: return wgpu::BlendFactor::Constant;
        case RHI::BlendFactor::FactorInverse: return wgpu::BlendFactor::OneMinusConstant;
        case RHI::BlendFactor::ColorSource1: return wgpu::BlendFactor::Src1;
        case RHI::BlendFactor::ColorSource1Inverse: return wgpu::BlendFactor::OneMinusSrc1;
        case RHI::BlendFactor::AlphaSource1: return wgpu::BlendFactor::Src1Alpha;
        case RHI::BlendFactor::AlphaSource1Inverse: return wgpu::BlendFactor::OneMinusSrc1Alpha;
        case RHI::BlendFactor::Invalid: return wgpu::BlendFactor::Undefined;
        default:
            AZ_Assert(false, "Unsuported BlendFactor %d", factor);
            return wgpu::BlendFactor::Undefined;
        }
    }

    wgpu::LoadOp ConvertLoadOp(RHI::AttachmentLoadAction action)
    {
        switch (action)
        {
        case RHI::AttachmentLoadAction::Load: return wgpu::LoadOp::Load;
        case RHI::AttachmentLoadAction::Clear: return wgpu::LoadOp::Clear;
        case RHI::AttachmentLoadAction::DontCare: return wgpu::LoadOp::Clear; // WebGPU doesn't have a DontCare load op
        case RHI::AttachmentLoadAction::None: return wgpu::LoadOp::Load;
        default:
            AZ_Assert(false, "Unsuported AttachmentLoadAction %d", action);
            return wgpu::LoadOp::Undefined;
        }       
    }

    wgpu::StoreOp ConvertStoreOp(RHI::AttachmentStoreAction action)
    {
        switch (action)
        {
        case RHI::AttachmentStoreAction::Store: return wgpu::StoreOp::Store;
        case RHI::AttachmentStoreAction::DontCare: return wgpu::StoreOp::Discard;
        case RHI::AttachmentStoreAction::None: return wgpu::StoreOp::Store;
        default:
            AZ_Assert(false, "Unsuported AttachmentStoreAction %d", action);
            return wgpu::StoreOp::Undefined;
        }
    }

    wgpu::Color ConvertClearValue(RHI::ClearValue clearValue)
    {
        AZ_Assert(clearValue.m_type != RHI::ClearValueType::DepthStencil, "Invalid clear value type");
        wgpu::Color color;
        if (clearValue.m_type == RHI::ClearValueType::Vector4Float)
        {
            color.r = clearValue.m_vector4Float[0];
            color.g = clearValue.m_vector4Float[1];
            color.b = clearValue.m_vector4Float[2];
            color.a = clearValue.m_vector4Float[3];
        }
        else
        {
            color.r = aznumeric_caster(clearValue.m_vector4Uint[0]);
            color.g = aznumeric_caster(clearValue.m_vector4Uint[1]);
            color.b = aznumeric_caster(clearValue.m_vector4Uint[2]);
            color.a = aznumeric_caster(clearValue.m_vector4Uint[3]);
        }

        return color;
    }

    wgpu::AddressMode ConvertAddressMode(RHI::AddressMode mode)
    {
        switch (mode)
        {
        case RHI::AddressMode::Wrap: return wgpu::AddressMode::Repeat;
        case RHI::AddressMode::Mirror: return wgpu::AddressMode::MirrorRepeat;
        case RHI::AddressMode::Clamp: return wgpu::AddressMode::ClampToEdge;
        default:
            AZ_Assert(false, "Invalid SamplerAddressMode %d", mode);
            return wgpu::AddressMode::Undefined;
        }
    }

    wgpu::FilterMode ConvertFilterMode(RHI::FilterMode mode)
    {
        switch (mode)
        {
        case RHI::FilterMode::Point: return wgpu::FilterMode::Nearest;
        case RHI::FilterMode::Linear: return wgpu::FilterMode::Linear;
        default:
            AZ_Assert(false, "Invalid SamplerFilterMode %d", mode);
            return wgpu::FilterMode::Undefined;
        }
    }

    wgpu::MipmapFilterMode ConvertMipMapFilterMode(RHI::FilterMode mode)
    {
        switch (mode)
        {
        case RHI::FilterMode::Point: return wgpu::MipmapFilterMode::Nearest;
        case RHI::FilterMode::Linear: return wgpu::MipmapFilterMode::Linear;
        default:
            AZ_Assert(false, "Invalid SamplerFilterMode %d", mode);
            return wgpu::MipmapFilterMode::Undefined;
        }
    }

    wgpu::BufferUsage ConvertBufferBindFlags(RHI::BufferBindFlags bindFlags)
    {
        wgpu::BufferUsage usages = wgpu::BufferUsage::None;
        if (RHI::CheckBitsAny(bindFlags, RHI::BufferBindFlags::InputAssembly | RHI::BufferBindFlags::DynamicInputAssembly))
        {
            usages |= wgpu::BufferUsage::Index | wgpu::BufferUsage::Vertex;
        }

        if (RHI::CheckBitsAny(bindFlags, RHI::BufferBindFlags::Constant))
        {
            usages |= wgpu::BufferUsage::Uniform;
        }

        if (RHI::CheckBitsAny(bindFlags, RHI::BufferBindFlags::ShaderReadWrite))
        {
            usages |= wgpu::BufferUsage::Storage;
        }

        if (RHI::CheckBitsAny(bindFlags, RHI::BufferBindFlags::CopyRead))
        {
            usages |= wgpu::BufferUsage::CopySrc;
        }

        if (RHI::CheckBitsAny(bindFlags, RHI::BufferBindFlags::CopyWrite))
        {
            usages |= wgpu::BufferUsage::CopyDst;
        }

        if (RHI::CheckBitsAny(bindFlags, RHI::BufferBindFlags::Indirect))
        {
            usages |= wgpu::BufferUsage::Indirect;
        }
        return usages;
    }

    wgpu::MapMode ConvertMapMode(RHI::HostMemoryAccess access)
    {
        switch (access)
        {
        case RHI::HostMemoryAccess::Read: return wgpu::MapMode::Read;
        case RHI::HostMemoryAccess::Write: return wgpu::MapMode::Write;
        default:
            AZ_Assert(false, "Invalid HostMemoryAccess %d", access);
            return wgpu::MapMode::None;
        }
    }

    wgpu::IndexFormat ConvertIndexFormat(RHI::IndexFormat format)
    {
        switch (format)
        {
        case RHI::IndexFormat::Uint16: return wgpu::IndexFormat::Uint16;
        case RHI::IndexFormat::Uint32: return wgpu::IndexFormat::Uint32;
        default:
            AZ_Assert(false, "Invalid IndexFormat %d", format);
            return wgpu::IndexFormat::Undefined;
        }
    }

    wgpu::TextureViewDimension ConvertImageType(RHI::ShaderInputImageType type)
    {
        switch (type)
        {
        case RHI::ShaderInputImageType::Unknown: return wgpu::TextureViewDimension::Undefined;
        case RHI::ShaderInputImageType::Image1D: return wgpu::TextureViewDimension::e1D;
        case RHI::ShaderInputImageType::Image2D: return wgpu::TextureViewDimension::e2D;
        case RHI::ShaderInputImageType::Image2DMultisample: return wgpu::TextureViewDimension::e2D;
        case RHI::ShaderInputImageType::Image2DArray: return wgpu::TextureViewDimension::e2DArray;
        case RHI::ShaderInputImageType::Image2DMultisampleArray: return wgpu::TextureViewDimension::e2DArray;
        case RHI::ShaderInputImageType::ImageCube: return wgpu::TextureViewDimension::Cube;
        case RHI::ShaderInputImageType::ImageCubeArray:
            return wgpu::TextureViewDimension::CubeArray;
        case RHI::ShaderInputImageType::Image3D: return wgpu::TextureViewDimension::e3D;
        default:
            AZ_Assert(false, "Unsupported image type %d", type);
            return wgpu::TextureViewDimension::Undefined;
        }
    }

    wgpu::SamplerBindingType ConvertReductionType(RHI::ReductionType type)
    {
        switch (type)
        {
        case RHI::ReductionType::Filter: return wgpu::SamplerBindingType::Filtering;
        case RHI::ReductionType::Comparison: return wgpu::SamplerBindingType::Comparison;
        case RHI::ReductionType::Minimum: return wgpu::SamplerBindingType::NonFiltering;
        case RHI::ReductionType::Maximum: return wgpu::SamplerBindingType::NonFiltering;
        default:
            AZ_Assert(false, "Unsupported ReductionType %d", type);
            return wgpu::SamplerBindingType::Undefined;
        }
    }

    wgpu::TextureSampleType ConvertSampleType(RHI::ShaderInputImageSampleType type)
    {
        switch (type)
        {
        case RHI::ShaderInputImageSampleType::Float: return wgpu::TextureSampleType::Float;
        case RHI::ShaderInputImageSampleType::UnfilterableFloat: return wgpu::TextureSampleType::UnfilterableFloat;
        case RHI::ShaderInputImageSampleType::Depth: return wgpu::TextureSampleType::Depth;
        case RHI::ShaderInputImageSampleType::Sint: return wgpu::TextureSampleType::Sint;
        case RHI::ShaderInputImageSampleType::Uint: return wgpu::TextureSampleType::Uint;
        case RHI::ShaderInputImageSampleType::Unknown: return wgpu::TextureSampleType::Undefined;
        default:
            AZ_Assert(false, "Unsupported ShaderInputImageSampleType %d", type);
            return wgpu::TextureSampleType::Undefined;
        }
    }

    wgpu::SamplerBindingType ConvertSamplerBindingType(RHI::ShaderInputSamplerType type)
    {
        switch (type)
        {
        case RHI::ShaderInputSamplerType::Filtering: return wgpu::SamplerBindingType::Filtering;
        case RHI::ShaderInputSamplerType::NonFiltering: return wgpu::SamplerBindingType::NonFiltering;
        case RHI::ShaderInputSamplerType::Comparison: return wgpu::SamplerBindingType::Comparison;
        case RHI::ShaderInputSamplerType::Unknown: return wgpu::SamplerBindingType::Undefined;
        default:
            AZ_Assert(false, "Unsupported ShaderInputSamplerType %d", type);
            return wgpu::SamplerBindingType::Undefined;
        }
    }
}
