/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Mesh/MeshInfoBus.h>
#include <Atom/Feature/RayTracing/RayTracingFeatureProcessorInterface.h>
#include <Atom/Feature/RayTracing/RayTracingIndexList.h>
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
#include <RayTracing/RayTracingResourceList.h>

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

        // forward declaration
        class MeshFeatureProcessorInterface;
        class TransformServiceFeatureProcessorInterface;

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
                const FallbackPBR::MaterialParameters& material,
                RHI::RayTracingAccelerationStructureInstanceInclusionMask instanceMask,
                uint32_t localInstanceIndex) override;
            void SetProceduralGeometryTransform(
                const Uuid& uuid, const Transform& transform, const Vector3& nonUniformScale = Vector3::CreateOne()) override;
            void SetProceduralGeometryLocalInstanceIndex(const Uuid& uuid, uint32_t localInstanceIndex) override;
            void RemoveProceduralGeometry(const Uuid& uuid) override;
            int GetProceduralGeometryCount(ProceduralGeometryTypeWeakHandle geometryTypeHandle) const override;
            const ProceduralGeometryTypeList& GetProceduralGeometryTypes() const override { return m_proceduralGeometryTypes; }
            const ProceduralGeometryList& GetProceduralGeometries() const override { return m_proceduralGeometry; }

            void AddMesh(const AZ::Uuid& uuid, const Mesh& rayTracingMesh, const SubMeshVector& subMeshes) override;
            void RemoveMesh(const AZ::Uuid& uuid) override;
            void SetMeshTransform(const AZ::Uuid& uuid, const AZ::Transform transform, const AZ::Vector3 nonUniformScale) override;
            const SubMeshVector& GetSubMeshes() const override { return m_subMeshes; }
            SubMeshVector& GetSubMeshes() override { return m_subMeshes; }
            const MeshMap& GetMeshMap() override { return m_meshes; }
            uint32_t GetSubMeshCount() const override { return m_subMeshCount; }
            uint32_t GetSkinnedMeshCount() const override { return m_skinnedMeshCount; }

            Data::Instance<RPI::ShaderResourceGroup> GetRayTracingSceneSrg() const override { return m_rayTracingSceneSrg; }
            void Render(const RenderPacket&) override;
            void BeginFrame(int deviceIndex) override;
            uint32_t GetRevision() const override { return m_revision; }
            uint32_t GetBuiltRevision(int deviceIndex) const override;
            void SetBuiltRevision(int deviceIndex, uint32_t revision) override;
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

            RHI::MultiDevice::DeviceMask GetDeviceMask() const override { return m_deviceMask; }

        private:
            AZ_DISABLE_COPY_MOVE(RayTracingFeatureProcessor);

            void UpdateBlasInstances();
            void UpdateProceduralGeometryInfoBuffer();
            void UpdateRayTracingSceneSrg();
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

            // current revision number of ray tracing data
            uint32_t m_revision = 0;

            // Currently built revision by device
            AZStd::unordered_map<int, uint32_t> m_builtRevisions;

            // latest tlas revision number
            uint32_t m_tlasRevision = 0;

            uint32_t m_proceduralGeometryTypeRevision = 0;

            // total number of ray tracing sub-meshes
            uint32_t m_subMeshCount = 0;

            // TLAS attachmentId
            RHI::AttachmentId m_tlasAttachmentId;

            // cached TransformServiceFeatureProcessor
            TransformServiceFeatureProcessorInterface* m_transformServiceFeatureProcessor = nullptr;
            MeshFeatureProcessorInterface* m_meshFeatureProcessor = nullptr;

            // mutex for the mesh and BLAS lists
            AZStd::mutex m_mutex;

            // mutex for the m_blasBuilt flag manipulation
            AZStd::mutex m_blasBuiltMutex;

            RPI::RingBuffer m_proceduralGeometryInfoGpuBuffer{ "ProceduralGeometryInfo", RPI::CommonBufferPoolType::ReadOnly, RHI::Format::R32G32_UINT };
            // update flags
            bool m_proceduralGeometryInfoBufferNeedsUpdate = false;

            // side list for looking up existing BLAS objects so they can be re-used when the same mesh is added multiple times
            BlasInstanceMap m_blasInstanceMap;

            BlasBuildList m_blasToCreate;
            AZStd::unordered_map<int, BlasBuildList> m_blasToBuild;
            AZStd::unordered_map<int, BlasBuildList> m_blasToCompact;
            BlasBuildList m_skinnedBlasIds;

            struct BlasFrameEvent
            {
                int m_frameIndex = -1;
            };
            // List of Blas instances that are enqueued for compaction
            // The frame index corresponds to the frame where the compaction query is ready
            AZStd::unordered_map<int, AZStd::unordered_map<Data::AssetId, BlasFrameEvent>> m_blasEnqueuedForCompact;

            // List of Blas instances where the compacted Blas is already built
            // The frame index corresponds to the frame where the uncompacted Blas is no longer needed and can be deleted
            AZStd::unordered_map<int, AZStd::unordered_map<Data::AssetId, BlasFrameEvent>> m_uncompactedBlasEnqueuedForDeletion;

            int m_frameIndex = 0;
            int m_updatedFrameIndex = 0;

            // RayTracingIndexList implements an internal freelist chain stored inside the list itself, allowing entries to be
            // reused after elements are removed.

            uint32_t m_skinnedMeshCount = 0;

            ProceduralGeometryTypeList m_proceduralGeometryTypes;
            ProceduralGeometryList m_proceduralGeometry;
            AZStd::unordered_map<Uuid, size_t> m_proceduralGeometryLookup;

            RHI::Ptr<RHI::RayTracingCompactionQueryPool> m_compactionQueryPool;

            RHI::MultiDevice::DeviceMask m_deviceMask = {};
        };
    }
}
