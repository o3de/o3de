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

#include <RayTracing/RayTracingFeatureProcessor.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>

namespace AZ
{
    namespace Render
    {
        void RayTracingFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<RayTracingFeatureProcessor, FeatureProcessor>()
                    ->Version(0);
            }
        }

        void RayTracingFeatureProcessor::Activate()
        {
            RHI::Ptr<RHI::Device> device = RHI::RHISystemInterface::Get()->GetDevice();
            m_rayTracingEnabled = device->GetFeatures().m_rayTracing;

            if (!m_rayTracingEnabled)
            {
                return;
            }

            m_transformServiceFeatureProcessor = GetParentScene()->GetFeatureProcessor<TransformServiceFeatureProcessor>();

            // initialize the ray tracing buffer pools
            m_bufferPools = RHI::RayTracingBufferPools::CreateRHIRayTracingBufferPools();
            m_bufferPools->Init(device);

            // create TLAS attachmentId
            AZStd::string uuidString = AZ::Uuid::CreateRandom().ToString<AZStd::string>();
            m_tlasAttachmentId = RHI::AttachmentId(AZStd::string::format("RayTracingTlasAttachmentId_%s", uuidString.c_str()));

            // create the TLAS object
            m_tlas = AZ::RHI::RayTracingTlas::CreateRHIRayTracingTlas();       
        }

        void RayTracingFeatureProcessor::SetMesh(const ObjectId objectId, const SubMeshVector& subMeshes)
        {
            if (!m_rayTracingEnabled)
            {
                return;
            }

            RHI::Ptr<RHI::Device> device = RHI::RHISystemInterface::Get()->GetDevice();
            uint32_t objectIndex = objectId.GetIndex();

            MeshMap::iterator itMesh = m_meshes.find(objectIndex);
            if (itMesh == m_meshes.end())
            {
                m_meshes.insert(AZStd::make_pair(objectIndex, Mesh{ subMeshes }));
            }
            else
            {
                // updating an existing entry
                // decrement the mesh count by the number of meshes in the existing entry in case the number of meshes changed
                m_subMeshCount -= aznumeric_cast<uint32_t>(itMesh->second.m_subMeshes.size());
                m_meshes[objectIndex].m_subMeshes = subMeshes;
            }

            // create the BLAS buffers for each sub-mesh
            // Note: the buffer is just reserved here, the BLAS is built in the RayTracingAccelerationStructurePass
            Mesh& mesh = m_meshes[objectIndex];
            for (auto& subMesh : mesh.m_subMeshes)
            {
                RHI::RayTracingBlasDescriptor blasDescriptor;
                blasDescriptor.Build()
                    ->Geometry()
                        ->VertexFormat(subMesh.m_vertexFormat)
                        ->VertexBuffer(subMesh.m_positionVertexBufferView)
                        ->IndexBuffer(subMesh.m_indexBufferView)
                ;

                // create the BLAS object
                subMesh.m_blas = AZ::RHI::RayTracingBlas::CreateRHIRayTracingBlas();

                // create the buffers from the descriptor
                subMesh.m_blas->CreateBuffers(*device, &blasDescriptor, *m_bufferPools);
            }

            // set initial transform
            mesh.m_matrix3x4 = m_transformServiceFeatureProcessor->GetMatrix3x4ForId(objectId);

            m_revision++;
            m_subMeshCount += aznumeric_cast<uint32_t>(subMeshes.size());
        }

        void RayTracingFeatureProcessor::RemoveMesh(const ObjectId objectId)
        {
            if (!m_rayTracingEnabled)
            {
                return;
            }

            MeshMap::iterator itMesh = m_meshes.find(objectId.GetIndex());
            if (itMesh != m_meshes.end())
            {
                m_subMeshCount -= aznumeric_cast<uint32_t>(itMesh->second.m_subMeshes.size());
                m_meshes.erase(itMesh);
                m_revision++;
            }
        }

        void RayTracingFeatureProcessor::SetMeshMatrix3x4(const ObjectId objectId, const AZ::Matrix3x4 matrix3x4)
        {
            if (!m_rayTracingEnabled)
            {
                return;
            }

            MeshMap::iterator itMesh = m_meshes.find(objectId.GetIndex());
            if (itMesh != m_meshes.end())
            {
                itMesh->second.m_matrix3x4 = matrix3x4;
                m_revision++;
            }
        }
    }        
}
