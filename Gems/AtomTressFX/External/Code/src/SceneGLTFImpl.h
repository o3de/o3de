// Copyright(c) 2019 Advanced Micro Devices, Inc.All rights reserved.
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

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

// Windows Header Files:
#include <windows.h>
#include <windowsx.h>
#include <DirectXMath.h>
using namespace DirectX;
class GLTFCommon;

// C RunTime Header Files
#include <malloc.h>
#include <map>
#include <vector>
#include <mutex>
#include <fstream>
#include <memory>
#include <cassert>
#include "GLTF/GLTFTexturesAndBuffers.h"
#include "Base/DynamicBufferRing.h"
#include "Base/StaticBufferPool.h"
#include "Base/ShaderCompiler.h"
#include "GLTF/GltfPbrPass.h"
#include "GLTF/GltfDepthPass.h"

#include "TressFXCommon.h"

struct EI_RenderTargetSet;

class EI_Scene
{
public:

    struct State
    {
        float time;
        Camera camera;

        float iblFactor;
        float emmisiveFactor;
    };

    EI_Scene() : m_pGLTFTexturesAndBuffers(nullptr), m_pGLTFCommon(nullptr) {}
    ~EI_Scene() { OnDestroy(); }
    void            OnCreate(EI_Device* device, EI_RenderTargetSet* renderTargetSet, EI_RenderTargetSet* shadowRenderTargetSet, const std::string& path, const std::string& fileName, const std::string& bonePrefix, float startOffset);
    void            OnDestroy();
    void            OnBeginFrame(float deltaTime, float aspect);
    void            OnRender();
    void            OnRenderLight(uint32_t LightIndex);
    void            OnResize(uint32_t width, uint32_t height);

    uint32_t        GetSceneLightCount() const { return static_cast<uint32_t>(m_SceneLightInfo.size()); }
    const Light& GetSceneLightInfo(uint32_t Index) const { return m_SceneLightInfo[Index]; }

    AMD::float4x4 GetMV() const { return *(AMD::float4x4*)&m_state.camera.GetView(); }
    AMD::float4x4 GetMVP() const { return *(AMD::float4x4*)&(m_state.camera.GetView() * m_state.camera.GetProjection()); }
    AMD::float4x4 GetInvViewProjMatrix() const { return *(AMD::float4x4*) & (XMMatrixInverse(nullptr, m_state.camera.GetView() * m_state.camera.GetProjection())); }
    AMD::float4 GetCameraPos() const { return *(AMD::float4*)&m_state.camera.GetPosition(); }
    std::vector<XMMATRIX> GetWorldSpaceSkeletonMats(int skinNumber) const { return m_pGLTFCommon->m_pCurrentFrameTransformedData->m_worldSpaceSkeletonMats[skinNumber]; }

    int GetBoneIdByName(int skinNumber, const char * name);

private:
    void ComputeGlobalIdxToSkinIdx();
    EI_Device* m_pDevice;
    std::unique_ptr<EI_GLTFTexturesAndBuffers>	m_pGLTFTexturesAndBuffers;

    GLTFCommon*							m_pGLTFCommon;

    std::unique_ptr<EI_GltfPbrPass>		m_gltfPBR;
    std::unique_ptr<EI_GltfDepthPass>	m_gltfDepth;
    
    State                               m_state;

    std::string m_bonePrefix;
    float m_startOffset = 0;

    // cache inverse index mapping because cauldron doesnt do that
    std::vector<std::vector<int>> m_globalIdxToSkinIdx;

    std::vector<Light>                           m_SceneLightInfo;

    float m_animationTime;
    float m_roll;
    float m_pitch;
    float m_distance;
};
