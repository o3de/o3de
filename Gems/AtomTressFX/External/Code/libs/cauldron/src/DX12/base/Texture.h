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
#pragma once

#include "ResourceViewHeaps.h"
#include "UploadHeap.h"
#include "Misc/ImgLoader.h"

namespace CAULDRON_DX12
{
    // This class provides functionality to create a 2D-texture from a DDS or any texture format from WIC file.
    class Texture
    {
    public:
        Texture();
        virtual                 ~Texture();
        virtual void            OnDestroy();

        // different ways to init a texture
        virtual bool InitFromFile(Device* pDevice, UploadHeap* pUploadHeap, const char *szFilename, bool useSRGB = false, float cutOff = 1.0f);
        INT32 Init(Device* pDevice, const char *pDebugName, const CD3DX12_RESOURCE_DESC *pDesc, D3D12_RESOURCE_STATES initialState, const D3D12_CLEAR_VALUE *pClearValue);
        INT32 InitRenderTarget(Device* pDevice, const char *pDebugName, const CD3DX12_RESOURCE_DESC *pDesc, D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_RENDER_TARGET);
        INT32 InitDepthStencil(Device* pDevice, const char *pDebugName, const CD3DX12_RESOURCE_DESC *pDesc);
        bool InitBuffer(Device* pDevice, const char *pDebugName, const CD3DX12_RESOURCE_DESC *pDesc, uint32_t structureSize, D3D12_RESOURCE_STATES state);     // structureSize needs to be 0 if using a valid DXGI_FORMAT
        bool InitCounter(Device* pDevice, const char *pDebugName, const CD3DX12_RESOURCE_DESC *pCounterDesc, uint32_t counterSize, D3D12_RESOURCE_STATES state);
        bool InitFromData(Device* pDevice, const char *pDebugName, UploadHeap& uploadHeap, const IMG_INFO& header, const void* data);

        // explicit functions for creating RTVs, SRVs and UAVs
        void CreateRTV(uint32_t index, RTV *pRV, D3D12_RENDER_TARGET_VIEW_DESC *pRtvDesc);
        void CreateSRV(uint32_t index, CBV_SRV_UAV *pRV, D3D12_SHADER_RESOURCE_VIEW_DESC *pSrvDesc);
        void CreateUAV(uint32_t index, Texture *pCounterTex, CBV_SRV_UAV *pRV, D3D12_UNORDERED_ACCESS_VIEW_DESC *pUavDesc);

        // less explicit functions of the above ones
        void CreateDSV(uint32_t index, DSV *pRV, int arraySlice = 1);
        void CreateUAV(uint32_t index, CBV_SRV_UAV *pRV, int mipLevel = -1);
        void CreateBufferUAV(uint32_t index, Texture *pCounterTex, CBV_SRV_UAV *pRV);
        void CreateRTV(uint32_t index, RTV *pRV, int mipLevel = -1, int arraySize = -1, int firstArraySlice = -1);
        void CreateSRV(uint32_t index, CBV_SRV_UAV *pRV, int mipLevel = -1, int arraySize = -1, int firstArraySlice = -1);        
        void CreateCubeSRV(uint32_t index, CBV_SRV_UAV *pRV);

        // accessors
        uint32_t GetWidth() const { return m_header.width; }
        uint32_t GetHeight() const { return m_header.height; }
        DXGI_FORMAT GetFormat() const { return m_header.format; }
        ID3D12Resource* GetResource() const { return m_pResource; }
        uint32_t GetMipCount() const { return m_header.mipMapCount; }
        uint32_t GetArraySize() const { return m_header.arraySize; }

    protected:
        void CreateTextureCommitted(Device* pDevice, const char *pDebugName, bool useSRGB = false);
        void LoadAndUpload(Device* pDevice, UploadHeap* pUploadHeap, ImgLoader *pDds, ID3D12Resource *pRes);

        ID3D12Resource*         m_pResource = nullptr;

        IMG_INFO                m_header = {};
        uint32_t                m_structuredBufferStride = 0;

        bool                    isDXT(DXGI_FORMAT format)const;
        bool                    isCubemap()const;
    };
}
