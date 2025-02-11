/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/RayTracing/RayTracingIndexList.h>
#include <Atom/RHI/RayTracingAccelerationStructure.h>
#include <Atom/RHI/RayTracingCompactionQueryPool.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Public/GpuQuery/Query.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Reflect/Image/Image.h>
#include <Atom/Utils/StableDynamicArray.h>

namespace AZ::Render
{
    static const uint32_t RayTracingGlobalSrgBindingSlot = 0;
    static const uint32_t RayTracingSceneSrgBindingSlot = 1;
    static const uint32_t RayTracingMaterialSrgBindingSlot = 2;

    static const uint32_t RayTracingTlasInstanceElementSize = 64;

    enum class RayTracingSubMeshBufferFlags : uint32_t
    {
        None = 0,

        Tangent = AZ_BIT(0),
        Bitangent = AZ_BIT(1),
        UV = AZ_BIT(2)
    };
    AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::Render::RayTracingSubMeshBufferFlags);

    enum class RayTracingSubMeshTextureFlags : uint32_t
    {
        None = 0,
        BaseColor = AZ_BIT(0),
        Normal = AZ_BIT(1),
        Metallic = AZ_BIT(2),
        Roughness = AZ_BIT(3),
        Emissive = AZ_BIT(4)
    };
    AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::Render::RayTracingSubMeshTextureFlags);

    //! This feature processor manages ray tracing data for a Scene
    class RayTracingFeatureProcessorInterface : public RPI::FeatureProcessor
    {
    public:
        AZ_RTTI(AZ::Render::RayTracingFeatureProcessorInterface, "{84C37D5E-3676-4E39-A0E6-CB048E2F7E5E}", AZ::RPI::FeatureProcessor);

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

            // id for accessing the blas instance (assetId, subMeshIdx)
            AZStd::pair<Data::AssetId, int> m_blasInstanceId;

            // submesh material
            SubMeshMaterial m_material;

            // parent mesh
            Mesh* m_mesh = nullptr;

        private:
            friend class RayTracingFeatureProcessor;

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
                AZ::Vector3 m_outerObbHalfLengths;
                AZ::Vector3 m_innerObbHalfLengths;
                bool m_useParallaxCorrection = false;
                float m_exposure = 0.0f;

                Data::Instance<RPI::Image> m_reflectionProbeCubeMap;
            };

            ReflectionProbe m_reflectionProbe;

            // indices of subMeshes in the subMesh list
            IndexVector m_subMeshIndices;
        };

        //! Contains data for procedural geometry which uses an intersection shader for hit detection
        struct ProceduralGeometryType
        {
            Name m_name;
            Data::Instance<RPI::Shader> m_intersectionShader;
            Name m_intersectionShaderName;
            AZStd::unordered_map<int, uint32_t> m_bindlessBufferIndices;
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
        virtual ProceduralGeometryTypeHandle RegisterProceduralGeometryType(
            const AZStd::string& name,
            const Data::Instance<RPI::Shader>& intersectionShader,
            const AZStd::string& intersectionShaderName,
            const AZStd::unordered_map<int, uint32_t>& bindlessBufferIndices = {}) = 0;

        //! Sets the bindlessBufferIndices of a procedural geometry type. This is necessary if the buffer, whose bindless read index was
        //! passed to `RegisterProceduralGeometryType`, is resized or recreated.
        //! \param geometryTypeHandle A weak handle of a procedural geometry type (obtained by calling `.GetWeakHandle()` on the handle
        //! returned by `RegisterProceduralGeometryType`.
        //! \param bindlessBufferIndices A single 32-bit value which can be queried in the intersection shader with
        //! `GetBindlessBufferIndex()`.
        virtual void SetProceduralGeometryTypeBindlessBufferIndex(
            ProceduralGeometryTypeWeakHandle geometryTypeHandle, const AZStd::unordered_map<int, uint32_t>& bindlessBufferIndices) = 0;

        //! Adds a procedural geometry to the ray tracing scene.
        //! \param geometryTypeHandle A weak handle of a procedural geometry type (obtained by calling `.GetWeakHandle()` on the handle
        //! returned by `RegisterProceduralGeometryType`.
        //! \param uuid The Uuid this geometry instance should be associated with.
        //! \param aabb The axis-aligned bounding box of this geometry instance.
        //! \param material The material of this geometry instance.
        //! \param instanceMask Used to include/exclude mesh instances from TraceRay() calls.
        //! \param localInstanceIndex An index which can be queried in the intersection shader with `GetLocalInstanceIndex()` and can be
        //! used together with `GetBindlessBufferIndex()` to access per-instance geometry data.
        virtual void AddProceduralGeometry(
            ProceduralGeometryTypeWeakHandle geometryTypeHandle,
            const Uuid& uuid,
            const Aabb& aabb,
            const SubMeshMaterial& material,
            RHI::RayTracingAccelerationStructureInstanceInclusionMask instanceMask,
            uint32_t localInstanceIndex) = 0;

        //! Sets the transform of a procedural geometry instance.
        //! \param uuid The Uuid of the procedural geometry which must have been added with `AddProceduralGeometry` before.
        //! \param transform The transform of the procedural geometry instance.
        //! \param nonUniformScale The non-uniform scale of the procedural geometry instance.
        virtual void SetProceduralGeometryTransform(
            const Uuid& uuid, const Transform& transform, const Vector3& nonUniformScale = Vector3::CreateOne()) = 0;

        //! Sets the local index by which this instance can be addressed in the intersection shader.
        //! \param uuid The Uuid of the procedural geometry which must have been added with `AddProceduralGeometry` before.
        //! \param localInstanceIndex An index which can be queried in the intersection shader with `GetLocalInstanceIndex()` and can be
        //! used together with `GetBindlessBufferIndex()` to access per-instance geometry data.
        virtual void SetProceduralGeometryLocalInstanceIndex(const Uuid& uuid, uint32_t localInstanceIndex) = 0;

        //! Sets the material of a procedural geometry instance.
        //! \param uuid The Uuid of the procedural geometry which must have been added with `AddProceduralGeometry` before.
        //! \param material The material of the procedural geometry instance.
        virtual void SetProceduralGeometryMaterial(const Uuid& uuid, const SubMeshMaterial& material) = 0;

        //! Removes a procedural geometry instance from the ray tracing scene.
        //! \param uuid The Uuid of the procedrual geometry which must have been added with `AddProceduralGeometry` before.
        virtual void RemoveProceduralGeometry(const Uuid& uuid) = 0;

        //! Returns the number of procedural geometry instances of a given procedural geometry type.
        //! \param geometryTypeHandle A weak handle of a procedural geometry type(obtained by calling `.GetWeakHandle()` on the handle
        //! returned by `RegisterProceduralGeometryType`.
        //! \return The number of procedural geometry instances of this type.
        virtual int GetProceduralGeometryCount(ProceduralGeometryTypeWeakHandle geometryTypeHandle) const = 0;

        //! Adds ray tracing data for a mesh.
        //! This will cause an update to the RayTracing acceleration structure on the next frame
        virtual void AddMesh(const AZ::Uuid& uuid, const Mesh& rayTracingMesh, const SubMeshVector& subMeshes) = 0;

        //! Removes ray tracing data for a mesh.
        //! This will cause an update to the RayTracing acceleration structure on the next frame
        virtual void RemoveMesh(const AZ::Uuid& uuid) = 0;

        //! Sets the ray tracing mesh transform
        //! This will cause an update to the RayTracing acceleration structure on the next frame
        virtual void SetMeshTransform(const AZ::Uuid& uuid, const AZ::Transform transform, const AZ::Vector3 nonUniformScale) = 0;

        //! Sets the reflection probe for a mesh
        virtual void SetMeshReflectionProbe(const AZ::Uuid& uuid, const Mesh::ReflectionProbe& reflectionProbe) = 0;

        //! Sets the material for a mesh
        virtual void SetMeshMaterials(const AZ::Uuid& uuid, const SubMeshMaterialVector& subMeshMaterials) = 0;

        //! Retrieves the map of all subMeshes in the scene
        virtual const SubMeshVector& GetSubMeshes() const = 0;
        virtual SubMeshVector& GetSubMeshes() = 0;

        //! Mesh data for meshes that should be included in ray tracing operations,
        //! this is a map of the mesh UUID to the ray tracing data for the sub-meshes
        using MeshMap = AZStd::map<AZ::Uuid, Mesh>;
        virtual const MeshMap& GetMeshMap() = 0;

        //! Retrieves the RayTracingSceneSrg
        virtual Data::Instance<RPI::ShaderResourceGroup> GetRayTracingSceneSrg() const = 0;

        //! Retrieves the RayTracingMaterialSrg
        virtual Data::Instance<RPI::ShaderResourceGroup> GetRayTracingMaterialSrg() const = 0;

        //! Retrieves the RayTracingTlas
        virtual const RHI::Ptr<RHI::RayTracingTlas>& GetTlas() const = 0;
        virtual RHI::Ptr<RHI::RayTracingTlas>& GetTlas() = 0;

        //! Retrieves the revision number of the ray tracing data.
        //! This is used to determine if the RayTracingShaderTable needs to be rebuilt.
        virtual uint32_t GetRevision() const = 0;

        //! Retrieves the revision number of the procedural geometry data of the ray tracing data.
        //! This is used to determine if the RayTracingPipelineState needs to be recreated.
        virtual uint32_t GetProceduralGeometryTypeRevision() const = 0;

        //! Provide access to the mutex protecting the blasBuilt flag
        virtual AZStd::mutex& GetBlasBuiltMutex() = 0;

        //! REturns the number of skinned meshes
        virtual uint32_t GetSkinnedMeshCount() const = 0;

        //! Retrieves the buffer pools used for ray tracing operations.
        virtual RHI::RayTracingBufferPools& GetBufferPools() = 0;

        //! Retrieves the total number of ray tracing meshes.
        virtual uint32_t GetSubMeshCount() const = 0;

        //! Returns true if the ray tracing scene contains mesh geometry
        virtual bool HasMeshGeometry() const = 0;

        //! Returns true if the ray tracing scene contains procedural geometry
        virtual bool HasProceduralGeometry() const = 0;

        //! Returns true if the ray tracing scene contains mesh or procedural geometry
        virtual bool HasGeometry() const = 0;

        //! Retrieves the attachmentId of the Tlas for this scene
        virtual RHI::AttachmentId GetTlasAttachmentId() const = 0;

        //! Retrieves the GPU buffer containing information for all ray tracing meshes.
        virtual const Data::Instance<RPI::Buffer> GetMeshInfoGpuBuffer() const = 0;

        //! Retrieves the GPU buffer containing information for all ray tracing materials.
        virtual const Data::Instance<RPI::Buffer> GetMaterialInfoGpuBuffer() const = 0;

        //! If necessary recreates TLAS buffers and updates the ray tracing SRGs. Should only be called by the
        //! RayTracingAccelerationStructurePass.
        virtual void BeginFrame() = 0;

        //! Updates the RayTracingSceneSrg and RayTracingMaterialSrg, called after the TLAS is allocated in the
        //! RayTracingAccelerationStructurePass
        virtual void UpdateRayTracingSrgs() = 0;

        struct SubMeshBlasInstance
        {
            // Uncompacted Blas for the submesh
            // When acceleration structure compaction is enabled, this will be deleted after the compacted Blas is ready
            RHI::Ptr<RHI::RayTracingBlas> m_blas;

            // Compacted Blas
            // Should be empty after creation
            // This is created after the uncompacted Blas is built, if compaction is enabled for this submesh
            RHI::Ptr<RHI::RayTracingBlas> m_compactBlas;

            // Query for getting the compacted size of the acceleration structure buffer
            // If this is set the RayTracingAccelerationStructurePass will compact this blas instance
            // Either none, or all SubMeshBlasInstances in a MeshBlasInstance must have compaction enabled
            RHI::Ptr<RHI::RayTracingCompactionQuery> m_compactionSizeQuery;

            //! Descriptor from which the m_blas is built
            RHI::RayTracingBlasDescriptor m_blasDescriptor;
        };

        struct MeshBlasInstance
        {
            uint32_t m_count = 0;
            AZStd::vector<SubMeshBlasInstance> m_subMeshes;

            // Flags indicating if the Blas objects in the sub-mesh list are already built
            RHI::MultiDevice::DeviceMask m_blasBuilt = RHI::MultiDevice::NoDevices;
            bool m_isSkinnedMesh = false;
        };

        using BlasInstanceMap = AZStd::unordered_map<AZ::Data::AssetId, MeshBlasInstance>;
        virtual BlasInstanceMap& GetBlasInstances() = 0;

        using BlasBuildList = AZStd::unordered_set<AZ::Data::AssetId>;

        //! Returns the list of Blas instance asset ids that need to be built for the given device
        //! The returned asset ids can be used to access the Blas instance returned by GetBlasInstances
        //! The caller is responsible for deleting entries that where enqueued for building
        virtual BlasBuildList& GetBlasBuildList(int deviceIndex) = 0;

        //! Returns the asset id of all skinned mesh Blas instances in the scene
        //! The returned asset ids can be used to access the Blas instance returned by GetBlasInstances
        virtual const BlasBuildList& GetSkinnedMeshBlasList() = 0;

        //! Returns the list of Blas instance asset ids that are ready for compaction
        //! The returned asset ids can be used to access the Blas instance returned by GetBlasInstances
        //! The caller is responsible for deleting entries that where enqueued for building
        virtual BlasBuildList& GetBlasCompactionList(int deviceIndex) = 0;

        //! Signals that the compaction size queries of the asset have been enqueued
        //! The mesh will be inserted into the queue returned by GetBlasCompactionList when the compacted size is ready
        virtual const void MarkBlasInstanceForCompaction(int deviceIndex, Data::AssetId assetId) = 0;

        //! Signals that the Blas compaction has been enqueued
        //! The original uncompacted BLAS will be deleted when it's no longer needed
        virtual const void MarkBlasInstanceAsCompactionEnqueued(int deviceIndex, Data::AssetId assetId) = 0;

        //! Retrieves the list of all procedural geometry types in the scene
        virtual const ProceduralGeometryTypeList& GetProceduralGeometryTypes() const = 0;

        //! Retrieves the list of all procedural geometry instances in the scene
        virtual const ProceduralGeometryList& GetProceduralGeometries() const = 0;
    };
} // namespace AZ::Render
