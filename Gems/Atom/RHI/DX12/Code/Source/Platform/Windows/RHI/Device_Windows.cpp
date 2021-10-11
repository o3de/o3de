/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <RHI/Device.h>
#include <RHI/CommandQueueContext.h>
#include <RHI/NsightAftermath.h>
#include <RHI/PhysicalDevice.h>
#include <RHI/WindowsVersionQuery.h>
#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/conversions.h>
#include <Atom/RHI/ValidationLayer.h>
#include <Atom/RHI/FactoryManagerBus.h>
#include <comdef.h>

namespace AZ
{
    namespace DX12
    {
        namespace Platform
        {
            void DeviceShutdownInternal(ID3D12DeviceX* device)
            {
                AZ_UNUSED(device);
#ifdef AZ_DEBUG_BUILD
                ID3D12DebugDevice* dx12DebugDevice = nullptr;
                if (device)
                {
                    device->QueryInterface(&dx12DebugDevice);
                }
                if (dx12DebugDevice)
                {
                    dx12DebugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
                    dx12DebugDevice->Release();
                }
#endif
            }

            void DeviceCompileMemoryStatisticsInternal(RHI::MemoryStatisticsBuilder& builder, IDXGIAdapterX* dxgiAdapter)
            {
                DXGI_QUERY_VIDEO_MEMORY_INFO memoryInfo;

                if (S_OK == dxgiAdapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &memoryInfo))
                {
                    RHI::MemoryStatistics::Heap* heapStats = builder.AddHeap();
                    heapStats->m_name = Name("Device");
                    heapStats->m_memoryUsage.m_budgetInBytes = memoryInfo.Budget;
                    heapStats->m_memoryUsage.m_reservedInBytes = memoryInfo.CurrentReservation;
                    heapStats->m_memoryUsage.m_residentInBytes = memoryInfo.CurrentUsage;
                }

                if (S_OK == dxgiAdapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &memoryInfo))
                {
                    RHI::MemoryStatistics::Heap* heapStats = builder.AddHeap();
                    heapStats->m_name = Name("Host");
                    heapStats->m_memoryUsage.m_budgetInBytes = memoryInfo.Budget;
                    heapStats->m_memoryUsage.m_reservedInBytes = memoryInfo.CurrentReservation;
                    heapStats->m_memoryUsage.m_residentInBytes = memoryInfo.CurrentUsage;
                }
            }

            D3D12_RESOURCE_STATES GetRayTracingAccelerationStructureResourceState()
            {
                return D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
            }
        }

        void EnableD3DDebugLayer()
        {
            Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
            {
                debugController->EnableDebugLayer();
            }
        }

        void EnableGPUBasedValidation()
        {
            Microsoft::WRL::ComPtr<ID3D12Debug1> debugController1;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController1))))
            {
                debugController1->SetEnableGPUBasedValidation(TRUE);
                debugController1->SetEnableSynchronizedCommandQueueValidation(TRUE);
            }

            Microsoft::WRL::ComPtr<ID3D12Debug2> debugController2;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController2))))
            {
                debugController2->SetGPUBasedValidationFlags(D3D12_GPU_BASED_VALIDATION_FLAGS_NONE);
            }
        }

        void EnableDebugDeviceFeatures(Microsoft::WRL::ComPtr<ID3D12DeviceX>& dx12Device)
        {
            Microsoft::WRL::ComPtr<ID3D12DebugDevice> debugDevice;
            if (SUCCEEDED(dx12Device->QueryInterface(debugDevice.GetAddressOf())))
            {
                debugDevice->SetFeatureMask(D3D12_DEBUG_FEATURE_ALLOW_BEHAVIOR_CHANGING_DEBUG_AIDS | D3D12_DEBUG_FEATURE_CONSERVATIVE_RESOURCE_STATE_TRACKING);
            }
        }

        void EnableBreakOnD3DError(Microsoft::WRL::ComPtr<ID3D12DeviceX>& dx12Device)
        {
            Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
            if (SUCCEEDED(dx12Device->QueryInterface(infoQueue.GetAddressOf())))
            {
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            }
        }

        bool IsRunningWindows10_0_17763()
        {
            Platform::WindowsVersion windowsVersion;
            if (!Platform::GetWindowsVersion(&windowsVersion))
            {
                return false;
            }
            return windowsVersion.m_majorVersion == 10 && windowsVersion.m_minorVersion == 0 && windowsVersion.m_buildVersion == 17763;
        }

        void AddDebugFilters(Microsoft::WRL::ComPtr<ID3D12DeviceX>& dx12Device, RHI::ValidationMode validationMode)
        {
            AZStd::vector<D3D12_MESSAGE_SEVERITY> enabledSeverities;
            AZStd::vector<D3D12_MESSAGE_ID> disabledMessages;

            // These severities should be seen all the time
            enabledSeverities.push_back(D3D12_MESSAGE_SEVERITY_CORRUPTION);
            enabledSeverities.push_back(D3D12_MESSAGE_SEVERITY_ERROR);
            enabledSeverities.push_back(D3D12_MESSAGE_SEVERITY_WARNING);
            enabledSeverities.push_back(D3D12_MESSAGE_SEVERITY_MESSAGE);

            if (validationMode == RHI::ValidationMode::Verbose)
            {
                // Verbose only filters
                enabledSeverities.push_back(D3D12_MESSAGE_SEVERITY_INFO);
            }

            // [GFX TODO][ATOM-4573] - We keep getting this warning when reading from query buffers on a job thread
            // while a command queue thread is submitting a command list that is using the same buffer, but in a
            // different region. We should add validation elsewhere to make sure that multi-threaded access continues to
            // be valid and possibly find a way to restore this warning to catch other cases that could be invalid.
            disabledMessages.push_back(D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_GPU_WRITTEN_READBACK_RESOURCE_MAPPED);

            // Windows build 10.0.17763 (AKA version 1809) has a bug where the D3D Debug layer throws the error COPY_DESCRIPTORS_INVALID_RANGES when it shouldn't.
            // This was fixed in subsequent builds, however, Amazon IT is still deploying this version to new machines as of the time this comment was written.
            if (IsRunningWindows10_0_17763())
            {
                disabledMessages.push_back(D3D12_MESSAGE_ID_COPY_DESCRIPTORS_INVALID_RANGES);
            }

            // [GFX TODO][ATOM-4712] - Fix PipelineLibrary Loading. These warnings were silenced for a release and need to be fixed properly.
#if defined (AZ_DX12_USE_PIPELINE_LIBRARY)
            disabledMessages.push_back(D3D12_MESSAGE_ID_LOADPIPELINE_NAMENOTFOUND);
            disabledMessages.push_back(D3D12_MESSAGE_ID_STOREPIPELINE_DUPLICATENAME);
#endif

            Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
            if (SUCCEEDED(dx12Device->QueryInterface(infoQueue.GetAddressOf())))
            {
                D3D12_INFO_QUEUE_FILTER filter{ };
                filter.AllowList.NumSeverities = azlossy_cast<UINT>(enabledSeverities.size());
                filter.AllowList.pSeverityList = enabledSeverities.data();
                filter.DenyList.NumIDs = azlossy_cast<UINT>(disabledMessages.size());
                filter.DenyList.pIDList = disabledMessages.data();

                // Clear out the existing filters since we're taking full control of them
                infoQueue->PushEmptyStorageFilter();

                [[maybe_unused]] HRESULT addedOk = infoQueue->AddStorageFilterEntries(&filter);
                AZ_Assert(addedOk == S_OK, "D3DInfoQueue AddStorageFilterEntries failed");

                infoQueue->AddApplicationMessage(D3D12_MESSAGE_SEVERITY_MESSAGE, "D3D12 Debug Filters setup");
            }
        }

        RHI::ValidationMode GetValidationMode()
        {
            RHI::ValidationMode validationMode = RHI::ValidationMode::Disabled;
            RHI::FactoryManagerBus::BroadcastResult(validationMode, &RHI::FactoryManagerRequest::DetermineValidationMode);
            return validationMode;
        }

        RHI::ResultCode Device::InitSubPlatform(RHI::PhysicalDevice& physicalDeviceBase)
        {
#if defined(USE_NSIGHT_AFTERMATH)
            // Enable Nsight Aftermath GPU crash dump creation.
            // This needs to be done before the D3D device is created.
            m_gpuCrashTracker.EnableGPUCrashDumps();
#endif
            PhysicalDevice& physicalDevice = static_cast<PhysicalDevice&>(physicalDeviceBase);
            RHI::ValidationMode validationMode = GetValidationMode();

            if (validationMode != RHI::ValidationMode::Disabled)
            {
                EnableD3DDebugLayer();
                if (validationMode == RHI::ValidationMode::GPU)
                {
                    EnableGPUBasedValidation();
                }
            }

            Microsoft::WRL::ComPtr<ID3D12DeviceX> dx12Device;
            if (FAILED(D3D12CreateDevice(physicalDevice.GetAdapter(), D3D_FEATURE_LEVEL_12_0, IID_GRAPHICS_PPV_ARGS(dx12Device.GetAddressOf()))))
            {
                AZ_Error("Device", false, "Failed to initialize the device. Check the debug layer for more info.");
                return RHI::ResultCode::Fail;
            }

            if (validationMode != RHI::ValidationMode::Disabled)
            {
                EnableDebugDeviceFeatures(dx12Device);
                EnableBreakOnD3DError(dx12Device);

                AddDebugFilters(dx12Device, validationMode);
            }

            m_dx12Device = dx12Device.Get();
            m_dxgiFactory = physicalDevice.GetFactory();
            m_dxgiAdapter = physicalDevice.GetAdapter();

            m_isAftermathInitialized = Aftermath::InitializeAftermath(m_dx12Device);
            return RHI::ResultCode::Success;
        }

        RHI::ResultCode Device::CreateSwapChain(
            IUnknown* window,
            const DXGI_SWAP_CHAIN_DESCX& swapChainDesc,
            RHI::Ptr<IDXGISwapChainX>& outSwapChain)
        {
            Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChainPtr;

            HRESULT hr = m_dxgiFactory->CreateSwapChainForHwnd(
                m_commandQueueContext.GetCommandQueue(RHI::HardwareQueueClass::Graphics).GetPlatformQueue(),
                reinterpret_cast<HWND>(window),
                &swapChainDesc,
                nullptr,
                nullptr,
                &swapChainPtr);

            if (FAILED(hr))
            {
                _com_error error(hr);
                AZ_Error("Device", false, "Failed to initialize SwapChain with error 0x%x(%s) Check the debug layer for more info.\nDimensions: %u x %u DXGI_FORMAT: %u", hr, error.ErrorMessage(), swapChainDesc.Width, swapChainDesc.Height, swapChainDesc.Format);
                return RHI::ResultCode::Fail;
            }

            Microsoft::WRL::ComPtr<IDXGISwapChainX> swapChainX;
            swapChainPtr->QueryInterface(IID_GRAPHICS_PPV_ARGS(swapChainX.GetAddressOf()));

            outSwapChain = swapChainX.Get();
            return RHI::ResultCode::Success;
        }

        RHI::ResultCode Device::CreateSwapChain(
            [[maybe_unused]] const DXGI_SWAP_CHAIN_DESCX& swapChainDesc,
            [[maybe_unused]] AZStd::array<RHI::Ptr<ID3D12Resource>, RHI::Limits::Device::FrameCountMax>& outSwapChainResources)
        {
            AZ_Assert(false, "Wrong Device::CreateSwapChain function called on Windows.");
            return RHI::ResultCode::Fail;
        }

        AZStd::vector<RHI::Format> Device::GetValidSwapChainImageFormats(const RHI::WindowHandle& windowHandle) const
        {
            AZStd::vector<RHI::Format> formatsList;

            // Follows Microsoft's HDR sample code for determining if the connected display supports HDR.
            // Enumerates all of the detected displays and determines which one has the largest intersection with the
            // region of the window handle parameter.
            // If the display for this region supports wide color gamut, then a wide color gamut format is added to
            // the list of supported formats.
            // https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/UWP/D3D12HDR/src/D3D12HDR.cpp

            HWND hWnd = reinterpret_cast<HWND>(windowHandle.GetIndex());
            RECT windowRect = {};
            GetWindowRect(hWnd, &windowRect);

            UINT outputIndex = 0;
            Microsoft::WRL::ComPtr<IDXGIOutput> bestOutput;
            Microsoft::WRL::ComPtr<IDXGIOutput> currentOutput;
            RECT intersectRect;
            int bestIntersectionArea = -1;
            while (m_dxgiAdapter->EnumOutputs(outputIndex, &currentOutput) == S_OK)
            {
                // Get the rectangle bounds of current output
                DXGI_OUTPUT_DESC outputDesc;
                currentOutput->GetDesc(&outputDesc);
                RECT outputRect = outputDesc.DesktopCoordinates;
                int intersectionArea = 0;
                if (IntersectRect(&intersectRect, &windowRect, &outputRect))
                {
                    intersectionArea = (intersectRect.bottom - intersectRect.top) * (intersectRect.right - intersectRect.left);
                }
                if (intersectionArea > bestIntersectionArea)
                {
                    bestOutput = currentOutput;
                    bestIntersectionArea = intersectionArea;
                }

                outputIndex++;
            }

            if (bestOutput)
            {
                Microsoft::WRL::ComPtr<IDXGIOutput6> output6;
                [[maybe_unused]] HRESULT hr = bestOutput.As(&output6);
                AZ_Assert(S_OK == hr, "Failed to get IDXGIOutput6 structure.");
                DXGI_OUTPUT_DESC1 outputDesc;
                output6->GetDesc1(&outputDesc);
                if (outputDesc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)
                {
                    // HDR is supported
                    formatsList.push_back(RHI::Format::R10G10B10A2_UNORM);
                }
            }

            // Fallback default 8-bit format
            formatsList.push_back(RHI::Format::R8G8B8A8_UNORM);

            return formatsList;
        }

        void Device::BeginFrameInternal()
        {
            m_commandQueueContext.Begin();
        }
    }
}
