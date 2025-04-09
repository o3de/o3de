/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/RayTracing/RayTracingFeatureProcessorInterface.h>
#include <Atom/Feature/RayTracing/RayTracingIndexList.h>
#include <Atom/Feature/TransformService/TransformServiceFeatureProcessorInterface.h>
#include <Atom/RHI/DeviceBufferView.h>
#include <Atom/RHI/DeviceImageView.h>
#include <Atom/RHI/IndexBufferView.h>
#include <Atom/RHI/RayTracingAccelerationStructure.h>
#include <Atom/RHI/RayTracingBufferPools.h>
#include <Atom/RHI/RayTracingCompactionQueryPool.h>
#include <Atom/RHI/StreamBufferView.h>
#include <Atom/RPI.Public/Buffer/RingBuffer.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Reflect/Image/Image.h>
#include <Atom/Utils/StableDynamicArray.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Transform.h>

// this define specifies that the mesh buffers and material textures are stored in the Bindless Srg
// Note1: The previous implementation using separate unbounded arrays is preserved since it demonstrates a TDR caused by
//        the RHI unbounded array allocation.  This define and the previous codepath can be removed once the TDR is
//        investigated and resolved.
// Note2: There are corresponding USE_BINDLESS_SRG defines in the RayTracingSceneSrg.azsli and RayTracingMaterialSrg.azsli
//        shader files that must match the setting of this define.
#define USE_BINDLESS_SRG 1

namespace AZ
{
    namespace Render
    {
        //! This feature processor manages ray tracing data for a Scene
        class RayTracingFeatureProcessor
            : public RayTracingFeatureProcessorInterface
        {
        public:
            AZ_CLASS_ALLOCATOR(RayTracingFeatureProcessor, AZ::SystemAllocator)

            AZ_RTTI(AZ::Render::RayTracingFeatureProcessor, "{5017EFD3-A996-44B0-9ED2-C47609A2EE8D}", AZ::Render::RayTracingFeatureProcessorInterface);

            static void Reflect(AZ::ReflectContext* context);

            RayTracingFeatureProcessor() = default;
            virtual ~RayTracingFeatureProcessor() = default;

            // FeatureProcessor overrides
            void Activate() override;
            void Deactivate() override;
            void OnRenderPipelineChanged(RPI::RenderPipeline* renderPipeline, RPI::SceneNotification::RenderPipelineChangeType changeType) override;

            // RayTracingFeatureProcessorInterface overrides
            ProceduralGeometryTypeHandle RegisterProceduralGeometryType(
                const AZStd::string& name,
                const Data::Instance<RPI::Shader>& intersectionShader,
                const AZStd::string& intersectionShaderName,
                const AZStd::unordered_map<int, uint32_t>& bindlessBufferIndices = {}) override;
            void SetProceduralGeometryTypeBindlessBufferIndex(
                ProceduralGeometryTypeWeakHandle geometryTypeHandle, const AZStd::unordered_map<int, uint32_t>& bindlessBufferIndices) override;
            void AddProceduralGeometry(
                ProceduralGeometryTypeWeakHandle geometryTypeHandle,
                const Uuid& uuid,
                const Aabb& aabb,
                const SubMeshMaterial& material,
                RHI::RayTracingAccelerationStructureInstanceInclusionMask instanceMask,
                uint32_t localInstanceIndex) override;
            void SetProceduralGeometryTransform(
                const Uuid& uuid, const Transform& transform, const Vector3& nonUniformScale = Vector3::CreateOne()) override;
            void SetProceduralGeometryLocalInstanceIndex(const Uuid& uuid, uint32_t localInstanceIndex) override;
            void SetProceduralGeometryMaterial(const Uuid& uuid, const SubMeshMaterial& material) override;
            void RemoveProceduralGeometry(const Uuid& uuid) override;
            int GetProceduralGeometryCount(ProceduralGeometryTypeWeakHandle geometryTypeHandle) const override;
            const ProceduralGeometryTypeList& GetProceduralGeometryTypes() const override { return m_proceduralGeometryTypes; }
            const ProceduralGeometryList& GetProceduralGeometries() const override { return m_proceduralGeometry; }

            void AddMesh(const AZ::Uuid& uuid, const Mesh& rayTracingMesh, const SubMeshVector& subMeshes) override;
            void RemoveMesh(const AZ::Uuid& uuid) override;
            void SetMeshTransform(const AZ::Uuid& uuid, const AZ::Transform transform, const AZ::Vector3 nonUniformScale) override;
            void SetMeshReflectionProbe(const AZ::Uuid& uuid, const Mesh::ReflectionProbe& reflectionProbe) override;
            void SetMeshMaterials(const AZ::Uuid& uuid, const SubMeshMaterialVector& subMeshMaterials) override;
            const SubMeshVector& GetSubMeshes() const override { return m_subMeshes; }
            SubMeshVector& GetSubMeshes() override { return m_subMeshes; }
            const MeshMap& GetMeshMap() override { return m_meshes; }
            uint32_t GetSubMeshCount() const override { return m_subMeshCount; }
            uint32_t GetSkinnedMeshCount() const override { return m_skinnedMeshCount; }

            Data::Instance<RPI::ShaderResourceGroup> GetRayTracingSceneSrg() const override { return m_rayTracingSceneSrg; }
            Data::Instance<RPI::ShaderResourceGroup> GetRayTracingMaterialSrg() const override { return m_rayTracingMaterialSrg; }
            const Data::Instance<RPI::Buffer> GetMeshInfoGpuBuffer() const override { return m_meshInfoGpuBuffer.GetCurrentBuffer(); }
            const Data::Instance<RPI::Buffer> GetMaterialInfoGpuBuffer() const override { return m_materialInfoGpuBuffer.GetCurrentBuffer(); }
            void Render(const RenderPacket&) override;
            void BeginFrame() override;
            uint32_t GetRevision() const override { return m_revision; }
            uint32_t GetProceduralGeometryTypeRevision() const override { return m_proceduralGeometryTypeRevision; }
            RHI::RayTracingBufferPools& GetBufferPools() override { return *m_bufferPools; }
            void UpdateRayTracingSrgs() override;

            const RHI::Ptr<RHI::RayTracingTlas>& GetTlas() const override { return m_tlas; }
            RHI::Ptr<RHI::RayTracingTlas>& GetTlas()  override{ return m_tlas; }
            RHI::AttachmentId GetTlasAttachmentId() const override { return m_tlasAttachmentId; }
            BlasInstanceMap& GetBlasInstances() override { return m_blasInstanceMap; }
            AZStd::mutex& GetBlasBuiltMutex() override { return m_blasBuiltMutex; }
            BlasBuildList& GetBlasBuildList(int deviceIndex) override { return m_blasToBuild[deviceIndex]; }
            const BlasBuildList& GetSkinnedMeshBlasList() override { return m_skinnedBlasIds; };
            BlasBuildList& GetBlasCompactionList(int deviceIndex) override { return m_blasToCompact[deviceIndex]; }
            const void MarkBlasInstanceForCompaction(int deviceIndex, Data::AssetId assetId) override;
            const void MarkBlasInstanceAsCompactionEnqueued(int deviceIndex, Data::AssetId assetId) override;

            bool HasMeshGeometry() const override { return m_subMeshCount != 0; }
            bool HasProceduralGeometry() const override { return !m_proceduralGeometry.empty(); }
            bool HasGeometry() const override { return HasMeshGeometry() || HasProceduralGeometry(); }

        private:
            AZ_DISABLE_COPY_MOVE(RayTracingFeatureProcessor);

            void UpdateBlasInstances();
            void UpdateMeshInfoBuffer();
            void UpdateProceduralGeometryInfoBuffer();
            void UpdateMaterialInfoBuffer();
            void UpdateIndexLists();
            void UpdateRayTracingSceneSrg();
            void UpdateRayTracingMaterialSrg();
            void RemoveBlasInstance(Data::AssetId id);

            static RHI::RayTracingAccelerationStructureBuildFlags CreateRayTracingAccelerationStructureBuildFlags(bool isSkinnedMesh);

            // flag indicating if RayTracing is enabled, currently based on device support
            bool m_rayTracingEnabled = false;

            // mesh data for meshes that should be included in ray tracing operations,
            // this is a map of the mesh UUID to the ray tracing data for the sub-meshes
            MeshMap m_meshes;
            SubMeshVector m_subMeshes;

            // buffer pools used in ray tracing operations
            RHI::Ptr<RHI::RayTracingBufferPools> m_bufferPools;

            // ray tracing acceleration structure (TLAS)
            RHI::Ptr<RHI::RayTracingTlas> m_tlas;

            // RayTracingScene and RayTracingMaterial asset and Srgs
            Data::Asset<RPI::ShaderAsset> m_rayTracingSrgAsset;
            Data::Instance<RPI::ShaderResourceGroup> m_rayTracingSceneSrg;
            Data::Instance<RPI::ShaderResourceGroup> m_rayTracingMaterialSrg;

            // current revision number of ray tracing data
            uint32_t m_revision = 0;

            // latest tlas revision number
            uint32_t m_tlasRevision = 0;

            uint32_t m_proceduralGeometryTypeRevision = 0;

            // total number of ray tracing sub-meshes
            uint32_t m_subMeshCount = 0;

            // TLAS attachmentId
            RHI::AttachmentId m_tlasAttachmentId;

            // cached TransformServiceFeatureProcessor
            TransformServiceFeatureProcessorInterface* m_transformServiceFeatureProcessor = nullptr;

            // mutex for the mesh and BLAS lists
            AZStd::mutex m_mutex;

            // mutex for the m_blasBuilt flag manipulation
            AZStd::mutex m_blasBuiltMutex;

            // structure for data in the m_meshInfoBuffer, shaders that use the buffer must match this type
            struct MeshInfo
            {
                // byte offsets into the mesh buffer views
                uint32_t m_indexByteOffset = 0;
                uint32_t m_positionByteOffset = 0;
                uint32_t m_normalByteOffset = 0;
                uint32_t m_tangentByteOffset = 0;
                uint32_t m_bitangentByteOffset = 0;
                uint32_t m_uvByteOffset = 0;

                RayTracingSubMeshBufferFlags m_bufferFlags = RayTracingSubMeshBufferFlags::None;
                uint32_t m_bufferStartIndex = 0;

                AZStd::array<float, 12> m_worldInvTranspose; // float3x4
            };

            // vector of MeshInfo, transferred to the meshInfoGpuBuffer
            using MeshInfoVector = AZStd::vector<MeshInfo>;
            AZStd::unordered_map<int, MeshInfoVector> m_meshInfos;
            RPI::RingBuffer m_meshInfoGpuBuffer{ "RayTracingMeshInfo", RPI::CommonBufferPoolType::ReadOnly, sizeof(MeshInfo) };
            RPI::RingBuffer m_proceduralGeometryInfoGpuBuffer{ "ProceduralGeometryInfo", RPI::CommonBufferPoolType::ReadOnly, RHI::Format::R32G32_UINT };

            // structure for data in the m_materialInfoBuffer, shaders that use the buffer must match this type
            struct alignas(16) MaterialInfo
            {
                AZStd::array<float, 4> m_baseColor;       // float4
                AZStd::array<float, 4> m_irradianceColor; // float4
                AZStd::array<float, 3> m_emissiveColor;   // float3
                float m_metallicFactor = 0.0f;
                float m_roughnessFactor = 0.0f;
                RayTracingSubMeshTextureFlags m_textureFlags = RayTracingSubMeshTextureFlags::None;
                uint32_t m_textureStartIndex = InvalidIndex;
                uint32_t m_reflectionProbeCubeMapIndex = InvalidIndex;

                // reflection probe data, must match the structure in ReflectionProbeData.azlsi
                struct alignas(16) ReflectionProbeData
                {
                    AZStd::array<float, 12> m_modelToWorld;        // float3x4
                    AZStd::array<float, 12> m_modelToWorldInverse; // float3x4
                    AZStd::array<float, 3>  m_outerObbHalfLengths; // float3
                    float m_exposure = 0.0f;
                    AZStd::array<float, 3>  m_innerObbHalfLengths; // float3
                    uint32_t m_useReflectionProbe = 0;
                    uint32_t m_useParallaxCorrection = 0;
                    AZStd::array<float, 3> m_padding;
                };
                ReflectionProbeData m_reflectionProbeData;
            };

            // vector of MaterialInfo, transferred to the materialInfoGpuBuffer
            using MaterialInfoVector = AZStd::vector<MaterialInfo>;
            AZStd::unordered_map<int, MaterialInfoVector> m_materialInfos;
            AZStd::unordered_map<int, MaterialInfoVector> m_proceduralGeometryMaterialInfos;
            RPI::RingBuffer m_materialInfoGpuBuffer{ "RayTracingMaterialInfo", RPI::CommonBufferPoolType::ReadOnly, sizeof(MaterialInfo) };

            // update flags
            bool m_meshInfoBufferNeedsUpdate = false;
            bool m_proceduralGeometryInfoBufferNeedsUpdate = false;
            bool m_materialInfoBufferNeedsUpdate = false;
            bool m_indexListNeedsUpdate = false;

            // side list for looking up existing BLAS objects so they can be re-used when the same mesh is added multiple times
            BlasInstanceMap m_blasInstanceMap;

            BlasBuildList m_blasToCreate;
            AZStd::unordered_map<int, BlasBuildList> m_blasToBuild;
            AZStd::unordered_map<int, BlasBuildList> m_blasToCompact;
            BlasBuildList m_skinnedBlasIds;

            // Mutex for the queues that are modified by the RaytracingAccelerationStructurePasses
            // The BuildCommandList of these passes are on different threads so we need lock access to the queues
            AZStd::mutex m_queueMutex;

            struct BlasFrameEvent
            {
                int m_frameIndex = -1;
                RHI::MultiDevice::DeviceMask m_deviceMask;
            };
            // List of Blas instances that are enqueued for compaction
            // The frame index corresponds to the frame where the compaction query is ready
            AZStd::unordered_map<Data::AssetId, BlasFrameEvent> m_blasEnqueuedForCompact;

            // List of Blas instances where the compacted Blas is already built
            // The frame index corresponds to the frame where the uncompacted Blas is no longer needed and can be deleted
            AZStd::unordered_map<Data::AssetId, BlasFrameEvent> m_uncompactedBlasEnqueuedForDeletion;

            int m_frameIndex = 0;
            int m_updatedFrameIndex = 0;

#if !USE_BINDLESS_SRG
            // Mesh buffer and material texture resources are managed with a RayTracingResourceList, which contains an internal
            // indirection list.  This allows resource entries to be swapped inside the RayTracingResourceList when removing entries,
            // without invalidating the indices held here in the m_meshBufferIndices and m_materialTextureIndices lists.
            
            // mesh buffer and material texture resource lists, accessed by the shader through an unbounded array
            RayTracingResourceList<RHI::BufferView> m_meshBuffers;
            RayTracingResourceList<const RHI::ImageView> m_materialTextures;
#endif

            // RayTracingIndexList implements an internal freelist chain stored inside the list itself, allowing entries to be
            // reused after elements are removed.

            // mesh buffer and material texture index lists, which contain the array indices of the mesh resources
            static const uint32_t NumMeshBuffersPerMesh = 6;
            AZStd::unordered_map<int, RayTracingIndexList<NumMeshBuffersPerMesh>> m_meshBufferIndices;

            static const uint32_t NumMaterialTexturesPerMesh = 5;
            AZStd::unordered_map<int, RayTracingIndexList<NumMaterialTexturesPerMesh>> m_materialTextureIndices;

            // Gpu buffers for the mesh and material index lists
            RPI::RingBuffer m_meshBufferIndicesGpuBuffer{ "RayTracingMeshBufferIndices", RPI::CommonBufferPoolType::ReadOnly, RHI::Format::R32_UINT };
            RPI::RingBuffer m_materialTextureIndicesGpuBuffer{ "RayTracingMaterialTextureIndices", RPI::CommonBufferPoolType::ReadOnly, RHI::Format::R32_UINT };

            uint32_t m_skinnedMeshCount = 0;

            ProceduralGeometryTypeList m_proceduralGeometryTypes;
            ProceduralGeometryList m_proceduralGeometry;
            AZStd::unordered_map<Uuid, size_t> m_proceduralGeometryLookup;

            RHI::Ptr<RHI::RayTracingCompactionQueryPool> m_compactionQueryPool;

            void ConvertMaterial(MaterialInfo& materialInfo, const SubMeshMaterial& subMeshMaterial, int deviceIndex);
        };
    }
}
