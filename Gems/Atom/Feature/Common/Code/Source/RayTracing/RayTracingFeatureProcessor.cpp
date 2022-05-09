/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RayTracing/RayTracingFeatureProcessor.h>
#include <Atom/Feature/TransformService/TransformServiceFeatureProcessor.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/Feature/ImageBasedLights/ImageBasedLightFeatureProcessor.h>
#include <CoreLights/DirectionalLightFeatureProcessor.h>
#include <CoreLights/SimplePointLightFeatureProcessor.h>
#include <CoreLights/SimpleSpotLightFeatureProcessor.h>
#include <CoreLights/PointLightFeatureProcessor.h>
#include <CoreLights/DiskLightFeatureProcessor.h>
#include <CoreLights/CapsuleLightFeatureProcessor.h>
#include <CoreLights/QuadLightFeatureProcessor.h>

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

            // load the RayTracingSrg asset asset
            m_rayTracingSrgAsset = RPI::AssetUtils::LoadCriticalAsset<RPI::ShaderAsset>("shaderlib/atom/features/rayTracing/raytracingsrgs.azshader");
            if (!m_rayTracingSrgAsset.IsReady())
            {
                AZ_Assert(false, "Failed to load RayTracingSrg asset");
                return;
            }

            // create the RayTracingSceneSrg
            m_rayTracingSceneSrg = RPI::ShaderResourceGroup::Create(m_rayTracingSrgAsset, Name("RayTracingSceneSrg"));
            AZ_Assert(m_rayTracingSceneSrg, "Failed to create RayTracingSceneSrg");

            // create the RayTracingMaterialSrg
            const AZ::Name rayTracingMaterialSrgName("RayTracingMaterialSrg");
            m_rayTracingMaterialSrg = RPI::ShaderResourceGroup::Create(m_rayTracingSrgAsset, Name("RayTracingMaterialSrg"));
            AZ_Assert(m_rayTracingMaterialSrg, "Failed to create RayTracingMaterialSrg");
        }

        void RayTracingFeatureProcessor::SetMesh(const ObjectId objectId, const AZ::Data::AssetId& assetId, const SubMeshVector& subMeshes)
        {
            if (!m_rayTracingEnabled)
            {
                return;
            }

            RHI::Ptr<RHI::Device> device = RHI::RHISystemInterface::Get()->GetDevice();
            uint32_t objectIndex = objectId.GetIndex();

            // check to see if we already have this mesh
            MeshMap::iterator itMesh = m_meshes.find(objectIndex);
            if (itMesh != m_meshes.end())
            {
                AZ_Assert(false, "SetMesh called on an existing Mesh objectId, call RemoveMesh first");
                return;
            }

            // lock the mutex to protect the mesh and BLAS lists
            AZStd::unique_lock<AZStd::mutex> lock(m_mutex);

            // add the mesh
            m_meshes.insert(AZStd::make_pair(objectIndex, Mesh{ assetId }));
            Mesh& mesh = m_meshes[objectIndex];

            // add the subMeshes to the end of the global subMesh vector
            // Note 1: the MeshInfo and MaterialInfo vectors are parallel with the subMesh vector
            // Note 2: the list of indices for the subMeshes in the global vector are stored in the parent Mesh
            IndexVector subMeshIndices;
            uint32_t subMeshGlobalIndex = aznumeric_cast<uint32_t>(m_subMeshes.size());
            for (uint32_t subMeshIndex = 0; subMeshIndex < subMeshes.size(); ++subMeshIndex, ++subMeshGlobalIndex)
            {
                SubMesh& subMesh = m_subMeshes.emplace_back(subMeshes[subMeshIndex]);
                subMesh.m_mesh = &mesh;
                subMesh.m_subMeshIndex = subMeshIndex;
                subMesh.m_globalIndex = subMeshGlobalIndex;

                // add to the list of global subMeshIndices, which will be stored in the Mesh
                subMeshIndices.push_back(subMeshGlobalIndex);

                // add MeshInfo and MaterialInfo entries
                m_meshInfos.emplace_back();
                m_materialInfos.emplace_back();
            }

            mesh.m_subMeshIndices = subMeshIndices;

            // search for an existing BLAS instance entry for this mesh using the assetId
            BlasInstanceMap::iterator itMeshBlasInstance = m_blasInstanceMap.find(assetId);
            if (itMeshBlasInstance == m_blasInstanceMap.end())
            {
                // make a new BLAS map entry for this mesh
                MeshBlasInstance meshBlasInstance;
                meshBlasInstance.m_count = 1;
                meshBlasInstance.m_subMeshes.reserve(mesh.m_subMeshIndices.size());
                itMeshBlasInstance = m_blasInstanceMap.insert({ assetId, meshBlasInstance }).first;
            }
            else
            {
                itMeshBlasInstance->second.m_count++;
            }

            // create the BLAS buffers for each sub-mesh, or re-use existing BLAS objects if they were already created.
            // Note: all sub-meshes must either create new BLAS objects or re-use existing ones, otherwise it's an error (it's the same model in both cases)
            // Note: the buffer is just reserved here, the BLAS is built in the RayTracingAccelerationStructurePass
            [[maybe_unused]] bool blasInstanceFound = false;
            for (uint32_t subMeshIndex = 0; subMeshIndex < mesh.m_subMeshIndices.size(); ++subMeshIndex)
            {
                SubMesh& subMesh = m_subMeshes[mesh.m_subMeshIndices[subMeshIndex]];

                RHI::RayTracingBlasDescriptor blasDescriptor;
                blasDescriptor.Build()
                    ->Geometry()
                        ->VertexFormat(subMesh.m_positionFormat)
                        ->VertexBuffer(subMesh.m_positionVertexBufferView)
                        ->IndexBuffer(subMesh.m_indexBufferView)
                ;

                // determine if we have an existing BLAS object for this subMesh
                if (itMeshBlasInstance->second.m_subMeshes.size() >= subMeshIndex + 1)
                {
                    // re-use existing BLAS
                    subMesh.m_blas = itMeshBlasInstance->second.m_subMeshes[subMeshIndex].m_blas;

                    // keep track of the fact that we re-used a BLAS
                    blasInstanceFound = true;
                }
                else
                {
                    AZ_Assert(blasInstanceFound == false, "Partial set of RayTracingBlas objects found for mesh");

                    // create the BLAS object and store it in the BLAS list
                    RHI::Ptr<RHI::RayTracingBlas> rayTracingBlas = AZ::RHI::RayTracingBlas::CreateRHIRayTracingBlas();
                    itMeshBlasInstance->second.m_subMeshes.push_back({ rayTracingBlas });

                    // create the buffers from the BLAS descriptor
                    rayTracingBlas->CreateBuffers(*device, &blasDescriptor, *m_bufferPools);

                    // store the BLAS in the mesh
                    subMesh.m_blas = rayTracingBlas;
                }
            }

            // set initial transform
            mesh.m_transform = m_transformServiceFeatureProcessor->GetTransformForId(objectId);
            mesh.m_nonUniformScale = m_transformServiceFeatureProcessor->GetNonUniformScaleForId(objectId);

            AZ::Transform noScaleTransform = mesh.m_transform;
            noScaleTransform.ExtractUniformScale();
            AZ::Matrix3x3 rotationMatrix = Matrix3x3::CreateFromTransform(noScaleTransform);
            rotationMatrix = rotationMatrix.GetInverseFull().GetTranspose();
            Matrix3x4 worldInvTranspose3x4 = Matrix3x4::CreateFromMatrix3x3(rotationMatrix);

            // store the mesh buffers and material textures in the resource lists
            for (uint32_t subMeshIndex : mesh.m_subMeshIndices)
            {
                SubMesh& subMesh = m_subMeshes[subMeshIndex];
                MeshInfo& meshInfo = m_meshInfos[subMesh.m_globalIndex];
                MaterialInfo& materialInfo = m_materialInfos[subMesh.m_globalIndex];

                subMesh.m_irradianceColor.StoreToFloat4(meshInfo.m_irradianceColor.data());
                worldInvTranspose3x4.StoreToRowMajorFloat12(meshInfo.m_worldInvTranspose.data());
                meshInfo.m_bufferFlags = subMesh.m_bufferFlags;

                // add mesh buffers
                meshInfo.m_bufferStartIndex = m_meshBufferIndices.AddEntry(
                {
                    m_meshBuffers.AddResource(subMesh.m_indexShaderBufferView.get()),
                    m_meshBuffers.AddResource(subMesh.m_positionShaderBufferView.get()),
                    m_meshBuffers.AddResource(subMesh.m_normalShaderBufferView.get()),
                    m_meshBuffers.AddResource(subMesh.m_tangentShaderBufferView.get()),
                    m_meshBuffers.AddResource(subMesh.m_bitangentShaderBufferView.get()),
                    m_meshBuffers.AddResource(subMesh.m_uvShaderBufferView.get())
                });
                
                meshInfo.m_indexByteOffset = subMesh.m_indexBufferView.GetByteOffset();
                meshInfo.m_positionByteOffset = subMesh.m_positionVertexBufferView.GetByteOffset();
                meshInfo.m_normalByteOffset = subMesh.m_normalVertexBufferView.GetByteOffset();
                meshInfo.m_tangentByteOffset = subMesh.m_tangentShaderBufferView ? subMesh.m_tangentVertexBufferView.GetByteOffset() : 0;
                meshInfo.m_tangentByteOffset = subMesh.m_bitangentShaderBufferView ? subMesh.m_bitangentVertexBufferView.GetByteOffset() : 0;
                meshInfo.m_tangentByteOffset = subMesh.m_uvShaderBufferView ? subMesh.m_uvVertexBufferView.GetByteOffset() : 0;

                // add material textures
                subMesh.m_baseColor.StoreToFloat4(materialInfo.m_baseColor.data());
                materialInfo.m_metallicFactor = subMesh.m_metallicFactor;
                materialInfo.m_roughnessFactor = subMesh.m_roughnessFactor;
                materialInfo.m_textureFlags = subMesh.m_textureFlags;

                materialInfo.m_textureStartIndex = m_materialTextureIndices.AddEntry(
                {
                    m_materialTextures.AddResource(subMesh.m_baseColorImageView.get()),
                    m_materialTextures.AddResource(subMesh.m_normalImageView.get()),
                    m_materialTextures.AddResource(subMesh.m_metallicImageView.get()),
                    m_materialTextures.AddResource(subMesh.m_roughnessImageView.get())
                });
            }

            m_revision++;
            m_subMeshCount += aznumeric_cast<uint32_t>(subMeshes.size());

            m_meshInfoBufferNeedsUpdate = true;
            m_materialInfoBufferNeedsUpdate = true;
            m_indexListNeedsUpdate = true;
        }

        void RayTracingFeatureProcessor::RemoveMesh(const ObjectId objectId)
        {
            if (!m_rayTracingEnabled)
            {
                return;
            }

            // lock the mutex to protect the mesh and BLAS lists
            AZStd::unique_lock<AZStd::mutex> lock(m_mutex);

            MeshMap::iterator itMesh = m_meshes.find(objectId.GetIndex());
            if (itMesh != m_meshes.end())
            {
                Mesh& mesh = itMesh->second;

                // decrement the count from the BLAS instances, and check to see if we can remove them
                BlasInstanceMap::iterator itBlas = m_blasInstanceMap.find(mesh.m_assetId);
                if (itBlas != m_blasInstanceMap.end())
                {
                    itBlas->second.m_count--;
                    if (itBlas->second.m_count == 0)
                    {
                        m_blasInstanceMap.erase(itBlas);
                    }
                }

                // remove the SubMeshes
                for (auto& subMeshIndex : mesh.m_subMeshIndices)
                {
                    SubMesh& subMesh = m_subMeshes[subMeshIndex];
                    uint32_t globalIndex = subMesh.m_globalIndex;

                    MeshInfo& meshInfo = m_meshInfos[globalIndex];
                    MaterialInfo& materialInfo = m_materialInfos[globalIndex];

                    m_meshBufferIndices.RemoveEntry(meshInfo.m_bufferStartIndex);
                    m_materialTextureIndices.RemoveEntry(materialInfo.m_textureStartIndex);

                    m_meshBuffers.RemoveResource(subMesh.m_indexShaderBufferView.get());
                    m_meshBuffers.RemoveResource(subMesh.m_positionShaderBufferView.get());
                    m_meshBuffers.RemoveResource(subMesh.m_normalShaderBufferView.get());
                    m_meshBuffers.RemoveResource(subMesh.m_tangentShaderBufferView.get());
                    m_meshBuffers.RemoveResource(subMesh.m_bitangentShaderBufferView.get());
                    m_meshBuffers.RemoveResource(subMesh.m_uvShaderBufferView.get());

                    m_materialTextures.RemoveResource(subMesh.m_baseColorImageView.get());
                    m_materialTextures.RemoveResource(subMesh.m_normalImageView.get());
                    m_materialTextures.RemoveResource(subMesh.m_metallicImageView.get());
                    m_materialTextures.RemoveResource(subMesh.m_roughnessImageView.get());
                    
                    if (globalIndex < m_subMeshes.size() - 1)
                    {
                        // the subMesh we're removing is in the middle of the global lists, remove by swapping the last element to its position in the list
                        m_subMeshes[globalIndex] = m_subMeshes.back();
                        m_meshInfos[globalIndex] = m_meshInfos.back();
                        m_materialInfos[globalIndex] = m_materialInfos.back();

                        // update the global index for the swapped subMesh
                        m_subMeshes[globalIndex].m_globalIndex = globalIndex;

                        // update the global index in the parent Mesh' subMesh list
                        Mesh* swappedSubMeshParent = m_subMeshes[globalIndex].m_mesh;
                        uint32_t swappedSubMeshIndex = m_subMeshes[globalIndex].m_subMeshIndex;
                        swappedSubMeshParent->m_subMeshIndices[swappedSubMeshIndex] = globalIndex;
                    }

                    m_subMeshes.pop_back();
                    m_meshInfos.pop_back();
                    m_materialInfos.pop_back();
                }

                // remove from the Mesh list
                m_subMeshCount -= aznumeric_cast<uint32_t>(mesh.m_subMeshIndices.size());
                m_meshes.erase(itMesh);
                m_revision++;

                // reset all data structures if all meshes were removed (i.e., empty scene)
                if (m_subMeshCount == 0)
                {
                    m_meshes.clear();
                    m_subMeshes.clear();
                    m_meshInfos.clear();
                    m_materialInfos.clear();

                    m_meshBufferIndices.Reset();
                    m_materialTextureIndices.Reset();

                    m_meshBuffers.Reset();
                    m_materialTextures.Reset();
                }
            }

            m_meshInfoBufferNeedsUpdate = true;
            m_materialInfoBufferNeedsUpdate = true;
            m_indexListNeedsUpdate = true;
        }

        void RayTracingFeatureProcessor::SetMeshTransform(const ObjectId objectId, const AZ::Transform transform, const AZ::Vector3 nonUniformScale)
        {
            if (!m_rayTracingEnabled)
            {
                return;
            }

            MeshMap::iterator itMesh = m_meshes.find(objectId.GetIndex());
            if (itMesh != m_meshes.end())
            {
                Mesh& mesh = itMesh->second;
                mesh.m_transform = transform;
                mesh.m_nonUniformScale = nonUniformScale;
                m_revision++;

                // create a world inverse transpose 3x4 matrix
                AZ::Transform noScaleTransform = mesh.m_transform;
                noScaleTransform.ExtractUniformScale();
                AZ::Matrix3x3 rotationMatrix = Matrix3x3::CreateFromTransform(noScaleTransform);
                rotationMatrix = rotationMatrix.GetInverseFull().GetTranspose();
                Matrix3x4 worldInvTranspose3x4 = Matrix3x4::CreateFromMatrix3x3(rotationMatrix);

                // update all MeshInfos for this Mesh with the new transform
                for (const auto& subMeshIndex : mesh.m_subMeshIndices)
                {
                    MeshInfo& meshInfo = m_meshInfos[subMeshIndex];
                    worldInvTranspose3x4.StoreToRowMajorFloat12(meshInfo.m_worldInvTranspose.data());
                }
            }

            m_meshInfoBufferNeedsUpdate = true;
        }

        void RayTracingFeatureProcessor::UpdateRayTracingSrgs()
        {
            AZ_PROFILE_SCOPE(AzRender, "RayTracingFeatureProcessor::UpdateRayTracingSrgs");

            if (!m_tlas->GetTlasBuffer())
            {
                return;
            }

            if (m_rayTracingSceneSrg->IsQueuedForCompile() || m_rayTracingMaterialSrg->IsQueuedForCompile())
            {
                //[GFX TODO][ATOM-14792] AtomSampleViewer: Reset scene and feature processors before switching to sample
                return;
            }

            // lock the mutex to protect the mesh and BLAS lists
            AZStd::unique_lock<AZStd::mutex> lock(m_mutex);

            if (m_subMeshCount > 0)
            {
                UpdateMeshInfoBuffer();
                UpdateMaterialInfoBuffer();
                UpdateIndexLists();
            }

            UpdateRayTracingSceneSrg();
            UpdateRayTracingMaterialSrg();
        }

        void RayTracingFeatureProcessor::UpdateMeshInfoBuffer()
        {
            if (m_meshInfoBufferNeedsUpdate)
            {
                // advance to the next buffer in the frame list
                m_currentMeshInfoFrameIndex = (m_currentMeshInfoFrameIndex + 1) % BufferFrameCount;

                // update mesh info buffer
                Data::Instance<RPI::Buffer>& currentMeshInfoGpuBuffer = m_meshInfoGpuBuffer[m_currentMeshInfoFrameIndex];
                uint32_t newMeshByteCount = m_subMeshCount * sizeof(MeshInfo);

                if (currentMeshInfoGpuBuffer == nullptr)
                {
                    // allocate the MeshInfo structured buffer
                    RPI::CommonBufferDescriptor desc;
                    desc.m_poolType = RPI::CommonBufferPoolType::ReadOnly;
                    desc.m_bufferName = "RayTracingMeshInfo";
                    desc.m_byteCount = newMeshByteCount;
                    desc.m_elementSize = sizeof(MeshInfo);
                    currentMeshInfoGpuBuffer = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
                }
                else if (currentMeshInfoGpuBuffer->GetBufferSize() < newMeshByteCount)
                {
                    // resize for the new sub-mesh count
                    currentMeshInfoGpuBuffer->Resize(newMeshByteCount);
                }

                currentMeshInfoGpuBuffer->UpdateData(m_meshInfos.data(), newMeshByteCount);

                m_meshInfoBufferNeedsUpdate = false;
            }
        }

        void RayTracingFeatureProcessor::UpdateMaterialInfoBuffer()
        {
            if (m_materialInfoBufferNeedsUpdate)
            {
                // advance to the next buffer in the frame list
                m_currentMaterialInfoFrameIndex = (m_currentMaterialInfoFrameIndex + 1) % BufferFrameCount;

                // update MaterialInfo buffer
                Data::Instance<RPI::Buffer>& currentMaterialInfoGpuBuffer = m_materialInfoGpuBuffer[m_currentMaterialInfoFrameIndex];
                uint32_t newMaterialInfoByteCount = m_subMeshCount * sizeof(MaterialInfo);
        
                if (currentMaterialInfoGpuBuffer == nullptr)
                {
                    // allocate the MaterialInfo structured buffer
                    RPI::CommonBufferDescriptor desc;
                    desc.m_poolType = RPI::CommonBufferPoolType::ReadOnly;
                    desc.m_bufferName = "RayTracingMaterialInfo";
                    desc.m_byteCount = newMaterialInfoByteCount;
                    desc.m_elementSize = sizeof(MaterialInfo);
                    currentMaterialInfoGpuBuffer = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
                }
                else if (currentMaterialInfoGpuBuffer->GetBufferSize() < newMaterialInfoByteCount)
                {
                    // resize for the new sub-mesh count
                    currentMaterialInfoGpuBuffer->Resize(newMaterialInfoByteCount);
                }
               
                currentMaterialInfoGpuBuffer->UpdateData(m_materialInfos.data(), newMaterialInfoByteCount);

                m_materialInfoBufferNeedsUpdate = false;
            }
        }

        void RayTracingFeatureProcessor::UpdateIndexLists()
        {
            if (m_indexListNeedsUpdate)
            {
                // advance to the next buffer in the frame list
                m_currentIndexListFrameIndex = (m_currentIndexListFrameIndex + 1) % BufferFrameCount;

                // update mesh buffer indices buffer
                Data::Instance<RPI::Buffer>& currentMeshBufferIndicesGpuBuffer = m_meshBufferIndicesGpuBuffer[m_currentIndexListFrameIndex];
                uint32_t newMeshBufferIndicesByteCount = aznumeric_cast<uint32_t>(m_meshBufferIndices.GetIndexList().size()) * sizeof(uint32_t);

                if (currentMeshBufferIndicesGpuBuffer == nullptr)
                {
                    // allocate the MeshBufferIndices buffer
                    RPI::CommonBufferDescriptor desc;
                    desc.m_poolType = RPI::CommonBufferPoolType::ReadOnly;
                    desc.m_bufferName = "RayTracingMeshBufferIndices";
                    desc.m_byteCount = newMeshBufferIndicesByteCount;
                    desc.m_elementSize = sizeof(IndexVector::value_type);
                    desc.m_elementFormat = RHI::Format::R32_UINT;
                    currentMeshBufferIndicesGpuBuffer = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
                }
                else if (currentMeshBufferIndicesGpuBuffer->GetBufferSize() < newMeshBufferIndicesByteCount)
                {
                    // resize for the new index count
                    currentMeshBufferIndicesGpuBuffer->Resize(newMeshBufferIndicesByteCount);
                }

                // resolve to the true indices using the indirection list
                // Note: this is done on the CPU to avoid double-indirection in the shader
                IndexVector resolvedMeshBufferIndices(m_meshBufferIndices.GetIndexList().size());
                uint32_t resolvedMeshBufferIndex = 0;
                for (auto& meshBufferIndex : m_meshBufferIndices.GetIndexList())
                {
                    if (!m_meshBufferIndices.IsValidIndex(meshBufferIndex))
                    {
                        resolvedMeshBufferIndices[resolvedMeshBufferIndex++] = InvalidIndex;
                    }
                    else
                    {
                        resolvedMeshBufferIndices[resolvedMeshBufferIndex++] = m_meshBuffers.GetIndirectionList()[meshBufferIndex];
                    }
                }

                currentMeshBufferIndicesGpuBuffer->UpdateData(resolvedMeshBufferIndices.data(), newMeshBufferIndicesByteCount);

                // update material texture indices buffer
                Data::Instance<RPI::Buffer>& currentMaterialTextureIndicesGpuBuffer = m_materialTextureIndicesGpuBuffer[m_currentIndexListFrameIndex];
                uint32_t newMaterialTextureIndicesByteCount = aznumeric_cast<uint32_t>(m_materialTextureIndices.GetIndexList().size()) * sizeof(uint32_t);

                if (currentMaterialTextureIndicesGpuBuffer == nullptr)
                {
                    // allocate the MaterialInfo structured buffer
                    RPI::CommonBufferDescriptor desc;
                    desc.m_poolType = RPI::CommonBufferPoolType::ReadOnly;
                    desc.m_bufferName = "RayTracingMaterialTextureIndices";
                    desc.m_byteCount = newMaterialTextureIndicesByteCount;
                    desc.m_elementSize = sizeof(IndexVector::value_type);
                    desc.m_elementFormat = RHI::Format::R32_UINT;
                    currentMaterialTextureIndicesGpuBuffer = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
                }
                else if (currentMaterialTextureIndicesGpuBuffer->GetBufferSize() < newMaterialTextureIndicesByteCount)
                {
                    // resize for the new index count
                    currentMaterialTextureIndicesGpuBuffer->Resize(newMaterialTextureIndicesByteCount);
                }

                // resolve to the true indices using the indirection list
                // Note: this is done on the CPU to avoid double-indirection in the shader
                IndexVector resolvedMaterialTextureIndices(m_materialTextureIndices.GetIndexList().size());
                uint32_t resolvedMaterialTextureIndex = 0;
                for (auto& materialTextureIndex : m_materialTextureIndices.GetIndexList())
                {
                    if (!m_materialTextureIndices.IsValidIndex(materialTextureIndex))
                    {
                        resolvedMaterialTextureIndices[resolvedMaterialTextureIndex++] = InvalidIndex;
                    }
                    else
                    {
                        resolvedMaterialTextureIndices[resolvedMaterialTextureIndex++] = m_materialTextures.GetIndirectionList()[materialTextureIndex];
                    }
                }

                currentMaterialTextureIndicesGpuBuffer->UpdateData(resolvedMaterialTextureIndices.data(), newMaterialTextureIndicesByteCount);

                m_indexListNeedsUpdate = false;
            }
        }

        void RayTracingFeatureProcessor::UpdateRayTracingSceneSrg()
        {
            const RHI::ShaderResourceGroupLayout* srgLayout = m_rayTracingSceneSrg->GetLayout();
            RHI::ShaderInputImageIndex imageIndex;
            RHI::ShaderInputBufferIndex bufferIndex;
            RHI::ShaderInputConstantIndex constantIndex;

            // TLAS
            uint32_t tlasBufferByteCount = aznumeric_cast<uint32_t>(m_tlas->GetTlasBuffer()->GetDescriptor().m_byteCount);
            RHI::BufferViewDescriptor bufferViewDescriptor = RHI::BufferViewDescriptor::CreateRayTracingTLAS(tlasBufferByteCount);

            bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_scene"));
            m_rayTracingSceneSrg->SetBufferView(bufferIndex, m_tlas->GetTlasBuffer()->GetBufferView(bufferViewDescriptor).get());

            // directional lights
            const auto directionalLightFP = GetParentScene()->GetFeatureProcessor<DirectionalLightFeatureProcessor>();
            bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_directionalLights"));
            m_rayTracingSceneSrg->SetBufferView(bufferIndex, directionalLightFP->GetLightBuffer()->GetBufferView());

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_directionalLightCount"));
            m_rayTracingSceneSrg->SetConstant(constantIndex, directionalLightFP->GetLightCount());

            // simple point lights
            const auto simplePointLightFP = GetParentScene()->GetFeatureProcessor<SimplePointLightFeatureProcessor>();
            bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_simplePointLights"));
            m_rayTracingSceneSrg->SetBufferView(bufferIndex, simplePointLightFP->GetLightBuffer()->GetBufferView());

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_simplePointLightCount"));
            m_rayTracingSceneSrg->SetConstant(constantIndex, simplePointLightFP->GetLightCount());

            // simple spot lights
            const auto simpleSpotLightFP = GetParentScene()->GetFeatureProcessor<SimpleSpotLightFeatureProcessor>();
            bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_simpleSpotLights"));
            m_rayTracingSceneSrg->SetBufferView(bufferIndex, simpleSpotLightFP->GetLightBuffer()->GetBufferView());

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_simpleSpotLightCount"));
            m_rayTracingSceneSrg->SetConstant(constantIndex, simpleSpotLightFP->GetLightCount());

            // point lights (sphere)
            const auto pointLightFP = GetParentScene()->GetFeatureProcessor<PointLightFeatureProcessor>();
            bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_pointLights"));
            m_rayTracingSceneSrg->SetBufferView(bufferIndex, pointLightFP->GetLightBuffer()->GetBufferView());

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_pointLightCount"));
            m_rayTracingSceneSrg->SetConstant(constantIndex, pointLightFP->GetLightCount());

            // disk lights
            const auto diskLightFP = GetParentScene()->GetFeatureProcessor<DiskLightFeatureProcessor>();
            bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_diskLights"));
            m_rayTracingSceneSrg->SetBufferView(bufferIndex, diskLightFP->GetLightBuffer()->GetBufferView());

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_diskLightCount"));
            m_rayTracingSceneSrg->SetConstant(constantIndex, diskLightFP->GetLightCount());

            // capsule lights
            const auto capsuleLightFP = GetParentScene()->GetFeatureProcessor<CapsuleLightFeatureProcessor>();
            bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_capsuleLights"));
            m_rayTracingSceneSrg->SetBufferView(bufferIndex, capsuleLightFP->GetLightBuffer()->GetBufferView());

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_capsuleLightCount"));
            m_rayTracingSceneSrg->SetConstant(constantIndex, capsuleLightFP->GetLightCount());

            // quad lights
            const auto quadLightFP = GetParentScene()->GetFeatureProcessor<QuadLightFeatureProcessor>();
            bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_quadLights"));
            m_rayTracingSceneSrg->SetBufferView(bufferIndex, quadLightFP->GetLightBuffer()->GetBufferView());

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_quadLightCount"));
            m_rayTracingSceneSrg->SetConstant(constantIndex, quadLightFP->GetLightCount());

            // diffuse environment map for sky hits
            ImageBasedLightFeatureProcessor* imageBasedLightFeatureProcessor = GetParentScene()->GetFeatureProcessor<ImageBasedLightFeatureProcessor>();
            if (imageBasedLightFeatureProcessor)
            {
                imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_diffuseEnvMap"));
                m_rayTracingSceneSrg->SetImage(imageIndex, imageBasedLightFeatureProcessor->GetDiffuseImage());

                constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_iblOrientation"));
                m_rayTracingSceneSrg->SetConstant(constantIndex, imageBasedLightFeatureProcessor->GetOrientation());

                constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_iblExposure"));
                m_rayTracingSceneSrg->SetConstant(constantIndex, imageBasedLightFeatureProcessor->GetExposure());
            }

            bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_meshInfo"));
            m_rayTracingSceneSrg->SetBufferView(bufferIndex, m_meshInfoGpuBuffer[m_currentMeshInfoFrameIndex]->GetBufferView());

            bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_meshBufferIndices"));
            m_rayTracingSceneSrg->SetBufferView(bufferIndex, m_meshBufferIndicesGpuBuffer[m_currentIndexListFrameIndex]->GetBufferView());

            RHI::ShaderInputBufferUnboundedArrayIndex bufferUnboundedArrayIndex = srgLayout->FindShaderInputBufferUnboundedArrayIndex(AZ::Name("m_meshBuffers"));
            m_rayTracingSceneSrg->SetBufferViewUnboundedArray(bufferUnboundedArrayIndex, m_meshBuffers.GetResourceList());
            m_rayTracingSceneSrg->Compile();
        }

        void RayTracingFeatureProcessor::UpdateRayTracingMaterialSrg()
        {
            const RHI::ShaderResourceGroupLayout* srgLayout = m_rayTracingMaterialSrg->GetLayout();
            RHI::ShaderInputBufferIndex bufferIndex;

            bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_materialInfo"));
            m_rayTracingMaterialSrg->SetBufferView(bufferIndex, m_materialInfoGpuBuffer[m_currentMaterialInfoFrameIndex]->GetBufferView());

            bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_materialTextureIndices"));
            m_rayTracingMaterialSrg->SetBufferView(bufferIndex, m_materialTextureIndicesGpuBuffer[m_currentIndexListFrameIndex]->GetBufferView());

            RHI::ShaderInputImageUnboundedArrayIndex textureUnboundedArrayIndex = srgLayout->FindShaderInputImageUnboundedArrayIndex(AZ::Name("m_materialTextures"));
            m_rayTracingMaterialSrg->SetImageViewUnboundedArray(textureUnboundedArrayIndex, m_materialTextures.GetResourceList());
            m_rayTracingMaterialSrg->Compile();
        }
    }        
}
