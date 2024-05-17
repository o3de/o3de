/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <RayTracing/RayTracingResourceList.h>
#include <RayTracing/RayTracingIndexList.h>
#include <Atom/Feature/TransformService/TransformServiceFeatureProcessor.h>
#include <Atom/RHI/IndexBufferView.h>
#include <Atom/RHI/StreamBufferView.h>
#include <Atom/RHI/RayTracingAccelerationStructure.h>
#include <Atom/RHI/RayTracingBufferPools.h>
#include <Atom/RHI/DeviceBufferView.h>
#include <Atom/RHI/DeviceImageView.h>
#include <Atom/RPI.Public/Buffer/RingBuffer.h>
#include <Atom/RPI.Public/Shader/Shader.h>
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
        static const uint32_t RayTracingGlobalSrgBindingSlot = 0;
        static const uint32_t RayTracingSceneSrgBindingSlot = 1;
        static const uint32_t RayTracingMaterialSrgBindingSlot = 2;

        static const uint32_t RayTracingTlasInstanceElementSize = 64;

        enum class RayTracingSubMeshBufferFlags : uint32_t
        {
            None = 0,

            Tangent     = AZ_BIT(0),
            Bitangent   = AZ_BIT(1),
            UV          = AZ_BIT(2)
        };
        AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::Render::RayTracingSubMeshBufferFlags);

        enum class RayTracingSubMeshTextureFlags : uint32_t
        {
            None = 0,
            BaseColor   = AZ_BIT(0),
            Normal      = AZ_BIT(1),
            Metallic    = AZ_BIT(2),
            Roughness   = AZ_BIT(3),
            Emissive    = AZ_BIT(4)
        };
        AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::Render::RayTracingSubMeshTextureFlags);

        //! This feature processor manages ray tracing data for a Scene
        class RayTracingFeatureProcessor
            : public RPI::FeatureProcessor
        {
        public:
            AZ_CLASS_ALLOCATOR(RayTracingFeatureProcessor, AZ::SystemAllocator)

            AZ_RTTI(AZ::Render::RayTracingFeatureProcessor, "{5017EFD3-A996-44B0-9ED2-C47609A2EE8D}", AZ::RPI::FeatureProcessor);

            static void Reflect(AZ::ReflectContext* context);

            RayTracingFeatureProcessor() = default;
            virtual ~RayTracingFeatureProcessor() = default;

            // FeatureProcessor overrides ...
            void Activate() override;
            void Deactivate() override;
            void OnRenderPipelineChanged(RPI::RenderPipeline* renderPipeline, RPI::SceneNotification::RenderPipelineChangeType changeType) override;

            struct Mesh;

            //! Contains material data for a single subMesh
            struct SubMeshMaterial
            {
                // color of the bounced light from this sub-mesh
                AZ::Color m_irradianceColor = AZ::Color(1.0f);

                // material data
                AZ::Color m_baseColor = AZ::Color(0.0f);
                float m_metallicFactor = 0.0f;
                float m_roughnessFactor = 0.0f;
                AZ::Color m_emissiveColor = AZ::Color(0.0f);

                // material texture usage flags
                RayTracingSubMeshTextureFlags m_textureFlags = RayTracingSubMeshTextureFlags::None;

                // material textures
                RHI::Ptr<const RHI::ImageView> m_baseColorImageView;
                RHI::Ptr<const RHI::ImageView> m_normalImageView;
                RHI::Ptr<const RHI::ImageView> m_metallicImageView;
                RHI::Ptr<const RHI::ImageView> m_roughnessImageView;
                RHI::Ptr<const RHI::ImageView> m_emissiveImageView;
            };

            //! Contains data for a single subMesh
            struct SubMesh
            {
                // vertex streams
                RHI::Format m_positionFormat = RHI::Format::Unknown;

                RHI::StreamBufferView m_positionVertexBufferView;
                RHI::Ptr<RHI::BufferView> m_positionShaderBufferView;

                RHI::Format m_normalFormat = RHI::Format::Unknown;
                RHI::StreamBufferView m_normalVertexBufferView;
                RHI::Ptr<RHI::BufferView> m_normalShaderBufferView;

                RHI::Format m_tangentFormat = RHI::Format::Unknown;
                RHI::StreamBufferView m_tangentVertexBufferView;
                RHI::Ptr<RHI::BufferView> m_tangentShaderBufferView;

                RHI::Format m_bitangentFormat = RHI::Format::Unknown;
                RHI::StreamBufferView m_bitangentVertexBufferView;
                RHI::Ptr<RHI::BufferView> m_bitangentShaderBufferView;

                RHI::Format m_uvFormat = RHI::Format::Unknown;
                RHI::StreamBufferView m_uvVertexBufferView;
                RHI::Ptr<RHI::BufferView> m_uvShaderBufferView;

                // index buffer
                RHI::IndexBufferView m_indexBufferView;
                RHI::Ptr<RHI::BufferView> m_indexShaderBufferView;

                // vertex buffer usage flags
                RayTracingSubMeshBufferFlags m_bufferFlags = RayTracingSubMeshBufferFlags::None;

                // ray tracing Blas
                RHI::Ptr<RHI::RayTracingBlas> m_blas;

                // submesh material
                SubMeshMaterial m_material;

                // parent mesh
                Mesh* m_mesh = nullptr;

            private:
                friend RayTracingFeatureProcessor;

                // index of this mesh in the subMesh list, also applies to the MeshInfo and MaterialInfo entries
                uint32_t m_globalIndex = InvalidIndex;

                // index of this mesh in the parent Mesh's subMesh list
                uint32_t m_subMeshIndex = InvalidIndex;
            };

            using SubMeshVector = AZStd::vector<SubMesh>;
            using SubMeshMaterialVector = AZStd::vector<SubMeshMaterial>;
            using IndexVector = AZStd::vector<uint32_t>;

            //! Contains data for the top level mesh, including the list of sub-meshes
            struct Mesh
            {
                // assetId of the model
                AZ::Data::AssetId m_assetId = AZ::Data::AssetId{};

                // transform
                AZ::Transform m_transform = AZ::Transform::CreateIdentity();

                // non-uniform scale
                AZ::Vector3 m_nonUniformScale = AZ::Vector3::CreateOne();

                // instance mask. Used to include/exclude mesh instances from TraceRay() calls
                uint32_t m_instanceMask = 0;

                bool m_isSkinnedMesh = false;

                // reflection probe
                struct ReflectionProbe
                {
                    AZ::Transform m_modelToWorld;
                    AZ::Vector3   m_outerObbHalfLengths;
                    AZ::Vector3   m_innerObbHalfLengths;
                    bool          m_useParallaxCorrection = false;
                    float         m_exposure = 0.0f;

                    Data::Instance<RPI::Image> m_reflectionProbeCubeMap;
                };

                ReflectionProbe m_reflectionProbe;

            private:
                friend RayTracingFeatureProcessor;

                // indices of subMeshes in the subMesh list
                IndexVector m_subMeshIndices;
            };

            //! Contains data for procedural geometry which uses an intersection shader for hit detection
            struct ProceduralGeometryType
            {
                Name m_name;
                Data::Instance<RPI::Shader> m_intersectionShader;
                Name m_intersectionShaderName;
                uint32_t m_bindlessBufferIndex;
                int m_instanceCount = 0;
            };
            using ProceduralGeometryTypeHandle = StableDynamicArrayHandle<ProceduralGeometryType>;
            using ProceduralGeometryTypeWeakHandle = StableDynamicArrayWeakHandle<ProceduralGeometryType>;

            //! Contains data for procedural geometry instances
            struct ProceduralGeometry
            {
                Uuid m_uuid;
                ProceduralGeometryTypeWeakHandle m_typeHandle;
                Aabb m_aabb;
                uint32_t m_instanceMask;
                Transform m_transform = Transform::CreateIdentity();
                AZ::Vector3 m_nonUniformScale = Vector3::CreateOne();
                RHI::Ptr<RHI::RayTracingBlas> m_blas;
                uint32_t m_localInstanceIndex;
            };
            using ProceduralGeometryTypeList = StableDynamicArray<ProceduralGeometryType>;
            using ProceduralGeometryList = AZStd::vector<ProceduralGeometry>;

            //! Registers a new procedural geometry type, which uses an intersection shader to determine hits for ray tracing.
            //! \param name The name this procedural geometry type should be associated with. It must be unique within the ray tracing
            //! pipeline as it is used to match hit group records to hit groups.
            //! \param intersectionShader The intersection that should be used for procedural geometry of this type. The intersection shader
            //! *must* include the file \<Atom/Features/RayTracing/RayTracingSrgs.azsli\> and must use the struct
            //! `ProceduralGeometryIntersectionAttributes` to forward its hit parameters to ReportHit().
            //! \param intersectionShaderName The name of the intersection shader entry function within m_intersectionShader.
            //! \param bindlessBufferIndex A single 32-bit value which can be queried in the intersection shader with
            //! `GetBindlessBufferIndex()`. This value is in most cases obtained by calling GetBindlessReadIndex() on a RHI BufferView, such
            //! that it can be accessed with `Bindless::GetByteAddressBuffer(GetBindlessBufferIndex())` in the intersection shader.
            //! \return A handle to the created type. If this handle is destroyed (by falling out of scope or calling `.Free()`), this
            //! procedural geometry type is also destroyed. This handle should be regarded as an opaque pointer, meaning that no member
            //! variables should be accessed or changed directly.
            ProceduralGeometryTypeHandle RegisterProceduralGeometryType(
                const AZStd::string& name,
                const Data::Instance<RPI::Shader>& intersectionShader,
                const AZStd::string& intersectionShaderName,
                uint32_t bindlessBufferIndex = static_cast<uint32_t>(-1));

            //! Sets the bindlessBufferIndex of a procedural geometry type. This is necessary if the buffer, whose bindless read index was
            //! passed to `RegisterProceduralGeometryType`, is resized or recreated.
            //! \param geometryTypeHandle A weak handle of a procedural geometry type (obtained by calling `.GetWeakHandle()` on the handle
            //! returned by `RegisterProceduralGeometryType`.
            //! \param bindlessBufferIndex A single 32-bit value which can be queried in the intersection shader with
            //! `GetBindlessBufferIndex()`.
            void SetProceduralGeometryTypeBindlessBufferIndex(
                ProceduralGeometryTypeWeakHandle geometryTypeHandle, uint32_t bindlessBufferIndex);

            //! Adds a procedural geometry to the ray tracing scene.
            //! \param geometryTypeHandle A weak handle of a procedural geometry type (obtained by calling `.GetWeakHandle()` on the handle
            //! returned by `RegisterProceduralGeometryType`.
            //! \param uuid The Uuid this geometry instance should be associated with.
            //! \param aabb The axis-aligned bounding box of this geometry instance.
            //! \param material The material of this geometry instance.
            //! \param instanceMask Used to include/exclude mesh instances from TraceRay() calls.
            //! \param localInstanceIndex An index which can be queried in the intersection shader with `GetLocalInstanceIndex()` and can be
            //! used together with `GetBindlessBufferIndex()` to access per-instance geometry data.
            void AddProceduralGeometry(
                ProceduralGeometryTypeWeakHandle geometryTypeHandle,
                const Uuid& uuid,
                const Aabb& aabb,
                const SubMeshMaterial& material,
                RHI::RayTracingAccelerationStructureInstanceInclusionMask instanceMask,
                uint32_t localInstanceIndex);

            //! Sets the transform of a procedural geometry instance.
            //! \param uuid The Uuid of the procedural geometry which must have been added with `AddProceduralGeometry` before.
            //! \param transform The transform of the procedural geometry instance.
            //! \param nonUniformScale The non-uniform scale of the procedural geometry instance.
            void SetProceduralGeometryTransform(
                const Uuid& uuid, const Transform& transform, const Vector3& nonUniformScale = Vector3::CreateOne());

            //! Sets the local index by which this instance can be addressed in the intersection shader.
            //! \param uuid The Uuid of the procedural geometry which must have been added with `AddProceduralGeometry` before.
            //! \param localInstanceIndex An index which can be queried in the intersection shader with `GetLocalInstanceIndex()` and can be
            //! used together with `GetBindlessBufferIndex()` to access per-instance geometry data.
            void SetProceduralGeometryLocalInstanceIndex(const Uuid& uuid, uint32_t localInstanceIndex);

            //! Removes a procedural geometry instance from the ray tracing scene.
            //! \param uuid The Uuid of the procedrual geometry which must have been added with `AddProceduralGeometry` before.
            void RemoveProceduralGeometry(const Uuid& uuid);

            //! Returns the number of procedural geometry instances of a given procedural geometry type.
            //! \param geometryTypeHandle A weak handle of a procedural geometry type(obtained by calling `.GetWeakHandle()` on the handle
            //! returned by `RegisterProceduralGeometryType`.
            //! \return The number of procedural geometry instances of this type.
            int GetProceduralGeometryCount(ProceduralGeometryTypeWeakHandle geometryTypeHandle) const;

            //! Adds ray tracing data for a mesh.
            //! This will cause an update to the RayTracing acceleration structure on the next frame
            void AddMesh(const AZ::Uuid& uuid, const Mesh& rayTracingMesh, const SubMeshVector& subMeshes);

            //! Removes ray tracing data for a mesh.
            //! This will cause an update to the RayTracing acceleration structure on the next frame
            void RemoveMesh(const AZ::Uuid& uuid);

            //! Sets the ray tracing mesh transform
            //! This will cause an update to the RayTracing acceleration structure on the next frame
            void SetMeshTransform(const AZ::Uuid& uuid, const AZ::Transform transform, const AZ::Vector3 nonUniformScale);

            //! Sets the reflection probe for a mesh
            void SetMeshReflectionProbe(const AZ::Uuid& uuid, const Mesh::ReflectionProbe& reflectionProbe);

            //! Sets the material for a mesh
            void SetMeshMaterials(const AZ::Uuid& uuid, const SubMeshMaterialVector& subMeshMaterials);

            //! Retrieves the map of all subMeshes in the scene
            const SubMeshVector& GetSubMeshes() const { return m_subMeshes; }
            SubMeshVector& GetSubMeshes() { return m_subMeshes; }

            //! Retrieves the RayTracingSceneSrg
            Data::Instance<RPI::ShaderResourceGroup> GetRayTracingSceneSrg() const { return m_rayTracingSceneSrg; }

            //! Retrieves the RayTracingMaterialSrg
            Data::Instance<RPI::ShaderResourceGroup> GetRayTracingMaterialSrg() const { return m_rayTracingMaterialSrg; }

            //! Retrieves the RayTracingTlas
            const RHI::Ptr<RHI::RayTracingTlas>& GetTlas() const { return m_tlas; }
            RHI::Ptr<RHI::RayTracingTlas>& GetTlas() { return m_tlas; }

            //! Retrieves the revision number of the ray tracing data.
            //! This is used to determine if the RayTracingShaderTable needs to be rebuilt.
            uint32_t GetRevision() const { return m_revision; }

            //! Retrieves the revision number of the procedural geometry data of the ray tracing data.
            //! This is used to determine if the RayTracingPipelineState needs to be recreated.
            uint32_t GetProceduralGeometryTypeRevision() const { return m_proceduralGeometryTypeRevision; }

            uint32_t GetSkinnedMeshCount() const
            {
                return m_skinnedMeshCount;
            }

            //! Retrieves the buffer pools used for ray tracing operations.
            RHI::RayTracingBufferPools& GetBufferPools() { return *m_bufferPools; }

            //! Retrieves the total number of ray tracing meshes.
            uint32_t GetSubMeshCount() const { return m_subMeshCount; }

            //! Returns true if the ray tracing scene contains mesh geometry
            bool HasMeshGeometry() const { return m_subMeshCount != 0; }

            //! Returns true if the ray tracing scene contains procedural geometry
            bool HasProceduralGeometry() const { return !m_proceduralGeometry.empty(); }

            //! Returns true if the ray tracing scene contains mesh or procedural geometry
            bool HasGeometry() const { return HasMeshGeometry() || HasProceduralGeometry(); }

            //! Retrieves the attachmentId of the Tlas for this scene
            RHI::AttachmentId GetTlasAttachmentId() const { return m_tlasAttachmentId; }

            //! Retrieves the GPU buffer containing information for all ray tracing meshes.
            const Data::Instance<RPI::Buffer> GetMeshInfoGpuBuffer() const { return m_meshInfoGpuBuffer.GetCurrentBuffer(); }

            //! Retrieves the GPU buffer containing information for all ray tracing materials.
            const Data::Instance<RPI::Buffer> GetMaterialInfoGpuBuffer() const { return m_materialInfoGpuBuffer.GetCurrentBuffer(); }

            //! Updates the RayTracingSceneSrg and RayTracingMaterialSrg, called after the TLAS is allocated in the RayTracingAccelerationStructurePass
            void UpdateRayTracingSrgs();

            struct SubMeshBlasInstance
            {
                RHI::Ptr<RHI::RayTracingBlas> m_blas;
            };

            struct MeshBlasInstance
            {
                uint32_t m_count = 0;
                AZStd::vector<SubMeshBlasInstance> m_subMeshes;

                // flag indicating if the Blas objects in the sub-mesh list are built
                bool m_blasBuilt = false;
                bool m_isSkinnedMesh = false;
            };

            using BlasInstanceMap = AZStd::unordered_map<AZ::Data::AssetId, MeshBlasInstance>;
            BlasInstanceMap& GetBlasInstances() { return m_blasInstanceMap; }

            const ProceduralGeometryTypeList& GetProceduralGeometryTypes() const { return m_proceduralGeometryTypes; }
            const ProceduralGeometryList& GetProceduralGeometries() const { return m_proceduralGeometry; }

        private:

            AZ_DISABLE_COPY_MOVE(RayTracingFeatureProcessor);

            void UpdateMeshInfoBuffer();
            void UpdateProceduralGeometryInfoBuffer();
            void UpdateMaterialInfoBuffer();
            void UpdateIndexLists();
            void UpdateRayTracingSceneSrg();
            void UpdateRayTracingMaterialSrg();

            static RHI::RayTracingAccelerationStructureBuildFlags CreateRayTracingAccelerationStructureBuildFlags(bool isSkinnedMesh);

            // flag indicating if RayTracing is enabled, currently based on device support
            bool m_rayTracingEnabled = false;

            // mesh data for meshes that should be included in ray tracing operations,
            // this is a map of the mesh UUID to the ray tracing data for the sub-meshes
            using MeshMap = AZStd::map<AZ::Uuid, Mesh>;
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

            uint32_t m_proceduralGeometryTypeRevision = 0;

            // total number of ray tracing sub-meshes
            uint32_t m_subMeshCount = 0;

            // TLAS attachmentId
            RHI::AttachmentId m_tlasAttachmentId;

            // cached TransformServiceFeatureProcessor
            TransformServiceFeatureProcessor* m_transformServiceFeatureProcessor = nullptr;

            // mutex for the mesh and BLAS lists
            AZStd::mutex m_mutex;

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
            MeshInfoVector m_meshInfos;
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

            void ConvertMaterial(MaterialInfo& materialInfo, const SubMeshMaterial& subMeshMaterial, int deviceIndex);
        };
    }
}
