/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */
#pragma once

#include <IRenderer.h>
#include <MathConversion.h>

namespace UnitTest
{
    class CryRenderMeshStub
        : public IRenderMesh
    {
    public:
        explicit CryRenderMeshStub(const AZStd::vector<AZ::Vector3>& vertices)
        {
            m_positions.reserve(vertices.size());
            for (const auto& vertex : vertices)
            {
                m_positions.emplace_back(AZVec3ToLYVec3(vertex));
            }
        }

        int GetNumVerts() const override
        {
            return static_cast<int>(m_positions.size());
        }

        byte* GetPosPtr(int32& nStride, uint32 /*nFlags*/) override
        {
            nStride = sizeof(Vec3);
            return reinterpret_cast<byte*>(m_positions.data());
        }

        AZStd::vector<Vec3> m_positions;

        // ----------------------------------------
        // IRenderMesh unused functions ...
        void AddRef() override {}
        int Release() override { return 0; }
        bool CanRender() override { return false; }
        const char* GetTypeName() override { return ""; }
        const char* GetSourceName() const override { return ""; }
        int  GetIndicesCount() override { return 0; }
        int  GetVerticesCount() override { return 0; }
        AZ::Vertex::Format GetVertexFormat() override { return {}; }
        ERenderMeshType GetMeshType() override { return eRMT_Dynamic;  }
        float GetGeometricMeanFaceArea() const override { return 0.0f; }
        bool CheckUpdate(uint32 /*nStreamMask*/) override { return false; }
        int GetStreamStride(int /*nStream*/) const override { return 0; }
        const uintptr_t GetVBStream(int /*nStream*/) const override { return 0; }
        const uintptr_t GetIBStream() const override { return 0; }
        int GetNumInds() const override { return 0; }
        const eRenderPrimitiveType GetPrimitiveType() const override { return static_cast<eRenderPrimitiveType>(0); }
        void SetSkinned(bool /*bSkinned*/ = true) override {}
        uint GetSkinningWeightCount() const override { return 0; }
        size_t SetMesh(CMesh& /*mesh*/, int /*nSecColorsSetOffset*/, uint32 /*flags*/, bool /*requiresLock*/) override { return 0; }
        void CopyTo(IRenderMesh* /*pDst*/, int /*nAppendVtx*/ = 0, bool /*bDynamic*/ = false, bool /*fullCopy*/ = true) override {}
        void SetSkinningDataVegetation(struct SMeshBoneMapping_uint8* /*pBoneMapping*/) override {}
        void SetSkinningDataCharacter(CMesh& /*mesh*/, struct SMeshBoneMapping_uint16* /*pBoneMapping*/, struct SMeshBoneMapping_uint16* /*pExtraBoneMapping*/) override {}
        IIndexedMesh* GetIndexedMesh(IIndexedMesh* /*pIdxMesh*/ = 0) override { return nullptr; }
        int GetRenderChunksCount(_smart_ptr<IMaterial> /*pMat*/, int& /*nRenderTrisCount*/) override { return 0; }
        IRenderMesh* GenerateMorphWeights() override { return nullptr; }
        IRenderMesh* GetMorphBuddy() override { return nullptr; }
        void SetMorphBuddy(IRenderMesh* /*pMorph*/) override {}
        bool UpdateVertices(const void* /*pVertBuffer*/, int /*nVertCount*/, int /*nOffset*/, int /*nStream*/, uint32 /*copyFlags*/, bool /*requiresLock*/ = true) override { return false; }
        bool UpdateIndices(const vtx_idx* /*pNewInds*/, int /*nInds*/, int /*nOffsInd*/, uint32 /*copyFlags*/, bool /*requiresLock*/ = true) override { return false; }
        void SetCustomTexID(int /*nCustomTID*/) override {}
        void SetChunk(int /*nIndex*/, CRenderChunk& /*chunk*/) override {}
        void SetChunk(_smart_ptr<IMaterial> /*pNewMat*/, int /*nFirstVertId*/, int /*nVertCount*/, int /*nFirstIndexId*/, int /*nIndexCount*/, float /*texelAreaDensity*/, const AZ::Vertex::Format& /*vertexFormat*/, int /*nMatID*/ = 0) override {}
        void SetRenderChunks(CRenderChunk* /*pChunksArray*/, int /*nCount*/, bool /*bSubObjectChunks*/) override {}
        void GenerateQTangents() override {}
        void CreateChunksSkinned() override {}
        void NextDrawSkinned() override {}
        IRenderMesh* GetVertexContainer() override { return nullptr; }
        void SetVertexContainer(IRenderMesh* /*pBuf*/) override {}
        TRenderChunkArray m_chunk;
        TRenderChunkArray& GetChunks() override { return m_chunk; }
        TRenderChunkArray& GetChunksSkinned() override { return m_chunk; }
        TRenderChunkArray& GetChunksSubObjects() override { return m_chunk; }
        void SetBBox(const Vec3& /*vBoxMin*/, const Vec3& /*vBoxMax*/) override {}
        void GetBBox(Vec3& /*vBoxMin*/, Vec3& /*vBoxMax*/) override {}
        void UpdateBBoxFromMesh() override {}
        uint32* GetPhysVertexMap() override { return nullptr; }
        bool IsEmpty() override { return false; }
        byte* GetPosPtrNoCache(int32& /*nStride*/, uint32 /*nFlags*/) override { return nullptr; }
        byte* GetColorPtr(int32& /*nStride*/, uint32 /*nFlags*/) override { return nullptr; }
        byte* GetNormPtr(int32& /*nStride*/, uint32 /*nFlags*/) override { return nullptr; }
        byte* GetUVPtrNoCache(int32& /*nStride*/, uint32 /*nFlags*/, uint32 /*uvSetIndex*/ = 0) override { return nullptr; }
        byte* GetUVPtr(int32& /*nStride*/, uint32 /*nFlags*/, uint32 /*uvSetIndex*/ = 0) override { return nullptr; }
        byte* GetTangentPtr(int32& /*nStride*/, uint32 /*nFlags*/) override { return nullptr; }
        byte* GetQTangentPtr(int32& /*nStride*/, uint32 /*nFlags*/) override { return nullptr; }
        byte* GetHWSkinPtr(int32& /*nStride*/, uint32 /*nFlags*/, bool /*remapped*/ = false) override { return nullptr; }
        byte* GetVelocityPtr(int32& /*nStride*/, uint32 /*nFlags*/) override { return nullptr; }
        void UnlockStream(int /*nStream*/) override {}
        void UnlockIndexStream() override {}
        vtx_idx* GetIndexPtr(uint32 /*nFlags*/, int32 /*nOffset*/ = 0) override { return nullptr; }
        const PodArray<std::pair<int, int> >* GetTrisForPosition(const Vec3& /*vPos*/, _smart_ptr<IMaterial> /*pMaterial*/) override { return nullptr; }
        float GetExtent(EGeomForm /*eForm*/) override { return 0.0f; }
        void GetRandomPos(PosNorm& /*ran*/, EGeomForm /*eForm*/, SSkinningData const* /*pSkinning*/ = NULL) override {}
        void Render(const struct SRendParams& /*rParams*/, CRenderObject* /*pObj*/, _smart_ptr<IMaterial> /*pMaterial*/, const SRenderingPassInfo& /*passInfo*/, bool /*bSkinned*/ = false) override {}
        void Render(CRenderObject* /*pObj*/, const SRenderingPassInfo& /*passInfo*/, const SRendItemSorter& /*rendItemSorter*/) override {}
        void AddRenderElements(_smart_ptr<IMaterial> /*pIMatInfo*/, CRenderObject* /*pObj*/, const SRenderingPassInfo& /*passInfo*/, int /*nSortId*/ = EFSLIST_GENERAL, int /*nAW*/ = 1) override {}
        void AddRE(_smart_ptr<IMaterial> /*pMaterial*/, CRenderObject* /*pObj*/, IShader* /*pEf*/, const SRenderingPassInfo& /*passInfo*/, int /*nList*/, int /*nAW*/, const SRendItemSorter& /*rendItemSorter*/) override {}
        void SetREUserData(float* /*pfCustomData*/, float /*fFogScale*/ = 0, float /*fAlpha*/ = 1) override {}
        void DebugDraw(const struct SGeometryDebugDrawInfo& /*info*/, uint32 /*nVisibleChunksMask*/ = ~0, float /*fExtrdueScale*/ = 0.01f) override {}
        size_t GetMemoryUsage(ICrySizer* /*pSizer*/, EMemoryUsageArgument /*nType*/) const override { return 0; }
        void GetMemoryUsage(ICrySizer* /*pSizer*/) const override {}
        int GetAllocatedBytes(bool /*bVideoMem*/) const override { return 0; }
        float GetAverageTrisNumPerChunk(_smart_ptr<IMaterial> /*pMat*/) override { return 0.0f; }
        int GetTextureMemoryUsage(const _smart_ptr<IMaterial> /*pMaterial*/, ICrySizer* /*pSizer*/ = NULL, bool /*bStreamedIn*/ = true) const override { return 0; }
        void KeepSysMesh(bool /*keep*/) override {} 
        void UnKeepSysMesh() override {}
        void SetMeshLod(int /*nLod*/) override {}
        void LockForThreadAccess() override {}
        void UnLockForThreadAccess() override {}
        volatile int* SetAsyncUpdateState(void) override { return nullptr; }
        void CreateRemappedBoneIndicesPair(const DynArray<JointIdType>& /*arrRemapTable*/, const uint /*pairGuid*/) override {}
        void ReleaseRemappedBoneIndicesPair(const uint /*pairGuid*/) override {}
        void OffsetPosition(const Vec3& /*delta*/) override {}
    };
} // namespace UnitTest
