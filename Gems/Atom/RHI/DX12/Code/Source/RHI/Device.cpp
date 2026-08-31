/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/RHIMemoryStatisticsInterface.h>
#include <RHI/Device.h>
#include <RHI/PhysicalDevice.h>
#include <RHI/Conversions.h>
#include <RHI/DescriptorContext.h>
#include <RHI/Fence.h>
#include <Atom/RHI/MemoryStatisticsBuilder.h>
#include <Atom/RHI.Reflect/DX12/PlatformLimitsDescriptor.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Utils/TypeHash.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Memory/AllocatorInstance.h>

#include <Atom/RHI.Reflect/DX12/DX12Bus.h>

// ---- Phase-0 throwaway hardware mesh-shader spike (REMOVE after validation) ----
// Set to 1 to compile + run a one-shot D3D12 mesh-shader smoke test during device
// init: validates R1 (hand-rolled stream-subobject mesh PSO, since the vendored
// d3dx12.h has no stream helpers), R2 (DispatchMesh via a QI'd ID3D12GraphicsCommandList6),
// and no DEVICE_HUNG, by rendering a red fullscreen triangle to a 16x16 offscreen
// RTV and reading back the centre pixel. Output goes to the "MeshSpike" trace window.
//
// VALIDATED 2026-06-22 on AMD RDNA: all green (PSO create OK, DispatchMesh OK,
// no DEVICE_HUNG, centre pixel = red). Now DEACTIVATED but KEPT as the proven
// reference implementation for the Phase-2 real stream-PSO + DispatchMesh path.
// Uncomment the #define below to re-run the smoke test.
// #define O3DE_MESH_SHADER_SPIKE 1
#if defined(O3DE_MESH_SHADER_SPIKE)
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/containers/vector.h>
#endif

namespace AZ
{
    namespace DX12
    {
#ifdef USE_AMD_D3D12MA
        namespace
        {
            constexpr D3D12MA::ALLOCATOR_FLAGS s_D3d12maAllocatorFlags = static_cast<D3D12MA::ALLOCATOR_FLAGS>(D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED);
            D3D12MA::ALLOCATION_CALLBACKS s_AllocationCallbacks = {};

            // constant value attached to D3D12MA cpu memory allocations
            constexpr uintptr_t s_D3d12maAllocationPrivateData = 0x1200A110C;

            // utility functions to forward cpu mem allocations to o3de memory systems
            static void* D3d12maAllocate(size_t size, size_t alignment, [[maybe_unused]] void* privateData)
            {
                AZ_Assert(reinterpret_cast<uintptr_t>(privateData) == s_D3d12maAllocationPrivateData, "Incorrect private data value passed from D3D12MA during memory allocation");
                void* memory = AZ::AllocatorInstance<AZ::SystemAllocator>::Get().allocate(size, alignment);
                return memory;
            }

            static void D3d12maFree(void* memory, [[maybe_unused]] void* privateData)
            {
                AZ_Assert(reinterpret_cast<uintptr_t>(privateData) == s_D3d12maAllocationPrivateData, "Incorrect private data value passed from D3D12MA during memory deallocation");
                if(memory)
                {
                    AZ::AllocatorInstance<AZ::SystemAllocator>::Get().deallocate(memory);
                }
            }

            static void D3d12maRelease(D3D12MA::Allocation& allocation)
            {
                allocation.Release();
            }
        }
#endif
        namespace Platform
        {
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

#ifdef USE_AMD_D3D12MA
            resultCode = InitD3d12maAllocator();
            if (resultCode != RHI::ResultCode::Success)
            {
                return resultCode;
            }
#endif
            InitFeatures();

            return RHI::ResultCode::Success;
        }

        RHI::ResultCode Device::InitializeLimits()
        {
            m_allocationInfoCache.SetInitFunction([](auto& cache) { cache.set_capacity(64); });

            {
                ReleaseQueue::Descriptor releaseQueueDescriptor;
                releaseQueueDescriptor.m_collectLatency = m_descriptor.m_frameCountMax;
                m_releaseQueue.Init(releaseQueueDescriptor);

#ifdef USE_AMD_D3D12MA
                D3d12maReleaseQueue::Descriptor D3d12maReleaseQueueDescriptor;
                D3d12maReleaseQueueDescriptor.m_collectLatency = m_descriptor.m_frameCountMax;
                D3d12maReleaseQueueDescriptor.m_collectFunction = &D3d12maRelease;
                m_D3d12maReleaseQueue.Init(D3d12maReleaseQueueDescriptor);
#endif
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

            // First point at which both the native device and a real graphics queue exist, which is
            // what a GPU profiler needs to create its timestamp query heap and resolve command lists.
            // Null unless the Profiler gem is enabled and has already activated - this gem must not
            // depend on it, so there is nothing to fall back to if the ordering goes the other way.
            if (auto* gpuProfiler = AZ::Interface<AZ::Debug::GpuProfiler>::Get())
            {
                gpuProfiler->InitNativeDevice(
                    GetDevice(), m_commandQueueContext.GetCommandQueue(RHI::HardwareQueueClass::Graphics).GetPlatformQueue());
            }

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
#ifdef USE_AMD_D3D12MA
            m_D3d12maReleaseQueue.Shutdown();
            m_dx12MemAlloc = nullptr;
#endif
            m_dxgiFactory = nullptr;
            m_dxgiAdapter = nullptr;

            ShutdownSubPlatform();

            m_dx12Device = nullptr;
        }

#ifdef USE_AMD_D3D12MA
        RHI::ResultCode Device::InitD3d12maAllocator()
        {
            // Create D3d12ma allocator
            D3D12MA::ALLOCATOR_DESC desc = {};
            desc.Flags = s_D3d12maAllocatorFlags;
            desc.pDevice = m_dx12Device.get();
            desc.pAdapter = m_dxgiAdapter.get();

            s_AllocationCallbacks.pAllocate = &D3d12maAllocate;
            s_AllocationCallbacks.pFree = &D3d12maFree;
            s_AllocationCallbacks.pPrivateData = reinterpret_cast<void*>(s_D3d12maAllocationPrivateData);
            desc.pAllocationCallbacks = &s_AllocationCallbacks;

            D3D12MA::Allocator* dx12MemAlloc = nullptr;
            if (HRESULT result = D3D12MA::CreateAllocator(&desc, &dx12MemAlloc); !AssertSuccess(result))
            {
                AZ_Error("Device", false, "Failed to initialize the D3D12MemoryAllocator.");
                return ConvertResult(result);
            }
            m_dx12MemAlloc = dx12MemAlloc;
            return RHI::ResultCode::Success;
        }
#endif

#if defined(O3DE_MESH_SHADER_SPIKE)
        namespace
        {
            // One subobject in a D3D12 pipeline-state stream: a void*-aligned
            // { type-tag, payload } pair. Hand-rolled because the vendored d3dx12.h
            // has no CD3DX12_PIPELINE_STATE_STREAM helpers (plan risk R1).
            template<typename T, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type>
            struct alignas(void*) SpikeSubobject
            {
                D3D12_PIPELINE_STATE_SUBOBJECT_TYPE m_type = Type;
                T m_value{};
            };

            AZStd::vector<uint8_t> SpikeLoadBlob(const char* path)
            {
                AZStd::vector<uint8_t> bytes;
                AZ::IO::SystemFile file;
                if (!file.Open(path, AZ::IO::SystemFile::SF_OPEN_READ_ONLY))
                {
                    return bytes;
                }
                bytes.resize(static_cast<size_t>(file.Length()));
                if (!bytes.empty())
                {
                    file.Read(bytes.size(), bytes.data());
                }
                file.Close();
                return bytes;
            }

            // Throwaway: intentionally leaks its COM objects (runs once at startup).
            void RunMeshShaderSpike(ID3D12DeviceX* device)
            {
                const char* msPath = "F:\\engine\\_spike\\tri_ms.dxil";
                const char* psPath = "F:\\engine\\_spike\\tri_ps.dxil";
                AZStd::vector<uint8_t> ms = SpikeLoadBlob(msPath);
                AZStd::vector<uint8_t> ps = SpikeLoadBlob(psPath);
                if (ms.empty() || ps.empty())
                {
                    AZ_TracePrintf("MeshSpike", "FAIL: could not load DXIL (ms=%zu ps=%zu) from %s\n", ms.size(), ps.size(), msPath);
                    return;
                }
                AZ_TracePrintf("MeshSpike", "loaded DXIL ms=%zu ps=%zu bytes\n", ms.size(), ps.size());

                // Empty root signature -- NO ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT (invalid for a mesh PSO).
                D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
                rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
                ID3DBlob* sigBlob = nullptr;
                ID3DBlob* errBlob = nullptr;
                HRESULT hr = D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob);
                if (FAILED(hr))
                {
                    AZ_TracePrintf("MeshSpike", "FAIL: D3D12SerializeRootSignature hr=0x%08X\n", static_cast<unsigned>(hr));
                    return;
                }
                ID3D12RootSignature* rootSig = nullptr;
                hr = device->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSig));
                sigBlob->Release();
                if (errBlob)
                {
                    errBlob->Release();
                }
                if (FAILED(hr))
                {
                    AZ_TracePrintf("MeshSpike", "FAIL: CreateRootSignature hr=0x%08X\n", static_cast<unsigned>(hr));
                    return;
                }

                // R1: hand-rolled stream-subobject mesh PSO (MS + PS, no IA, no primitive topology).
                struct MeshPsoStream
                {
                    SpikeSubobject<ID3D12RootSignature*, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE> m_rootSig;
                    SpikeSubobject<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS> m_ms;
                    SpikeSubobject<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS> m_ps;
                    SpikeSubobject<D3D12_RASTERIZER_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER> m_raster;
                    SpikeSubobject<D3D12_BLEND_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND> m_blend;
                    SpikeSubobject<D3D12_DEPTH_STENCIL_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL> m_depth;
                    SpikeSubobject<DXGI_SAMPLE_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC> m_sampleDesc;
                    SpikeSubobject<UINT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK> m_sampleMask;
                    SpikeSubobject<D3D12_RT_FORMAT_ARRAY, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS> m_rtFormats;
                } stream;

                stream.m_rootSig.m_value = rootSig;
                stream.m_ms.m_value.pShaderBytecode = ms.data();
                stream.m_ms.m_value.BytecodeLength = ms.size();
                stream.m_ps.m_value.pShaderBytecode = ps.data();
                stream.m_ps.m_value.BytecodeLength = ps.size();

                D3D12_RASTERIZER_DESC raster = {};
                raster.FillMode = D3D12_FILL_MODE_SOLID;
                raster.CullMode = D3D12_CULL_MODE_NONE;
                raster.DepthClipEnable = TRUE;
                stream.m_raster.m_value = raster;

                D3D12_BLEND_DESC blend = {};
                blend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
                stream.m_blend.m_value = blend;

                D3D12_DEPTH_STENCIL_DESC depth = {};
                depth.DepthEnable = FALSE;
                depth.StencilEnable = FALSE;
                stream.m_depth.m_value = depth;

                stream.m_sampleDesc.m_value.Count = 1;
                stream.m_sampleDesc.m_value.Quality = 0;
                stream.m_sampleMask.m_value = 0xFFFFFFFFu;

                D3D12_RT_FORMAT_ARRAY rtf = {};
                rtf.NumRenderTargets = 1;
                rtf.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
                stream.m_rtFormats.m_value = rtf;

                D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
                streamDesc.SizeInBytes = sizeof(stream);
                streamDesc.pPipelineStateSubobjectStream = &stream;
                ID3D12PipelineState* pso = nullptr;
                hr = device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pso));
                AZ_TracePrintf("MeshSpike", "R1 hand-rolled stream-PSO CreatePipelineState hr=0x%08X -> %s\n",
                    static_cast<unsigned>(hr), SUCCEEDED(hr) ? "OK" : "FAIL");
                if (FAILED(hr))
                {
                    return;
                }

                // 16x16 offscreen render target.
                const UINT w = 16;
                const UINT h = 16;
                D3D12_HEAP_PROPERTIES defaultHeap = {};
                defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;
                D3D12_RESOURCE_DESC texDesc = {};
                texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
                texDesc.Width = w;
                texDesc.Height = h;
                texDesc.DepthOrArraySize = 1;
                texDesc.MipLevels = 1;
                texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                texDesc.SampleDesc.Count = 1;
                texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
                texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
                ID3D12Resource* rtTex = nullptr;
                hr = device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &texDesc,
                    D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr, IID_PPV_ARGS(&rtTex));
                if (FAILED(hr))
                {
                    AZ_TracePrintf("MeshSpike", "FAIL: CreateCommittedResource(rt) hr=0x%08X\n", static_cast<unsigned>(hr));
                    return;
                }

                D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
                rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
                rtvHeapDesc.NumDescriptors = 1;
                ID3D12DescriptorHeap* rtvHeap = nullptr;
                device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));
                D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
                device->CreateRenderTargetView(rtTex, nullptr, rtvHandle);

                // Readback buffer sized to the texture's copyable footprint.
                D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
                UINT numRows = 0;
                UINT64 rowSizeBytes = 0;
                UINT64 totalBytes = 0;
                device->GetCopyableFootprints(&texDesc, 0, 1, 0, &footprint, &numRows, &rowSizeBytes, &totalBytes);
                D3D12_HEAP_PROPERTIES readbackHeap = {};
                readbackHeap.Type = D3D12_HEAP_TYPE_READBACK;
                D3D12_RESOURCE_DESC bufDesc = {};
                bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
                bufDesc.Width = totalBytes;
                bufDesc.Height = 1;
                bufDesc.DepthOrArraySize = 1;
                bufDesc.MipLevels = 1;
                bufDesc.Format = DXGI_FORMAT_UNKNOWN;
                bufDesc.SampleDesc.Count = 1;
                bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
                ID3D12Resource* readback = nullptr;
                hr = device->CreateCommittedResource(&readbackHeap, D3D12_HEAP_FLAG_NONE, &bufDesc,
                    D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readback));
                if (FAILED(hr))
                {
                    AZ_TracePrintf("MeshSpike", "FAIL: CreateCommittedResource(readback) hr=0x%08X\n", static_cast<unsigned>(hr));
                    return;
                }

                // Independent DIRECT queue / allocator / list (isolated from Atom's).
                D3D12_COMMAND_QUEUE_DESC qDesc = {};
                qDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
                ID3D12CommandQueue* queue = nullptr;
                device->CreateCommandQueue(&qDesc, IID_PPV_ARGS(&queue));
                ID3D12CommandAllocator* alloc = nullptr;
                device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&alloc));
                ID3D12GraphicsCommandList* list = nullptr;
                device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, alloc, nullptr, IID_PPV_ARGS(&list));

                // R2: reach DispatchMesh via QI to List6 (do NOT bump the base typedef).
                ID3D12GraphicsCommandList6* list6 = nullptr;
                hr = list->QueryInterface(IID_PPV_ARGS(&list6));
                AZ_TracePrintf("MeshSpike", "R2 QueryInterface(ID3D12GraphicsCommandList6) hr=0x%08X -> %s\n",
                    static_cast<unsigned>(hr), SUCCEEDED(hr) ? "OK" : "FAIL");
                if (FAILED(hr))
                {
                    return;
                }

                list6->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
                const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
                list6->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
                D3D12_VIEWPORT vp = { 0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h), 0.0f, 1.0f };
                list6->RSSetViewports(1, &vp);
                D3D12_RECT scissor = { 0, 0, static_cast<LONG>(w), static_cast<LONG>(h) };
                list6->RSSetScissorRects(1, &scissor);
                list6->SetGraphicsRootSignature(rootSig);
                list6->SetPipelineState(pso);
                list6->DispatchMesh(1, 1, 1);

                D3D12_RESOURCE_BARRIER barrier = {};
                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier.Transition.pResource = rtTex;
                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
                barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
                barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                list6->ResourceBarrier(1, &barrier);

                D3D12_TEXTURE_COPY_LOCATION dst = {};
                dst.pResource = readback;
                dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
                dst.PlacedFootprint = footprint;
                D3D12_TEXTURE_COPY_LOCATION src = {};
                src.pResource = rtTex;
                src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                src.SubresourceIndex = 0;
                list6->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
                list6->Close();

                ID3D12CommandList* lists[] = { list6 };
                queue->ExecuteCommandLists(1, lists);
                ID3D12Fence* fence = nullptr;
                device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
                queue->Signal(fence, 1);
                if (fence->GetCompletedValue() < 1)
                {
                    HANDLE evt = CreateEventW(nullptr, FALSE, FALSE, nullptr);
                    if (evt)
                    {
                        fence->SetEventOnCompletion(1, evt);
                        WaitForSingleObject(evt, 5000);
                        CloseHandle(evt);
                    }
                }

                const HRESULT drr = device->GetDeviceRemovedReason();
                AZ_TracePrintf("MeshSpike", "DispatchMesh submitted+waited. GetDeviceRemovedReason=0x%08X -> %s\n",
                    static_cast<unsigned>(drr), (drr == S_OK) ? "OK (no hang)" : "DEVICE REMOVED/HUNG");

                void* mapped = nullptr;
                D3D12_RANGE readRange = { 0, static_cast<SIZE_T>(totalBytes) };
                if (SUCCEEDED(readback->Map(0, &readRange, &mapped)) && mapped)
                {
                    const UINT rowPitch = footprint.Footprint.RowPitch;
                    const uint8_t* base = static_cast<const uint8_t*>(mapped);
                    const uint8_t* px = base + (h / 2) * rowPitch + (w / 2) * 4;
                    const bool isRed = (px[0] > 200 && px[1] < 50 && px[2] < 50);
                    AZ_TracePrintf("MeshSpike", "centre pixel RGBA=(%u,%u,%u,%u) EXPECT ~(255,0,0,255) red -> %s\n",
                        px[0], px[1], px[2], px[3], isRed ? "TRIANGLE RENDERED (PASS)" : "NOT RED (FAIL)");
                    D3D12_RANGE noWrite = { 0, 0 };
                    readback->Unmap(0, &noWrite);
                }
                AZ_TracePrintf("MeshSpike", "spike complete\n");
            }
        } // anonymous namespace
#endif // O3DE_MESH_SHADER_SPIKE

        void Device::InitFeatures()
        {
            m_features.m_geometryShader = true;
            m_features.m_computeShader = true;
            m_features.m_independentBlend = true;
            m_features.m_dualSourceBlending = true;
            D3D12_FEATURE_DATA_D3D12_OPTIONS2 options2;
            GetDevice()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS2, &options2, sizeof(options2));
            m_features.m_customSamplePositions =
                options2.ProgrammableSamplePositionsTier != D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_NOT_SUPPORTED;
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
            m_features.m_signalFenceFromCPU = true;
            m_features.m_crossDeviceFences = true;
            m_features.m_crossDeviceDeviceMemory = true;

            // DXGI_SCALING_ASPECT_RATIO_STRETCH is only compatible with CreateSwapChainForCoreWindow or CreateSwapChainForComposition,
            // not Win32 window handles and associated methods (cannot find an MSDN source for that)
            // Source: https://stackoverflow.com/questions/58586223/d3d11-createswapchainforhwnd-fails-with-either-dxgi-error-invalid-call-or-e-inva
            // Create swapchain would fail if uses DXGI_SCALING_ASPECT_RATIO_STRETCH
            m_features.m_swapchainScalingFlags = RHI::ScalingFlags::Stretch;
                        
            D3D12_FEATURE_DATA_D3D12_OPTIONS options;
            GetDevice()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options));
            // DX12's tile resource implementation uses undefined swizzle tile layout which only requires tier 1
            m_features.m_tiledResource = options.TiledResourcesTier >= D3D12_TILED_RESOURCES_TIER_1;

            // Check support of wive operation
            D3D12_FEATURE_DATA_SHADER_MODEL shaderModel;
            shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_0;
            if (FAILED(GetDevice()->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel))))
            {
                AZ_Warning("DX12",  false, "Failed to check feature D3D12_FEATURE_SHADER_MODEL");
                m_features.m_waveOperation = false;
            }
            else
            {
                m_features.m_waveOperation = shaderModel.HighestShaderModel >= D3D_SHADER_MODEL_6_0;
            }

#ifdef AZ_DX12_DXR_SUPPORT
            D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5;
            GetDevice()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5));
            m_features.m_rayTracing = options5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
#else
            m_features.m_rayTracing = false;
#endif

            m_features.m_float16 = (options.MinPrecisionSupport & D3D12_SHADER_MIN_PRECISION_SUPPORT_16_BIT) != 0;

            m_features.m_unboundedArrays = true;

#ifdef O3DE_DX12_VRS_SUPPORT
            D3D12_FEATURE_DATA_D3D12_OPTIONS6 options6;
            GetDevice()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &options6, sizeof(options6));
            switch (options6.VariableShadingRateTier)
            {
            case D3D12_VARIABLE_SHADING_RATE_TIER::D3D12_VARIABLE_SHADING_RATE_TIER_1:
                {
                    m_features.m_shadingRateTypeMask = RHI::ShadingRateTypeFlags::PerDraw;
                    m_features.m_shadingRateMask =
                        RHI::ShadingRateFlags::Rate1x1 |
                        RHI::ShadingRateFlags::Rate1x2 |
                        RHI::ShadingRateFlags::Rate2x1 |
                        RHI::ShadingRateFlags::Rate2x2;                   
                }
                break;
            case D3D12_VARIABLE_SHADING_RATE_TIER::D3D12_VARIABLE_SHADING_RATE_TIER_2:
                {
                    m_features.m_shadingRateTypeMask =
                        RHI::ShadingRateTypeFlags::PerDraw |
                        RHI::ShadingRateTypeFlags::PerRegion |
                        RHI::ShadingRateTypeFlags::PerPrimitive;
                    m_features.m_shadingRateMask =
                        RHI::ShadingRateFlags::Rate1x1 |
                        RHI::ShadingRateFlags::Rate1x2 |
                        RHI::ShadingRateFlags::Rate2x1 |
                        RHI::ShadingRateFlags::Rate2x2;
                    m_features.m_dynamicShadingRateImage = true;
                }
                break;
            default:
                break;
            }

            if (options6.AdditionalShadingRatesSupported)
            {
                m_features.m_shadingRateMask |=
                    RHI::ShadingRateFlags::Rate2x4 |
                    RHI::ShadingRateFlags::Rate4x2 |
                    RHI::ShadingRateFlags::Rate4x4;
            }

            m_limits.m_shadingRateTileSize = RHI::Size(options6.ShadingRateImageTileSize, options6.ShadingRateImageTileSize, 1);
#endif

            // Mesh-shader capability probe (Phase 0 spike — log only; the persistent
            // m_features.m_meshShader bit lands in Phase 1). Hardware mesh+amplification
            // shaders need OPTIONS7.MeshShaderTier >= TIER_1 AND Shader Model >= 6.5.
            {
                D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7{};
                const bool meshTierOk =
                    SUCCEEDED(GetDevice()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7))) &&
                    options7.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED;

                // CheckFeatureSupport returns min(requested, supported), so querying 6.5
                // yields exactly 6.5 when supported and a lower value otherwise.
                D3D12_FEATURE_DATA_SHADER_MODEL sm65{ D3D_SHADER_MODEL_6_5 };
                const bool sm65Ok =
                    SUCCEEDED(GetDevice()->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &sm65, sizeof(sm65))) &&
                    sm65.HighestShaderModel >= D3D_SHADER_MODEL_6_5;

                m_features.m_meshShader = meshTierOk && sm65Ok;

                AZ_TracePrintf(
                    "DX12",
                    "Mesh-shader probe: MeshShaderTier=%d SM6.5=%d -> meshShader=%d\n",
                    static_cast<int>(options7.MeshShaderTier),
                    sm65Ok ? 1 : 0,
                    (meshTierOk && sm65Ok) ? 1 : 0);
            }

            m_limits.m_maxImageDimension1D = D3D12_REQ_TEXTURE1D_U_DIMENSION;
            m_limits.m_maxImageDimension2D = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
            m_limits.m_maxImageDimension3D = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
            m_limits.m_maxImageDimensionCube = D3D12_REQ_TEXTURECUBE_DIMENSION;
            m_limits.m_maxImageArraySize = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
            m_limits.m_minConstantBufferViewOffset = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
            m_limits.m_maxIndirectDrawCount = static_cast<uint32_t>(-1);
            m_limits.m_maxIndirectDispatchCount = static_cast<uint32_t>(-1);
            m_limits.m_maxConstantBufferSize = D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 4u * 4u; // 4096 vectors * 4 values per vector * 4 bytes per value
            m_limits.m_maxBufferSize = D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_C_TERM * (1024u * 1024u); // 2048 MB

#if defined(O3DE_MESH_SHADER_SPIKE)
            RunMeshShaderSpike(GetDevice());
#endif
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
            AZ_PROFILE_FUNCTION(RHI);
            m_commandQueueContext.End();

            m_commandListAllocator.Collect();

            m_descriptorContext->GarbageCollect();

            m_stagingMemoryAllocator.GarbageCollect();

            m_releaseQueue.Collect();
#ifdef USE_AMD_D3D12MA
            m_D3d12maReleaseQueue.Collect();
#endif
        }

        void Device::WaitForIdleInternal()
        {
            m_commandQueueContext.WaitForIdle();
            m_releaseQueue.Collect(true);
#ifdef USE_AMD_D3D12MA
            m_D3d12maReleaseQueue.Collect(true);
#endif
        }

        AZStd::chrono::microseconds Device::GpuTimestampToMicroseconds(uint64_t gpuTimestamp, RHI::HardwareQueueClass queueClass) const
        {
            auto durationInSeconds = AZStd::chrono::duration<double>(double(gpuTimestamp) / m_commandQueueContext.GetCommandQueue(queueClass).GetGpuTimestampFrequency());
            return AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(durationInSeconds);
        }

        AZStd::pair<uint64_t, uint64_t> Device::GetCalibratedTimestamp(RHI::HardwareQueueClass queueClass)
        {
            return m_commandQueueContext.GetCommandQueue(queueClass).GetClockCalibration();
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

            formatsCapabilities[static_cast<uint32_t>(RHI::Format::R8_UINT)] |= RHI::FormatCapabilities::ShadingRate;
        }

        RHI::ResourceMemoryRequirements Device::GetResourceMemoryRequirements(const RHI::ImageDescriptor& descriptor)
        {
            // GetPLACEDImageAllocationInfo, not GetImageAllocationInfo. Every caller of this is the
            // aliased/transient heap picking a heap offset for a resource that CreateImagePlaced will
            // then stamp with GetPlacedTextureAlignment() (4MB for MSAA) in resourceDesc.Alignment --
            // and D3D12 validates the offset against THAT, not against the driver's raw value.
            // Desktop hides the discrepancy because GetResourceAllocationInfo already reports 4MB for
            // MSAA there, so the max() is a no-op and Windows behaviour is unchanged. Xbox reports
            // less, the allocator picked a 64KB-aligned offset, and CreatePlacedResource failed with
            //   "the resource must be aligned to 4194304 ... resource offset in the heap is 199491584"
            // at the first frame-graph compile. Keep the two alignment sources agreeing here.
            D3D12_RESOURCE_ALLOCATION_INFO allocationInfo;
            GetPlacedImageAllocationInfo(descriptor, allocationInfo);

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
#ifdef USE_AMD_D3D12MA
            m_D3d12maReleaseQueue.Notify(notifyFunction);
#endif
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
            AZ_PROFILE_FUNCTION(RHI);

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

            D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;
            DX12RequirementBus::Broadcast(&DX12RequirementBus::Events::CollectAllocatorExtraHeapFlags, heapFlags, heapType);

            Microsoft::WRL::ComPtr<ID3D12Resource> resource;
            HRESULT result = m_dx12Device->CreateCommittedResource(
                &heapProperties,
                heapFlags,
                &resourceDesc,
                initialState,
                (isOutputMergerAttachment && optimizedClearValue) ? &clearValue : nullptr,
                IID_GRAPHICS_PPV_ARGS(resource.GetAddressOf()));

            AZ_RHI_DUMP_POOL_INFO_ON_FAIL(SUCCEEDED(result));
            AssertSuccess(result);

            D3D12_RESOURCE_ALLOCATION_INFO allocationInfo;
            GetImageAllocationInfo(imageDescriptor, allocationInfo);

            return MemoryView(resource.Get(), 0, allocationInfo.SizeInBytes, allocationInfo.Alignment, MemoryViewType::Image, nullptr, 0);
        }

        // initialState is [[maybe_unused]] because it is only read inside
        // #ifdef AZ_DX12_DXR_SUPPORT below. A DX12 build without DXR leaves it unreferenced,
        // which is C4100 and an error under warnings-as-errors.
        void Device::ConvertBufferDescriptorToResourceDesc(
            const RHI::BufferDescriptor& bufferDescriptor,
            [[maybe_unused]] D3D12_RESOURCE_STATES initialState,
            D3D12_RESOURCE_DESC& output)
        {
            ConvertBufferDescriptor(bufferDescriptor, output);
#ifdef AZ_DX12_DXR_SUPPORT
            if (initialState == D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE)
            {
                output.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            }
#endif
        }

#ifdef USE_AMD_D3D12MA
        MemoryView Device::CreateD3d12maBuffer(
            const RHI::BufferDescriptor& bufferDescriptor, D3D12_RESOURCE_STATES initialState, D3D12_HEAP_TYPE heapType)
        {
            D3D12_RESOURCE_DESC resourceDesc;
            ConvertBufferDescriptorToResourceDesc(bufferDescriptor, initialState, resourceDesc);

            D3D12MA::ALLOCATION_DESC allocDesc = {};
            allocDesc.HeapType = heapType;
            DX12RequirementBus::Broadcast(&DX12RequirementBus::Events::CollectAllocatorExtraHeapFlags, allocDesc.ExtraHeapFlags, heapType);

            D3D12MA::Allocation* allocation = nullptr;
            Microsoft::WRL::ComPtr<ID3D12Resource> resource;
            AssertSuccess(m_dx12MemAlloc->CreateResource(
                &allocDesc,
                &resourceDesc,
                initialState,
                NULL,
                &allocation,
                IID_GRAPHICS_PPV_ARGS(resource.GetAddressOf())));

            return MemoryView(allocation, resource.Get(), 0, allocation->GetSize(), allocation->GetAlignment(), MemoryViewType::Buffer);
        }
#endif

        MemoryView Device::CreateBufferCommitted(
            const RHI::BufferDescriptor& bufferDescriptor,
            D3D12_RESOURCE_STATES initialState,
            D3D12_HEAP_TYPE heapType)
        {
            D3D12_RESOURCE_DESC resourceDesc;
            ConvertBufferDescriptorToResourceDesc(bufferDescriptor, initialState, resourceDesc);

            D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;
            DX12RequirementBus::Broadcast(&DX12RequirementBus::Events::CollectAllocatorExtraHeapFlags, heapFlags, heapType);

            CD3DX12_HEAP_PROPERTIES heapProperties(heapType);
            Microsoft::WRL::ComPtr<ID3D12Resource> resource;
            HRESULT result = m_dx12Device->CreateCommittedResource(
                &heapProperties, heapFlags, &resourceDesc, initialState, nullptr, IID_GRAPHICS_PPV_ARGS(resource.GetAddressOf()));
            AZ_RHI_DUMP_POOL_INFO_ON_FAIL(SUCCEEDED(result));
            AssertSuccess(result);

            D3D12_RESOURCE_ALLOCATION_INFO allocationInfo;
            allocationInfo.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
            allocationInfo.SizeInBytes = RHI::AlignUp(resourceDesc.Width, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

            return MemoryView(resource.Get(), 0, allocationInfo.SizeInBytes, allocationInfo.Alignment, MemoryViewType::Buffer, nullptr, 0);
        }

        MemoryView Device::CreateCrossDeviceCapableBuffer(
            const RHI::BufferDescriptor& bufferDescriptor, D3D12_RESOURCE_STATES initialState, D3D12_HEAP_TYPE heapType)
        {
            // We cannot create a committed resource for cross device buffers as this is not allowed in DX12:
            // https://learn.microsoft.com/en-us/windows/win32/direct3d12/shared-heaps#sharing-heaps-across-adapters
            // Instead we create a heap that can be shared cross adapter and a placed resource
            D3D12_RESOURCE_DESC resourceDesc;
            ConvertBufferDescriptorToResourceDesc(bufferDescriptor, initialState, resourceDesc);
            resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER;

            D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;
            DX12RequirementBus::Broadcast(&DX12RequirementBus::Events::CollectAllocatorExtraHeapFlags, heapFlags, heapType);
            heapFlags |= D3D12_HEAP_FLAG_SHARED | D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER;

            D3D12_RESOURCE_ALLOCATION_INFO allocationInfo;
            allocationInfo.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
            allocationInfo.SizeInBytes = RHI::AlignUp(resourceDesc.Width, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

            CD3DX12_HEAP_PROPERTIES heapProperties(heapType);
            D3D12_HEAP_DESC heapDesc{};
            heapDesc.Properties = heapProperties;
            heapDesc.Alignment = allocationInfo.Alignment;
            heapDesc.Flags = heapFlags;
            heapDesc.SizeInBytes = allocationInfo.SizeInBytes;
            ID3D12Heap* heap = nullptr;
            m_dx12Device->CreateHeap1(&heapDesc, nullptr, IID_GRAPHICS_PPV_ARGS(&heap));

            auto memoryView = CreateBufferPlaced(bufferDescriptor, initialState, heap, 0, true);
            memoryView.MarkHeapAsOwnedByMemoryView();
            return memoryView;
        }

        MemoryView Device::CreateBufferPlaced(
            const RHI::BufferDescriptor& bufferDescriptor,
            D3D12_RESOURCE_STATES initialState,
            ID3D12Heap* heap,
            size_t heapByteOffset,
            bool importedFromCrossDevice)
        {
            D3D12_RESOURCE_DESC resourceDesc;
            ConvertBufferDescriptor(bufferDescriptor, resourceDesc);
            if (importedFromCrossDevice)
            {
                resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER;
            }

            D3D12_RESOURCE_ALLOCATION_INFO allocationInfo;
            allocationInfo.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
            allocationInfo.SizeInBytes = RHI::AlignUp(resourceDesc.Width, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

            Microsoft::WRL::ComPtr<ID3D12Resource> resource;
            HRESULT result = m_dx12Device->CreatePlacedResource(
                heap, 
                heapByteOffset, 
                &resourceDesc, 
                initialState, 
                nullptr, 
                IID_GRAPHICS_PPV_ARGS(resource.GetAddressOf())
            );
            AZ_RHI_DUMP_POOL_INFO_ON_FAIL(SUCCEEDED(result));
            AssertSuccess(result);

            return MemoryView(
                resource.Get(), 0, allocationInfo.SizeInBytes, allocationInfo.Alignment, MemoryViewType::Buffer, heap, heapByteOffset);
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
            HRESULT result = m_dx12Device->CreatePlacedResource(
                heap,
                heapByteOffset, 
                &resourceDesc,
                initialState, 
                (isOutputMergerAttachment && optimizedClearValue) ? &clearValue : nullptr, 
                IID_GRAPHICS_PPV_ARGS(resource.GetAddressOf())
            );
            AZ_RHI_DUMP_POOL_INFO_ON_FAIL(SUCCEEDED(result));
            AssertSuccess(result);

            return MemoryView(
                resource.Get(), 0, allocationInfo.SizeInBytes, allocationInfo.Alignment, MemoryViewType::Image, heap, heapByteOffset);
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
            HRESULT result = m_dx12Device->CreateReservedResource(
                &resourceDesc, 
                initialState, 
                nullptr, 
                IID_GRAPHICS_PPV_ARGS(resource.GetAddressOf())
            );
            AZ_RHI_DUMP_POOL_INFO_ON_FAIL(SUCCEEDED(result));
            AssertSuccess(result);

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

            return MemoryView(resource.Get(), 0, allocationInfo.SizeInBytes, allocationInfo.Alignment, MemoryViewType::Image, nullptr, 0);
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
#ifdef USE_AMD_D3D12MA
            if (auto* D3d12maAllocation = memoryView.GetD3d12maAllocation())
            {
                m_D3d12maReleaseQueue.QueueForCollect(D3d12maAllocation);
            }
            else
            {
#endif
                m_releaseQueue.QueueForCollect(memoryView.GetMemory());
                if (memoryView.IsHeapOwnedByMemoryView())
                {
                    m_releaseQueue.QueueForCollect(memoryView.GetHeap());
                }
#ifdef USE_AMD_D3D12MA
            }
#endif
        }


        MemoryView Device::AcquireStagingMemory(size_t size, size_t alignment)
        {
            return m_stagingMemoryAllocator.Allocate(size, alignment);
        }

        CommandList* Device::AcquireCommandList(RHI::HardwareQueueClass hardwareQueueClass)
        {
            return m_commandListAllocator.Allocate(hardwareQueueClass);
        }

        RHI::ConstPtr<PipelineLayout> Device::AcquirePipelineLayout(const RHI::PipelineLayoutDescriptor& descriptor, bool forceMeshRootSignatureFlags, bool forceRayTracingRootSignatureFlags)
        {
            return m_pipelineLayoutCache.Allocate(descriptor, forceMeshRootSignatureFlags, forceRayTracingRootSignatureFlags);
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

        RHI::ShadingRateImageValue Device::ConvertShadingRate(RHI::ShadingRate rate) const
        {            
            return RHI::ShadingRateImageValue{ static_cast<uint8_t>(ConvertShadingRateEnum(rate)), 0 };
        }

        RHI::ResultCode Device::InitInternalBindlessSrg(const RHI::BindlessSrgDescriptor& bindlessSrgDesc)
        {
            m_bindlesSrgBindingSlot = bindlessSrgDesc.m_bindlesSrgBindingSlot;
            return RHI::ResultCode::Success;
        }

        uint32_t Device::GetBindlessSrgSlot() const
        {
            return m_bindlesSrgBindingSlot;
        }
    }
}
