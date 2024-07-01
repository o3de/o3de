/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ImageDescriptor.h>
#include <Atom/RHI.Reflect/ImageViewDescriptor.h>
#include <Atom/RHI.Reflect/BufferDescriptor.h>
#include <Atom/RHI.Reflect/BufferViewDescriptor.h>
#include <Atom/RHI.Reflect/RenderStates.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayout.h>
#include <Atom/RHI.Reflect/InputStreamLayout.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/ClearValue.h>
#include <Atom/RHI.Reflect/MemoryEnums.h>
#include <Atom/RHI.Reflect/MultisampleState.h>
#include <Atom/RHI.Reflect/QueryPoolDescriptor.h>
#include <Atom/RHI.Reflect/VariableRateShadingEnums.h>
#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DeviceDrawItem.h>
#include <Atom/RHI/DeviceQuery.h>
#include <AzCore/std/containers/vector.h>
#include <RHI/DX12.h>

namespace AZ
{
    namespace DX12
    {
        class Image;
        class Buffer;

        D3D12_PRIMITIVE_TOPOLOGY_TYPE ConvertToTopologyType(RHI::PrimitiveTopology type);

        D3D12_PRIMITIVE_TOPOLOGY ConvertTopology(RHI::PrimitiveTopology topology);

        AZStd::vector<D3D12_INPUT_ELEMENT_DESC> ConvertInputElements(const RHI::InputStreamLayout& layout);

        D3D12_RESOURCE_DIMENSION ConvertImageDimension(RHI::ImageDimension dimension);

        D3D12_CLEAR_VALUE ConvertClearValue(RHI::Format format, RHI::ClearValue clearValue);

        D3D12_RESOURCE_FLAGS ConvertImageBindFlags(RHI::ImageBindFlags bindFlags);

        D3D12_RESOURCE_FLAGS ConvertBufferBindFlags(RHI::BufferBindFlags bindFlags);

        D3D12_DESCRIPTOR_RANGE_TYPE ConvertShaderInputBufferAccess(RHI::ShaderInputBufferAccess access);

        D3D12_DESCRIPTOR_RANGE_TYPE ConvertShaderInputImageAccess(RHI::ShaderInputImageAccess access);

        D3D12_COMMAND_LIST_TYPE ConvertHardwareQueueClass(RHI::HardwareQueueClass type);

        D3D12_HEAP_TYPE ConvertHeapType(RHI::HeapMemoryLevel heapMemoryLevel, RHI::HostMemoryAccess hostMemoryAccess);

        D3D12_RESOURCE_STATES ConvertInitialResourceState(RHI::HeapMemoryLevel heapMemoryLevel, RHI::HostMemoryAccess hostMemoryAccess);

        D3D12_SAMPLE_POSITION ConvertSamplePosition(const RHI::SamplePosition& position);

        D3D12_QUERY_HEAP_TYPE ConvertQueryHeapType(RHI::QueryType type);

        D3D12_QUERY_TYPE ConvertQueryType(RHI::QueryType type, RHI::QueryControlFlags flags);

        D3D12_PREDICATION_OP ConvertPredicationOp(RHI::PredicationOp op);

        D3D12_SRV_DIMENSION ConvertSRVDimension(AZ::RHI::ShaderInputImageType type);

        D3D12_UAV_DIMENSION ConvertUAVDimension(AZ::RHI::ShaderInputImageType type);

        DXGI_FORMAT ConvertFormat(RHI::Format format, bool raiseAsserts = true);

        D3D12_TEXTURE_ADDRESS_MODE ConvertAddressMode(RHI::AddressMode addressMode);

        D3D12_STATIC_BORDER_COLOR ConvertBorderColor(RHI::BorderColor color);

        D3D12_BLEND ConvertBlendFactor(RHI::BlendFactor factor);

        D3D12_BLEND_OP ConvertBlendOp(RHI::BlendOp op);

        D3D12_BLEND_DESC ConvertBlendState(const RHI::BlendState& blend);

        D3D12_FILL_MODE ConvertFillMode(RHI::FillMode mode);

        D3D12_CULL_MODE ConvertCullMode(RHI::CullMode mode);

        D3D12_RASTERIZER_DESC ConvertRasterState(const RHI::RasterState& raster);

        D3D12_COMPARISON_FUNC ConvertComparisonFunc(RHI::ComparisonFunc func);

        D3D12_STENCIL_OP ConvertStencilOp(RHI::StencilOp op);

        D3D12_DEPTH_WRITE_MASK ConvertDepthWriteMask(RHI::DepthWriteMask mask);

        D3D12_DEPTH_STENCIL_DESC ConvertDepthStencilState(const RHI::DepthStencilState& depthStencil);

        AZStd::vector<D3D12_INPUT_ELEMENT_DESC> ConvertInputElements(const RHI::InputStreamLayout& layout);

        D3D12_FILTER_TYPE ConvertFilterMode(RHI::FilterMode mode);

        D3D12_FILTER_REDUCTION_TYPE ConvertReductionType(RHI::ReductionType reductionType);

        D3D12_SHADER_VISIBILITY ConvertShaderStageMask(RHI::ShaderStageMask mask);

        void ConvertBorderColor(RHI::BorderColor color, float outputColor[4]);

        void ConvertBufferDescriptor(
            const RHI::BufferDescriptor& descriptor,
            D3D12_RESOURCE_DESC& resourceDesc);

        void ConvertImageDescriptor(
            const RHI::ImageDescriptor& descriptor,
             D3D12_RESOURCE_DESC& resourceDesc);

        DXGI_FORMAT ConvertImageViewFormat(
            const Image& image,
            const RHI::ImageViewDescriptor& imageViewDescriptor);

        void ConvertBufferView(
            const Buffer& buffer,
            const RHI::BufferViewDescriptor& bufferViewDescriptor,
            D3D12_SHADER_RESOURCE_VIEW_DESC& shaderResourceView);

        void ConvertBufferView(
            const Buffer& buffer,
            const RHI::BufferViewDescriptor& bufferViewDescriptor,
            D3D12_UNORDERED_ACCESS_VIEW_DESC& unorderedAccessView);

        void ConvertBufferView(
            const Buffer& buffer,
            const RHI::BufferViewDescriptor& bufferViewDescriptor,
            D3D12_CONSTANT_BUFFER_VIEW_DESC& constantBufferView);

        void ConvertImageView(
            const Image& image,
            const RHI::ImageViewDescriptor& imageViewDescriptor,
            D3D12_RENDER_TARGET_VIEW_DESC& renderTargetView);

        void ConvertImageView(
            const Image& image,
            const RHI::ImageViewDescriptor& imageViewDescriptor,
            D3D12_DEPTH_STENCIL_VIEW_DESC& depthStencilView);

        void ConvertImageView(
            const Image& image,
            const RHI::ImageViewDescriptor& imageViewDescriptor,
            D3D12_SHADER_RESOURCE_VIEW_DESC& shaderResourceView);

        void ConvertImageView(
            const Image& image,
            const RHI::ImageViewDescriptor& imageViewDescriptor,
            D3D12_UNORDERED_ACCESS_VIEW_DESC& unorderedAccessView);

        uint16_t ConvertImageAspectToPlaneSlice(RHI::ImageAspect aspect);
        RHI::ImageAspectFlags ConvertPlaneSliceToImageAspectFlags(uint16_t planeSlice);

        void ConvertSamplerState(const RHI::SamplerState& state, D3D12_SAMPLER_DESC& samplerDesc);

        void ConvertStaticSampler(
            const RHI::SamplerState& state,
            uint32_t shaderRegister,
            uint32_t shaderRegisterSpace,
            D3D12_SHADER_VISIBILITY shaderVisibility,
            D3D12_STATIC_SAMPLER_DESC& staticSamplerDesc);
    
        uint8_t ConvertColorWriteMask(uint8_t writeMask);

        D3D12_SHADING_RATE ConvertShadingRateEnum(RHI::ShadingRate rate);

        D3D12_SHADING_RATE_COMBINER ConvertShadingRateCombiner(RHI::ShadingRateCombinerOp op);

        RHI::ResultCode ConvertResult(HRESULT result);

        DXGI_SCALING ConvertScaling(RHI::Scaling scaling);
    }
}
