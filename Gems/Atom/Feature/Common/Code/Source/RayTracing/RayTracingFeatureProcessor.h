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

#include <Atom/Feature/TransformService/TransformServiceFeatureProcessor.h>
#include <Atom/RHI/RayTracingAccelerationStructure.h>
#include <Atom/RHI/RayTracingBufferPools.h>
#include <Atom/RHI/BufferView.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Transform.h>

namespace AZ
{
    namespace Render
    {
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
                // vertex/index buffer data
                RHI::Format m_vertexFormat = RHI::Format::Unknown;
                RHI::StreamBufferView m_positionVertexBufferView;
                RHI::Ptr<RHI::BufferView> m_positionShaderBufferView;
                RHI::StreamBufferView m_normalVertexBufferView;
                RHI::Ptr<RHI::BufferView> m_normalShaderBufferView;
                RHI::IndexBufferView m_indexBufferView;
                RHI::Ptr<RHI::BufferView> m_indexShaderBufferView;

                // color of the bounced light from this sub-mesh
                AZ::Color m_irradianceColor;

                // ray tracing Blas
                RHI::Ptr<RHI::RayTracingBlas> m_blas;
            };
            using SubMeshVector = AZStd::vector<SubMesh>;

            //! Contains data for the top level mesh, including the list of sub-meshes
            struct Mesh
            {
                // sub-mesh list
                SubMeshVector m_subMeshes;

                // mesh transform
                AZ::Matrix3x4 m_matrix3x4 = AZ::Matrix3x4::CreateIdentity();

                // flag indicating if the Blas objects in the sub-meshes are built
                bool m_blasBuilt = false;
            };

            using MeshMap = AZStd::map<uint32_t, Mesh>;
            using ObjectId = TransformServiceFeatureProcessorInterface::ObjectId;

            //! Sets ray tracing data for a mesh.
            //! This will cause an update to the RayTracing acceleration structure on the next frame
            void SetMesh(const ObjectId objectId, const SubMeshVector& subMeshes);

            //! Removes ray tracing data for a mesh.
            //! This will cause an update to the RayTracing acceleration structure on the next frame
            void RemoveMesh(const ObjectId objectId);

            //! Sets the ray tracing mesh transform
            //! This will cause an update to the RayTracing acceleration structure on the next frame
            void SetMeshMatrix3x4(const ObjectId objectId, const AZ::Matrix3x4 matrix3x4);

            //! Retrieves ray tracing data for all meshes in the scene
            const MeshMap& GetMeshes() const { return m_meshes; }
            MeshMap& GetMeshes() { return m_meshes; }

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

        private:

            AZ_DISABLE_COPY_MOVE(RayTracingFeatureProcessor);

            // flag indicating if RayTracing is enabled, currently based on device support
            bool m_rayTracingEnabled = false;

            // mesh data for meshes that should be included in ray tracing operations,
            // this is a map of the mesh object Id to the ray tracing data for the sub-meshes
            MeshMap m_meshes;

            // buffer pools used in ray tracing operations
            RHI::Ptr<RHI::RayTracingBufferPools> m_bufferPools;

            // ray tracing acceleration structure (TLAS)
            RHI::Ptr<RHI::RayTracingTlas> m_tlas;

            // current revision number of ray tracing data
            uint32_t m_revision = 0;

            // total number of ray tracing sub-meshes
            uint32_t m_subMeshCount = 0;

            // TLAS attachmentId
            RHI::AttachmentId m_tlasAttachmentId;

            // cached TransformServiceFeatureProcessor
            TransformServiceFeatureProcessor* m_transformServiceFeatureProcessor = nullptr;
        };
    }
}
