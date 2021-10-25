/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/TransformService/TransformServiceFeatureProcessor.h>
#include <Atom/RHI/RayTracingAccelerationStructure.h>
#include <Atom/RHI/RayTracingBufferPools.h>
#include <Atom/RHI/BufferView.h>
#include <Atom/RHI/ImageView.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Transform.h>

namespace AZ
{
    namespace Render
    {
        static const uint32_t RayTracingGlobalSrgBindingSlot = 0;
        static const uint32_t RayTracingSceneSrgBindingSlot = 1;
        static const uint32_t RayTracingMaterialSrgBindingSlot = 2;

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
            Roughness   = AZ_BIT(3)
        };
        AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::Render::RayTracingSubMeshTextureFlags);

        //! This feature processor manages ray tracing data for a Scene
        class RayTracingFeatureProcessor
            : public RPI::FeatureProcessor
        {
        public:

            AZ_RTTI(AZ::Render::RayTracingFeatureProcessor, "{5017EFD3-A996-44B0-9ED2-C47609A2EE8D}", RPI::FeatureProcessor);

            static void Reflect(AZ::ReflectContext* context);

            RayTracingFeatureProcessor() = default;
            virtual ~RayTracingFeatureProcessor() = default;

            // FeatureProcessor overrides ...
            void Activate() override;

            //! Contains data for a single sub-mesh
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

                // color of the bounced light from this sub-mesh
                AZ::Color m_irradianceColor = AZ::Color(1.0f);

                // ray tracing Blas
                RHI::Ptr<RHI::RayTracingBlas> m_blas;

                // material data
                AZ::Color m_baseColor = AZ::Color(0.0f);
                float m_metallicFactor = 0.0f;
                float m_roughnessFactor = 0.0f;

                // material texture usage flags
                RayTracingSubMeshTextureFlags m_textureFlags = RayTracingSubMeshTextureFlags::None;

                // material textures
                RHI::Ptr<const RHI::ImageView> m_baseColorImageView;
                RHI::Ptr<const RHI::ImageView> m_normalImageView;
                RHI::Ptr<const RHI::ImageView> m_metallicImageView;
                RHI::Ptr<const RHI::ImageView> m_roughnessImageView;
            };
            using SubMeshVector = AZStd::vector<SubMesh>;

            //! Contains data for the top level mesh, including the list of sub-meshes
            struct Mesh
            {
                // assetId of the model
                AZ::Data::AssetId m_assetId = AZ::Data::AssetId{};

                // sub-mesh list
                SubMeshVector m_subMeshes;

                // mesh transform
                AZ::Transform m_transform = AZ::Transform::CreateIdentity();

                // mesh non-uniform scale
                AZ::Vector3 m_nonUniformScale = AZ::Vector3::CreateOne();

                // flag indicating if the Blas objects in the sub-meshes are built
                bool m_blasBuilt = false;
            };

            using MeshMap = AZStd::map<uint32_t, Mesh>;
            using ObjectId = TransformServiceFeatureProcessorInterface::ObjectId;

            //! Sets ray tracing data for a mesh.
            //! This will cause an update to the RayTracing acceleration structure on the next frame
            void SetMesh(const ObjectId objectId, const AZ::Data::AssetId& assetId, const SubMeshVector& subMeshes);

            //! Removes ray tracing data for a mesh.
            //! This will cause an update to the RayTracing acceleration structure on the next frame
            void RemoveMesh(const ObjectId objectId);

            //! Sets the ray tracing mesh transform
            //! This will cause an update to the RayTracing acceleration structure on the next frame
            void SetMeshTransform(const ObjectId objectId, const AZ::Transform transform,
                const AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne());

            //! Retrieves ray tracing data for all meshes in the scene
            const MeshMap& GetMeshes() const { return m_meshes; }
            MeshMap& GetMeshes() { return m_meshes; }

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

            //! Retrieves the buffer pools used for ray tracing operations.
            RHI::RayTracingBufferPools& GetBufferPools() { return *m_bufferPools; }

            //! Retrieves the total number of ray tracing meshes.
            uint32_t GetSubMeshCount() const { return m_subMeshCount; }

            //! Retrieves the attachmentId of the Tlas for this scene
            RHI::AttachmentId GetTlasAttachmentId() const { return m_tlasAttachmentId; }

            //! Retrieves the GPU buffer containing information for all ray tracing meshes.
            const Data::Instance<RPI::Buffer> GetMeshInfoBuffer() const { return m_meshInfoBuffer; }

            //! Retrieves the GPU buffer containing information for all ray tracing materials.
            const Data::Instance<RPI::Buffer> GetMaterialInfoBuffer() const { return m_materialInfoBuffer; }

            //! Updates the RayTracingSceneSrg and RayTracingMaterialSrg, called after the TLAS is allocated in the RayTracingAccelerationStructurePass
            void UpdateRayTracingSrgs();

        private:

            AZ_DISABLE_COPY_MOVE(RayTracingFeatureProcessor);

            void UpdateMeshInfoBuffer();
            void UpdateMaterialInfoBuffer();
            void UpdateRayTracingSceneSrg();
            void UpdateRayTracingMaterialSrg();

            // flag indicating if RayTracing is enabled, currently based on device support
            bool m_rayTracingEnabled = false;

            // mesh data for meshes that should be included in ray tracing operations,
            // this is a map of the mesh object Id to the ray tracing data for the sub-meshes
            MeshMap m_meshes;

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
                uint32_t m_indexOffset;
                uint32_t m_positionOffset;
                uint32_t m_normalOffset;
                uint32_t m_tangentOffset;
                uint32_t m_bitangentOffset;
                uint32_t m_uvOffset;
                float m_padding0[2];

                AZStd::array<float, 4> m_irradianceColor;   // float4
                AZStd::array<float, 9> m_worldInvTranspose; // float3x3
                float m_padding1;

                RayTracingSubMeshBufferFlags m_bufferFlags = RayTracingSubMeshBufferFlags::None;
                uint32_t m_bufferStartIndex = 0;
            };

            // buffer containing a MeshInfo for each sub-mesh
            Data::Instance<RPI::Buffer> m_meshInfoBuffer;

            // structure for data in the m_materialInfoBuffer, shaders that use the buffer must match this type
            struct MaterialInfo
            {
                AZStd::array<float, 4> m_baseColor;   // float4
                float m_metallicFactor = 0.0f;
                float m_roughnessFactor = 0.0f;
                RayTracingSubMeshTextureFlags m_textureFlags = RayTracingSubMeshTextureFlags::None;
                uint32_t m_textureStartIndex = 0;
            };

            // buffer containing a MaterialInfo for each sub-mesh
            Data::Instance<RPI::Buffer> m_materialInfoBuffer;

            // flag indicating we need to update the meshInfo buffer
            bool m_meshInfoBufferNeedsUpdate = false;

            // flag indicating we need to update the materialInfo buffer
            bool m_materialInfoBufferNeedsUpdate = false;

            // side list for looking up existing BLAS objects so they can be re-used when the same mesh is added multiple times
            struct SubMeshBlasInstance
            {
                RHI::Ptr<RHI::RayTracingBlas> m_blas;
            };

            struct MeshBlasInstance
            {
                uint32_t m_count = 0;
                AZStd::vector<SubMeshBlasInstance> m_subMeshes;
            };

            using BlasInstanceMap = AZStd::unordered_map<AZ::Data::AssetId, MeshBlasInstance>;
            BlasInstanceMap m_blasInstanceMap;

            // Cache view pointers so we dont need to update them if none changed from frame to frame.
            AZStd::vector<const RHI::BufferView*> m_meshBuffers;
            AZStd::vector<const RHI::ImageView*> m_materialTextures;
        };
    }
}
