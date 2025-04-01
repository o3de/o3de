/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <AzCore/Utils/Utils.h>
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

#include <dx12ma/D3D12MemAlloc.h>

namespace AZ
{
    namespace DX12
    {
        namespace Platform
        {
            void DeviceCompileMemoryStatisticsInternal(RHI::MemoryStatisticsBuilder& builder, IDXGIAdapterX* dxgiAdapter)
            {
                DXGI_QUERY_VIDEO_MEMORY_INFO memoryInfo;

                if (S_OK == dxgiAdapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &memoryInfo))
                {
                    RHI::MemoryStatistics::Heap* heapStats = builder.AddHeap();
                    heapStats->m_name = Name("Device");
                    heapStats->m_memoryUsage.m_budgetInBytes = memoryInfo.Budget;
                    heapStats->m_memoryUsage.m_totalResidentInBytes = memoryInfo.CurrentReservation;
                    heapStats->m_memoryUsage.m_usedResidentInBytes = memoryInfo.CurrentUsage;
                }

                if (S_OK == dxgiAdapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &memoryInfo))
                {
                    RHI::MemoryStatistics::Heap* heapStats = builder.AddHeap();
                    heapStats->m_name = Name("Host");
                    heapStats->m_memoryUsage.m_budgetInBytes = memoryInfo.Budget;
                    heapStats->m_memoryUsage.m_totalResidentInBytes = memoryInfo.CurrentReservation;
                    heapStats->m_memoryUsage.m_usedResidentInBytes = memoryInfo.CurrentUsage;
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
            Microsoft::WRL::ComPtr<ID3D12DebugDevice2> debugDevice;
            if (SUCCEEDED(dx12Device->QueryInterface(debugDevice.GetAddressOf())))
            {
                D3D12_DEBUG_FEATURE featureFlags{ D3D12_DEBUG_FEATURE_ALLOW_BEHAVIOR_CHANGING_DEBUG_AIDS };
                debugDevice->SetDebugParameter(D3D12_DEBUG_DEVICE_PARAMETER_FEATURE_FLAGS, &featureFlags, sizeof(featureFlags));
                featureFlags = D3D12_DEBUG_FEATURE_CONSERVATIVE_RESOURCE_STATE_TRACKING;
                debugDevice->SetDebugParameter(D3D12_DEBUG_DEVICE_PARAMETER_FEATURE_FLAGS, &featureFlags, sizeof(featureFlags));
            }
        }

        void EnableBreakOnD3DError(Microsoft::WRL::ComPtr<ID3D12DeviceX>& dx12Device)
        {
            Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
            if (SUCCEEDED(dx12Device->QueryInterface(infoQueue.GetAddressOf())))
            {
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
                //Un-comment this if you want to break on warnings too
                //infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
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

            // Disabling this message because it is harmless, yet it overwhelms the Editor log when the D3D Debug Layer is enabled.
            // D3D12 WARNING: ID3D12CommandList::DrawIndexedInstanced: Element [6] in the current Input Layout's declaration references input slot 6,
            //                but there is no Buffer bound to this slot. This is OK, as reads from an empty slot are defined to return 0.
            //                It is also possible the developer knows the data will not be used anyway.
            //                This is only a problem if the developer actually intended to bind an input Buffer here.
            //                [ EXECUTION WARNING #202: COMMAND_LIST_DRAW_VERTEX_BUFFER_NOT_SET]
            disabledMessages.push_back(D3D12_MESSAGE_ID_COMMAND_LIST_DRAW_VERTEX_BUFFER_NOT_SET);

            // Windows build 10.0.17763 (AKA version 1809) has a bug where the D3D Debug layer throws the error COPY_DESCRIPTORS_INVALID_RANGES when it shouldn't.
            // This was fixed in subsequent builds, however, Amazon IT is still deploying this version to new machines as of the time this comment was written.
            if (IsRunningWindows10_0_17763())
            {
                disabledMessages.push_back(D3D12_MESSAGE_ID_COPY_DESCRIPTORS_INVALID_RANGES);
            }

            // We disable these warnings as the our current implementation of Pipeline Library will trigger these warnings unknowingly. For example
            // it will always first try to load a pso from pipelinelibrary triggering D3D12_MESSAGE_ID_LOADPIPELINE_NAMENOTFOUND (for the first time) before storing the PSO in a library.
            // Similarly when we merge multiple pipeline libraries (in multiple threads) we may trigger D3D12_MESSAGE_ID_STOREPIPELINE_DUPLICATENAME as it is possible to save
            // a PSO already saved in the main library. 
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

                // DRED has a perf cost on some drivers/hw so only enable it if RHI validation is enabled.
#ifdef __ID3D12DeviceRemovedExtendedDataSettings1_INTERFACE_DEFINED__
                Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedDataSettings1> pDredSettings;
#else
                Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedDataSettings> pDredSettings;
#endif
                if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pDredSettings))))
                {
                    // Turn on auto-breadcrumbs and page fault reporting.
                    pDredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
                    pDredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
#ifdef __ID3D12DeviceRemovedExtendedDataSettings1_INTERFACE_DEFINED__
                    pDredSettings->SetBreadcrumbContextEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
#endif
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
            
            InitDeviceRemovalHandle();

            m_isAftermathInitialized = Aftermath::InitializeAftermath(m_dx12Device);

            return RHI::ResultCode::Success;
        }

        void Device::ShutdownSubPlatform()
        {
            UnregisterWait(m_waitHandle);
            m_deviceFence.reset();

#ifdef AZ_DEBUG_BUILD
            ID3D12DebugDevice* dx12DebugDevice = nullptr;
            if (m_dx12Device)
            {
                m_dx12Device->QueryInterface(&dx12DebugDevice);
            }
            if (dx12DebugDevice)
            {
                dx12DebugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
                dx12DebugDevice->Release();
            }
#endif
        }

        const char* GetAllocationTypeString(D3D12_DRED_ALLOCATION_TYPE type)
        {
            switch (type)
            {
            case D3D12_DRED_ALLOCATION_TYPE_COMMAND_QUEUE:
                return "D3D12_DRED_ALLOCATION_TYPE_COMMAND_QUEUE";
            case D3D12_DRED_ALLOCATION_TYPE_COMMAND_ALLOCATOR:
                return "D3D12_DRED_ALLOCATION_TYPE_COMMAND_ALLOCATOR";
            case D3D12_DRED_ALLOCATION_TYPE_PIPELINE_STATE:
                return "D3D12_DRED_ALLOCATION_TYPE_PIPELINE_STATE";
            case D3D12_DRED_ALLOCATION_TYPE_COMMAND_LIST:
                return "D3D12_DRED_ALLOCATION_TYPE_COMMAND_LIST";
            case D3D12_DRED_ALLOCATION_TYPE_FENCE:
                return "D3D12_DRED_ALLOCATION_TYPE_FENCE";
            case D3D12_DRED_ALLOCATION_TYPE_DESCRIPTOR_HEAP:
                return "D3D12_DRED_ALLOCATION_TYPE_DESCRIPTOR_HEAP";
            case D3D12_DRED_ALLOCATION_TYPE_HEAP:
                return "D3D12_DRED_ALLOCATION_TYPE_HEAP";
            case D3D12_DRED_ALLOCATION_TYPE_QUERY_HEAP:
                return "D3D12_DRED_ALLOCATION_TYPE_QUERY_HEAP";
            case D3D12_DRED_ALLOCATION_TYPE_COMMAND_SIGNATURE:
                return "D3D12_DRED_ALLOCATION_TYPE_COMMAND_SIGNATURE";
            case D3D12_DRED_ALLOCATION_TYPE_PIPELINE_LIBRARY:
                return "D3D12_DRED_ALLOCATION_TYPE_PIPELINE_LIBRARY";
            case D3D12_DRED_ALLOCATION_TYPE_VIDEO_DECODER:
                return "D3D12_DRED_ALLOCATION_TYPE_VIDEO_DECODER";
            case D3D12_DRED_ALLOCATION_TYPE_VIDEO_PROCESSOR:
                return "D3D12_DRED_ALLOCATION_TYPE_VIDEO_PROCESSOR";
            case D3D12_DRED_ALLOCATION_TYPE_RESOURCE:
                return "D3D12_DRED_ALLOCATION_TYPE_RESOURCE";
            case D3D12_DRED_ALLOCATION_TYPE_PASS:
                return "D3D12_DRED_ALLOCATION_TYPE_PASS";
            case D3D12_DRED_ALLOCATION_TYPE_CRYPTOSESSION:
                return "D3D12_DRED_ALLOCATION_TYPE_CRYPTOSESSION";
            case D3D12_DRED_ALLOCATION_TYPE_CRYPTOSESSIONPOLICY:
                return "D3D12_DRED_ALLOCATION_TYPE_CRYPTOSESSIONPOLICY";
            case D3D12_DRED_ALLOCATION_TYPE_PROTECTEDRESOURCESESSION:
                return "D3D12_DRED_ALLOCATION_TYPE_PROTECTEDRESOURCESESSION";
            case D3D12_DRED_ALLOCATION_TYPE_VIDEO_DECODER_HEAP:
                return "D3D12_DRED_ALLOCATION_TYPE_VIDEO_DECODER_HEAP";
            case D3D12_DRED_ALLOCATION_TYPE_COMMAND_POOL:
                return "D3D12_DRED_ALLOCATION_TYPE_COMMAND_POOL";
            case D3D12_DRED_ALLOCATION_TYPE_COMMAND_RECORDER:
                return "D3D12_DRED_ALLOCATION_TYPE_COMMAND_RECORDER";
            case D3D12_DRED_ALLOCATION_TYPE_STATE_OBJECT:
                return "D3D12_DRED_ALLOCATION_TYPE_STATE_OBJECT";
            case D3D12_DRED_ALLOCATION_TYPE_METACOMMAND:
                return "D3D12_DRED_ALLOCATION_TYPE_METACOMMAND";
            case D3D12_DRED_ALLOCATION_TYPE_SCHEDULINGGROUP:
                return "D3D12_DRED_ALLOCATION_TYPE_SCHEDULINGGROUP";
            case D3D12_DRED_ALLOCATION_TYPE_VIDEO_MOTION_ESTIMATOR:
                return "D3D12_DRED_ALLOCATION_TYPE_VIDEO_MOTION_ESTIMATOR";
            case D3D12_DRED_ALLOCATION_TYPE_VIDEO_MOTION_VECTOR_HEAP:
                return "D3D12_DRED_ALLOCATION_TYPE_VIDEO_MOTION_VECTOR_HEAP";
            case D3D12_DRED_ALLOCATION_TYPE_VIDEO_EXTENSION_COMMAND:
                return "D3D12_DRED_ALLOCATION_TYPE_VIDEO_EXTENSION_COMMAND";

            // NOTE: These enums are not defined in Win10 SDKs 10.0.19041.0 and older
            // case D3D12_DRED_ALLOCATION_TYPE_VIDEO_ENCODER:
            // case D3D12_DRED_ALLOCATION_TYPE_VIDEO_ENCODER_HEAP:

            case D3D12_DRED_ALLOCATION_TYPE_INVALID:
                return "D3D12_DRED_ALLOCATION_TYPE_INVALID";
            default:
                return "Unrecognized DRED allocation type!";
            }
        }

        const char* GetBreadcrumpOpString(D3D12_AUTO_BREADCRUMB_OP op)
        {
            switch (op)
            {
            case D3D12_AUTO_BREADCRUMB_OP_SETMARKER:
                return "D3D12_AUTO_BREADCRUMB_OP_SETMARKER";
            case D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT:
                return "D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT";
            case D3D12_AUTO_BREADCRUMB_OP_ENDEVENT:
                return "D3D12_AUTO_BREADCRUMB_OP_ENDEVENT";
            case D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED:
                return "D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED";
            case D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED:
                return "D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED";
            case D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT:
                return "D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT";
            case D3D12_AUTO_BREADCRUMB_OP_DISPATCH:
                return "D3D12_AUTO_BREADCRUMB_OP_DISPATCH";
            case D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION:
                return "D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION";
            case D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION:
                return "D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION";
            case D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE:
                return "D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE";
            case D3D12_AUTO_BREADCRUMB_OP_COPYTILES:
                return "D3D12_AUTO_BREADCRUMB_OP_COPYTILES";
            case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE:
                return "D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE";
            case D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW:
                return "D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW";
            case D3D12_AUTO_BREADCRUMB_OP_CLEARUNORDEREDACCESSVIEW:
                return "D3D12_AUTO_BREADCRUMB_OP_CLEARUNORDEREDACCESSVIEW";
            case D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW:
                return "D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW";
            case D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER:
                return "D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER";
            case D3D12_AUTO_BREADCRUMB_OP_EXECUTEBUNDLE:
                return "D3D12_AUTO_BREADCRUMB_OP_EXECUTEBUNDLE";
            case D3D12_AUTO_BREADCRUMB_OP_PRESENT:
                return "D3D12_AUTO_BREADCRUMB_OP_PRESENT";
            case D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA:
                return "D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA";
            case D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION:
                return "D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION";
            case D3D12_AUTO_BREADCRUMB_OP_ENDSUBMISSION:
                return "D3D12_AUTO_BREADCRUMB_OP_ENDSUBMISSION";
            case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME:
                return "D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME";
            case D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES:
                return "D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES";
            case D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT:
                return "D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT";
            case D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT64:
                return "D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT64";
            case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCEREGION:
                return "D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCEREGION";
            case D3D12_AUTO_BREADCRUMB_OP_WRITEBUFFERIMMEDIATE:
                return "D3D12_AUTO_BREADCRUMB_OP_WRITEBUFFERIMMEDIATE";
            case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME1:
                return "D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME1";
            case D3D12_AUTO_BREADCRUMB_OP_SETPROTECTEDRESOURCESESSION:
                return "D3D12_AUTO_BREADCRUMB_OP_SETPROTECTEDRESOURCESESSION";
            case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME2:
                return "D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME2";
            case D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES1:
                return "D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES1";
            case D3D12_AUTO_BREADCRUMB_OP_BUILDRAYTRACINGACCELERATIONSTRUCTURE:
                return "D3D12_AUTO_BREADCRUMB_OP_BUILDRAYTRACINGACCELERATIONSTRUCTURE";
            case D3D12_AUTO_BREADCRUMB_OP_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO:
                return "D3D12_AUTO_BREADCRUMB_OP_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO";
            case D3D12_AUTO_BREADCRUMB_OP_COPYRAYTRACINGACCELERATIONSTRUCTURE:
                return "D3D12_AUTO_BREADCRUMB_OP_COPYRAYTRACINGACCELERATIONSTRUCTURE";
            case D3D12_AUTO_BREADCRUMB_OP_DISPATCHRAYS:
                return "D3D12_AUTO_BREADCRUMB_OP_DISPATCHRAYS";
            case D3D12_AUTO_BREADCRUMB_OP_INITIALIZEMETACOMMAND:
                return "D3D12_AUTO_BREADCRUMB_OP_INITIALIZEMETACOMMAND";
            case D3D12_AUTO_BREADCRUMB_OP_EXECUTEMETACOMMAND:
                return "D3D12_AUTO_BREADCRUMB_OP_EXECUTEMETACOMMAND";
            case D3D12_AUTO_BREADCRUMB_OP_ESTIMATEMOTION:
                return "D3D12_AUTO_BREADCRUMB_OP_ESTIMATEMOTION";
            case D3D12_AUTO_BREADCRUMB_OP_RESOLVEMOTIONVECTORHEAP:
                return "D3D12_AUTO_BREADCRUMB_OP_RESOLVEMOTIONVECTORHEAP";
            case D3D12_AUTO_BREADCRUMB_OP_SETPIPELINESTATE1:
                return "D3D12_AUTO_BREADCRUMB_OP_SETPIPELINESTATE1";
            case D3D12_AUTO_BREADCRUMB_OP_INITIALIZEEXTENSIONCOMMAND:
                return "D3D12_AUTO_BREADCRUMB_OP_INITIALIZEEXTENSIONCOMMAND";
            case D3D12_AUTO_BREADCRUMB_OP_EXECUTEEXTENSIONCOMMAND:
                return "D3D12_AUTO_BREADCRUMB_OP_EXECUTEEXTENSIONCOMMAND";

            // Disable due to the current minimum windows version doesn't have this enum
            // case D3D12_AUTO_BREADCRUMB_OP_DISPATCHMESH:
                // return "D3D12_AUTO_BREADCRUMB_OP_DISPATCHMESH";
            }
            return "unkown op";
        }

        bool Device::AssertSuccess(HRESULT hr)
        {
            return DX12::AssertSuccess(hr);
        }

        void Device::OnDeviceRemoved()
        {
            // It's possible this function is called many times at same time from different threads.
            // We want the other threads are blocked until the device removal is fully handled. 
            AZStd::lock_guard<AZStd::mutex> lock(m_onDeviceRemovedMutex);

            if (m_onDeviceRemoved)
            {
                return;
            }
            m_onDeviceRemoved = true;

            ID3D12Device* removedDevice = m_dx12Device.get();
            HRESULT removedReason = removedDevice->GetDeviceRemovedReason();

#if defined(AZ_FORCE_CPU_GPU_INSYNC)
            AZ_TracePrintf("Device", "The last executing pass before device removal was: %s\n", GetLastExecutingScope().data());
#endif
            AZ_TracePrintf("Device", "Device was removed because of the following reason:\n");
            const char* removedReasonString;

            switch (removedReason)
            {
            case DXGI_ERROR_DEVICE_HUNG:
                AZ_TracePrintf(
                    "DX12",
                    "DXGI_ERROR_DEVICE_HUNG - The application's device failed due to badly formed commands sent by the "
                    "application. This is an design-time issue that should be investigated and fixed.\n");
                removedReasonString = "DXGI_ERROR_DEVICE_HUNG";
                break;
            case DXGI_ERROR_DEVICE_REMOVED:
                AZ_TracePrintf(
                    "DX12",
                    "DXGI_ERROR_DEVICE_REMOVED - The video card has been physically removed from the system, or a driver upgrade "
                    "for the video card has occurred. The application should destroy and recreate the device. For help debugging "
                    "the problem, call ID3D10Device::GetDeviceRemovedReason.\n");
                removedReasonString = "DXGI_ERROR_DEVICE_REMOVED";
                break;
            case DXGI_ERROR_DEVICE_RESET:
                AZ_TracePrintf(
                    "DX12",
                    "DXGI_ERROR_DEVICE_RESET - The device failed due to a badly formed command. This is a run-time issue; The "
                    "application should destroy and recreate the device.\n");
                removedReasonString = "DXGI_ERROR_DEVICE_RESET";
                break;
            case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
                AZ_TracePrintf(
                    "DX12",
                    "DXGI_ERROR_DRIVER_INTERNAL_ERROR - The driver encountered a problem and was put into the device removed "
                    "state.\n");
                removedReasonString = "DXGI_ERROR_DRIVER_INTERNAL_ERROR";
                break;
            case DXGI_ERROR_INVALID_CALL:
                AZ_TracePrintf(
                    "DX12",
                    "DXGI_ERROR_INVALID_CALL - The application provided invalid parameter data; this must be debugged and fixed "
                    "before the application is released.\n");
                removedReasonString = "DXGI_ERROR_INVALID_CALL";
                break;
            case DXGI_ERROR_ACCESS_DENIED:
                AZ_TracePrintf(
                    "DX12",
                    "DXGI_ERROR_ACCESS_DENIED - You tried to use a resource to which you did not have the required access "
                    "privileges. This error is most typically caused when you write to a shared resource with read-only access.\n");
                removedReasonString = "DXGI_ERROR_ACCESS_DENIED";
                break;
            case S_OK:
                AZ_TracePrintf("DX12", "S_OK - The method succeeded without an error.\n");
                removedReasonString = "S_OK (?)";
                break;
            default:
                AZ_TracePrintf(
                    "DX12",
                    "DXGI error code: %X\n", removedReason);
                removedReasonString = "Unknown DXGI error";
                break;
            }
           
            // Perform app-specific device removed operation, such as logging or inspecting DRED output
#ifdef __ID3D12DeviceRemovedExtendedDataSettings1_INTERFACE_DEFINED__
            Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedData1> pDred;
#else
            Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedData> pDred;
#endif

            if (SUCCEEDED(removedDevice->QueryInterface(IID_PPV_ARGS(&pDred))))
            {
#ifdef __ID3D12DeviceRemovedExtendedDataSettings1_INTERFACE_DEFINED__
                D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 dredAutoBreadcrumbsOutput;
                HRESULT hr = pDred->GetAutoBreadcrumbsOutput1(&dredAutoBreadcrumbsOutput);
#else
                D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT dredAutoBreadcrumbsOutput;
                HRESULT hr = pDred->GetAutoBreadcrumbsOutput(&dredAutoBreadcrumbsOutput);
#endif

                if (SUCCEEDED(hr))
                {
                    // Emit DRED output to a separate log file in ~/.o3de/DRED with timestamp label
                    // We write to a file instead of writing to the debug console or stdout because in
                    // a device removed scenario, asserts and log spew will likely occur all over the place.
                    // Writing the breadcrumbs to a separate file gives us a pristine timeline to inspect
                    // the source of the TDR.
                    AZ::IO::Path path = AZ::Utils::GetO3deLogsDirectory().c_str();
                    path /= "DRED";
                    AZ::IO::SystemFile::CreateDir(path.c_str());

                    time_t ltime;
                    time(&ltime);
                    tm today;
#if AZ_TRAIT_USE_SECURE_CRT_FUNCTIONS
                    localtime_s(&today, &ltime);
#else
                    today = *localtime(&ltime);
#endif
                    char sTemp[128];
                    strftime(sTemp, sizeof(sTemp), "%Y%m%d.%H%M%S", &today);
                    AZStd::string filename = AZStd::string::format("%s/DRED_%s.log", path.c_str(), sTemp);

                    AZ::IO::SystemFile dredLog;
                    if (!dredLog.Open(filename.c_str(), AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY))
                    {
                        AZ_TracePrintf("DRED", "Failed to open file %s for writing\n", filename.c_str());
                        return;
                    }
                    AZ_TracePrintf("DRED", "Device removed! Writing DRED log to %s\n", filename.c_str());

                    AZStd::string line = AZStd::string::format("===BEGIN DRED LOG===\n"
                        "\nRemoval reason: %s\n", removedReasonString);
                    dredLog.Write(line.data(), line.size());


                    // Walk all breakcrumb nodes, emitting the operation, context (if available), and mark
                    // any region where an error has occurred
                    auto* currentNode = dredAutoBreadcrumbsOutput.pHeadAutoBreadcrumbNode;
                    AZ::u32 index = 0;
                    while (currentNode)
                    {
                        const wchar_t* cmdListName = currentNode->pCommandListDebugNameW;
                        const wchar_t* cmdQueueName = currentNode->pCommandQueueDebugNameW;
                        AZ::u32 expected = currentNode->BreadcrumbCount;
                        AZ::u32 actual = *currentNode->pLastBreadcrumbValue;

                        // An error is known to occur if this node executed anything and the number of
                        // breadcrumbs reached doesn't match the expected count.
                        bool errorOccurred = actual > 0 && actual < expected;

                        line = AZStd::string::format(
                            "Node %u on %S cmdlist (%p) submitted on %S queue (%p) "
                            "reached %u out of %u breadcrumbs\n",
                            index,
                            cmdListName ? cmdListName : L"Unknown",
                            currentNode->pCommandList,
                            cmdQueueName ? cmdQueueName : L"Unknown",
                            currentNode->pCommandQueue,
                            actual, expected);

                        dredLog.Write(line.data(), line.size());

                        if (actual == 0)
                        {
                            // Don't bother logging nodes that don't submit anything
                            currentNode = currentNode->pNext;
                            ++index;
                            continue;
                        }

                        // Create lookup table for breadcrumb context entries
                        AZStd::unordered_map<AZ::u32, const wchar_t*> contextEntries;

#ifdef __ID3D12DeviceRemovedExtendedDataSettings1_INTERFACE_DEFINED__
                        contextEntries.reserve(currentNode->BreadcrumbContextsCount);

                        for (AZ::u32 i = 0; i != currentNode->BreadcrumbContextsCount; ++i)
                        {
                            D3D12_DRED_BREADCRUMB_CONTEXT& context = currentNode->pBreadcrumbContexts[i];
                            contextEntries[context.BreadcrumbIndex] = context.pContextString;
                        }
#endif

                        // Display all the breadcrumbs in this node, marking the region where the error
                        // may have occurred
                        AZ::u32 depth = 1;

                        for (AZ::u32 i = 0; i != expected; ++i)
                        {
                            D3D12_AUTO_BREADCRUMB_OP op = currentNode->pCommandHistory[i];

                            if (errorOccurred && i == actual)
                            {
                                // This is the first op that exceeds the number of ops that finished
                                line = "===ERROR START===\n";
                                dredLog.Write(line.data(), line.size());
                            }

                            if (op == D3D12_AUTO_BREADCRUMB_OP_ENDEVENT)
                            {
                                --depth;
                            }

                            // Check if we have an associated context for this op
                            if (contextEntries.contains(i))
                            {
                                line = AZStd::string::format("    %S : %s\n", contextEntries[i], GetBreadcrumpOpString(op));
                            }
                            else
                            {
                                line = AZStd::string::format("    %s\n", GetBreadcrumpOpString(op));
                            }

                            for (AZ::u32 j = 0; j != depth; ++j)
                            {
                                dredLog.Write("    ", 4);
                            }

                            dredLog.Write(line.data(), line.size());

                            // Encountering a begin event, add indentation for subsequent ops
                            if (op == D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT)
                            {
                                ++depth;
                            }

                        }

                        if (errorOccurred)
                        {
                            line = "===ERROR END===\n";
                            dredLog.Write(line.data(), line.size());
                        }

                        currentNode = currentNode->pNext;
                        ++index;
                    }

#ifdef __ID3D12DeviceRemovedExtendedDataSettings1_INTERFACE_DEFINED__
                    D3D12_DRED_PAGE_FAULT_OUTPUT1 pageFaultOutput;
                    if (SUCCEEDED(pDred->GetPageFaultAllocationOutput1(&pageFaultOutput)))
                    {
#else
                    D3D12_DRED_PAGE_FAULT_OUTPUT pageFaultOutput;
                    if (SUCCEEDED(pDred->GetPageFaultAllocationOutput(&pageFaultOutput)))
                    {
#endif
                        line = AZStd::string::format("Page fault occurred on address %zx\n\n"
                            "Dumping resident objects\n",
                            pageFaultOutput.PageFaultVA);

                        dredLog.Write(line.data(), line.size());

                        // Dump objects and their addresses
                        const auto* node = pageFaultOutput.pHeadExistingAllocationNode;
                        while (node)
                        {
                            line = AZStd::string::format("    0x%p (%S) %s\n",
#ifdef __ID3D12DeviceRemovedExtendedDataSettings1_INTERFACE_DEFINED__
                                node->pObject,
#else
                                0,
#endif
                                node->ObjectNameW ? node->ObjectNameW : L"Unknown",
                                GetAllocationTypeString(node->AllocationType));
                            dredLog.Write(line.data(), line.size());
                            node = node->pNext;
                        }

                        line = "Emitting recently freed objects:\n";
                        node = pageFaultOutput.pHeadRecentFreedAllocationNode;
                        while (node)
                        {
                            line = AZStd::string::format("    0x%p (%S) %s\n",
#ifdef __ID3D12DeviceRemovedExtendedDataSettings1_INTERFACE_DEFINED__
                                node->pObject,
#else
                                0,
#endif
                                node->ObjectNameW ? node->ObjectNameW : L"Unknown",
                                GetAllocationTypeString(node->AllocationType));
                            dredLog.Write(line.data(), line.size());
                            node = node->pNext;
                        }
                    }
                    else
                    {
                        line = "\nFailed to retrieve DRED page fault data\n";
                        dredLog.Write(line.data(), line.size());
                    }

                    // We write this epilogue to detect cases where the log writing was interrupted
                    line = "===END DRED LOG===\n";
                    dredLog.Write(line.data(), line.size());
                    dredLog.Close();
                    AZ_TracePrintf("DRED", "Finished writing DRED log to %s\n", filename.c_str());
                }
                else
                {
                    switch (hr)
                    {
                    case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE:
                        AZ_TracePrintf("Device", "Could not retrieve DRED bread crumbs: DXGI_ERROR_NOT_CURRENTLY_AVAILABLE\n");
                        break;

                    case DXGI_ERROR_UNSUPPORTED:
                        AZ_TracePrintf(
                            "Device", "Could not retrieve DRED bread crumbs (auto-breadcrumbs not enabled): DXGI_ERROR_UNSUPPORTED\n");
                        break;

                    default:
                        AZ_TracePrintf("Device", "Could not retrieve DRED bread crumbs (reason unknown)\n");
                        break;
                    }
                }
            }

            AZ_TracePrintf("Device", " ===========================End of OnDeviceRemoved================================\n");

            if (IsAftermathInitialized())
            {
                // Try outputting the name of the last scope that was executing on the GPU
                // There is a good chance that is the cause of the GPU crash and should be investigated first
                Aftermath::OutputLastScopeExecutingOnGPU(GetAftermathGPUCrashTracker());
            }

            SetDeviceRemoved();

            // Assert before continuing so users have a chance to inspect the TDR before the log output gets burried under the ensuing RHI errors
            AZ_Assert(false, "GPU device lost!");
        }

        void HandleDeviceRemoved(PVOID context, BOOLEAN)
        {
            Device* removedDevice = (Device*)context;
            removedDevice->OnDeviceRemoved();
        }

        void Device::InitDeviceRemovalHandle()
        {
            // Create fence to detect device removal
            Microsoft::WRL::ComPtr<ID3D12Fence> fencePtr;
            if (FAILED(m_dx12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_GRAPHICS_PPV_ARGS(fencePtr.GetAddressOf()))))
            {
                return;
            }
            m_deviceFence = fencePtr.Get();
            HANDLE deviceRemovedEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
            m_deviceFence->SetEventOnCompletion(UINT64_MAX, deviceRemovedEvent);

            RegisterWaitForSingleObject(
              &m_waitHandle,
              deviceRemovedEvent,
              HandleDeviceRemoved,
              this, // Pass the device as our context
              INFINITE, // No timeout
              0 // No flags
            );
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

        RHI::ResultCode Device::BeginFrameInternal()
        {
#ifdef USE_AMD_D3D12MA
            static uint32_t frameIndex = 0;
            if (m_dx12MemAlloc)
            {
                m_dx12MemAlloc->SetCurrentFrameIndex(++frameIndex);
            }
#endif
            m_commandQueueContext.Begin();
            return RHI::ResultCode::Success;
        }
    }
}

