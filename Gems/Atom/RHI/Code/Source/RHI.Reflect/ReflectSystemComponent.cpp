/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/AttachmentLoadStoreAction.h>
#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI.Reflect/BufferDescriptor.h>
#include <Atom/RHI.Reflect/BufferPoolDescriptor.h>
#include <Atom/RHI.Reflect/BufferScopeAttachmentDescriptor.h>
#include <Atom/RHI.Reflect/ClearValue.h>
#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RHI.Reflect/Handle.h>
#include <Atom/RHI.Reflect/ImageDescriptor.h>
#include <Atom/RHI.Reflect/ImagePoolDescriptor.h>
#include <Atom/RHI.Reflect/ImageScopeAttachmentDescriptor.h>
#include <Atom/RHI.Reflect/ImageSubresource.h>
#include <Atom/RHI.Reflect/IndirectBufferLayout.h>
#include <Atom/RHI.Reflect/Origin.h>
#include <Atom/RHI.Reflect/RenderStates.h>
#include <Atom/RHI.Reflect/PipelineLayoutDescriptor.h>
#include <Atom/RHI.Reflect/PipelineLibraryData.h>
#include <Atom/RHI.Reflect/ReflectSystemComponent.h>
#include <Atom/RHI.Reflect/RenderAttachmentLayout.h>
#include <Atom/RHI.Reflect/ResolveScopeAttachmentDescriptor.h>
#include <Atom/RHI.Reflect/Scissor.h>
#include <Atom/RHI.Reflect/ScopeAttachmentDescriptor.h>
#include <Atom/RHI.Reflect/ShaderDataMappings.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayout.h>
#include <Atom/RHI.Reflect/ShaderStageFunction.h>
#include <Atom/RHI.Reflect/StreamingImagePoolDescriptor.h>
#include <Atom/RHI.Reflect/InputStreamLayout.h>
#include <Atom/RHI.Reflect/Viewport.h>
#include <Atom/RHI.Reflect/PlatformLimitsDescriptor.h>
#include <Atom/RHI.Reflect/PhysicalDeviceDescriptor.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace AZ::RHI
{
    void ReflectSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<ReflectSystemComponent, AZ::Component>()
                ->Version(3);
        }

        ReflectNamedEnums(context);

        //////////////////////////////////////////////////////////////////////////
        // Pipeline State
        RasterState::Reflect(context);
        StencilOpState::Reflect(context);
        StencilState::Reflect(context);
        DepthState::Reflect(context);
        DepthStencilState::Reflect(context);
        TargetBlendState::Reflect(context);
        BlendState::Reflect(context);
        MultisampleState::Reflect(context);
        RenderStates::Reflect(context);
        PipelineLibraryData::Reflect(context);
        ReflectRenderStateEnums(context);
        ReflectSamplerStateEnums(context);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // RenderAttachmentConfiguration
        RenderAttachmentConfiguration::Reflect(context);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // InputStreamLayout
        ShaderSemantic::Reflect(context);
        StreamChannelDescriptor::Reflect(context);
        StreamBufferDescriptor::Reflect(context);
        InputStreamLayout::Reflect(context);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // ShaderResourceGroup
        SamplerState::Reflect(context);
        ShaderInputBufferDescriptor::Reflect(context);
        ShaderInputImageDescriptor::Reflect(context);
        ShaderInputBufferUnboundedArrayDescriptor::Reflect(context);
        ShaderInputImageUnboundedArrayDescriptor::Reflect(context);
        ShaderInputSamplerDescriptor::Reflect(context);
        ShaderInputConstantDescriptor::Reflect(context);
        ShaderInputStaticSamplerDescriptor::Reflect(context);
        ShaderResourceGroupLayout::Reflect(context);
        ShaderDataMappings::Reflect(context);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Shader
        ShaderStageFunction::Reflect(context);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // PipelineLayout
        PipelineLayoutDescriptor::Reflect(context);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Resource pool Descriptors
        ResourcePoolDescriptor::Reflect(context);
        ImagePoolDescriptor::Reflect(context);
        BufferPoolDescriptor::Reflect(context);
        StreamingImagePoolDescriptor::Reflect(context);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Attachments
        AttachmentLoadStoreAction::Reflect(context);
        ScopeAttachmentDescriptor::Reflect(context);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Buffers
        BufferDescriptor::Reflect(context);
        BufferViewDescriptor::Reflect(context);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Images
        ImageDescriptor::Reflect(context);
        ImageScopeAttachmentDescriptor::Reflect(context);
        ImageViewDescriptor::Reflect(context);
        ImageSubresource::Reflect(context);
        ImageSubresourceRange::Reflect(context);
        DeviceImageSubresourceLayout::Reflect(context);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Resolve MSAA attachments
        ResolveScopeAttachmentDescriptor::Reflect(context);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Indirect buffer Layout
        IndirectBufferLayout::Reflect(context);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Misc 
        ClearValue::Reflect(context);
        Size::Reflect(context);
        Interval::Reflect(context);
        Viewport::Reflect(context);
        Scissor::Reflect(context);
        HeapPagingParameters::Reflect(context);
        HeapMemoryHintParameters::Reflect(context);
        TransientAttachmentPoolBudgets::Reflect(context);
        PlatformLimits::Reflect(context);
        PlatformLimitsDescriptor::Reflect(context);
        Origin::Reflect(context);
        ReflectVendorIdEnums(context);
        PhysicalDeviceDriverValidator::Reflect(context);

        Handle<uint64_t>::Reflect(context);
        Handle<uint32_t>::Reflect(context);
        Handle<uint16_t>::Reflect(context);
        Handle<uint8_t>::Reflect(context);
        Handle<int64_t>::Reflect(context);
        Handle<int32_t>::Reflect(context);
        Handle<int16_t>::Reflect(context);
        Handle<int8_t>::Reflect(context);
            
        //////////////////////////////////////////////////////////////////////////
    }

    void ReflectSystemComponent::ReflectNamedEnums(AZ::ReflectContext* context)
    {
        // Currently, we only add the named enums we need for json serialization here
        SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);

        if (!serializeContext)
        {
            return;
        }

        serializeContext->Enum<DrawListSortType>()
            ->Value("KeyThenDepth", DrawListSortType::KeyThenDepth)
            ->Value("KeyThenReverseDepth", DrawListSortType::KeyThenReverseDepth)
            ->Value("DepthThenKey", DrawListSortType::DepthThenKey)
            ->Value("ReverseDepthThenKey", DrawListSortType::ReverseDepthThenKey)
            ;

        serializeContext->Enum<ScopeAttachmentAccess>()
            ->Value("Read", ScopeAttachmentAccess::Read)
            ->Value("Write", ScopeAttachmentAccess::Write)
            ->Value("ReadWrite", ScopeAttachmentAccess::ReadWrite)
            ;

        serializeContext->Enum<AttachmentLifetimeType>()
            ->Value("Imported", AttachmentLifetimeType::Imported)
            ->Value("Transient", AttachmentLifetimeType::Transient)
            ;

        serializeContext->Enum<ScopeAttachmentUsage>()
            ->Value("RenderTarget", ScopeAttachmentUsage::RenderTarget)
            ->Value("DepthStencil", ScopeAttachmentUsage::DepthStencil)
            ->Value("Shader", ScopeAttachmentUsage::Shader)
            ->Value("Copy", ScopeAttachmentUsage::Copy)
            ->Value("Resolve", ScopeAttachmentUsage::Resolve)
            ->Value("Predication", ScopeAttachmentUsage::Predication)
            ->Value("Indirect", ScopeAttachmentUsage::Indirect)
            ->Value("SubpassInput", ScopeAttachmentUsage::SubpassInput)
            ->Value("InputAssembly", ScopeAttachmentUsage::InputAssembly)
            ->Value("ShadingRate", ScopeAttachmentUsage::ShadingRate)
            ;

        serializeContext->Enum<ScopeAttachmentStage>()
            ->Value("VertexShader", ScopeAttachmentStage::VertexShader)
            ->Value("FragmentShader", ScopeAttachmentStage::FragmentShader)
            ->Value("ComputeShader", ScopeAttachmentStage::ComputeShader)
            ->Value("RayTracingShader", ScopeAttachmentStage::RayTracingShader)
            ->Value("EarlyFragmentTest", ScopeAttachmentStage::EarlyFragmentTest)
            ->Value("LateFragmentTest", ScopeAttachmentStage::LateFragmentTest)
            ->Value("ColorAttachmentOutput", ScopeAttachmentStage::ColorAttachmentOutput)
            ->Value("Copy", ScopeAttachmentStage::Copy)
            ->Value("Predication", ScopeAttachmentStage::Predication)
            ->Value("DrawIndirect", ScopeAttachmentStage::DrawIndirect)
            ->Value("VertexInput", ScopeAttachmentStage::VertexInput)
            ->Value("ShadingRate", ScopeAttachmentStage::ShadingRate)
            ->Value("AnyGraphics", ScopeAttachmentStage::AnyGraphics)
            ->Value("Any", ScopeAttachmentStage::Any)
            ;

        serializeContext->Enum<HardwareQueueClass>()
            ->Value("Graphics", HardwareQueueClass::Graphics)
            ->Value("Compute", HardwareQueueClass::Compute)
            ->Value("Copy", HardwareQueueClass::Copy)
            ;

        serializeContext->Enum<HardwareQueueClassMask>()
            ->Value("None", HardwareQueueClassMask::None)
            ->Value("Graphics", HardwareQueueClassMask::Graphics)
            ->Value("Compute", HardwareQueueClassMask::Compute)
            ->Value("Copy", HardwareQueueClassMask::Copy)
            ->Value("All", HardwareQueueClassMask::All)
            ;

        serializeContext->Enum<AttachmentLoadAction>()
            ->Value("Load", AttachmentLoadAction::Load)
            ->Value("Clear", AttachmentLoadAction::Clear)
            ->Value("DontCare", AttachmentLoadAction::DontCare)
            ->Value("None", AttachmentLoadAction::None)
            ;

        serializeContext->Enum<AttachmentStoreAction>()
            ->Value("Store", AttachmentStoreAction::Store)
            ->Value("DontCare", AttachmentStoreAction::DontCare)
            ->Value("None", AttachmentStoreAction::None)
            ;

        serializeContext->Enum<AttachmentType>()
            ->Value("Image", AttachmentType::Image)
            ->Value("Buffer", AttachmentType::Buffer)
            ->Value("Resolve", AttachmentType::Resolve)
            ;

        serializeContext->Enum<HeapMemoryLevel>()
            ->Value("Host", HeapMemoryLevel::Host)
            ->Value("Device", HeapMemoryLevel::Device)
            ;

        serializeContext->Enum<HostMemoryAccess>()
            ->Value("Write", HostMemoryAccess::Write)
            ->Value("Read", HostMemoryAccess::Read)
            ;

        serializeContext->Enum<BufferBindFlags>()
            ->Value("None", BufferBindFlags::None)
            ->Value("InputAssembly", BufferBindFlags::InputAssembly)
            ->Value("DynamicInputAssembly", BufferBindFlags::DynamicInputAssembly)
            ->Value("Constant", BufferBindFlags::Constant)
            ->Value("CopyRead", BufferBindFlags::CopyRead)
            ->Value("CopyWrite", BufferBindFlags::CopyWrite)
            ->Value("ShaderRead", BufferBindFlags::ShaderRead)
            ->Value("ShaderWrite", BufferBindFlags::ShaderWrite)
            ->Value("ShaderReadWrite", BufferBindFlags::ShaderReadWrite)
            ;

        serializeContext->Enum<ImageBindFlags>()
            ->Value("None", ImageBindFlags::None)
            ->Value("Color", ImageBindFlags::Color)
            ->Value("CopyRead", ImageBindFlags::CopyRead)
            ->Value("CopyWrite", ImageBindFlags::CopyWrite)
            ->Value("Depth", ImageBindFlags::Depth)
            ->Value("Stencil", ImageBindFlags::Stencil)
            ->Value("DepthStencil", ImageBindFlags::DepthStencil)
            ->Value("ShaderRead", ImageBindFlags::ShaderRead)
            ->Value("ShaderWrite", ImageBindFlags::ShaderWrite)
            ->Value("ShaderReadWrite", ImageBindFlags::ShaderReadWrite)
            ->Value("ShadingRate", ImageBindFlags::ShadingRate)
            ;

        serializeContext->Enum<ImageAspectFlags>()
            ->Value("None", ImageAspectFlags::None)
            ->Value("Color", ImageAspectFlags::Color)
            ->Value("Depth", ImageAspectFlags::Depth)
            ->Value("Stencil", ImageAspectFlags::Stencil)
            ->Value("DepthStencil", ImageAspectFlags::DepthStencil)
            ->Value("All", ImageAspectFlags::All)
            ;

        serializeContext->Enum<Format>()
            ->Value("R32G32B32A32_FLOAT", Format::R32G32B32A32_FLOAT)
            ->Value("R32G32B32A32_UINT", Format::R32G32B32A32_UINT)
            ->Value("R32G32B32A32_SINT", Format::R32G32B32A32_SINT)
            ->Value("R32G32B32_FLOAT", Format::R32G32B32_FLOAT)
            ->Value("R32G32B32_UINT", Format::R32G32B32_UINT)
            ->Value("R32G32B32_SINT", Format::R32G32B32_SINT)
            ->Value("R16G16B16A16_FLOAT", Format::R16G16B16A16_FLOAT)
            ->Value("R16G16B16A16_UNORM", Format::R16G16B16A16_UNORM)
            ->Value("R16G16B16A16_UINT", Format::R16G16B16A16_UINT)
            ->Value("R16G16B16A16_SNORM", Format::R16G16B16A16_SNORM)
            ->Value("R16G16B16A16_SINT", Format::R16G16B16A16_SINT)
            ->Value("R32G32_FLOAT", Format::R32G32_FLOAT)
            ->Value("R32G32_UINT", Format::R32G32_UINT)
            ->Value("R32G32_SINT", Format::R32G32_SINT)
            ->Value("D32_FLOAT_S8X24_UINT", Format::D32_FLOAT_S8X24_UINT)
            ->Value("R10G10B10A2_UNORM", Format::R10G10B10A2_UNORM)
            ->Value("R10G10B10A2_UINT", Format::R10G10B10A2_UINT)
            ->Value("R11G11B10_FLOAT", Format::R11G11B10_FLOAT)
            ->Value("R8G8B8A8_UNORM", Format::R8G8B8A8_UNORM)
            ->Value("R8G8B8A8_UNORM_SRGB", Format::R8G8B8A8_UNORM_SRGB)
            ->Value("R8G8B8A8_UINT", Format::R8G8B8A8_UINT)
            ->Value("R8G8B8A8_SNORM", Format::R8G8B8A8_SNORM)
            ->Value("R8G8B8A8_SINT", Format::R8G8B8A8_SINT)
            ->Value("R16G16_FLOAT", Format::R16G16_FLOAT)
            ->Value("R16G16_UNORM", Format::R16G16_UNORM)
            ->Value("R16G16_UINT", Format::R16G16_UINT)
            ->Value("R16G16_SNORM", Format::R16G16_SNORM)
            ->Value("R16G16_SINT", Format::R16G16_SINT)
            ->Value("D32_FLOAT", Format::D32_FLOAT)
            ->Value("R32_FLOAT", Format::R32_FLOAT)
            ->Value("R32_UINT", Format::R32_UINT)
            ->Value("R32_SINT", Format::R32_SINT)
            ->Value("D24_UNORM_S8_UINT", Format::D24_UNORM_S8_UINT)
            ->Value("R8G8_UNORM", Format::R8G8_UNORM)
            ->Value("R8G8_UINT", Format::R8G8_UINT)
            ->Value("R8G8_SNORM", Format::R8G8_SNORM)
            ->Value("R8G8_SINT", Format::R8G8_SINT)
            ->Value("R16_FLOAT", Format::R16_FLOAT)
            ->Value("D16_UNORM", Format::D16_UNORM)
            ->Value("R16_UNORM", Format::R16_UNORM)
            ->Value("R16_UINT", Format::R16_UINT)
            ->Value("R16_SNORM", Format::R16_SNORM)
            ->Value("R16_SINT", Format::R16_SINT)
            ->Value("R8_UNORM", Format::R8_UNORM)
            ->Value("R8_UINT", Format::R8_UINT)
            ->Value("R8_SNORM", Format::R8_SNORM)
            ->Value("R8_SINT", Format::R8_SINT)
            ->Value("A8_UNORM", Format::A8_UNORM)
            ->Value("R1_UNORM", Format::R1_UNORM)
            ->Value("R9G9B9E5_SHAREDEXP", Format::R9G9B9E5_SHAREDEXP)
            ->Value("R8G8_B8G8_UNORM", Format::R8G8_B8G8_UNORM)
            ->Value("G8R8_G8B8_UNORM", Format::G8R8_G8B8_UNORM)
            ->Value("BC1_UNORM", Format::BC1_UNORM)
            ->Value("BC1_UNORM_SRGB", Format::BC1_UNORM_SRGB)
            ->Value("BC2_UNORM", Format::BC2_UNORM)
            ->Value("BC2_UNORM_SRGB", Format::BC2_UNORM_SRGB)
            ->Value("BC3_UNORM", Format::BC3_UNORM)
            ->Value("BC3_UNORM_SRGB", Format::BC3_UNORM_SRGB)
            ->Value("BC4_UNORM", Format::BC4_UNORM)
            ->Value("BC4_SNORM", Format::BC4_SNORM)
            ->Value("BC5_UNORM", Format::BC5_UNORM)
            ->Value("BC5_SNORM", Format::BC5_SNORM)
            ->Value("B5G6R5_UNORM", Format::B5G6R5_UNORM)
            ->Value("B5G5R5A1_UNORM", Format::B5G5R5A1_UNORM)
            ->Value("B8G8R8A8_UNORM", Format::B8G8R8A8_UNORM)
            ->Value("B8G8R8X8_UNORM", Format::B8G8R8X8_UNORM)
            ->Value("R10G10B10_XR_BIAS_A2_UNORM", Format::R10G10B10_XR_BIAS_A2_UNORM)
            ->Value("B8G8R8A8_UNORM_SRGB", Format::B8G8R8A8_UNORM_SRGB)
            ->Value("B8G8R8X8_UNORM_SRGB", Format::B8G8R8X8_UNORM_SRGB)
            ->Value("BC6H_UF16", Format::BC6H_UF16)
            ->Value("BC6H_SF16", Format::BC6H_SF16)
            ->Value("BC7_UNORM", Format::BC7_UNORM)
            ->Value("BC7_UNORM_SRGB", Format::BC7_UNORM_SRGB)
            ->Value("AYUV", Format::AYUV)
            ->Value("Y410", Format::Y410)
            ->Value("Y416", Format::Y416)
            ->Value("NV12", Format::NV12)
            ->Value("P010", Format::P010)
            ->Value("P016", Format::P016)
            ->Value("YUY2", Format::YUY2)
            ->Value("Y210", Format::Y210)
            ->Value("Y216", Format::Y216)
            ->Value("NV11", Format::NV11)
            ->Value("AI44", Format::AI44)
            ->Value("IA44", Format::IA44)
            ->Value("P8", Format::P8)
            ->Value("A8P8", Format::A8P8)
            ->Value("B4G4R4A4_UNORM", Format::B4G4R4A4_UNORM)
            ->Value("R10G10B10_7E3_A2_FLOAT", Format::R10G10B10_7E3_A2_FLOAT)
            ->Value("R10G10B10_6E4_A2_FLOAT", Format::R10G10B10_6E4_A2_FLOAT)
            ->Value("D16_UNORM_S8_UINT", Format::D16_UNORM_S8_UINT)
            ->Value("X16_TYPELESS_G8_UINT", Format::X16_TYPELESS_G8_UINT)
            ->Value("P208", Format::P208)
            ->Value("V208", Format::V208)
            ->Value("V408", Format::V408)
            ->Value("EAC_R11_UNORM", Format::EAC_R11_UNORM)
            ->Value("EAC_R11_SNORM", Format::EAC_R11_SNORM)
            ->Value("EAC_RG11_UNORM", Format::EAC_RG11_UNORM)
            ->Value("EAC_RG11_SNORM", Format::EAC_RG11_SNORM)
            ->Value("ETC2_UNORM", Format::ETC2_UNORM)
            ->Value("ETC2_UNORM_SRGB", Format::ETC2_UNORM_SRGB)
            ->Value("ETC2A_UNORM", Format::ETC2A_UNORM)
            ->Value("ETC2A_UNORM_SRGB", Format::ETC2A_UNORM_SRGB)
            ->Value("ETC2A1_UNORM", Format::ETC2A1_UNORM)
            ->Value("ETC2A1_UNORM_SRGB", Format::ETC2A1_UNORM_SRGB)
            ->Value("PVRTC2_UNORM", Format::PVRTC2_UNORM)
            ->Value("PVRTC2_UNORM_SRGB", Format::PVRTC2_UNORM_SRGB)
            ->Value("PVRTC4_UNORM", Format::PVRTC4_UNORM)
            ->Value("PVRTC4_UNORM_SRGB", Format::PVRTC4_UNORM_SRGB)
            ->Value("ASTC_4x4_UNORM", Format::ASTC_4x4_UNORM)
            ->Value("ASTC_4x4_UNORM_SRGB", Format::ASTC_4x4_UNORM_SRGB)
            ->Value("ASTC_5x4_UNORM", Format::ASTC_5x4_UNORM)
            ->Value("ASTC_5x4_UNORM_SRGB", Format::ASTC_5x4_UNORM_SRGB)
            ->Value("ASTC_5x5_UNORM", Format::ASTC_5x5_UNORM)
            ->Value("ASTC_5x5_UNORM_SRGB", Format::ASTC_5x5_UNORM_SRGB)
            ->Value("ASTC_6x5_UNORM", Format::ASTC_6x5_UNORM)
            ->Value("ASTC_6x5_UNORM_SRGB", Format::ASTC_6x5_UNORM_SRGB)
            ->Value("ASTC_6x6_UNORM", Format::ASTC_6x6_UNORM)
            ->Value("ASTC_6x6_UNORM_SRGB", Format::ASTC_6x6_UNORM_SRGB)
            ->Value("ASTC_8x5_UNORM", Format::ASTC_8x5_UNORM)
            ->Value("ASTC_8x5_UNORM_SRGB", Format::ASTC_8x5_UNORM_SRGB)
            ->Value("ASTC_8x6_UNORM", Format::ASTC_8x6_UNORM)
            ->Value("ASTC_8x6_UNORM_SRGB", Format::ASTC_8x6_UNORM_SRGB)
            ->Value("ASTC_8x8_UNORM", Format::ASTC_8x8_UNORM)
            ->Value("ASTC_8x8_UNORM_SRGB", Format::ASTC_8x8_UNORM_SRGB)
            ->Value("ASTC_10x5_UNORM", Format::ASTC_10x5_UNORM)
            ->Value("ASTC_10x5_UNORM_SRGB", Format::ASTC_10x5_UNORM_SRGB)
            ->Value("ASTC_10x6_UNORM", Format::ASTC_10x6_UNORM)
            ->Value("ASTC_10x6_UNORM_SRGB", Format::ASTC_10x6_UNORM_SRGB)
            ->Value("ASTC_10x8_UNORM", Format::ASTC_10x8_UNORM)
            ->Value("ASTC_10x8_UNORM_SRGB", Format::ASTC_10x8_UNORM_SRGB)
            ->Value("ASTC_10x10_UNORM", Format::ASTC_10x10_UNORM)
            ->Value("ASTC_10x10_UNORM_SRGB", Format::ASTC_10x10_UNORM_SRGB)
            ->Value("ASTC_12x10_UNORM", Format::ASTC_12x10_UNORM)
            ->Value("ASTC_12x10_UNORM_SRGB", Format::ASTC_12x10_UNORM_SRGB)
            ->Value("ASTC_12x12_UNORM", Format::ASTC_12x12_UNORM)
            ->Value("ASTC_12x12_UNORM_SRGB", Format::ASTC_12x12_UNORM_SRGB)
            ->Value("A8B8G8R8_UNORM", Format::A8B8G8R8_UNORM)
            ->Value("A8B8G8R8_UNORM_SRGB", Format::A8B8G8R8_UNORM_SRGB)
            ->Value("A8B8G8R8_SNORM", Format::A8B8G8R8_SNORM)
            ->Value("R5G6B5_UNORM", Format::R5G6B5_UNORM)
            ->Value("B8G8R8A8_SNORM", Format::B8G8R8A8_SNORM)
            ;
    }
}
