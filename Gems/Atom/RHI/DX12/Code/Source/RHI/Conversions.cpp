/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/Bits.h>
#include <RHI/Conversions.h>
#include <RHI/Buffer.h>
#include <RHI/Image.h>

namespace AZ
{
    namespace DX12
    {
        namespace Platform
        {
            void FinalizeConvertBufferDescriptor(
                const RHI::BufferDescriptor& descriptor,
                D3D12_RESOURCE_DESC& resourceDesc);

            void FinalizeConvertImageDescriptor(
                const RHI::ImageDescriptor& descriptor,
                D3D12_RESOURCE_DESC& resourceDesc);

            void FinalizeConvertBufferView(
                const Buffer& buffer,
                const RHI::BufferViewDescriptor& bufferViewDescriptor,
                D3D12_SHADER_RESOURCE_VIEW_DESC& shaderResourceView);

            void FinalizeConvertBufferView(
                const Buffer& buffer,
                const RHI::BufferViewDescriptor& bufferViewDescriptor,
                D3D12_UNORDERED_ACCESS_VIEW_DESC& unorderedAccessView);

            void FinalizeConvertBufferView(
                const Buffer& buffer,
                const RHI::BufferViewDescriptor& bufferViewDescriptor,
                D3D12_CONSTANT_BUFFER_VIEW_DESC& constantBufferView);

            void FinalizeConvertImageView(
                const Image& image,
                const RHI::ImageViewDescriptor& imageViewDescriptor,
                D3D12_RENDER_TARGET_VIEW_DESC& renderTargetView);

            void FinalizeConvertImageView(
                const Image& image,
                const RHI::ImageViewDescriptor& imageViewDescriptor,
                D3D12_DEPTH_STENCIL_VIEW_DESC& depthStencilView);

            void FinalizeConvertImageView(
                const Image& image,
                const RHI::ImageViewDescriptor& imageViewDescriptor,
                D3D12_SHADER_RESOURCE_VIEW_DESC& shaderResourceView);

            void FinalizeConvertImageView(
                const Image& image,
                const RHI::ImageViewDescriptor& imageViewDescriptor,
                D3D12_UNORDERED_ACCESS_VIEW_DESC& unorderedAccessView);
        }

        D3D12_PRIMITIVE_TOPOLOGY_TYPE ConvertToTopologyType(RHI::PrimitiveTopology topology)
        {
            switch(topology)
            {
            case RHI::PrimitiveTopology::PointList:
                return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
            case RHI::PrimitiveTopology::LineList:
            case RHI::PrimitiveTopology::LineStrip:
            case RHI::PrimitiveTopology::LineStripAdj:
                return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
            case RHI::PrimitiveTopology::TriangleList:
            case RHI::PrimitiveTopology::TriangleListAdj:
            case RHI::PrimitiveTopology::TriangleStrip:
            case RHI::PrimitiveTopology::TriangleStripAdj:
                return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            case RHI::PrimitiveTopology::PatchList:
                return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
            case RHI::PrimitiveTopology::Undefined:
            default:
                return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
            }
        }

        D3D12_PRIMITIVE_TOPOLOGY ConvertTopology(RHI::PrimitiveTopology topology)
        {
            static const D3D12_PRIMITIVE_TOPOLOGY table[] =
            {
                D3D_PRIMITIVE_TOPOLOGY_UNDEFINED,
                D3D_PRIMITIVE_TOPOLOGY_POINTLIST,
                D3D_PRIMITIVE_TOPOLOGY_LINELIST,
                D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ,
                D3D_PRIMITIVE_TOPOLOGY_LINESTRIP,
                D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ,
                D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
                D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ,
                D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
                D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ,
            };
            return table[(uint32_t)topology];
        }

        AZStd::vector<D3D12_INPUT_ELEMENT_DESC> ConvertInputElements(const RHI::InputStreamLayout& layout)
        {
            AZStd::vector<D3D12_INPUT_ELEMENT_DESC> result;
            result.reserve(layout.GetStreamChannels().size());

            for (size_t i = 0; i < layout.GetStreamChannels().size(); ++i)
            {
                const RHI::StreamChannelDescriptor& channel = layout.GetStreamChannels()[i];
                const RHI::StreamBufferDescriptor& buffer = layout.GetStreamBuffers()[channel.m_bufferIndex];

                result.emplace_back();

                D3D12_INPUT_ELEMENT_DESC& desc = result.back();
                desc.SemanticName = channel.m_semantic.m_name.GetCStr();
                desc.SemanticIndex = channel.m_semantic.m_index;
                desc.AlignedByteOffset = channel.m_byteOffset;
                desc.InputSlot = channel.m_bufferIndex;
                desc.InputSlotClass = (buffer.m_stepFunction == RHI::StreamStepFunction::PerVertex) ? D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA : D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
                desc.InstanceDataStepRate = (buffer.m_stepFunction != RHI::StreamStepFunction::PerVertex) ? buffer.m_stepRate : 0;
                desc.Format = ConvertFormat(channel.m_format);
            }

            return result;
        }

        D3D12_RESOURCE_DIMENSION ConvertImageDimension(RHI::ImageDimension dimension)
        {
            switch (dimension)
            {
            case RHI::ImageDimension::Image1D:
                return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
            case RHI::ImageDimension::Image2D:
                return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            case RHI::ImageDimension::Image3D:
                return D3D12_RESOURCE_DIMENSION_TEXTURE3D;

            default:
                AZ_Assert(false, "failed to convert image type");
                return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            }
        }

        D3D12_CLEAR_VALUE ConvertClearValue(RHI::Format format, RHI::ClearValue clearValue)
        {
            switch (clearValue.m_type)
            {
            case RHI::ClearValueType::DepthStencil:
                return CD3DX12_CLEAR_VALUE(ConvertFormat(format), clearValue.m_depthStencil.m_depth, clearValue.m_depthStencil.m_stencil);;
            case RHI::ClearValueType::Vector4Float:
            {
                float color[] =
                {
                    clearValue.m_vector4Float[0],
                    clearValue.m_vector4Float[1],
                    clearValue.m_vector4Float[2],
                    clearValue.m_vector4Float[3]
                };
                return CD3DX12_CLEAR_VALUE(ConvertFormat(format), color);
            }
            case RHI::ClearValueType::Vector4Uint:
                AZ_Assert(false, "Can't convert unsigned type to DX12 clear value. Use float instead.");
                return CD3DX12_CLEAR_VALUE{};
            }
            return CD3DX12_CLEAR_VALUE{};
        }

        void ConvertBufferView(
            const Buffer& buffer,
            const RHI::BufferViewDescriptor& bufferViewDescriptor,
            D3D12_SHADER_RESOURCE_VIEW_DESC& shaderResourceView)
        {
            const uint32_t elementOffsetBase = static_cast<uint32_t>(buffer.GetMemoryView().GetOffset()) / bufferViewDescriptor.m_elementSize;
            const uint32_t elementOffset = elementOffsetBase + bufferViewDescriptor.m_elementOffset;

            if (elementOffsetBase * bufferViewDescriptor.m_elementSize != buffer.GetMemoryView().GetOffset())
            {
                AZ_Error("RHI DX12", false, "ConvertBufferView - SRV: buffer wasn't aligned with element size; buffer should be created"
                    " with proper alignment");
            }

            shaderResourceView = {};
            shaderResourceView.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

            if (RHI::CheckBitsAll(buffer.GetDescriptor().m_bindFlags, RHI::BufferBindFlags::RayTracingAccelerationStructure))
            {
#ifdef AZ_DX12_DXR_SUPPORT
                shaderResourceView.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
                shaderResourceView.Format = DXGI_FORMAT_UNKNOWN;
                shaderResourceView.RaytracingAccelerationStructure.Location = buffer.GetMemoryView().GetGpuAddress();
#else
                AZ_Assert(false, "RayTracingAccelerationStructure created on a platform that does not support ray tracing");
#endif
            }
            else
            {
                shaderResourceView.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                shaderResourceView.Format = ConvertFormat(bufferViewDescriptor.m_elementFormat);
                shaderResourceView.Buffer.FirstElement = elementOffset;
                shaderResourceView.Buffer.NumElements = bufferViewDescriptor.m_elementCount;

                if (bufferViewDescriptor.m_elementFormat == RHI::Format::R32_UINT)
                {
                    shaderResourceView.Format = DXGI_FORMAT_R32_TYPELESS;
                    shaderResourceView.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
                }
                else if (shaderResourceView.Format == DXGI_FORMAT_UNKNOWN)
                {
                    shaderResourceView.Buffer.StructureByteStride = bufferViewDescriptor.m_elementSize;
                }
            }

            Platform::FinalizeConvertBufferView(buffer, bufferViewDescriptor, shaderResourceView);
        }

        void ConvertBufferView(
            const Buffer& buffer,
            const RHI::BufferViewDescriptor& bufferViewDescriptor,
            D3D12_UNORDERED_ACCESS_VIEW_DESC& unorderedAccessView)
        {
            const uint32_t elementOffsetBase = static_cast<uint32_t>(buffer.GetMemoryView().GetOffset()) / bufferViewDescriptor.m_elementSize;
            const uint32_t elementOffset = elementOffsetBase + bufferViewDescriptor.m_elementOffset;

            if (elementOffsetBase * bufferViewDescriptor.m_elementSize != buffer.GetMemoryView().GetOffset())
            {
                AZ_Error("RHI DX12", false, "ConvertBufferView - UAV: buffer wasn't aligned with element size; buffer should be created"
                    " with proper alignment");
            }
            unorderedAccessView = {};
            unorderedAccessView.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            unorderedAccessView.Format = ConvertFormat(bufferViewDescriptor.m_elementFormat);
            unorderedAccessView.Buffer.FirstElement = elementOffset;
            unorderedAccessView.Buffer.NumElements = bufferViewDescriptor.m_elementCount;

            if (bufferViewDescriptor.m_elementFormat == RHI::Format::R32_UINT)
            {
                unorderedAccessView.Format = DXGI_FORMAT_R32_TYPELESS;
                unorderedAccessView.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
            }
            else if (unorderedAccessView.Format == DXGI_FORMAT_UNKNOWN)
            {
                unorderedAccessView.Buffer.StructureByteStride = bufferViewDescriptor.m_elementSize;
            }

            Platform::FinalizeConvertBufferView(buffer, bufferViewDescriptor, unorderedAccessView);
        }

        void ConvertBufferView(
            const Buffer& buffer,
            const RHI::BufferViewDescriptor& bufferViewDescriptor,
            D3D12_CONSTANT_BUFFER_VIEW_DESC& constantBufferView)
        {
            AZ_Assert(RHI::IsAligned(buffer.GetMemoryView().GetGpuAddress(), Alignment::Constant),
                "Constant Buffer memory is not aligned to %d bytes.", Alignment::Constant);

            const uint32_t bufferOffset = bufferViewDescriptor.m_elementOffset * bufferViewDescriptor.m_elementSize;
            AZ_Error("RHI DX12", RHI::IsAligned(bufferOffset, Alignment::Constant),
                "Buffer View offset is not aligned to %d bytes, the view won't have the appropiate alignment for Constant Buffer reads.", Alignment::Constant);

            // In DX12 Constant data reads must be a multiple of 256 bytes.
            // It's not a problem if the actual buffer size is smaller since the heap (where the buffer resides) must be multiples of 64KB.
            // This means the buffer view will never go out of heap memory, it might read pass the Constant Buffer size, but it will never be used.
            const uint32_t bufferSize = RHI::AlignUp(bufferViewDescriptor.m_elementCount * bufferViewDescriptor.m_elementSize, Alignment::Constant);

            constantBufferView.BufferLocation = buffer.GetMemoryView().GetGpuAddress() + bufferOffset;
            constantBufferView.SizeInBytes = bufferSize;

            Platform::FinalizeConvertBufferView(buffer, bufferViewDescriptor, constantBufferView);
        }

        DXGI_FORMAT ConvertImageViewFormat(
            const Image& image,
            const RHI::ImageViewDescriptor& imageViewDescriptor)
        {
            /**
             * The format of the image is pulled from the base image or the view depending
             * on if the user overrides it in the view. It is legal for a view to have an unknown
             * format, which just means we're falling back to the image format.
             */
            return imageViewDescriptor.m_overrideFormat != RHI::Format::Unknown ?
                ConvertFormat(imageViewDescriptor.m_overrideFormat) :
                ConvertFormat(image.GetDescriptor().m_format);
        }

        void ConvertImageView(
            const Image& image,
            const RHI::ImageViewDescriptor& imageViewDescriptor,
            D3D12_RENDER_TARGET_VIEW_DESC& renderTargetView)
        {
            const RHI::ImageDescriptor& imageDescriptor = image.GetDescriptor();

            renderTargetView = {};
            renderTargetView.Format = ConvertImageViewFormat(image, imageViewDescriptor);

            const bool bIsArray = imageDescriptor.m_arraySize > 1 || imageViewDescriptor.m_isArray;
            const bool bIsMsaa = imageDescriptor.m_multisampleState.m_samples > 1;

            uint32_t ArraySize = (imageViewDescriptor.m_arraySliceMax - imageViewDescriptor.m_arraySliceMin) + 1;
            ArraySize = AZStd::min<uint32_t>(ArraySize, imageDescriptor.m_arraySize);

            switch (imageDescriptor.m_dimension)
            {
            case RHI::ImageDimension::Image1D:
                if (bIsArray)
                {
                    renderTargetView.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
                    renderTargetView.Texture1DArray.ArraySize = static_cast<uint16_t>(ArraySize);
                    renderTargetView.Texture1DArray.FirstArraySlice = imageViewDescriptor.m_arraySliceMin;
                    renderTargetView.Texture1DArray.MipSlice = imageViewDescriptor.m_mipSliceMin;
                }
                else
                {
                    renderTargetView.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
                    renderTargetView.Texture1D.MipSlice = imageViewDescriptor.m_mipSliceMin;
                }
                break;
            case RHI::ImageDimension::Image2D:
                if (bIsArray)
                {
                    if (bIsMsaa)
                    {
                        renderTargetView.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
                        renderTargetView.Texture2DMSArray.ArraySize = static_cast<uint16_t>(ArraySize);
                        renderTargetView.Texture2DMSArray.FirstArraySlice = imageViewDescriptor.m_arraySliceMin;
                    }
                    else
                    {
                        renderTargetView.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                        renderTargetView.Texture2DArray.ArraySize = static_cast<uint16_t>(ArraySize);
                        renderTargetView.Texture2DArray.FirstArraySlice = imageViewDescriptor.m_arraySliceMin;
                        renderTargetView.Texture2DArray.MipSlice = imageViewDescriptor.m_mipSliceMin;
                    }
                }
                else
                {
                    if (bIsMsaa)
                    {
                        renderTargetView.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
                    }
                    else
                    {
                        renderTargetView.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                        renderTargetView.Texture2D.MipSlice = imageViewDescriptor.m_mipSliceMin;
                    }
                }
                break;
            case RHI::ImageDimension::Image3D:
                renderTargetView.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
                renderTargetView.Texture3D.MipSlice = imageViewDescriptor.m_mipSliceMin;
                renderTargetView.Texture3D.FirstWSlice = imageViewDescriptor.m_depthSliceMin;

                if (imageViewDescriptor.m_depthSliceMax == RHI::ImageViewDescriptor::HighestSliceIndex)
                {
                    renderTargetView.Texture3D.WSize = std::numeric_limits<UINT>::max();
                }
                else
                {
                    renderTargetView.Texture3D.WSize = (imageViewDescriptor.m_depthSliceMax - imageViewDescriptor.m_depthSliceMin) + 1;
                }
                break;
            default:
                AZ_Assert(false, "Image dimension error %d", imageDescriptor.m_dimension);
            }

            Platform::FinalizeConvertImageView(image, imageViewDescriptor, renderTargetView);
        }

        void ConvertImageView(
            const Image& image,
            const RHI::ImageViewDescriptor& imageViewDescriptor,
            D3D12_DEPTH_STENCIL_VIEW_DESC& depthStencilView)
        {
            const RHI::ImageDescriptor& imageDescriptor = image.GetDescriptor();

            depthStencilView = {};
            depthStencilView.Format = GetDSVFormat(ConvertImageViewFormat(image, imageViewDescriptor));

            const bool bIsArray = imageDescriptor.m_arraySize > 1 || imageViewDescriptor.m_isArray;;
            const bool bIsMsaa = imageDescriptor.m_multisampleState.m_samples > 1;

            uint32_t ArraySize = (imageViewDescriptor.m_arraySliceMax - imageViewDescriptor.m_arraySliceMin) + 1;
            ArraySize = AZStd::min<uint32_t>(ArraySize, imageDescriptor.m_arraySize);

            switch (imageDescriptor.m_dimension)
            {
            case RHI::ImageDimension::Image2D:
                if (bIsArray)
                {
                    if (bIsMsaa)
                    {
                        depthStencilView.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
                        depthStencilView.Texture2DMSArray.ArraySize = static_cast<uint16_t>(ArraySize);
                        depthStencilView.Texture2DMSArray.FirstArraySlice = imageViewDescriptor.m_arraySliceMin;
                    }
                    else
                    {
                        depthStencilView.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                        depthStencilView.Texture2DArray.ArraySize = static_cast<uint16_t>(ArraySize);
                        depthStencilView.Texture2DArray.FirstArraySlice = imageViewDescriptor.m_arraySliceMin;
                        depthStencilView.Texture2DArray.MipSlice = imageViewDescriptor.m_mipSliceMin;
                    }
                }
                else
                {
                    if (bIsMsaa)
                    {
                        depthStencilView.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
                    }
                    else
                    {
                        depthStencilView.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
                        depthStencilView.Texture2D.MipSlice = imageViewDescriptor.m_mipSliceMin;
                    }
                }
                break;

            default:
                AZ_Assert(false, "unsupported");
            }

            Platform::FinalizeConvertImageView(image, imageViewDescriptor, depthStencilView);
        }

        void ConvertImageView(
            const Image& image,
            const RHI::ImageViewDescriptor& imageViewDescriptor,
            D3D12_SHADER_RESOURCE_VIEW_DESC& shaderResourceView)
        {
            const RHI::ImageDescriptor& imageDescriptor = image.GetDescriptor();

            shaderResourceView = {};
            shaderResourceView.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            shaderResourceView.Format = GetSRVFormat(ConvertImageViewFormat(image, imageViewDescriptor));

            const bool bIsArray = imageDescriptor.m_arraySize > 1 || imageViewDescriptor.m_isArray;;
            const bool bIsMsaa = imageDescriptor.m_multisampleState.m_samples > 1;
            const bool bIsCubemap = imageViewDescriptor.m_isCubemap != 0;

            uint32_t ArraySize = (imageViewDescriptor.m_arraySliceMax - imageViewDescriptor.m_arraySliceMin) + 1;
            ArraySize = AZStd::min<uint32_t>(ArraySize, imageDescriptor.m_arraySize);

            AZ_Assert(imageViewDescriptor.m_mipSliceMax < imageDescriptor.m_mipLevels,
                "ImageViewDescriptor specifies a mipSliceMax of [%d], which must be strictly smaller than the mip level count [%d].",
                imageViewDescriptor.m_mipSliceMax, imageDescriptor.m_mipLevels);

            uint32_t mipLevelCount = (imageViewDescriptor.m_mipSliceMax - imageViewDescriptor.m_mipSliceMin) + 1;

            switch (imageDescriptor.m_dimension)
            {
            case RHI::ImageDimension::Image1D:
                if (bIsArray)
                {
                    shaderResourceView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
                    shaderResourceView.Texture1DArray.ArraySize = static_cast<uint16_t>(ArraySize);
                    shaderResourceView.Texture1DArray.FirstArraySlice = imageViewDescriptor.m_arraySliceMin;
                    shaderResourceView.Texture1DArray.MipLevels = mipLevelCount;
                    shaderResourceView.Texture1DArray.MostDetailedMip = imageViewDescriptor.m_mipSliceMin;
                }
                else
                {
                    shaderResourceView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
                    shaderResourceView.Texture1D.MipLevels = mipLevelCount;
                    shaderResourceView.Texture1D.MostDetailedMip = imageViewDescriptor.m_mipSliceMin;
                }
                break;
            case RHI::ImageDimension::Image2D:
                if (bIsArray)
                {
                    if (bIsMsaa)
                    {
                        shaderResourceView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
                        shaderResourceView.Texture2DMSArray.ArraySize = static_cast<uint16_t>(ArraySize);
                        shaderResourceView.Texture2DMSArray.FirstArraySlice = imageViewDescriptor.m_arraySliceMin;
                    }
                    else if (bIsCubemap)
                    {
                        uint32_t cubeSliceCount = (ArraySize / 6);
                        if (cubeSliceCount > 1)
                        {
                            shaderResourceView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
                            shaderResourceView.TextureCubeArray.First2DArrayFace = imageViewDescriptor.m_arraySliceMin;
                            shaderResourceView.TextureCubeArray.MipLevels = mipLevelCount;
                            shaderResourceView.TextureCubeArray.MostDetailedMip = imageViewDescriptor.m_mipSliceMin;
                            shaderResourceView.TextureCubeArray.NumCubes = cubeSliceCount;
                        }
                        else
                        {
                            shaderResourceView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
                            shaderResourceView.TextureCube.MipLevels = mipLevelCount;
                            shaderResourceView.TextureCube.MostDetailedMip = imageViewDescriptor.m_mipSliceMin;
                        }
                    }
                    else
                    {
                        shaderResourceView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                        shaderResourceView.Texture2DArray.ArraySize = static_cast<uint16_t>(ArraySize);
                        shaderResourceView.Texture2DArray.FirstArraySlice = imageViewDescriptor.m_arraySliceMin;
                        shaderResourceView.Texture2DArray.MipLevels = mipLevelCount;
                        shaderResourceView.Texture2DArray.MostDetailedMip = imageViewDescriptor.m_mipSliceMin;
                    }
                }
                else
                {
                    if (bIsMsaa)
                    {
                        shaderResourceView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
                    }
                    else
                    {
                        shaderResourceView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                        shaderResourceView.Texture2D.MipLevels = mipLevelCount;
                        shaderResourceView.Texture2D.MostDetailedMip = imageViewDescriptor.m_mipSliceMin;
                    }
                }
                break;
            case RHI::ImageDimension::Image3D:
                shaderResourceView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
                shaderResourceView.Texture3D.MipLevels = mipLevelCount;
                shaderResourceView.Texture3D.MostDetailedMip = imageViewDescriptor.m_mipSliceMin;
                break;
            default:
                AZ_Assert(false, "Image dimension error %d", imageDescriptor.m_dimension);
            }

            Platform::FinalizeConvertImageView(image, imageViewDescriptor, shaderResourceView);
        }

        void ConvertImageView(
            const Image& image,
            const RHI::ImageViewDescriptor& imageViewDescriptor,
            D3D12_UNORDERED_ACCESS_VIEW_DESC& unorderedAccessView)
        {
            const RHI::ImageDescriptor& imageDescriptor = image.GetDescriptor();

            unorderedAccessView = {};
            unorderedAccessView.Format = GetUAVFormat(ConvertImageViewFormat(image, imageViewDescriptor));

            const bool bIsArray = imageDescriptor.m_arraySize > 1 || imageViewDescriptor.m_isArray;;
            uint32_t ArraySize = (imageViewDescriptor.m_arraySliceMax - imageViewDescriptor.m_arraySliceMin) + 1;
            ArraySize = AZStd::min<uint32_t>(ArraySize, imageDescriptor.m_arraySize);

            switch (imageDescriptor.m_dimension)
            {
            case RHI::ImageDimension::Image1D:
                if (bIsArray)
                {
                    unorderedAccessView.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
                    unorderedAccessView.Texture1DArray.ArraySize = static_cast<uint16_t>(ArraySize);
                    unorderedAccessView.Texture1DArray.FirstArraySlice = imageViewDescriptor.m_arraySliceMin;
                    unorderedAccessView.Texture1DArray.MipSlice = imageViewDescriptor.m_mipSliceMin;
                }
                else
                {
                    unorderedAccessView.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
                    unorderedAccessView.Texture1D.MipSlice = imageViewDescriptor.m_mipSliceMin;
                }
                break;
            case RHI::ImageDimension::Image2D:
                if (bIsArray)
                {
                    unorderedAccessView.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                    unorderedAccessView.Texture2DArray.ArraySize = static_cast<uint16_t>(ArraySize);
                    unorderedAccessView.Texture2DArray.FirstArraySlice = imageViewDescriptor.m_arraySliceMin;
                    unorderedAccessView.Texture2DArray.MipSlice = imageViewDescriptor.m_mipSliceMin;
                }
                else
                {
                    unorderedAccessView.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                    unorderedAccessView.Texture2D.MipSlice = imageViewDescriptor.m_mipSliceMin;
                }
                break;
            case RHI::ImageDimension::Image3D:
                unorderedAccessView.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
                unorderedAccessView.Texture3D.MipSlice = imageViewDescriptor.m_mipSliceMin;
                unorderedAccessView.Texture3D.FirstWSlice = imageViewDescriptor.m_depthSliceMin;

                if (imageViewDescriptor.m_depthSliceMax == RHI::ImageViewDescriptor::HighestSliceIndex)
                {
                    unorderedAccessView.Texture3D.WSize = std::numeric_limits<UINT>::max();
                }
                else
                {
                    unorderedAccessView.Texture3D.WSize = (imageViewDescriptor.m_depthSliceMax - imageViewDescriptor.m_depthSliceMin) + 1;
                }
                break;
            default:
                AZ_Assert(false, "Image dimension error %d", imageDescriptor.m_dimension);
            }

            Platform::FinalizeConvertImageView(image, imageViewDescriptor, unorderedAccessView);
        }

        D3D12_RESOURCE_FLAGS ConvertBufferBindFlags(RHI::BufferBindFlags bufferFlags)
        {
            D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
            if (RHI::CheckBitsAll(bufferFlags, RHI::BufferBindFlags::ShaderWrite))
            {
                resourceFlags = RHI::SetBits(resourceFlags, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
            }
            if (RHI::CheckBitsAny(bufferFlags, RHI::BufferBindFlags::ShaderRead | RHI::BufferBindFlags::RayTracingAccelerationStructure))
            {
                resourceFlags = RHI::ResetBits(resourceFlags, D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);
            }
            return resourceFlags;
        }

        D3D12_RESOURCE_FLAGS ConvertImageBindFlags(RHI::ImageBindFlags imageFlags)
        {
            D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
            if (RHI::CheckBitsAll(imageFlags, RHI::ImageBindFlags::ShaderWrite))
            {
                resourceFlags = RHI::SetBits(resourceFlags, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
            }
            if (RHI::CheckBitsAll(imageFlags, RHI::ImageBindFlags::ShaderRead))
            {
                resourceFlags = RHI::ResetBits(resourceFlags, D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);
            }
            if (RHI::CheckBitsAll(imageFlags, RHI::ImageBindFlags::Color))
            {
                resourceFlags = RHI::SetBits(resourceFlags, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
            }
            if (RHI::CheckBitsAny(imageFlags, RHI::ImageBindFlags::DepthStencil))
            {
                resourceFlags = RHI::SetBits(resourceFlags, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
            }
            else
            {
                resourceFlags = RHI::ResetBits(resourceFlags, D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);
            }
            return resourceFlags;
        }

        void ConvertBufferDescriptor(
            const RHI::BufferDescriptor& descriptor,
            D3D12_RESOURCE_DESC& resourceDesc)
        {
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            resourceDesc.Alignment = 0;
            resourceDesc.Width = RHI::AlignUp(descriptor.m_byteCount, Alignment::CommittedBuffer);
            resourceDesc.Height = 1;
            resourceDesc.DepthOrArraySize = 1;
            resourceDesc.MipLevels = 1;
            resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
            resourceDesc.SampleDesc = DXGI_SAMPLE_DESC{ 1, 0 };
            resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            resourceDesc.Flags = ConvertBufferBindFlags(descriptor.m_bindFlags);

            Platform::FinalizeConvertBufferDescriptor(descriptor, resourceDesc);
        }

        void ConvertImageDescriptor(
            const RHI::ImageDescriptor& descriptor,
            D3D12_RESOURCE_DESC& resourceDesc)
        {
            resourceDesc.Dimension = ConvertImageDimension(descriptor.m_dimension);
            resourceDesc.Alignment = 0;
            resourceDesc.Width = descriptor.m_size.m_width;
            resourceDesc.Height = descriptor.m_size.m_height;
            resourceDesc.DepthOrArraySize = static_cast<uint16_t>(resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? descriptor.m_size.m_depth : descriptor.m_arraySize);
            resourceDesc.MipLevels = descriptor.m_mipLevels;
            resourceDesc.Format = GetBaseFormat(ConvertFormat(descriptor.m_format));
            resourceDesc.SampleDesc = DXGI_SAMPLE_DESC{ descriptor.m_multisampleState.m_samples, descriptor.m_multisampleState.m_quality };
            resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            resourceDesc.Flags = ConvertImageBindFlags(descriptor.m_bindFlags);

            Platform::FinalizeConvertImageDescriptor(descriptor, resourceDesc);
        }

        D3D12_DESCRIPTOR_RANGE_TYPE ConvertShaderInputBufferAccess(RHI::ShaderInputBufferAccess access)
        {
            static const D3D12_DESCRIPTOR_RANGE_TYPE Table[] =
            {
                D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                D3D12_DESCRIPTOR_RANGE_TYPE_UAV
            };

            return Table[static_cast<size_t>(access)];
        }

        D3D12_DESCRIPTOR_RANGE_TYPE ConvertShaderInputImageAccess(RHI::ShaderInputImageAccess access)
        {
            static const D3D12_DESCRIPTOR_RANGE_TYPE Table[] =
            {
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                D3D12_DESCRIPTOR_RANGE_TYPE_UAV
            };

            return Table[static_cast<size_t>(access)];
        }

        D3D12_COMMAND_LIST_TYPE ConvertHardwareQueueClass(RHI::HardwareQueueClass type)
        {
            static const D3D12_COMMAND_LIST_TYPE Table[] =
            {
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                D3D12_COMMAND_LIST_TYPE_COMPUTE,
                D3D12_COMMAND_LIST_TYPE_COPY
            };

            return Table[static_cast<size_t>(type)];
        }

        D3D12_HEAP_TYPE ConvertHeapType(RHI::HeapMemoryLevel heapMemoryLevel, RHI::HostMemoryAccess hostMemoryAccess)
        {
            switch (heapMemoryLevel)
            {
            case RHI::HeapMemoryLevel::Host:
                switch (hostMemoryAccess)
                {
                case RHI::HostMemoryAccess::Write:
                    return D3D12_HEAP_TYPE_UPLOAD;
                case RHI::HostMemoryAccess::Read:
                    return D3D12_HEAP_TYPE_READBACK;
                };
            case RHI::HeapMemoryLevel::Device:
                return D3D12_HEAP_TYPE_DEFAULT;
            }
            AZ_Assert(false, "Invalid Heap Type");
            return D3D12_HEAP_TYPE_CUSTOM;
        }

        D3D12_RESOURCE_STATES ConvertInitialResourceState(RHI::HeapMemoryLevel heapMemoryLevel, RHI::HostMemoryAccess hostMemoryAccess)
        {
            if (heapMemoryLevel == RHI::HeapMemoryLevel::Host)
            {
                return hostMemoryAccess == RHI::HostMemoryAccess::Write ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COPY_DEST;
            }
            return D3D12_RESOURCE_STATE_COMMON;
        }

        D3D12_SAMPLE_POSITION ConvertSamplePosition(const RHI::SamplePosition& position)
        {
            static const uint8_t offset = RHI::Limits::Pipeline::MultiSampleCustomLocationGridSize / 2;
            return D3D12_SAMPLE_POSITION{ static_cast<INT8>(position.m_x - offset), static_cast<INT8>(position.m_y - offset) };
        }

        D3D12_QUERY_HEAP_TYPE ConvertQueryHeapType(RHI::QueryType type)
        {
            static const D3D12_QUERY_HEAP_TYPE table[] =
            {
                D3D12_QUERY_HEAP_TYPE_OCCLUSION,
                D3D12_QUERY_HEAP_TYPE_TIMESTAMP,
                D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS
            };
            static_assert(RHI::QueryTypeCount == AZ_ARRAY_SIZE(table), "QueryType count does not match DX12 query heap type table size");
            AZ_Assert(static_cast<uint32_t>(type) < RHI::QueryTypeCount, "Unsupported query type");
            return table[(uint32_t)type];
        }

        D3D12_QUERY_TYPE ConvertQueryType(RHI::QueryType type, RHI::QueryControlFlags flags)
        {
            switch (type)
            {
            case RHI::QueryType::Occlusion:
                return RHI::CheckBitsAll(flags, RHI::QueryControlFlags::PreciseOcclusion) ? D3D12_QUERY_TYPE_OCCLUSION : D3D12_QUERY_TYPE_BINARY_OCCLUSION;
            case RHI::QueryType::PipelineStatistics:
                return D3D12_QUERY_TYPE_PIPELINE_STATISTICS;
            case RHI::QueryType::Timestamp:
                return D3D12_QUERY_TYPE_TIMESTAMP;
            default:
                AZ_Assert(false, "Invalid query type");
                return D3D12_QUERY_TYPE_OCCLUSION;
            }
        }

        D3D12_PREDICATION_OP ConvertPredicationOp(RHI::PredicationOp op)
        {
            static const D3D12_PREDICATION_OP table[] =
            {
                D3D12_PREDICATION_OP_EQUAL_ZERO,
                D3D12_PREDICATION_OP_NOT_EQUAL_ZERO
            };
            static_assert(static_cast<uint32_t>(RHI::PredicationOp::Count) == AZ_ARRAY_SIZE(table), "PredicationOp count does not match DX12 predication op type table size");
            AZ_Assert(static_cast<uint32_t>(op) < static_cast<uint32_t>(RHI::PredicationOp::Count), "Unsupported predication op");

            return table[static_cast<uint32_t>(op)];
        }

        D3D12_SRV_DIMENSION ConvertSRVDimension(AZ::RHI::ShaderInputImageType type)
        {
            switch (type)
            {
            case AZ::RHI::ShaderInputImageType::Image1D:
                return D3D12_SRV_DIMENSION_TEXTURE1D;
            case AZ::RHI::ShaderInputImageType::Image1DArray:
                return D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
            case AZ::RHI::ShaderInputImageType::Image2D:
                return D3D12_SRV_DIMENSION_TEXTURE2D;
            case AZ::RHI::ShaderInputImageType::Image2DArray:
                return D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            case AZ::RHI::ShaderInputImageType::Image2DMultisample:
                return D3D12_SRV_DIMENSION_TEXTURE2DMS;
            case AZ::RHI::ShaderInputImageType::Image2DMultisampleArray:
                return D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
            case AZ::RHI::ShaderInputImageType::Image3D:
                return D3D12_SRV_DIMENSION_TEXTURE3D;
            case AZ::RHI::ShaderInputImageType::ImageCube:
                return D3D12_SRV_DIMENSION_TEXTURECUBE;
            case AZ::RHI::ShaderInputImageType::ImageCubeArray:
                return D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
            case AZ::RHI::ShaderInputImageType::Unknown:
                return D3D12_SRV_DIMENSION_UNKNOWN;
            }

            AZ_Assert(false, "Unknown enum in ConvertSRVDimension");
            return D3D12_SRV_DIMENSION_UNKNOWN;
        }

        D3D12_UAV_DIMENSION ConvertUAVDimension(AZ::RHI::ShaderInputImageType type)
        {
            switch (type)
            {
            case RHI::ShaderInputImageType::Image1D:
                return D3D12_UAV_DIMENSION_TEXTURE1D;
            case RHI::ShaderInputImageType::Image1DArray:
                return D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
            case RHI::ShaderInputImageType::Image2D:
                return D3D12_UAV_DIMENSION_TEXTURE2D;
            case RHI::ShaderInputImageType::Image2DArray:
                return D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            case RHI::ShaderInputImageType::Image3D:
                return D3D12_UAV_DIMENSION_TEXTURE3D;
            case AZ::RHI::ShaderInputImageType::Unknown:
                return D3D12_UAV_DIMENSION_UNKNOWN;
            }

            AZ_Assert(false, "Unknown enum in ConvertUAVDimension");
            return D3D12_UAV_DIMENSION_UNKNOWN;
        }

        uint16_t ConvertImageAspectToPlaneSlice(RHI::ImageAspect aspect)
        {
            switch (aspect)
            {
            case RHI::ImageAspect::Color:
            case RHI::ImageAspect::Depth:
                return 0;
            case RHI::ImageAspect::Stencil:
                return 1;
            default:
                AZ_Assert(false, "Invalid image aspect %d", aspect);
                return 0;
            }
        }

        RHI::ImageAspectFlags ConvertPlaneSliceToImageAspectFlags(uint16_t planeSlice)
        {
            if (planeSlice == 0)
            {
                return RHI::ImageAspectFlags::Depth | RHI::ImageAspectFlags::Color;
            }
            else if (planeSlice == 1)
            {
                return RHI::ImageAspectFlags::Stencil;
            }
            return RHI::ImageAspectFlags::None;
        }
    
        DXGI_FORMAT ConvertFormat(RHI::Format format, [[maybe_unused]] bool raiseAsserts)
        {
            switch (format)
            {
            case RHI::Format::Unknown:
                return DXGI_FORMAT_UNKNOWN;
            case RHI::Format::R32G32B32A32_FLOAT:
                return DXGI_FORMAT_R32G32B32A32_FLOAT;
            case RHI::Format::R32G32B32A32_UINT:
                return DXGI_FORMAT_R32G32B32A32_UINT;
            case RHI::Format::R32G32B32A32_SINT:
                return DXGI_FORMAT_R32G32B32A32_SINT;
            case RHI::Format::R32G32B32_FLOAT:
                return DXGI_FORMAT_R32G32B32_FLOAT;
            case RHI::Format::R32G32B32_UINT:
                return DXGI_FORMAT_R32G32B32_UINT;
            case RHI::Format::R32G32B32_SINT:
                return DXGI_FORMAT_R32G32B32_SINT;
            case RHI::Format::R16G16B16A16_FLOAT:
                return DXGI_FORMAT_R16G16B16A16_FLOAT;
            case RHI::Format::R16G16B16A16_UNORM:
                return DXGI_FORMAT_R16G16B16A16_UNORM;
            case RHI::Format::R16G16B16A16_UINT:
                return DXGI_FORMAT_R16G16B16A16_UINT;
            case RHI::Format::R16G16B16A16_SNORM:
                return DXGI_FORMAT_R16G16B16A16_SNORM;
            case RHI::Format::R16G16B16A16_SINT:
                return DXGI_FORMAT_R16G16B16A16_SINT;
            case RHI::Format::R32G32_FLOAT:
                return DXGI_FORMAT_R32G32_FLOAT;
            case RHI::Format::R32G32_UINT:
                return DXGI_FORMAT_R32G32_UINT;
            case RHI::Format::R32G32_SINT:
                return DXGI_FORMAT_R32G32_SINT;
            case RHI::Format::D32_FLOAT_S8X24_UINT:
                return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
            case RHI::Format::R10G10B10A2_UNORM:
                return DXGI_FORMAT_R10G10B10A2_UNORM;
            case RHI::Format::R10G10B10A2_UINT:
                return DXGI_FORMAT_R10G10B10A2_UINT;
            case RHI::Format::R11G11B10_FLOAT:
                return DXGI_FORMAT_R11G11B10_FLOAT;
            case RHI::Format::R8G8B8A8_UNORM:
                return DXGI_FORMAT_R8G8B8A8_UNORM;
            case RHI::Format::R8G8B8A8_UNORM_SRGB:
                return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            case RHI::Format::R8G8B8A8_UINT:
                return DXGI_FORMAT_R8G8B8A8_UINT;
            case RHI::Format::R8G8B8A8_SNORM:
                return DXGI_FORMAT_R8G8B8A8_SNORM;
            case RHI::Format::R8G8B8A8_SINT:
                return DXGI_FORMAT_R8G8B8A8_SINT;
            case RHI::Format::R16G16_FLOAT:
                return DXGI_FORMAT_R16G16_FLOAT;
            case RHI::Format::R16G16_UNORM:
                return DXGI_FORMAT_R16G16_UNORM;
            case RHI::Format::R16G16_UINT:
                return DXGI_FORMAT_R16G16_UINT;
            case RHI::Format::R16G16_SNORM:
                return DXGI_FORMAT_R16G16_SNORM;
            case RHI::Format::R16G16_SINT:
                return DXGI_FORMAT_R16G16_SINT;
            case RHI::Format::D32_FLOAT:
                return DXGI_FORMAT_D32_FLOAT;
            case RHI::Format::R32_FLOAT:
                return DXGI_FORMAT_R32_FLOAT;
            case RHI::Format::R32_UINT:
                return DXGI_FORMAT_R32_UINT;
            case RHI::Format::R32_SINT:
                return DXGI_FORMAT_R32_SINT;
            case RHI::Format::D24_UNORM_S8_UINT:
                return DXGI_FORMAT_D24_UNORM_S8_UINT;
            case RHI::Format::R8G8_UNORM:
                return DXGI_FORMAT_R8G8_UNORM;
            case RHI::Format::R8G8_UINT:
                return DXGI_FORMAT_R8G8_UINT;
            case RHI::Format::R8G8_SNORM:
                return DXGI_FORMAT_R8G8_SNORM;
            case RHI::Format::R8G8_SINT:
                return DXGI_FORMAT_R8G8_SINT;
            case RHI::Format::R16_FLOAT:
                return DXGI_FORMAT_R16_FLOAT;
            case RHI::Format::D16_UNORM:
                return DXGI_FORMAT_D16_UNORM;
            case RHI::Format::R16_UNORM:
                return DXGI_FORMAT_R16_UNORM;
            case RHI::Format::R16_UINT:
                return DXGI_FORMAT_R16_UINT;
            case RHI::Format::R16_SNORM:
                return DXGI_FORMAT_R16_SNORM;
            case RHI::Format::R16_SINT:
                return DXGI_FORMAT_R16_SINT;
            case RHI::Format::R8_UNORM:
                return DXGI_FORMAT_R8_UNORM;
            case RHI::Format::R8_UINT:
                return DXGI_FORMAT_R8_UINT;
            case RHI::Format::R8_SNORM:
                return DXGI_FORMAT_R8_SNORM;
            case RHI::Format::R8_SINT:
                return DXGI_FORMAT_R8_SINT;
            case RHI::Format::A8_UNORM:
                return DXGI_FORMAT_A8_UNORM;
            case RHI::Format::R1_UNORM:
                return DXGI_FORMAT_R1_UNORM;
            case RHI::Format::R9G9B9E5_SHAREDEXP:
                return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
            case RHI::Format::R8G8_B8G8_UNORM:
                return DXGI_FORMAT_R8G8_B8G8_UNORM;
            case RHI::Format::G8R8_G8B8_UNORM:
                return DXGI_FORMAT_G8R8_G8B8_UNORM;
            case RHI::Format::BC1_UNORM:
                return DXGI_FORMAT_BC1_UNORM;
            case RHI::Format::BC1_UNORM_SRGB:
                return DXGI_FORMAT_BC1_UNORM_SRGB;
            case RHI::Format::BC2_UNORM:
                return DXGI_FORMAT_BC2_UNORM;
            case RHI::Format::BC2_UNORM_SRGB:
                return DXGI_FORMAT_BC2_UNORM_SRGB;
            case RHI::Format::BC3_UNORM:
                return DXGI_FORMAT_BC3_UNORM;
            case RHI::Format::BC3_UNORM_SRGB:
                return DXGI_FORMAT_BC3_UNORM_SRGB;
            case RHI::Format::BC4_UNORM:
                return DXGI_FORMAT_BC4_UNORM;
            case RHI::Format::BC4_SNORM:
                return DXGI_FORMAT_BC4_SNORM;
            case RHI::Format::BC5_UNORM:
                return DXGI_FORMAT_BC5_UNORM;
            case RHI::Format::BC5_SNORM:
                return DXGI_FORMAT_BC5_SNORM;
            case RHI::Format::B5G6R5_UNORM:
                return DXGI_FORMAT_B5G6R5_UNORM;
            case RHI::Format::B5G5R5A1_UNORM:
                return DXGI_FORMAT_B5G5R5A1_UNORM;
            case RHI::Format::B8G8R8A8_UNORM:
                return DXGI_FORMAT_B8G8R8A8_UNORM;
            case RHI::Format::B8G8R8X8_UNORM:
                return DXGI_FORMAT_B8G8R8X8_UNORM;
            case RHI::Format::R10G10B10_XR_BIAS_A2_UNORM:
                return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;
            case RHI::Format::B8G8R8A8_UNORM_SRGB:
                return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
            case RHI::Format::B8G8R8X8_UNORM_SRGB:
                return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
            case RHI::Format::BC6H_UF16:
                return DXGI_FORMAT_BC6H_UF16;
            case RHI::Format::BC6H_SF16:
                return DXGI_FORMAT_BC6H_SF16;
            case RHI::Format::BC7_UNORM:
                return DXGI_FORMAT_BC7_UNORM;
            case RHI::Format::BC7_UNORM_SRGB:
                return DXGI_FORMAT_BC7_UNORM_SRGB;
            case RHI::Format::AYUV:
                return DXGI_FORMAT_AYUV;
            case RHI::Format::Y410:
                return DXGI_FORMAT_Y410;
            case RHI::Format::Y416:
                return DXGI_FORMAT_Y416;
            case RHI::Format::NV12:
                return DXGI_FORMAT_NV12;
            case RHI::Format::P010:
                return DXGI_FORMAT_P010;
            case RHI::Format::P016:
                return DXGI_FORMAT_P016;
            case RHI::Format::YUY2:
                return DXGI_FORMAT_YUY2;
            case RHI::Format::Y210:
                return DXGI_FORMAT_Y210;
            case RHI::Format::Y216:
                return DXGI_FORMAT_Y216;
            case RHI::Format::NV11:
                return DXGI_FORMAT_NV11;
            case RHI::Format::AI44:
                return DXGI_FORMAT_AI44;
            case RHI::Format::IA44:
                return DXGI_FORMAT_IA44;
            case RHI::Format::P8:
                return DXGI_FORMAT_P8;
            case RHI::Format::A8P8:
                return DXGI_FORMAT_A8P8;
            case RHI::Format::B4G4R4A4_UNORM:
                return DXGI_FORMAT_B4G4R4A4_UNORM;
            case RHI::Format::P208:
                return DXGI_FORMAT_P208;
            case RHI::Format::V208:
                return DXGI_FORMAT_V208;
            case RHI::Format::V408:
                return DXGI_FORMAT_V408;

            default:
                AZ_Assert(!raiseAsserts, "unhandled conversion in ConvertFormat");
                return DXGI_FORMAT_UNKNOWN;
            }
        }

        D3D12_FILTER_TYPE ConvertFilterMode(RHI::FilterMode mode)
        {
            switch (mode)
            {
            case RHI::FilterMode::Point:
                return D3D12_FILTER_TYPE_POINT;
            case RHI::FilterMode::Linear:
                return D3D12_FILTER_TYPE_LINEAR;
            }

            AZ_Assert(false, "bad conversion in ConvertFilterMode");
            return D3D12_FILTER_TYPE_POINT;
        }

        D3D12_FILTER_REDUCTION_TYPE ConvertReductionType(RHI::ReductionType reductionType)
        {
            switch (reductionType)
            {
            case RHI::ReductionType::Filter:
                return D3D12_FILTER_REDUCTION_TYPE_STANDARD;
            case RHI::ReductionType::Comparison:
                return D3D12_FILTER_REDUCTION_TYPE_COMPARISON;
            case RHI::ReductionType::Minimum:
                return D3D12_FILTER_REDUCTION_TYPE_MINIMUM;
            case RHI::ReductionType::Maximum:
                return D3D12_FILTER_REDUCTION_TYPE_MAXIMUM;
            }

            AZ_Assert(false, "bad conversion in ConvertReductionType");
            return D3D12_FILTER_REDUCTION_TYPE_STANDARD;
        }

        D3D12_TEXTURE_ADDRESS_MODE ConvertAddressMode(RHI::AddressMode addressMode)
        {
            switch (addressMode)
            {
            case RHI::AddressMode::Wrap:
                return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            case RHI::AddressMode::Clamp:
                return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            case RHI::AddressMode::Mirror:
                return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
            case RHI::AddressMode::MirrorOnce:
                return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
            case RHI::AddressMode::Border:
                return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            }

            AZ_Assert(false, "bad conversion in ConvertAddressMode");
            return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        }

        void ConvertBorderColor(RHI::BorderColor color, float outputColor[4])
        {
            switch (color)
            {
            case RHI::BorderColor::OpaqueBlack:
                outputColor[0] = outputColor[1] = outputColor[2] = 0.0f;
                outputColor[3] = 1.0f;
                return;
            case RHI::BorderColor::TransparentBlack:
                outputColor[0] = outputColor[1] = outputColor[2] = outputColor[3] = 0.0f;
                return;
            case RHI::BorderColor::OpaqueWhite:
                outputColor[0] = outputColor[1] = outputColor[2] = outputColor[3] = 1.0f;
                return;
            }
        }

        D3D12_STATIC_BORDER_COLOR ConvertBorderColor(RHI::BorderColor color)
        {
            switch (color)
            {
            case RHI::BorderColor::OpaqueBlack:
                return D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
            case RHI::BorderColor::TransparentBlack:
                return D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
            case RHI::BorderColor::OpaqueWhite:
                return D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
            }
            return D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        }

        void ConvertSamplerState(const RHI::SamplerState& state, D3D12_SAMPLER_DESC& samplerDesc)
        {
            D3D12_FILTER filter;
            D3D12_FILTER_REDUCTION_TYPE reduction = ConvertReductionType(state.m_reductionType);
            if (state.m_anisotropyEnable)
            {
                filter = D3D12_ENCODE_ANISOTROPIC_FILTER(reduction);
            }
            else
            {
                D3D12_FILTER_TYPE min = ConvertFilterMode(state.m_filterMin);
                D3D12_FILTER_TYPE mag = ConvertFilterMode(state.m_filterMag);
                D3D12_FILTER_TYPE mip = ConvertFilterMode(state.m_filterMip);
                filter = D3D12_ENCODE_BASIC_FILTER(min, mag, mip, reduction);
            }

            samplerDesc.AddressU = ConvertAddressMode(state.m_addressU);
            samplerDesc.AddressV = ConvertAddressMode(state.m_addressV);
            samplerDesc.AddressW = ConvertAddressMode(state.m_addressW);
            ConvertBorderColor(state.m_borderColor, samplerDesc.BorderColor);
            samplerDesc.ComparisonFunc = ConvertComparisonFunc(state.m_comparisonFunc);
            samplerDesc.Filter = filter;
            samplerDesc.MaxAnisotropy = uint8_t(state.m_anisotropyMax);
            samplerDesc.MaxLOD = uint8_t(state.m_mipLodMax);
            samplerDesc.MinLOD = uint8_t(state.m_mipLodMin);
            samplerDesc.MipLODBias = state.m_mipLodBias;
        }

        void ConvertStaticSampler(
            const RHI::SamplerState& state,
            uint32_t shaderRegister,
            uint32_t shaderRegisterSpace,
            D3D12_SHADER_VISIBILITY shaderVisibility,
            D3D12_STATIC_SAMPLER_DESC& staticSamplerDesc)
        {
            D3D12_SAMPLER_DESC samplerDesc;
            ConvertSamplerState(state, samplerDesc);

            staticSamplerDesc.AddressU = samplerDesc.AddressU;
            staticSamplerDesc.AddressV = samplerDesc.AddressV;
            staticSamplerDesc.AddressW = samplerDesc.AddressW;
            staticSamplerDesc.BorderColor = ConvertBorderColor(state.m_borderColor);
            staticSamplerDesc.ComparisonFunc = samplerDesc.ComparisonFunc;
            staticSamplerDesc.Filter = samplerDesc.Filter;
            staticSamplerDesc.MaxAnisotropy = samplerDesc.MaxAnisotropy;
            staticSamplerDesc.MaxLOD = samplerDesc.MaxLOD;
            staticSamplerDesc.MinLOD = samplerDesc.MinLOD;
            staticSamplerDesc.MipLODBias = samplerDesc.MipLODBias;
            staticSamplerDesc.ShaderRegister = shaderRegister;
            staticSamplerDesc.RegisterSpace = shaderRegisterSpace;
            staticSamplerDesc.ShaderVisibility = shaderVisibility;
        }

        D3D12_SHADER_VISIBILITY ConvertShaderStageMask(RHI::ShaderStageMask mask)
        {
            // If more than one stage is in the mask, we return the visibility for all stages
            // since D3D12_SHADER_VISIBILITY is NOT a mask, but an enum per stage.
            if (RHI::CountBitsSet(static_cast<uint64_t>(mask)) > 1)
            {
                return D3D12_SHADER_VISIBILITY_ALL;
            }

            switch (mask)
            {
            case RHI::ShaderStageMask::None:
                // [GFX_TODO][ATOM-1696]The resource is unused. Not sure which stage to set here.
                return D3D12_SHADER_VISIBILITY_ALL;
            case RHI::ShaderStageMask::Vertex:
                return D3D12_SHADER_VISIBILITY_VERTEX;
            case RHI::ShaderStageMask::Geometry:
                return D3D12_SHADER_VISIBILITY_GEOMETRY;
            case RHI::ShaderStageMask::Fragment:
                return D3D12_SHADER_VISIBILITY_PIXEL;
            case RHI::ShaderStageMask::Compute:
                // Compute always uses D3D12_SHADER_VISIBILITY_ALL (since there is only one active stage)
                return D3D12_SHADER_VISIBILITY_ALL;
            case RHI::ShaderStageMask::RayTracing:
                return D3D12_SHADER_VISIBILITY_ALL;
            default:
                AZ_Assert(false, "Invalid shader stage mask %d", static_cast<uint32_t>(mask));
                return D3D12_SHADER_VISIBILITY_ALL;
            }
        }

        D3D12_BLEND ConvertBlendFactor(RHI::BlendFactor factor)
        {
            static const D3D12_BLEND table[] =
            {
                D3D12_BLEND_ZERO,
                D3D12_BLEND_ONE,
                D3D12_BLEND_SRC_COLOR,
                D3D12_BLEND_INV_SRC_COLOR,
                D3D12_BLEND_SRC_ALPHA,
                D3D12_BLEND_INV_SRC_ALPHA,
                D3D12_BLEND_DEST_ALPHA,
                D3D12_BLEND_INV_DEST_ALPHA,
                D3D12_BLEND_DEST_COLOR,
                D3D12_BLEND_INV_DEST_COLOR,
                D3D12_BLEND_SRC_ALPHA_SAT,
                D3D12_BLEND_BLEND_FACTOR,
                D3D12_BLEND_INV_BLEND_FACTOR,
                D3D12_BLEND_SRC1_COLOR,
                D3D12_BLEND_INV_SRC1_COLOR,
                D3D12_BLEND_SRC1_ALPHA,
                D3D12_BLEND_INV_SRC1_ALPHA
            };

            return table[(uint32_t)factor];
        }

        D3D12_BLEND_OP ConvertBlendOp(RHI::BlendOp op)
        {
            static const D3D12_BLEND_OP table[] =
            {
                D3D12_BLEND_OP_ADD,
                D3D12_BLEND_OP_SUBTRACT,
                D3D12_BLEND_OP_REV_SUBTRACT,
                D3D12_BLEND_OP_MIN,
                D3D12_BLEND_OP_MAX
            };

            return table[(uint32_t)op];
        }

        D3D12_BLEND_DESC ConvertBlendState(const RHI::BlendState& blend)
        {
            D3D12_BLEND_DESC desc;
            desc.AlphaToCoverageEnable = blend.m_alphaToCoverageEnable;
            desc.IndependentBlendEnable = blend.m_independentBlendEnable;

            for (uint32_t i = 0; i < 8; ++i)
            {
                const RHI::TargetBlendState& src = blend.m_targets[i];
                D3D12_RENDER_TARGET_BLEND_DESC& dst = desc.RenderTarget[i];

                dst.BlendEnable = src.m_enable;
                dst.BlendOp = ConvertBlendOp(src.m_blendOp);
                dst.BlendOpAlpha = ConvertBlendOp(src.m_blendAlphaOp);
                dst.DestBlend = ConvertBlendFactor(src.m_blendDest);
                dst.DestBlendAlpha = ConvertBlendFactor(src.m_blendAlphaDest);
                dst.RenderTargetWriteMask = ConvertColorWriteMask(static_cast<uint8_t>(src.m_writeMask));
                dst.SrcBlend = ConvertBlendFactor(src.m_blendSource);
                dst.SrcBlendAlpha = ConvertBlendFactor(src.m_blendAlphaSource);
                dst.LogicOp = D3D12_LOGIC_OP_CLEAR;
                dst.LogicOpEnable = false;
            }
            return desc;
        }

        D3D12_FILL_MODE ConvertFillMode(RHI::FillMode mode)
        {
            static const D3D12_FILL_MODE table[] =
            {
                D3D12_FILL_MODE_SOLID,
                D3D12_FILL_MODE_WIREFRAME
            };
            return table[(uint32_t)mode];
        }

        D3D12_CULL_MODE ConvertCullMode(RHI::CullMode mode)
        {
            static const D3D12_CULL_MODE table[] =
            {
                D3D12_CULL_MODE_NONE,
                D3D12_CULL_MODE_FRONT,
                D3D12_CULL_MODE_BACK
            };
            return table[(uint32_t)mode];
        }

        D3D12_RASTERIZER_DESC ConvertRasterState(const RHI::RasterState& raster)
        {
            D3D12_RASTERIZER_DESC desc = {};
            desc.FrontCounterClockwise = true;
            desc.CullMode = ConvertCullMode(raster.m_cullMode);
            desc.DepthBias = raster.m_depthBias;
            desc.DepthBiasClamp = raster.m_depthBiasClamp;
            desc.SlopeScaledDepthBias = raster.m_depthBiasSlopeScale;
            desc.DepthClipEnable = raster.m_depthClipEnable;
            desc.FillMode = ConvertFillMode(raster.m_fillMode);
            desc.MultisampleEnable = raster.m_multisampleEnable;
            desc.ConservativeRaster = raster.m_conservativeRasterEnable ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
            desc.ForcedSampleCount = raster.m_forcedSampleCount;
            return desc;
        }

        D3D12_COMPARISON_FUNC ConvertComparisonFunc(RHI::ComparisonFunc func)
        {
            static const D3D12_COMPARISON_FUNC table[] =
            {
                D3D12_COMPARISON_FUNC_NEVER,
                D3D12_COMPARISON_FUNC_LESS,
                D3D12_COMPARISON_FUNC_EQUAL,
                D3D12_COMPARISON_FUNC_LESS_EQUAL,
                D3D12_COMPARISON_FUNC_GREATER,
                D3D12_COMPARISON_FUNC_NOT_EQUAL,
                D3D12_COMPARISON_FUNC_GREATER_EQUAL,
                D3D12_COMPARISON_FUNC_ALWAYS
            };
            return table[(uint32_t)func];
        }

        D3D12_STENCIL_OP ConvertStencilOp(RHI::StencilOp op)
        {
            static const D3D12_STENCIL_OP table[] =
            {
                D3D12_STENCIL_OP_KEEP,
                D3D12_STENCIL_OP_ZERO,
                D3D12_STENCIL_OP_REPLACE,
                D3D12_STENCIL_OP_INCR_SAT,
                D3D12_STENCIL_OP_DECR_SAT,
                D3D12_STENCIL_OP_INVERT,
                D3D12_STENCIL_OP_INCR,
                D3D12_STENCIL_OP_DECR
            };
            return table[(uint32_t)op];
        }

        D3D12_DEPTH_WRITE_MASK ConvertDepthWriteMask(RHI::DepthWriteMask mask)
        {
            static const D3D12_DEPTH_WRITE_MASK table[] =
            {
                D3D12_DEPTH_WRITE_MASK_ZERO,
                D3D12_DEPTH_WRITE_MASK_ALL
            };
            return table[(uint32_t)mask];
        }
    
        uint8_t ConvertColorWriteMask(uint8_t writeMask)
        {            
            uint8_t dflags = 0;
            if(writeMask == 0)
            {
                return dflags;
            }
            
            if(RHI::CheckBitsAll(writeMask, static_cast<uint8_t>(RHI::WriteChannelMask::ColorWriteMaskAll)))
            {
                return D3D12_COLOR_WRITE_ENABLE_ALL;
            }
            
            if (RHI::CheckBitsAny(writeMask, static_cast<uint8_t>(RHI::WriteChannelMask::ColorWriteMaskRed)))
            {
                dflags |= D3D12_COLOR_WRITE_ENABLE_RED;
            }
            if (RHI::CheckBitsAny(writeMask, static_cast<uint8_t>(RHI::WriteChannelMask::ColorWriteMaskGreen)))
            {
                dflags |= D3D12_COLOR_WRITE_ENABLE_GREEN;
            }
            if (RHI::CheckBitsAny(writeMask, static_cast<uint8_t>(RHI::WriteChannelMask::ColorWriteMaskBlue)))
            {
                dflags |= D3D12_COLOR_WRITE_ENABLE_BLUE;
            }
            if (RHI::CheckBitsAny(writeMask, static_cast<uint8_t>(RHI::WriteChannelMask::ColorWriteMaskAlpha)))
            {
                dflags |= D3D12_COLOR_WRITE_ENABLE_ALPHA;
            }
            return dflags;
        }

        D3D12_SHADING_RATE ConvertShadingRateEnum(RHI::ShadingRate rate)
        {
            static const D3D12_SHADING_RATE table[] =
            {
                D3D12_SHADING_RATE_1X1,
                D3D12_SHADING_RATE_1X2,
                D3D12_SHADING_RATE_2X1,
                D3D12_SHADING_RATE_2X2,
                D3D12_SHADING_RATE_2X4,
                D3D12_SHADING_RATE_4X2,
                D3D12_SHADING_RATE_4X4
            };
            return table[(uint32_t)rate];
        }

        D3D12_SHADING_RATE_COMBINER ConvertShadingRateCombiner(RHI::ShadingRateCombinerOp op)
        {
            switch (op)
            {
            case RHI::ShadingRateCombinerOp::Passthrough:
                return D3D12_SHADING_RATE_COMBINER_PASSTHROUGH;
            case RHI::ShadingRateCombinerOp::Override:
                return D3D12_SHADING_RATE_COMBINER_OVERRIDE;
            case RHI::ShadingRateCombinerOp::Min:
                return D3D12_SHADING_RATE_COMBINER_MIN;
            case RHI::ShadingRateCombinerOp::Max:
                return D3D12_SHADING_RATE_COMBINER_MAX;
            default:
                AZ_Assert(false, "Invalid shading rate combiner operation %d", op);
                return D3D12_SHADING_RATE_COMBINER_PASSTHROUGH;
            }
        }

        D3D12_DEPTH_STENCIL_DESC ConvertDepthStencilState(const RHI::DepthStencilState& depthStencil)
        {
            D3D12_DEPTH_STENCIL_DESC desc;
            desc.BackFace.StencilDepthFailOp = ConvertStencilOp(depthStencil.m_stencil.m_backFace.m_depthFailOp);
            desc.BackFace.StencilFailOp = ConvertStencilOp(depthStencil.m_stencil.m_backFace.m_failOp);
            desc.BackFace.StencilPassOp = ConvertStencilOp(depthStencil.m_stencil.m_backFace.m_passOp);
            desc.BackFace.StencilFunc = ConvertComparisonFunc(depthStencil.m_stencil.m_backFace.m_func);
            desc.FrontFace.StencilDepthFailOp = ConvertStencilOp(depthStencil.m_stencil.m_frontFace.m_depthFailOp);
            desc.FrontFace.StencilFailOp = ConvertStencilOp(depthStencil.m_stencil.m_frontFace.m_failOp);
            desc.FrontFace.StencilPassOp = ConvertStencilOp(depthStencil.m_stencil.m_frontFace.m_passOp);
            desc.FrontFace.StencilFunc = ConvertComparisonFunc(depthStencil.m_stencil.m_frontFace.m_func);
            desc.DepthEnable = depthStencil.m_depth.m_enable;
            desc.DepthFunc = ConvertComparisonFunc(depthStencil.m_depth.m_func);
            desc.DepthWriteMask = ConvertDepthWriteMask(depthStencil.m_depth.m_writeMask);
            desc.StencilEnable = depthStencil.m_stencil.m_enable;
            desc.StencilReadMask = static_cast<UINT8>(depthStencil.m_stencil.m_readMask);
            desc.StencilWriteMask = static_cast<UINT8>(depthStencil.m_stencil.m_writeMask);
            return desc;
        }

        RHI::ResultCode ConvertResult(HRESULT result)
        {
            switch(result)
            {
                case S_OK:
                case S_FALSE:
                    return RHI::ResultCode::Success;
                case E_OUTOFMEMORY:
                    return RHI::ResultCode::OutOfMemory;
                case E_INVALIDARG:
                    return RHI::ResultCode::InvalidArgument;
                case DXGI_ERROR_INVALID_CALL:
                case E_NOTIMPL:
                    return RHI::ResultCode::InvalidOperation;
                default:
                    return RHI::ResultCode::Fail;
            }
        }

        DXGI_SCALING ConvertScaling(RHI::Scaling scaling)
        {
            switch(scaling)
            {
                case RHI::Scaling::None:
                    return DXGI_SCALING_NONE;
                case RHI::Scaling::Stretch:
                    return DXGI_SCALING_STRETCH;
                case RHI::Scaling::AspectRatioStretch:
                    return DXGI_SCALING_ASPECT_RATIO_STRETCH;
                default:
                    return DXGI_SCALING_STRETCH;
            }
        }
    }
}
