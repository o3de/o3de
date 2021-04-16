// AMD AMDUtils code
// 
// Copyright(c) 2018 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "stdafx.h"
#include "Device.h"
#include "Misc/Misc.h"
#include "Base/Helper.h"

#include <dxgi1_4.h>

#ifdef _DEBUG
#pragma comment(lib, "dxguid.lib")
#include <DXGIDebug.h>
#endif 

namespace CAULDRON_DX12
{
    Device::Device()
    {
    }

    Device::~Device()
    {
    }

    void Device::OnCreate(const char *pAppName, const char *pEngine, bool bValidationEnabled, HWND hWnd)
    {
        // Enable the D3D12 debug layer
        //
        // Note that it turns out the validation and debug layer are know to cause 
        // deadlocks in certain circumstances, for example when the vsync interval 
        // is 0 and full screen is used
        if (bValidationEnabled)
        {
            ID3D12Debug1 *pDebugController;
            if (D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugController)) == S_OK)
            {
                pDebugController->EnableDebugLayer();               
                pDebugController->SetEnableGPUBasedValidation(TRUE);

                pDebugController->SetEnableSynchronizedCommandQueueValidation(TRUE);
                pDebugController->Release();
            }
        }

        // Create Device
        //
        AGSReturnCode result = agsInit(&m_agsContext, nullptr, &m_agsGPUInfo);

        // if AGS got initialized because the AMD driver is installed but the HW is not AMD then deinit.
        if ((result == AGS_SUCCESS) && m_agsContext && (m_agsGPUInfo.devices[0].vendorId != 0x1002))
        {
            agsDeInit(m_agsContext);
            m_agsContext = NULL;
        }

        UINT factoryFlags = 0;
#ifdef _DEBUG
        factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
        IDXGIFactory *pFactory;
        CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&pFactory));

        pFactory->EnumAdapters(0, &m_pAdapter);
        pFactory->Release();


        if (m_agsContext)
        {
            UserMarker::SetAgsContext(m_agsContext);

            AGSDX12DeviceCreationParams creationParams = {};
            creationParams.pAdapter = m_pAdapter;
            creationParams.iid = __uuidof(m_pDevice);
            creationParams.FeatureLevel = D3D_FEATURE_LEVEL_12_0;

            AGSDX12ExtensionParams extensionParams = {};
            AGSDX12ReturnedParams returnedParams = {};

            // Create AGS Device
            //
            AGSReturnCode rc = agsDriverExtensionsDX12_CreateDevice(m_agsContext, &creationParams, &extensionParams, &returnedParams);
            if (rc == AGS_SUCCESS)
            {
                m_pDevice = returnedParams.pDevice;
                SetName(m_pDevice, "device");
            }
        }
        else
        {
            ThrowIfFailed(D3D12CreateDevice(m_pAdapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_pDevice)));
            SetName(m_pDevice, "device");
        }

        // Check for FP16 support
        //
        D3D12_FEATURE_DATA_D3D12_OPTIONS featureDataOptions = {};
        m_pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureDataOptions, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));
        m_fp16Supported = (featureDataOptions.MinPrecisionSupport & D3D12_SHADER_MIN_PRECISION_SUPPORT_16_BIT) != 0;

        // create queues
        //
        {
            D3D12_COMMAND_QUEUE_DESC queueDesc = {};
            queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            queueDesc.NodeMask = 0;
            m_pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pDirectQueue));
            SetName(m_pDirectQueue, "DirectQueue");
        }
        {
            D3D12_COMMAND_QUEUE_DESC queueDesc = {};
            queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
            queueDesc.NodeMask = 0;
            m_pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pComputeQueue));
            SetName(m_pComputeQueue, "ComputeQueue");
        }
    }

    void Device::CreatePipelineCache()
    {
    }

    void Device::DestroyPipelineCache()
    {
    }

    void Device::OnDestroy()
    {
        m_pDirectQueue->Release();
        m_pComputeQueue->Release();
        m_pAdapter->Release();

        if (m_agsContext)
        {
            agsDriverExtensionsDX12_DestroyDevice(m_agsContext, m_pDevice, nullptr);
            agsDeInit(m_agsContext);
            m_agsContext = nullptr;
        }
        else
        {
            m_pDevice->Release();
        }

#ifdef _DEBUG
        // Report live objects
        {
            IDXGIDebug1 *pDxgiDebug;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDxgiDebug))))
            {
                pDxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
                pDxgiDebug->Release();
            }
        }
#endif
    }

    void Device::GPUFlush()
    {
        // sync direct queue
        {
            ID3D12Fence* pFence;
            ThrowIfFailed(m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence)));

            ThrowIfFailed(m_pDirectQueue->Signal(pFence, 1));

            HANDLE mHandleFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            pFence->SetEventOnCompletion(1, mHandleFenceEvent);
            WaitForSingleObject(mHandleFenceEvent, INFINITE);
            CloseHandle(mHandleFenceEvent);

            pFence->Release();
        }
        // sync compute queue
        {
            ID3D12Fence* pFence;
            ThrowIfFailed(m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence)));

            ThrowIfFailed(m_pComputeQueue->Signal(pFence, 1));

            HANDLE mHandleFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            pFence->SetEventOnCompletion(1, mHandleFenceEvent);
            WaitForSingleObject(mHandleFenceEvent, INFINITE);
            CloseHandle(mHandleFenceEvent);

            pFence->Release();
        }
    }
}
