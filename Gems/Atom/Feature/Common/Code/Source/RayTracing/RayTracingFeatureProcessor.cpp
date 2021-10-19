/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RayTracing/RayTracingFeatureProcessor.h>
#include <AzCore/Debug/EventTrace.h>
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

            // lock the mutex to protect the mesh and BLAS lists
            AZStd::unique_lock<AZStd::mutex> lock(m_mutex);

            MeshMap::iterator itMesh = m_meshes.find(objectIndex);
            if (itMesh == m_meshes.end())
            {
                m_meshes.insert(AZStd::make_pair(objectIndex, Mesh{ assetId, subMeshes }));
            }
            else
            {
                // updating an existing entry
                // decrement the mesh count by the number of meshes in the existing entry in case the number of meshes changed
                m_subMeshCount -= aznumeric_cast<uint32_t>(itMesh->second.m_subMeshes.size());
                m_meshes[objectIndex].m_subMeshes = subMeshes;
            }

            Mesh& mesh = m_meshes[objectIndex];

            // search for an existing BLAS instance entry for this mesh using the assetId
            BlasInstanceMap::iterator itMeshBlasInstance = m_blasInstanceMap.find(assetId);
            if (itMeshBlasInstance == m_blasInstanceMap.end())
            {
                // make a new BLAS map entry for this mesh
                MeshBlasInstance meshBlasInstance;
                meshBlasInstance.m_count = 1;
                meshBlasInstance.m_subMeshes.reserve(mesh.m_subMeshes.size());
                itMeshBlasInstance = m_blasInstanceMap.insert({ assetId, meshBlasInstance }).first;
            }
            else
            {
                itMeshBlasInstance->second.m_count++;
            }

            // create the BLAS buffers for each sub-mesh, or re-use existing BLAS objects if they were already created.
            // Note: all sub-meshes must either create new BLAS objects or re-use existing ones, otherwise it's an error (it's the same model in both cases)
            // Note: the buffer is just reserved here, the BLAS is built in the RayTracingAccelerationStructurePass
            bool blasInstanceFound = false;
            for (uint32_t subMeshIndex = 0; subMeshIndex < mesh.m_subMeshes.size(); ++subMeshIndex)
            {
                SubMesh& subMesh = mesh.m_subMeshes[subMeshIndex];

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

                    // create the BLAS object
                    subMesh.m_blas = AZ::RHI::RayTracingBlas::CreateRHIRayTracingBlas();

                    // create the buffers from the descriptor
                    subMesh.m_blas->CreateBuffers(*device, &blasDescriptor, *m_bufferPools);

                    // store the BLAS in the side list
                    itMeshBlasInstance->second.m_subMeshes.push_back({ subMesh.m_blas });
                }
            }

            if (blasInstanceFound)
            {
                // set the mesh BLAS flag so we don't try to rebuild it in the RayTracingAccelerationStructurePass
                mesh.m_blasBuilt = true;
            }

            // set initial transform
            mesh.m_transform = m_transformServiceFeatureProcessor->GetTransformForId(objectId);
            mesh.m_nonUniformScale = m_transformServiceFeatureProcessor->GetNonUniformScaleForId(objectId);

            m_revision++;
            m_subMeshCount += aznumeric_cast<uint32_t>(subMeshes.size());

            m_meshInfoBufferNeedsUpdate = true;
            m_materialInfoBufferNeedsUpdate = true;
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
                m_subMeshCount -= aznumeric_cast<uint32_t>(itMesh->second.m_subMeshes.size());
                m_meshes.erase(itMesh);
                m_revision++;

                // decrement the count from the BLAS instances, and check to see if we can remove them
                BlasInstanceMap::iterator itBlas = m_blasInstanceMap.find(itMesh->second.m_assetId);
                if (itBlas != m_blasInstanceMap.end())
                {
                    itBlas->second.m_count--;
                    if (itBlas->second.m_count == 0)
                    {
                        m_blasInstanceMap.erase(itBlas);
                    }
                }
            }

            m_meshInfoBufferNeedsUpdate = true;
            m_materialInfoBufferNeedsUpdate = true;
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
                itMesh->second.m_transform = transform;
                itMesh->second.m_nonUniformScale = nonUniformScale;
                m_revision++;
            }

            m_meshInfoBufferNeedsUpdate = true;
        }

        void RayTracingFeatureProcessor::UpdateRayTracingSrgs()
        {
            if (!m_tlas->GetTlasBuffer())
            {
                return;
            }

            if (m_rayTracingSceneSrg->IsQueuedForCompile() || m_rayTracingMaterialSrg->IsQueuedForCompile())
            {
                //[GFX TODO][ATOM-14792] AtomSampleViewer: Reset scene and feature processors before switching to sample
                return;
            }

            // update the mesh info buffer with the latest ray tracing enabled meshes
            UpdateMeshInfoBuffer();

            // update the material info buffer with the latest ray tracing enabled meshes
            UpdateMaterialInfoBuffer();

            // update the RayTracingSceneSrg
            UpdateRayTracingSceneSrg();

            // update the RayTracingMaterialSrg
            UpdateRayTracingMaterialSrg();
        }

        void RayTracingFeatureProcessor::UpdateMeshInfoBuffer()
        {
            if (m_meshInfoBufferNeedsUpdate && (m_subMeshCount > 0))
            {
                TransformServiceFeatureProcessor* transformFeatureProcessor = GetParentScene()->GetFeatureProcessor<TransformServiceFeatureProcessor>();

                AZStd::vector<MeshInfo> meshInfos;
                meshInfos.reserve(m_subMeshCount);

                uint32_t newMeshByteCount = m_subMeshCount * sizeof(MeshInfo);

                if (m_meshInfoBuffer == nullptr)
                {
                    // allocate the MeshInfo structured buffer
                    RPI::CommonBufferDescriptor desc;
                    desc.m_poolType = RPI::CommonBufferPoolType::ReadOnly;
                    desc.m_bufferName = "RayTracingMeshInfo";
                    desc.m_byteCount = newMeshByteCount;
                    desc.m_elementSize = sizeof(MeshInfo);
                    m_meshInfoBuffer = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
                }
                else if (m_meshInfoBuffer->GetBufferSize() < newMeshByteCount)
                {
                    // resize for the new sub-mesh count
                    m_meshInfoBuffer->Resize(newMeshByteCount);
                }

                // keep track of the start index of the buffers for each mesh, this is put into the MeshInfo
                // entry for each mesh so it knows where to find the start of its buffers in the unbounded array
                uint32_t bufferStartIndex = 0;

                for (const auto& mesh : m_meshes)
                {
                    AZ::Transform meshTransform = transformFeatureProcessor->GetTransformForId(TransformServiceFeatureProcessorInterface::ObjectId(mesh.first));
                    AZ::Transform noScaleTransform = meshTransform;
                    noScaleTransform.ExtractUniformScale();
                    AZ::Matrix3x3 rotationMatrix = Matrix3x3::CreateFromTransform(noScaleTransform);
                    rotationMatrix = rotationMatrix.GetInverseFull().GetTranspose();

                    const RayTracingFeatureProcessor::SubMeshVector& subMeshes = mesh.second.m_subMeshes;
                    for (const auto& subMesh : subMeshes)
                    {
                        MeshInfo meshInfo;
                        meshInfo.m_indexOffset = subMesh.m_indexBufferView.GetByteOffset();
                        meshInfo.m_positionOffset = subMesh.m_positionVertexBufferView.GetByteOffset();
                        meshInfo.m_normalOffset = subMesh.m_normalVertexBufferView.GetByteOffset();

                        if (RHI::CheckBitsAll(subMesh.m_bufferFlags, RayTracingSubMeshBufferFlags::Tangent))
                        {
                            meshInfo.m_tangentOffset = subMesh.m_tangentVertexBufferView.GetByteOffset();
                        }

                        if (RHI::CheckBitsAll(subMesh.m_bufferFlags, RayTracingSubMeshBufferFlags::Bitangent))
                        {
                            meshInfo.m_bitangentOffset = subMesh.m_bitangentVertexBufferView.GetByteOffset();
                        }

                        if (RHI::CheckBitsAll(subMesh.m_bufferFlags, RayTracingSubMeshBufferFlags::UV))
                        {
                            meshInfo.m_uvOffset = subMesh.m_uvVertexBufferView.GetByteOffset();
                        }

                        subMesh.m_irradianceColor.StoreToFloat4(meshInfo.m_irradianceColor.data());
                        rotationMatrix.StoreToRowMajorFloat9(meshInfo.m_worldInvTranspose.data());
                        meshInfo.m_bufferFlags = subMesh.m_bufferFlags;
                        meshInfo.m_bufferStartIndex = bufferStartIndex;

                        // add the count of buffers present in this subMesh to the start index for the next subMesh
                        // note that the Index, Position, and Normal buffers are always counted since they are guaranteed
                        static const uint32_t RayTracingSubMeshFixedStreamCount = 3;
                        bufferStartIndex += (RayTracingSubMeshFixedStreamCount + RHI::CountBitsSet(aznumeric_cast<uint32_t>(meshInfo.m_bufferFlags)));

                        meshInfos.emplace_back(meshInfo);
                    }
                }

                m_meshInfoBuffer->UpdateData(meshInfos.data(), newMeshByteCount);
                m_meshInfoBufferNeedsUpdate = false;
            }
        }

        void RayTracingFeatureProcessor::UpdateMaterialInfoBuffer()
        {
            if (m_materialInfoBufferNeedsUpdate && (m_subMeshCount > 0))
            {
                AZStd::vector<MaterialInfo> materialInfos;
                materialInfos.reserve(m_subMeshCount);

                uint32_t newMaterialByteCount = m_subMeshCount * sizeof(MaterialInfo);

                if (m_materialInfoBuffer == nullptr)
                {
                    // allocate the MaterialInfo structured buffer
                    RPI::CommonBufferDescriptor desc;
                    desc.m_poolType = RPI::CommonBufferPoolType::ReadOnly;
                    desc.m_bufferName = "RayTracingMaterialInfo";
                    desc.m_byteCount = newMaterialByteCount;
                    desc.m_elementSize = sizeof(MaterialInfo);
                    m_materialInfoBuffer = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
                }
                else if (m_materialInfoBuffer->GetBufferSize() < newMaterialByteCount)
                {
                    // resize for the new sub-mesh count
                    m_materialInfoBuffer->Resize(newMaterialByteCount);
                }

                // keep track of the start index of the textures for each mesh, this is put into the MaterialInfo
                // entry for each mesh so it knows where to find the start of its textures in the unbounded array
                uint32_t textureStartIndex = 0;

                for (const auto& mesh : m_meshes)
                {
                    const RayTracingFeatureProcessor::SubMeshVector& subMeshes = mesh.second.m_subMeshes;
                    for (const auto& subMesh : subMeshes)
                    {
                        MaterialInfo materialInfo;
                        subMesh.m_baseColor.StoreToFloat4(materialInfo.m_baseColor.data());
                        materialInfo.m_metallicFactor = subMesh.m_metallicFactor;
                        materialInfo.m_roughnessFactor = subMesh.m_roughnessFactor;
                        materialInfo.m_textureFlags = subMesh.m_textureFlags;
                        materialInfo.m_textureStartIndex = textureStartIndex;

                        // add the count of textures present in this subMesh to the start index for the next subMesh
                        textureStartIndex += RHI::CountBitsSet(aznumeric_cast<uint32_t>(materialInfo.m_textureFlags));

                        materialInfos.emplace_back(materialInfo);
                    }
                }

                m_materialInfoBuffer->UpdateData(materialInfos.data(), newMaterialByteCount);
                m_materialInfoBufferNeedsUpdate = false;
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
            m_rayTracingSceneSrg->SetBufferView(bufferIndex, m_meshInfoBuffer->GetBufferView());

            if (m_subMeshCount)
            {
                AZStd::vector<const RHI::BufferView*> meshBuffers;
                for (const auto& mesh : m_meshes)
                {
                    const SubMeshVector& subMeshes = mesh.second.m_subMeshes;
                    for (const auto& subMesh : subMeshes)
                    {
                        // add the stream buffers for this sub-mesh to the mesh buffer list,
                        // this is sent to the shader as an unbounded array in the Srg
                        meshBuffers.push_back(subMesh.m_indexShaderBufferView.get());
                        meshBuffers.push_back(subMesh.m_positionShaderBufferView.get());
                        meshBuffers.push_back(subMesh.m_normalShaderBufferView.get());

                        if (RHI::CheckBitsAll(subMesh.m_bufferFlags, RayTracingSubMeshBufferFlags::Tangent))
                        {
                            meshBuffers.push_back(subMesh.m_tangentShaderBufferView.get());
                        }

                        if (RHI::CheckBitsAll(subMesh.m_bufferFlags, RayTracingSubMeshBufferFlags::Bitangent))
                        {
                            meshBuffers.push_back(subMesh.m_bitangentShaderBufferView.get());
                        }

                        if (RHI::CheckBitsAll(subMesh.m_bufferFlags, RayTracingSubMeshBufferFlags::UV))
                        {
                            meshBuffers.push_back(subMesh.m_uvShaderBufferView.get());
                        }
                    }
                }

                // Check if buffer view data changed from previous frame.
                // Look into making 'm_meshBuffers != meshBuffers' faster by possibly building a crc and doing a crc check.
                if (m_meshBuffers.size() != meshBuffers.size() || m_meshBuffers != meshBuffers)
                {
                    m_meshBuffers = meshBuffers;
                    RHI::ShaderInputBufferUnboundedArrayIndex bufferUnboundedArrayIndex = srgLayout->FindShaderInputBufferUnboundedArrayIndex(AZ::Name("m_meshBuffers"));
                    m_rayTracingSceneSrg->SetBufferViewUnboundedArray(bufferUnboundedArrayIndex, m_meshBuffers);
                }
            }

            m_rayTracingSceneSrg->Compile();
        }

        void RayTracingFeatureProcessor::UpdateRayTracingMaterialSrg()
        {
            const RHI::ShaderResourceGroupLayout* srgLayout = m_rayTracingMaterialSrg->GetLayout();
            RHI::ShaderInputBufferIndex bufferIndex;

            bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_materialInfo"));
            m_rayTracingMaterialSrg->SetBufferView(bufferIndex, m_materialInfoBuffer->GetBufferView());

            if (m_subMeshCount)
            {
                AZStd::vector<const RHI::ImageView*> materialTextures;
                for (const auto& mesh : m_meshes)
                {
                    const SubMeshVector& subMeshes = mesh.second.m_subMeshes;
                    for (const auto& subMesh : subMeshes)
                    {
                        // add the baseColor, normal, metallic, and roughness images for this sub-mesh to the material texture list,
                        // this is sent to the shader as an unbounded array in the Srg
                        if (RHI::CheckBitsAll(subMesh.m_textureFlags, RayTracingSubMeshTextureFlags::BaseColor))
                        {
                            materialTextures.push_back(subMesh.m_baseColorImageView.get());
                        }

                        if (RHI::CheckBitsAll(subMesh.m_textureFlags, RayTracingSubMeshTextureFlags::Normal))
                        {
                            materialTextures.push_back(subMesh.m_normalImageView.get());
                        }

                        if (RHI::CheckBitsAll(subMesh.m_textureFlags, RayTracingSubMeshTextureFlags::Metallic))
                        {
                            materialTextures.push_back(subMesh.m_metallicImageView.get());
                        }

                        if (RHI::CheckBitsAll(subMesh.m_textureFlags, RayTracingSubMeshTextureFlags::Roughness))
                        {
                            materialTextures.push_back(subMesh.m_roughnessImageView.get());
                        }
                    }
                }

                // Check if image view data changed from previous frame. 
                if (m_materialTextures.size() != materialTextures.size() || m_materialTextures != materialTextures)
                {
                    m_materialTextures = materialTextures;
                    RHI::ShaderInputImageUnboundedArrayIndex textureUnboundedArrayIndex = srgLayout->FindShaderInputImageUnboundedArrayIndex(AZ::Name("m_materialTextures"));
                    m_rayTracingMaterialSrg->SetImageViewUnboundedArray(textureUnboundedArrayIndex, materialTextures);
                }
            }

            m_rayTracingMaterialSrg->Compile();
        }
    }        
}
