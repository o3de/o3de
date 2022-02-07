// ----------------------------------------------------------------------------
// Compute-based skinning.
// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#ifndef TRESSFXBONESKINNING_H_
#define TRESSFXBONESKINNING_H_

// TressFX interfaces
#include "TressFXSDFInputMeshInterface.h"

// TressFX
#include "TressFXAsset.h"
#include "TressFXCommon.h"
#include "AMD_Types.h"

#include <vector>

class EI_Scene;

struct TressFXBoneSkinningUniformBuffer {
    AMD::float4 cColor;
    AMD::float4 vLightDir;
    AMD::sint4 g_NumMeshVertices;
    AMD::float4x4 mMW;
    AMD::float4x4 mMVP;
    AMD::float4x4 g_BoneSkinningMatrix[AMD_TRESSFX_MAX_NUM_BONES];
};

// Adi: this structure is required to for computing the per-frame SDF
class TressFXBoneSkinning : public TressFXSDFInputMeshInterface
{
public:

    // Initialize effects and buffers.
    void Initialize(EI_RenderTargetSet * renderPass, EI_Device* pDevice, EI_CommandContext& commandContext, const char * name);

    // Draw the mesh for debug purpose
    void DrawMesh(EI_CommandContext& commandContext);

    // Update and animate the mesh
    void Update(EI_CommandContext& commandContext, double fTime);

    Vector3 SkinPosition(int i);

    //inline SuAnimatedModel* GetModel() { return m_pModel; }

    bool LoadTressFXCollisionMeshData(EI_Scene * scene, const char* filePath, int skinNumber, const char* followBone);

    // TressFXSDFInputMeshInterface
    virtual EI_Resource& GetMeshBuffer() { return *m_CollMeshVertPositionsUAV; }
    virtual EI_Resource&  GetTrimeshVertexIndicesBuffer()
    {
        return *m_TrimeshVertexIndicesSRV;
    }
    virtual int  GetNumMeshVertices() { return m_NumVertices; }
    virtual int  GetNumMeshTriangle() { return m_NumTriangles; }
    virtual void GetBoundingBox(Vector3& min, Vector3& max);
    virtual void GetInitialBoundingBox(Vector3& min, Vector3& max);
    virtual size_t GetSizeOfMeshElement()
    {
        return 4 * sizeof(float) + 4 * sizeof(float);
    }  // position + normal

private:
    EI_Scene * m_pScene;

    std::vector<AMD::float3> m_pTempVertices;
    std::vector<AMD::float3> m_pTempNormals;
    std::vector<int>                  m_pTempIndices;

    std::vector<TressFXBoneSkinningData> boneSkinningData;

    // shader for rendering
    //SuEffectPtr m_pRenderEffect;
    std::unique_ptr<EI_PSO> m_pRenderEffect;

    // compute shader for bone skinning
    std::unique_ptr<EI_PSO> m_pComputeEffectSkinning;

    // color to render the mesh for debug purpose
    AMD::float4 m_MeshColor;

    // Stats for mesh topology
    AMD::uint32 m_NumVertices;
    AMD::uint32 m_NumTriangles;  // the size of indices buffer is 3 time of this.

    // UAV
    std::unique_ptr<EI_Resource> m_CollMeshVertPositionsUAV;

    // SRV
    std::unique_ptr<EI_Resource> m_TrimeshVertexIndicesSRV;
    std::unique_ptr<EI_Resource> m_BoneSkinningDataSRV;
    std::unique_ptr<EI_Resource> m_InitialVertexPositionsSRV;

    // uniform buffers
    std::unique_ptr<EI_Resource> m_pUniformBuffer;
    TressFXBoneSkinningUniformBuffer m_uniformBufferData;

    // Binding
    std::unique_ptr<EI_BindSet> m_pBindSet;

    // bounding box
    Vector3 m_bbMin;
    Vector3 m_bbMax;

    // GPU buffers
    std::unique_ptr<EI_Resource> m_pIndexBuffer;
    int m_numIndices;

    int m_skinNumber;
    int m_followBone;

    // animated model associated to this hair.
    //SuAnimatedModel* m_pModel;
};

#endif  // TRESSFXBONESKINNING_H_