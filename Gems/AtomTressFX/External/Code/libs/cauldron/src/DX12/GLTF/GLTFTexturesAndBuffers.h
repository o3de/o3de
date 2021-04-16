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

#include "Base/Texture.h"
#include "Base/UploadHeap.h"
#include "Base/StaticBufferPool.h"
#include "Base/DynamicBufferRing.h" 
#include "GLTF/GltfCommon.h"

namespace CAULDRON_DX12
{
    // This class takes a GltfCommon class (that holds all the non-GPU specific data) as an input and loads all the GPU specific data
    //
    struct Geometry
    {
        DXGI_FORMAT m_indexType;
        uint32_t m_NumIndices;
        D3D12_INDEX_BUFFER_VIEW m_IBV;
        std::vector<D3D12_VERTEX_BUFFER_VIEW> m_VBV;
    };

    class GLTFTexturesAndBuffers
    {
        Device     *m_pDevice;
        UploadHeap *m_pUploadHeap;

        const json::array_t *m_pTextureNodes;

        std::vector<Texture> m_textures;

        std::map<int, D3D12_GPU_VIRTUAL_ADDRESS> m_skeletonMatricesBuffer;
        std::vector<D3D12_CONSTANT_BUFFER_VIEW_DESC> m_InverseBindMatrices;

        StaticBufferPool *m_pStaticBufferPool;
        DynamicBufferRing *m_pDynamicBufferRing;

    public:
        GLTFCommon *m_pGLTFCommon;

        D3D12_GPU_VIRTUAL_ADDRESS m_perFrameConstants;

        bool OnCreate(Device* pDevice, GLTFCommon *pGLTFCommon, UploadHeap* pUploadHeap, StaticBufferPool *pStaticBufferPool, DynamicBufferRing *pDynamicBufferRing);
        void LoadTextures();
        void OnDestroy();

        void CreateIndexBuffer(tfAccessor indexBuffer, uint32_t *pNumIndices, DXGI_FORMAT *pIndexType, D3D12_INDEX_BUFFER_VIEW *pIBV);
        void CreateGeometry(tfAccessor indexBuffer, std::vector<tfAccessor> &vertexBuffers, Geometry *pGeometry);

        void SetPerFrameConstants();
        void SetSkinningMatricesForSkeletons();

        Texture *GetTextureViewByID(int id);
        D3D12_GPU_VIRTUAL_ADDRESS GetSkinningMatricesBuffer(int skinIndex);
        D3D12_GPU_VIRTUAL_ADDRESS GetPerFrameConstants() { return m_perFrameConstants; }
    };
}