/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <RHI/Device.h>
#include <RHI/PhysicalDevice.h>
#include <RHI/Conversions.h>
#include <RHI/DescriptorContext.h>
#include <RHI/Fence.h>
#include <Atom/RHI/MemoryStatisticsBuilder.h>
#include <Atom/RHI.Reflect/DX12/PlatformLimitsDescriptor.h>
#include <AzCore/Debug/EventTrace.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ
{
    namespace DX12
    {
        namespace Platform
        {
            void DeviceShutdownInternal(ID3D12DeviceX* device);
            void DeviceCompileMemoryStatisticsInternal(RHI::MemoryStatisticsBuilder& builder, IDXGIAdapterX* dxgiAdapter);
        }

        Device::Device()
        {
            RHI::Ptr<PlatformLimitsDescriptor> platformLimitsDescriptor = aznew PlatformLimitsDescriptor();
            platformLimitsDescriptor->LoadPlatformLimitsDescriptor(RHI::Factory::Get().GetName().GetCStr());
            m_descriptor.m_platformLimitsDescriptor = RHI::Ptr<RHI::PlatformLimitsDescriptor>(platformLimitsDescriptor);
        }

        RHI::Ptr<Device> Device::Create()
        {
            return aznew Device();
        }

        RHI::ResultCode Device::InitInternal(RHI::PhysicalDevice& physicalDevice)
        {
            RHI::ResultCode resultCode = InitSubPlatform(physicalDevice);
            if (resultCode != RHI::ResultCode::Success)
            {
                return resultCode;
            }

            InitFeatures();

            return RHI::ResultCode::Success;
        }

        RHI::ResultCode Device::InitializeLimits()
        {
            m_allocationInfoCache.SetInitFunction([](auto& cache) { cache.set_capacity(64); });

            {
                ReleaseQueue::Descriptor releaseQueueDescriptor;
                releaseQueueDescriptor.m_collectLatency = m_descriptor.m_frameCountMax - 1;
                m_releaseQueue.Init(releaseQueueDescriptor);
            }

            m_descriptorContext = AZStd::make_shared<DescriptorContext>();

            RHI::ConstPtr<RHI::PlatformLimitsDescriptor> rhiDescriptor = m_descriptor.m_platformLimitsDescriptor;
            RHI::ConstPtr<PlatformLimitsDescriptor> platLimitsDesc = azrtti_cast<const PlatformLimitsDescriptor*>(rhiDescriptor);
            AZ_Assert(platLimitsDesc != nullptr, "Missing PlatformLimits config file for DX12 backend");
            m_descriptorContext->Init(m_dx12Device.get(), platLimitsDesc);

            {
                CommandListAllocator::Descriptor commandListAllocatorDescriptor;
                commandListAllocatorDescriptor.m_device = this;
                commandListAllocatorDescriptor.m_frameCountMax = m_descriptor.m_frameCountMax;
                commandListAllocatorDescriptor.m_descriptorContext = m_descriptorContext;
                m_commandListAllocator.Init(commandListAllocatorDescriptor);
            }

            {
                StagingMemoryAllocator::Descriptor allocatorDesc;
                allocatorDesc.m_device = this;

                allocatorDesc.m_mediumPageSizeInBytes = static_cast<uint32_t>(platLimitsDesc->m_platformDefaultValues.m_mediumStagingBufferPageSizeInBytes);
                allocatorDesc.m_largePageSizeInBytes = static_cast<uint32_t>(platLimitsDesc->m_platformDefaultValues.m_largestStagingBufferPageSizeInBytes);
                allocatorDesc.m_collectLatency = m_descriptor.m_frameCountMax;
                m_stagingMemoryAllocator.Init(allocatorDesc);
            }

            m_pipelineLayoutCache.Init(*this);

            m_commandQueueContext.Init(*this);

            m_asyncUploadQueue.Init(*this, AsyncUploadQueue::Descriptor(platLimitsDesc->m_platformDefaultValues.m_asyncQueueStagingBufferSizeInBytes));

            m_samplerCache.SetCapacity(SamplerCacheCapacity);

            return RHI::ResultCode::Success;
        }

        void Device::PreShutdown()
        {
            // Any containers that maintain references to DeviceObjects need to be cleared here to ensure the device
            // refcount reaches 0 before shutdown.

            m_samplerCache.Clear();
            m_commandListAllocator.Shutdown();
            m_asyncUploadQueue.Shutdown();
            m_commandQueueContext.Shutdown();
        }

        void Device::ShutdownInternal()
        {
            

            m_allocationInfoCache.Clear();

            m_stagingMemoryAllocator.Shutdown();

            m_pipelineLayoutCache.Shutdown();

            m_descriptorContext = nullptr;

            m_releaseQueue.Shutdown();

            m_dxgiFactory = nullptr;
            m_dxgiAdapter = nullptr;

            Platform::DeviceShutdownInternal(m_dx12Device.get());

            m_dx12Device = nullptr;
        }

        void Device::InitFeatures()
        {
            m_features.m_tessellationShader = true;
            m_features.m_geometryShader = true;
            m_features.m_computeShader = true;
            m_features.m_independentBlend = true;
            m_features.m_dualSourceBlending = true;
            m_features.m_customResolvePositions = true;
            m_features.m_queryTypesMask[static_cast<uint32_t>(RHI::HardwareQueueClass::Graphics)] = RHI::QueryTypeFlags::All;
            m_features.m_queryTypesMask[static_cast<uint32_t>(RHI::HardwareQueueClass::Compute)] = RHI::QueryTypeFlags::PipelineStatistics | RHI::QueryTypeFlags::Timestamp;
            D3D12_FEATURE_DATA_D3D12_OPTIONS3 options3;
            GetDevice()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &options3, sizeof(options3));
            if (options3.CopyQueueTimestampQueriesSupported)
            {
                m_features.m_queryTypesMask[static_cast<uint32_t>(RHI::HardwareQueueClass::Copy)] = RHI::QueryTypeFlags::Timestamp;
            }            
            m_features.m_predication = true;
            m_features.m_occlusionQueryPrecise = true;
            m_features.m_indirectCommandTier = RHI::IndirectCommandTiers::Tier2;
            m_features.m_indirectDrawCountBufferSupported = true;
            m_features.m_indirectDispatchCountBufferSupported = true;
            m_features.m_indirectDrawStartInstanceLocationSupported = true;

#ifdef AZ_DX12_DXR_SUPPORT
            D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5;
            GetDevice()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5));
            m_features.m_rayTracing = options5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
#else
            m_features.m_rayTracing = false;
#endif

            m_features.m_unboundedArrays = true;

            m_limits.m_maxImageDimension1D = D3D12_REQ_TEXTURE1D_U_DIMENSION;
            m_limits.m_maxImageDimension2D = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
            m_limits.m_maxImageDimension3D = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
            m_limits.m_maxImageDimensionCube = D3D12_REQ_TEXTURECUBE_DIMENSION;
            m_limits.m_maxImageArraySize = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
            m_limits.m_minConstantBufferViewOffset = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
            m_limits.m_maxIndirectDrawCount = static_cast<uint32_t>(-1);
            m_limits.m_maxIndirectDispatchCount = static_cast<uint32_t>(-1);
        }

        void Device::CompileMemoryStatisticsInternal(RHI::MemoryStatisticsBuilder& builder)
        {
            Platform::DeviceCompileMemoryStatisticsInternal(builder, m_dxgiAdapter.get());

            m_stagingMemoryAllocator.ReportMemoryUsage(builder);
        }

        void Device::UpdateCpuTimingStatisticsInternal() const
        {
            m_commandQueueContext.UpdateCpuTimingStatistics();
        }

        void Device::EndFrameInternal()
        {
            AZ_TRACE_METHOD();
            m_commandQueueContext.End();

            m_commandListAllocator.Collect();

            m_descriptorContext->GarbageCollect();

            m_stagingMemoryAllocator.GarbageCollect();

            m_releaseQueue.Collect();
        }

        void Device::WaitForIdleInternal()
        {
            m_commandQueueContext.WaitForIdle();
            m_releaseQueue.Collect(true);
        }

        AZStd::chrono::microseconds Device::GpuTimestampToMicroseconds(uint64_t gpuTimestamp, RHI::HardwareQueueClass queueClass) const
        {
            auto durationInSeconds = AZStd::chrono::duration<double>(double(gpuTimestamp) / m_commandQueueContext.GetCommandQueue(queueClass).GetGpuTimestampFrequency());
            return AZStd::chrono::microseconds(durationInSeconds);
        }

        void Device::FillFormatsCapabilitiesInternal(FormatCapabilitiesList& formatsCapabilities)
        {
            for (uint32_t i = 0; i < formatsCapabilities.size(); ++i)
            {
                RHI::FormatCapabilities& flags = formatsCapabilities[i];
                D3D12_FEATURE_DATA_FORMAT_SUPPORT support{};
                support.Format = ConvertFormat(static_cast<RHI::Format>(i), false);
                GetDevice()->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &support, sizeof(support));
                flags = RHI::FormatCapabilities::None;

                if (RHI::CheckBitsAll(support.Support1, D3D12_FORMAT_SUPPORT1_IA_VERTEX_BUFFER))
                {
                    flags |= RHI::FormatCapabilities::VertexBuffer;
                }

                if (RHI::CheckBitsAll(support.Support1, D3D12_FORMAT_SUPPORT1_IA_INDEX_BUFFER))
                {
                    flags |= RHI::FormatCapabilities::IndexBuffer;
                }

                if (RHI::CheckBitsAll(support.Support1, D3D12_FORMAT_SUPPORT1_RENDER_TARGET))
                {
                    flags |= RHI::FormatCapabilities::RenderTarget;
                }

                if (RHI::CheckBitsAll(support.Support1, D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL))
                {
                    flags |= RHI::FormatCapabilities::DepthStencil;
                }

                if (RHI::CheckBitsAll(support.Support1, D3D12_FORMAT_SUPPORT1_BLENDABLE))
                {
                    flags |= RHI::FormatCapabilities::Blend;
                }

                if (RHI::CheckBitsAll(support.Support1, D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE))
                {
                    flags |= RHI::FormatCapabilities::Sample;
                }

                if (RHI::CheckBitsAll(support.Support2, D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD))
                {
                    flags |= RHI::FormatCapabilities::TypedLoadBuffer;
                }

                if (RHI::CheckBitsAll(support.Support2, D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE))
                {
                    flags |= RHI::FormatCapabilities::TypedStoreBuffer;
                }

                if (RHI::CheckBitsAll(support.Support2, D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_ADD))
                {
                    flags |= RHI::FormatCapabilities::AtomicBuffer;
                }
            }
        }

        RHI::ResourceMemoryRequirements Device::GetResourceMemoryRequirements(const RHI::ImageDescriptor& descriptor)
        {
            D3D12_RESOURCE_ALLOCATION_INFO allocationInfo;
            GetImageAllocationInfo(descriptor, allocationInfo);

            RHI::ResourceMemoryRequirements memoryRequirements;
            memoryRequirements.m_alignmentInBytes = allocationInfo.Alignment;
            memoryRequirements.m_sizeInBytes = allocationInfo.SizeInBytes;
            return memoryRequirements;
        }

        RHI::ResourceMemoryRequirements Device::GetResourceMemoryRequirements(const RHI::BufferDescriptor& descriptor)
        {
            RHI::ResourceMemoryRequirements memoryRequirements;
            memoryRequirements.m_alignmentInBytes = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
            memoryRequirements.m_sizeInBytes = RHI::AlignUp<size_t>(descriptor.m_byteCount, memoryRequirements.m_alignmentInBytes);
            return memoryRequirements;
        }

        void Device::ObjectCollectionNotify(RHI::ObjectCollectorNotifyFunction notifyFunction)
        {
            m_releaseQueue.Notify(notifyFunction);
        }

        //AZStd::vector<RHI::Format> Device::GetValidSwapChainImageFormats(const RHI::WindowHandle& windowHandle) const
        //{
        //    AZStd::vector<RHI::Format> formatsList;

        //    // Follows Microsoft's HDR sample code for determining if the connected display supports HDR.
        //    // Enumerates all of the detected displays and determines which one has the largest intersection with the 
        //    // region of the window handle parameter.
        //    // If the display for this region supports wide color gamut, then a wide color gamut format is added to
        //    // the list of supported formats.
        //    // https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/UWP/D3D12HDR/src/D3D12HDR.cpp

        //    HWND hWnd = reinterpret_cast<HWND>(windowHandle.GetIndex());
        //    RECT windowRect = {};
        //    GetWindowRect(hWnd, &windowRect);

        //    UINT outputIndex = 0;
        //    Microsoft::WRL::ComPtr<IDXGIOutput> bestOutput;
        //    Microsoft::WRL::ComPtr<IDXGIOutput> currentOutput;
        //    RECT intersectRect;
        //    int bestIntersectionArea = -1;
        //    while (m_dxgiAdapter->EnumOutputs(outputIndex, &currentOutput) != DXGI_ERROR_NOT_FOUND)
        //    {
        //        // Get the rectangle bounds of current output
        //        DXGI_OUTPUT_DESC outputDesc;
        //        currentOutput->GetDesc(&outputDesc);
        //        RECT outputRect = outputDesc.DesktopCoordinates;
        //        int intersectionArea = 0;
        //        if (IntersectRect(&intersectRect, &windowRect, &outputRect))
        //        {
        //            intersectionArea = (intersectRect.bottom - intersectRect.top) * (intersectRect.right - intersectRect.left);
        //        }
        //        if (intersectionArea > bestIntersectionArea)
        //        {
        //            bestOutput = currentOutput;
        //            bestIntersectionArea = intersectionArea;
        //        }

        //        outputIndex++;
        //    }

        //    Microsoft::WRL::ComPtr<IDXGIOutput6> output6;
        //    HRESULT hr = bestOutput.As(&output6);
        //    AZ_Assert(S_OK == hr, "Failed to get IDXGIOutput6 structure.");
        //    DXGI_OUTPUT_DESC1 outputDesc;
        //    output6->GetDesc1(&outputDesc);
        //    if (outputDesc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)
        //    {
        //        // HDR is supported
        //        formatsList.push_back(RHI::Format::R10G10B10A2_UNORM);
        //    }

        //    // Fallback default 8-bit format
        //    formatsList.push_back(RHI::Format::R8G8B8A8_UNORM);

        //    return formatsList;
        //}

        MemoryView Device::CreateImageCommitted(
            const RHI::ImageDescriptor& imageDescriptor,
            const RHI::ClearValue* optimizedClearValue,
            D3D12_RESOURCE_STATES initialState,
            D3D12_HEAP_TYPE heapType)
        {
            AZ_TRACE_METHOD();

            D3D12_RESOURCE_DESC resourceDesc;
            ConvertImageDescriptor(imageDescriptor, resourceDesc);
            CD3DX12_HEAP_PROPERTIES heapProperties(heapType);

            // Clear values only apply when the image is a render target or depth stencil.
            const bool isOutputMergerAttachment =
                RHI::CheckBitsAny(imageDescriptor.m_bindFlags, RHI::ImageBindFlags::Color | RHI::ImageBindFlags::DepthStencil);

            D3D12_CLEAR_VALUE clearValue;
            if (isOutputMergerAttachment && optimizedClearValue)
            {
                clearValue = ConvertClearValue(imageDescriptor.m_format, *optimizedClearValue);
            }

            Microsoft::WRL::ComPtr<ID3D12Resource> resource;
            AssertSuccess(m_dx12Device->CreateCommittedResource(
                &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, initialState, (isOutputMergerAttachment && optimizedClearValue) ? &clearValue : nullptr, IID_GRAPHICS_PPV_ARGS(resource.GetAddressOf())));

            D3D12_RESOURCE_ALLOCATION_INFO allocationInfo;
            GetImageAllocationInfo(imageDescriptor, allocationInfo);

            return MemoryView(resource.Get(), 0, allocationInfo.SizeInBytes, allocationInfo.Alignment, MemoryViewType::Image);
        }

        MemoryView Device::CreateBufferCommitted(
            const RHI::BufferDescriptor& bufferDescriptor,
            D3D12_RESOURCE_STATES initialState,
            D3D12_HEAP_TYPE heapType)
        {
            D3D12_RESOURCE_DESC resourceDesc;
            ConvertBufferDescriptor(bufferDescriptor, resourceDesc);
            CD3DX12_HEAP_PROPERTIES heapProperties(heapType);

#ifdef AZ_DX12_DXR_SUPPORT
            if (initialState == D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE)
            {
                resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            }
#endif

            Microsoft::WRL::ComPtr<ID3D12Resource> resource;
            AssertSuccess(m_dx12Device->CreateCommittedResource(
                &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, initialState, nullptr, IID_GRAPHICS_PPV_ARGS(resource.GetAddressOf())));

            D3D12_RESOURCE_ALLOCATION_INFO allocationInfo;
            allocationInfo.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
            allocationInfo.SizeInBytes = RHI::AlignUp(resourceDesc.Width, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

            return MemoryView(resource.Get(), 0, allocationInfo.SizeInBytes, allocationInfo.Alignment, MemoryViewType::Buffer);
        }

        MemoryView Device::CreateBufferPlaced(
            const RHI::BufferDescriptor& bufferDescriptor,
            D3D12_RESOURCE_STATES initialState,
            ID3D12Heap* heap,
            size_t heapByteOffset)
        {
            D3D12_RESOURCE_DESC resourceDesc;
            ConvertBufferDescriptor(bufferDescriptor, resourceDesc);

            D3D12_RESOURCE_ALLOCATION_INFO allocationInfo;
            allocationInfo.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
            allocationInfo.SizeInBytes = RHI::AlignUp(resourceDesc.Width, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

            Microsoft::WRL::ComPtr<ID3D12Resource> resource;
            AssertSuccess(m_dx12Device->CreatePlacedResource(
                heap, heapByteOffset, &resourceDesc, initialState, nullptr, IID_GRAPHICS_PPV_ARGS(resource.GetAddressOf())));

            return MemoryView(resource.Get(), 0, allocationInfo.SizeInBytes, allocationInfo.Alignment, MemoryViewType::Buffer);
        }

        static uint64_t GetPlacedTextureAlignment(const RHI::ImageDescriptor& imageDescriptor)
        {
            return (imageDescriptor.m_multisampleState.m_samples > 1)
                ? D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT
                : D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        }

        MemoryView Device::CreateImagePlaced(
            const RHI::ImageDescriptor& imageDescriptor,
            const RHI::ClearValue* optimizedClearValue,
            D3D12_RESOURCE_STATES initialState,
            ID3D12Heap* heap,
            size_t heapByteOffset)
        {
            D3D12_RESOURCE_DESC resourceDesc;
            ConvertImageDescriptor(imageDescriptor, resourceDesc);

            D3D12_RESOURCE_ALLOCATION_INFO allocationInfo{};
            GetPlacedImageAllocationInfo(imageDescriptor, allocationInfo);

            allocationInfo.Alignment = GetPlacedTextureAlignment(imageDescriptor);
            if (resourceDesc.Alignment == 0)
            {
                resourceDesc.Alignment = allocationInfo.Alignment;
            }

            // Clear values only apply when the image is a render target or depth stencil.
            const bool isOutputMergerAttachment =
                RHI::CheckBitsAny(imageDescriptor.m_bindFlags, RHI::ImageBindFlags::Color | RHI::ImageBindFlags::DepthStencil);

            D3D12_CLEAR_VALUE clearValue;
            if (isOutputMergerAttachment && optimizedClearValue)
            {
                clearValue = ConvertClearValue(imageDescriptor.m_format, *optimizedClearValue);

                if (RHI::CheckBitsAny(imageDescriptor.m_bindFlags, RHI::ImageBindFlags::DepthStencil))
                {
                    clearValue.Format = GetDSVFormat(clearValue.Format);
                }
            }

            Microsoft::WRL::ComPtr<ID3D12Resource> resource;
            AssertSuccess(m_dx12Device->CreatePlacedResource(
                heap,heapByteOffset, &resourceDesc,initialState, (isOutputMergerAttachment && optimizedClearValue) ? &clearValue : nullptr, IID_GRAPHICS_PPV_ARGS(resource.GetAddressOf())));

            return MemoryView(resource.Get(), 0, allocationInfo.SizeInBytes, allocationInfo.Alignment, MemoryViewType::Image);
        }

        MemoryView Device::CreateImageReserved(
            const RHI::ImageDescriptor& imageDescriptor,
            D3D12_RESOURCE_STATES initialState,
            ImageTileLayout& imageTileLayout)
        {
            D3D12_RESOURCE_DESC resourceDesc;
            ConvertImageDescriptor(imageDescriptor, resourceDesc);
            resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;

            AZ_Assert(
                RHI::CheckBitsAny(imageDescriptor.m_bindFlags, RHI::ImageBindFlags::Color | RHI::ImageBindFlags::DepthStencil) == false,
                "Reserved resources are not supported for color / depth stencil images.");

            Microsoft::WRL::ComPtr<ID3D12Resource> resource;
            AssertSuccess(m_dx12Device->CreateReservedResource(
                &resourceDesc, initialState, nullptr, IID_GRAPHICS_PPV_ARGS(resource.GetAddressOf())));

            uint32_t subresourceCount = resourceDesc.MipLevels * resourceDesc.DepthOrArraySize;
            imageTileLayout.m_subresourceTiling.resize(subresourceCount);

            uint32_t tileCount = 0;
            D3D12_TILE_SHAPE tileShape;
            D3D12_PACKED_MIP_INFO packedMipInfo;

            m_dx12Device->GetResourceTiling(
                resource.Get(), &tileCount, &packedMipInfo, &tileShape, &subresourceCount, 0, imageTileLayout.m_subresourceTiling.data());

            imageTileLayout.m_tileSize = RHI::Size(tileShape.WidthInTexels, tileShape.HeightInTexels, tileShape.DepthInTexels);
            imageTileLayout.m_tileCount = tileCount;
            imageTileLayout.m_tileCountPacked = packedMipInfo.NumTilesForPackedMips;
            imageTileLayout.m_tileCountStandard = tileCount - imageTileLayout.m_tileCountPacked;
            imageTileLayout.m_mipCount = packedMipInfo.NumStandardMips + packedMipInfo.NumPackedMips;
            imageTileLayout.m_mipCountStandard = packedMipInfo.NumStandardMips;
            imageTileLayout.m_mipCountPacked = packedMipInfo.NumPackedMips;

            D3D12_RESOURCE_ALLOCATION_INFO allocationInfo;
            GetImageAllocationInfo(imageDescriptor, allocationInfo);

            return MemoryView(resource.Get(), 0, allocationInfo.SizeInBytes, allocationInfo.Alignment, MemoryViewType::Image);
        }

        void Device::GetImageAllocationInfo(
            const RHI::ImageDescriptor& descriptor,
            D3D12_RESOURCE_ALLOCATION_INFO& info)
        {
            auto& allocationInfoCache = m_allocationInfoCache.GetStorage();

            const uint64_t hash = static_cast<uint64_t>(descriptor.GetHash());
            auto it = allocationInfoCache.get(hash);
            if (it != allocationInfoCache.end())
            {
                info = it->second;
            }
            else
            {
                D3D12_RESOURCE_DESC resourceDesc;
                ConvertImageDescriptor(descriptor, resourceDesc);
                info = m_dx12Device->GetResourceAllocationInfo(0, 1, &resourceDesc);
                AZ_Assert(info.SizeInBytes != uint64_t(-1), "Device::GetImageAllocationInfo - DX12 failed to get allocation info for the provided resource description.");
                allocationInfoCache.emplace(hash, info);
            }
        }

        void Device::GetPlacedImageAllocationInfo(
            const RHI::ImageDescriptor& descriptor,
            D3D12_RESOURCE_ALLOCATION_INFO& info)
        {
            GetImageAllocationInfo(descriptor, info);
            info.Alignment = AZStd::max<uint64_t>(info.Alignment, GetPlacedTextureAlignment(descriptor));
        }

        void Device::QueueForRelease(RHI::Ptr<ID3D12Object> dx12Object)
        {
            m_releaseQueue.QueueForCollect(AZStd::move(dx12Object));
        }

        void Device::QueueForRelease(const MemoryView& memoryView)
        {
            m_releaseQueue.QueueForCollect(memoryView.GetMemory());
        }

        MemoryView Device::AcquireStagingMemory(size_t size, size_t alignment)
        {
            return m_stagingMemoryAllocator.Allocate(size, alignment);
        }

        CommandList* Device::AcquireCommandList(RHI::HardwareQueueClass hardwareQueueClass)
        {
            return m_commandListAllocator.Allocate(hardwareQueueClass);
        }

        RHI::ConstPtr<PipelineLayout> Device::AcquirePipelineLayout(const RHI::PipelineLayoutDescriptor& descriptor)
        {
            return m_pipelineLayoutCache.Allocate(descriptor);
        }

        ID3D12DeviceX* Device::GetDevice()
        {
            return m_dx12Device.get();
        }

        RHI::ConstPtr<Sampler> Device::AcquireSampler(const RHI::SamplerState& state)
        {
            auto hash = static_cast<uint64_t>(state.GetHash());
            AZStd::lock_guard<AZStd::mutex> lock(m_samplerCacheMutex);
            Sampler* sampler = m_samplerCache.Find(hash);
            if (!sampler)
            {
                RHI::Ptr<Sampler> samplerPtr = Sampler::Create();
                samplerPtr->Init(*this, state);
                m_samplerCache.Insert(hash, samplerPtr);
                sampler = samplerPtr.get();
            }
            return RHI::ConstPtr<Sampler>(sampler);
        }

        const PhysicalDevice& Device::GetPhysicalDevice() const
        {
            return static_cast<const PhysicalDevice&>(Base::GetPhysicalDevice());
        }

        MemoryPageAllocator& Device::GetConstantMemoryPageAllocator()
        {
            return m_stagingMemoryAllocator.GetMediumPageAllocator();
        }

        CommandQueueContext& Device::GetCommandQueueContext()
        {
            return m_commandQueueContext;
        }

        AsyncUploadQueue& Device::GetAsyncUploadQueue()
        {
            return m_asyncUploadQueue;
        }

        DescriptorContext& Device::GetDescriptorContext()
        {
            return *m_descriptorContext;
        }

        bool Device::IsAftermathInitialized() const
        {
            return m_isAftermathInitialized;
        }

        RHI::ResultCode Device::CompactSRGMemory()
        {
            if (m_isDescriptorHeapCompactionNeeded)
            {
                m_isDescriptorHeapCompactionNeeded = false;
                return m_descriptorContext->CompactDescriptorHeap();
            }
            return RHI::ResultCode::Success;
        }

        void Device::DescriptorHeapCompactionNeeded()
        {
            m_isDescriptorHeapCompactionNeeded = true;
        }
    }
}
