/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/RayTracing/RayTracingPass.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/RayTracingAccelerationStructure.h>
#include <Atom/RHI/RayTracingCompactionQueryPool.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <CoreLights/CapsuleLightFeatureProcessor.h>
#include <CoreLights/DirectionalLightFeatureProcessor.h>
#include <CoreLights/DiskLightFeatureProcessor.h>
#include <CoreLights/PointLightFeatureProcessor.h>
#include <CoreLights/QuadLightFeatureProcessor.h>
#include <CoreLights/SimplePointLightFeatureProcessor.h>
#include <CoreLights/SimpleSpotLightFeatureProcessor.h>
#include <ImageBasedLights/ImageBasedLightFeatureProcessor.h>
#include <RayTracing/RayTracingFeatureProcessor.h>

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
                    ->Version(1);
            }
        }

        void RayTracingFeatureProcessor::Activate()
        {
            auto deviceMask{RHI::RHISystemInterface::Get()->GetRayTracingSupport()};
            m_rayTracingEnabled = (deviceMask != RHI::MultiDevice::NoDevices);

            if (!m_rayTracingEnabled)
            {
                return;
            }
            
            m_transformServiceFeatureProcessor = GetParentScene()->GetFeatureProcessor<TransformServiceFeatureProcessorInterface>();

            // initialize the ray tracing buffer pools
            m_bufferPools = aznew RHI::RayTracingBufferPools;
            m_bufferPools->Init(deviceMask);

            auto deviceCount = RHI::RHISystemInterface::Get()->GetDeviceCount();

            for (auto deviceIndex{0}; deviceIndex < deviceCount; ++deviceIndex)
            {
                if ((AZStd::to_underlying(deviceMask) >> deviceIndex) & 1)
                {
                    m_meshBufferIndices[deviceIndex] = {};
                    m_materialTextureIndices[deviceIndex] = {};
                    m_meshInfos[deviceIndex] = {};
                    m_materialInfos[deviceIndex] = {};
                    m_proceduralGeometryMaterialInfos[deviceIndex] = {};
                }
            }

            // create TLAS attachmentId
            AZStd::string uuidString = AZ::Uuid::CreateRandom().ToString<AZStd::string>();
            m_tlasAttachmentId = RHI::AttachmentId(AZStd::string::format("RayTracingTlasAttachmentId_%s", uuidString.c_str()));

            // create the TLAS object
            m_tlas = aznew RHI::RayTracingTlas;

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

            // Setup RayTracingCompactionQueryPool
            {
                auto rpiDesc = RPI::RPISystemInterface::Get()->GetDescriptor();
                RHI::RayTracingCompactionQueryPoolDescriptor desc;
                desc.m_deviceMask = RHI::RHISystemInterface::Get()->GetRayTracingSupport();
                desc.m_budget = rpiDesc.m_rayTracingSystemDescriptor.m_rayTracingCompactionQueryPoolSize;
                desc.m_readbackBufferPool = AZ::RPI::BufferSystemInterface::Get()->GetCommonBufferPool(RPI::CommonBufferPoolType::ReadBack);
                desc.m_copyBufferPool = AZ::RPI::BufferSystemInterface::Get()->GetCommonBufferPool(RPI::CommonBufferPoolType::ReadWrite);
                m_compactionQueryPool = aznew RHI::RayTracingCompactionQueryPool;
                m_compactionQueryPool->Init(desc);
            }

            EnableSceneNotification();
        }

        void RayTracingFeatureProcessor::Deactivate()
        {
            DisableSceneNotification();
        }

        RayTracingFeatureProcessor::ProceduralGeometryTypeHandle RayTracingFeatureProcessor::RegisterProceduralGeometryType(
            const AZStd::string& name,
            const Data::Instance<RPI::Shader>& intersectionShader,
            const AZStd::string& intersectionShaderName,
            const AZStd::unordered_map<int, uint32_t>& bindlessBufferIndices)
        {
            ProceduralGeometryTypeHandle geometryTypeHandle;

            {
                ProceduralGeometryType proceduralGeometryType;
                proceduralGeometryType.m_name = AZ::Name(name);
                proceduralGeometryType.m_intersectionShader = intersectionShader;
                proceduralGeometryType.m_intersectionShaderName = AZ::Name(intersectionShaderName);
                proceduralGeometryType.m_bindlessBufferIndices = bindlessBufferIndices;

                AZStd::unique_lock<AZStd::mutex> lock(m_mutex);
                geometryTypeHandle = m_proceduralGeometryTypes.insert(proceduralGeometryType);
            }

            m_proceduralGeometryTypeRevision++;
            return geometryTypeHandle;
        }

        void RayTracingFeatureProcessor::SetProceduralGeometryTypeBindlessBufferIndex(
            ProceduralGeometryTypeWeakHandle geometryTypeHandle, const AZStd::unordered_map<int, uint32_t>& bindlessBufferIndices)
        {
            if (!m_rayTracingEnabled)
            {
                return;
            }

            geometryTypeHandle->m_bindlessBufferIndices = bindlessBufferIndices;
            m_proceduralGeometryInfoBufferNeedsUpdate = true;
        }

        void RayTracingFeatureProcessor::AddProceduralGeometry(
            ProceduralGeometryTypeWeakHandle geometryTypeHandle,
            const Uuid& uuid,
            const Aabb& aabb,
            const SubMeshMaterial& material,
            RHI::RayTracingAccelerationStructureInstanceInclusionMask instanceMask,
            uint32_t localInstanceIndex)
        {
            if (!m_rayTracingEnabled)
            {
                return;
            }

            RHI::Ptr<AZ::RHI::RayTracingBlas> rayTracingBlas = aznew AZ::RHI::RayTracingBlas;
            RHI::RayTracingBlasDescriptor blasDescriptor;
            blasDescriptor.Build()
                ->AABB(aabb)
                ;
            rayTracingBlas->CreateBuffers(RHI::MultiDevice::AllDevices, &blasDescriptor, *m_bufferPools);

            ProceduralGeometry proceduralGeometry;
            proceduralGeometry.m_uuid = uuid;
            proceduralGeometry.m_typeHandle = geometryTypeHandle;
            proceduralGeometry.m_aabb = aabb;
            proceduralGeometry.m_instanceMask = static_cast<uint32_t>(instanceMask);
            proceduralGeometry.m_blas = rayTracingBlas;
            proceduralGeometry.m_localInstanceIndex = localInstanceIndex;

            MeshBlasInstance meshBlasInstance;
            meshBlasInstance.m_count = 1;
            SubMeshBlasInstance subMeshBlasInstance;
            subMeshBlasInstance.m_blas = rayTracingBlas;
            meshBlasInstance.m_subMeshes.push_back(AZStd::move(subMeshBlasInstance));

            MaterialInfo materialInfo;

            AZStd::unique_lock<AZStd::mutex> lock(m_mutex);

            m_proceduralGeometryLookup.emplace(uuid, m_proceduralGeometry.size());
            m_proceduralGeometry.push_back(proceduralGeometry);
            auto deviceCount = RHI::RHISystemInterface::Get()->GetDeviceCount();

            for (auto deviceIndex{0}; deviceIndex < deviceCount; ++deviceIndex)
            {
                m_proceduralGeometryMaterialInfos[deviceIndex].emplace_back();
                ConvertMaterial(m_proceduralGeometryMaterialInfos[deviceIndex].back(), material, deviceIndex);
            }
            m_blasInstanceMap.emplace(Data::AssetId(uuid), meshBlasInstance);

            RHI::MultiDeviceObject::IterateDevices(
                RHI::RHISystemInterface::Get()->GetRayTracingSupport(),
                [&](int deviceIndex)
                {
                    m_blasToBuild[deviceIndex].insert(Data::AssetId(uuid));
                    return true;
                });

            geometryTypeHandle->m_instanceCount++;

            m_revision++;
            m_proceduralGeometryInfoBufferNeedsUpdate = true;
            m_materialInfoBufferNeedsUpdate = true;
            m_indexListNeedsUpdate = true;
        }

        void RayTracingFeatureProcessor::SetProceduralGeometryTransform(
            const Uuid& uuid, const Transform& transform, const Vector3& nonUniformScale)
        {
            if (!m_rayTracingEnabled)
            {
                return;
            }

            AZStd::unique_lock<AZStd::mutex> lock(m_mutex);

            if (auto it = m_proceduralGeometryLookup.find(uuid); it != m_proceduralGeometryLookup.end())
            {
                m_proceduralGeometry[it->second].m_transform = transform;
                m_proceduralGeometry[it->second].m_nonUniformScale = nonUniformScale;
            }

            m_revision++;
        }

        void RayTracingFeatureProcessor::SetProceduralGeometryLocalInstanceIndex(const Uuid& uuid, uint32_t localInstanceIndex)
        {
            if (!m_rayTracingEnabled)
            {
                return;
            }

            AZStd::unique_lock<AZStd::mutex> lock(m_mutex);

            if (auto it = m_proceduralGeometryLookup.find(uuid); it != m_proceduralGeometryLookup.end())
            {
                m_proceduralGeometry[it->second].m_localInstanceIndex = localInstanceIndex;
            }

            m_proceduralGeometryInfoBufferNeedsUpdate = true;
        }

        void RayTracingFeatureProcessor::SetProceduralGeometryMaterial(
            const Uuid& uuid, const RayTracingFeatureProcessor::SubMeshMaterial& material)
        {
            if (!m_rayTracingEnabled)
            {
                return;
            }

            auto deviceCount = RHI::RHISystemInterface::Get()->GetDeviceCount();

            AZStd::unique_lock<AZStd::mutex> lock(m_mutex);

            for (auto deviceIndex{0}; deviceIndex < deviceCount; ++deviceIndex)
            {
                if (auto it = m_proceduralGeometryLookup.find(uuid); it != m_proceduralGeometryLookup.end())
                {
                    ConvertMaterial(m_proceduralGeometryMaterialInfos[deviceIndex][it->second], material, deviceIndex);
                }
            }

            m_materialInfoBufferNeedsUpdate = true;
        }

        void RayTracingFeatureProcessor::RemoveProceduralGeometry(const Uuid& uuid)
        {
            if (!m_rayTracingEnabled)
            {
                return;
            }

            AZStd::unique_lock<AZStd::mutex> lock(m_mutex);

            size_t materialInfoIndex = m_proceduralGeometryLookup[uuid];
            m_proceduralGeometry[materialInfoIndex].m_typeHandle->m_instanceCount--;

            if (materialInfoIndex < m_proceduralGeometry.size() - 1)
            {
                m_proceduralGeometryLookup[m_proceduralGeometry.back().m_uuid] = m_proceduralGeometryLookup[uuid];
                m_proceduralGeometry[materialInfoIndex] = m_proceduralGeometry.back();
                for (auto& [deviceIndex, materialInfos] : m_proceduralGeometryMaterialInfos)
                {
                    materialInfos[materialInfoIndex] = materialInfos.back();
                }
            }

            m_proceduralGeometry.pop_back();

            for (auto& [deviceIndex, materialInfos] : m_proceduralGeometryMaterialInfos)
            {
                materialInfos.pop_back();
            }

            m_proceduralGeometryLookup.erase(uuid);
            RemoveBlasInstance(uuid);

            m_revision++;
            m_proceduralGeometryInfoBufferNeedsUpdate = true;
            m_materialInfoBufferNeedsUpdate = true;
            m_indexListNeedsUpdate = true;
        }

        int RayTracingFeatureProcessor::GetProceduralGeometryCount(ProceduralGeometryTypeWeakHandle geometryTypeHandle) const
        {
            return geometryTypeHandle->m_instanceCount;
        }

        void RayTracingFeatureProcessor::AddMesh(const AZ::Uuid& uuid, const Mesh& rayTracingMesh, const SubMeshVector& subMeshes)
        {
            if (!m_rayTracingEnabled)
            {
                return;
            }

            // lock the mutex to protect the mesh and BLAS lists
            AZStd::unique_lock<AZStd::mutex> lock(m_mutex);

            // check to see if we already have this mesh
            MeshMap::iterator itMesh = m_meshes.find(uuid);
            if (itMesh != m_meshes.end())
            {
                AZ_Assert(false, "AddMesh called on an existing Mesh, call RemoveMesh first");
                return;
            }

            // add the mesh
            m_meshes.insert(AZStd::make_pair(uuid, rayTracingMesh));
            Mesh& mesh = m_meshes[uuid];

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
                for (auto& [deviceIndex, meshInfos] : m_meshInfos)
                {
                    meshInfos.emplace_back();
                }
                for (auto& [deviceIndex, materialInfos] : m_materialInfos)
                {
                    materialInfos.emplace_back();
                }
            }

            mesh.m_subMeshIndices = subMeshIndices;

            // search for an existing BLAS instance entry for this mesh using the assetId
            BlasInstanceMap::iterator itMeshBlasInstance = m_blasInstanceMap.find(mesh.m_assetId);
            if (itMeshBlasInstance == m_blasInstanceMap.end())
            {
                // make a new BLAS map entry for this mesh
                MeshBlasInstance meshBlasInstance;
                meshBlasInstance.m_count = 1;
                meshBlasInstance.m_subMeshes.reserve(mesh.m_subMeshIndices.size());
                meshBlasInstance.m_isSkinnedMesh = mesh.m_isSkinnedMesh;
                itMeshBlasInstance = m_blasInstanceMap.insert({ mesh.m_assetId, meshBlasInstance }).first;

                // Note: the build flags are set to be the same for each BLAS created for the mesh
                RHI::RayTracingAccelerationStructureBuildFlags buildFlags =
                    CreateRayTracingAccelerationStructureBuildFlags(mesh.m_isSkinnedMesh);
                for (uint32_t subMeshIndex = 0; subMeshIndex < mesh.m_subMeshIndices.size(); ++subMeshIndex)
                {
                    const SubMesh& subMesh = m_subMeshes[mesh.m_subMeshIndices[subMeshIndex]];

                    SubMeshBlasInstance subMeshBlasInstance;
                    subMeshBlasInstance.m_blasDescriptor.Build()
                        ->Geometry()
                        ->VertexFormat(subMesh.m_positionFormat)
                        ->VertexBuffer(subMesh.m_positionVertexBufferView)
                        ->IndexBuffer(subMesh.m_indexBufferView)
                        ->BuildFlags(buildFlags);

                    itMeshBlasInstance->second.m_subMeshes.push_back(subMeshBlasInstance);
                }
                m_blasToCreate.insert(mesh.m_assetId);
            }
            else
            {
                itMeshBlasInstance->second.m_count++;
                AZ_Assert(itMeshBlasInstance->second.m_subMeshes.size() == mesh.m_subMeshIndices.size(), "");
            }

            for (uint32_t subMeshIndex = 0; subMeshIndex < mesh.m_subMeshIndices.size(); ++subMeshIndex)
            {
                m_subMeshes[mesh.m_subMeshIndices[subMeshIndex]].m_blasInstanceId = { mesh.m_assetId, subMeshIndex };
            }

            AZ::Transform noScaleTransform = mesh.m_transform;
            noScaleTransform.ExtractUniformScale();
            AZ::Matrix3x3 rotationMatrix = Matrix3x3::CreateFromTransform(noScaleTransform);
            rotationMatrix = rotationMatrix.GetInverseFull().GetTranspose();
            Matrix3x4 worldInvTranspose3x4 = Matrix3x4::CreateFromMatrix3x3(rotationMatrix);

            Matrix3x4 reflectionProbeModelToWorld3x4 = Matrix3x4::CreateFromTransform(mesh.m_reflectionProbe.m_modelToWorld);

            // store the mesh buffers and material textures in the resource lists
            for (uint32_t subMeshIndex : mesh.m_subMeshIndices)
            {
                SubMesh& subMesh = m_subMeshes[subMeshIndex];
                AZ_Assert(subMesh.m_indexShaderBufferView.get(), "RayTracing Mesh IndexBuffer cannot be null");
                AZ_Assert(subMesh.m_positionShaderBufferView.get(), "RayTracing Mesh PositionBuffer cannot be null");
                AZ_Assert(subMesh.m_normalShaderBufferView.get(), "RayTracing Mesh NormalBuffer cannot be null");

                for (auto& [deviceIndex, meshInfos] : m_meshInfos)
                {
                    MeshInfo& meshInfo = meshInfos[subMesh.m_globalIndex];

                    worldInvTranspose3x4.StoreToRowMajorFloat12(meshInfo.m_worldInvTranspose.data());
                    meshInfo.m_bufferFlags = subMesh.m_bufferFlags;

                    meshInfo.m_indexByteOffset = subMesh.m_indexBufferView.GetByteOffset();
                    meshInfo.m_positionByteOffset = subMesh.m_positionVertexBufferView.GetByteOffset();
                    meshInfo.m_normalByteOffset = subMesh.m_normalVertexBufferView.GetByteOffset();
                    meshInfo.m_tangentByteOffset =
                        subMesh.m_tangentShaderBufferView ? subMesh.m_tangentVertexBufferView.GetByteOffset() : 0;
                    meshInfo.m_bitangentByteOffset =
                        subMesh.m_bitangentShaderBufferView ? subMesh.m_bitangentVertexBufferView.GetByteOffset() : 0;
                    meshInfo.m_uvByteOffset = subMesh.m_uvShaderBufferView ? subMesh.m_uvVertexBufferView.GetByteOffset() : 0;

                    auto& materialInfos{ m_materialInfos[deviceIndex] };
                    MaterialInfo& materialInfo = materialInfos[subMesh.m_globalIndex];
                    ConvertMaterial(materialInfo, subMesh.m_material, deviceIndex);

                    auto& meshBufferIndices = m_meshBufferIndices[deviceIndex];

                    // add mesh buffers
                    meshInfo.m_bufferStartIndex = meshBufferIndices.AddEntry(
                    {
#if USE_BINDLESS_SRG
                        subMesh.m_indexShaderBufferView.get() ? subMesh.m_indexShaderBufferView->GetDeviceBufferView(deviceIndex)->GetBindlessReadIndex() : InvalidIndex,
                        subMesh.m_positionShaderBufferView.get() ? subMesh.m_positionShaderBufferView->GetDeviceBufferView(deviceIndex)->GetBindlessReadIndex() : InvalidIndex,
                        subMesh.m_normalShaderBufferView.get() ? subMesh.m_normalShaderBufferView->GetDeviceBufferView(deviceIndex)->GetBindlessReadIndex() : InvalidIndex,
                        subMesh.m_tangentShaderBufferView.get() ? subMesh.m_tangentShaderBufferView->GetDeviceBufferView(deviceIndex)->GetBindlessReadIndex() : InvalidIndex,
                        subMesh.m_bitangentShaderBufferView.get() ? subMesh.m_bitangentShaderBufferView->GetDeviceBufferView(deviceIndex)->GetBindlessReadIndex() : InvalidIndex,
                        subMesh.m_uvShaderBufferView.get() ? subMesh.m_uvShaderBufferView->GetDeviceBufferView(deviceIndex)->GetBindlessReadIndex() : InvalidIndex
#else
                        m_meshBuffers.AddResource(subMesh.m_indexShaderBufferView.get()),
                        m_meshBuffers.AddResource(subMesh.m_positionShaderBufferView.get()),
                        m_meshBuffers.AddResource(subMesh.m_normalShaderBufferView.get()),
                        m_meshBuffers.AddResource(subMesh.m_tangentShaderBufferView.get()),
                        m_meshBuffers.AddResource(subMesh.m_bitangentShaderBufferView.get()),
                        m_meshBuffers.AddResource(subMesh.m_uvShaderBufferView.get())
#endif
                    });

                    // add reflection probe data
                    if (mesh.m_reflectionProbe.m_reflectionProbeCubeMap.get())
                    {
                        materialInfo.m_reflectionProbeCubeMapIndex = mesh.m_reflectionProbe.m_reflectionProbeCubeMap->GetImageView()->GetDeviceImageView(deviceIndex)->GetBindlessReadIndex();
                        if (materialInfo.m_reflectionProbeCubeMapIndex != InvalidIndex)
                        {
                            reflectionProbeModelToWorld3x4.StoreToRowMajorFloat12(materialInfo.m_reflectionProbeData.m_modelToWorld.data());
                            reflectionProbeModelToWorld3x4.GetInverseFull().StoreToRowMajorFloat12(materialInfo.m_reflectionProbeData.m_modelToWorldInverse.data());
                            mesh.m_reflectionProbe.m_outerObbHalfLengths.StoreToFloat3(materialInfo.m_reflectionProbeData.m_outerObbHalfLengths.data());
                            mesh.m_reflectionProbe.m_innerObbHalfLengths.StoreToFloat3(materialInfo.m_reflectionProbeData.m_innerObbHalfLengths.data());
                            materialInfo.m_reflectionProbeData.m_useReflectionProbe = true;
                            materialInfo.m_reflectionProbeData.m_useParallaxCorrection = mesh.m_reflectionProbe.m_useParallaxCorrection;
                            materialInfo.m_reflectionProbeData.m_exposure = mesh.m_reflectionProbe.m_exposure;
                        }
                    }
                }
            }

            m_revision++;
            m_subMeshCount += aznumeric_cast<uint32_t>(subMeshes.size());

            m_meshInfoBufferNeedsUpdate = true;
            m_materialInfoBufferNeedsUpdate = true;
            m_indexListNeedsUpdate = true;
        }

        void RayTracingFeatureProcessor::RemoveMesh(const AZ::Uuid& uuid)
        {
            if (!m_rayTracingEnabled)
            {
                return;
            }

            // lock the mutex to protect the mesh and BLAS lists
            AZStd::unique_lock<AZStd::mutex> lock(m_mutex);

            MeshMap::iterator itMesh = m_meshes.find(uuid);
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
                        if (itBlas->second.m_isSkinnedMesh)
                        {
                            --m_skinnedMeshCount;
                        }
                        RemoveBlasInstance(mesh.m_assetId);
                    }
                }

                // remove the SubMeshes
                for (auto& subMeshIndex : mesh.m_subMeshIndices)
                {
                    SubMesh& subMesh = m_subMeshes[subMeshIndex];
                    uint32_t globalIndex = subMesh.m_globalIndex;

                    for (auto& [deviceIndex, meshInfos] : m_meshInfos)
                    {
                        MeshInfo& meshInfo = meshInfos[globalIndex];
                        auto& meshBufferIndices = m_meshBufferIndices[deviceIndex];
                        meshBufferIndices.RemoveEntry(meshInfo.m_bufferStartIndex);
                    }
                    for (auto& [deviceIndex, materialTextureIndices] : m_materialTextureIndices)
                    {
                        MaterialInfo& materialInfo = m_materialInfos[deviceIndex][globalIndex];
                        materialTextureIndices.RemoveEntry(materialInfo.m_textureStartIndex);
                    }

#if !USE_BINDLESS_SRG
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
                    m_materialTextures.RemoveResource(subMesh.m_emissiveImageView.get());
#endif

                    if (globalIndex < m_subMeshes.size() - 1)
                    {
                        // the subMesh we're removing is in the middle of the global lists, remove by swapping the last element to its position in the list
                        m_subMeshes[globalIndex] = m_subMeshes.back();

                        for (auto& [deviceIndex, meshInfos] : m_meshInfos)
                        {
                            auto& materialInfos{ m_materialInfos[deviceIndex] };
                            meshInfos[globalIndex] = meshInfos.back();
                            materialInfos[globalIndex] = materialInfos.back();
                        }

                        // update the global index for the swapped subMesh
                        m_subMeshes[globalIndex].m_globalIndex = globalIndex;

                        // update the global index in the parent Mesh' subMesh list
                        Mesh* swappedSubMeshParent = m_subMeshes[globalIndex].m_mesh;
                        uint32_t swappedSubMeshIndex = m_subMeshes[globalIndex].m_subMeshIndex;
                        swappedSubMeshParent->m_subMeshIndices[swappedSubMeshIndex] = globalIndex;
                    }

                    m_subMeshes.pop_back();
                    for (auto& [deviceIndex, meshInfos] : m_meshInfos)
                    {
                        auto& materialInfos{ m_materialInfos[deviceIndex] };
                        meshInfos.pop_back();
                        materialInfos.pop_back();
                    }
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

                    for (auto& [deviceIndex, meshInfos] : m_meshInfos)
                    {
                        meshInfos.clear();
                    }
                    for (auto& [deviceIndex, materialInfos] : m_materialInfos)
                    {
                        materialInfos.clear();
                    }

                    for (auto& [deviceIndex, meshBufferIndices] : m_meshBufferIndices)
                    {
                        meshBufferIndices.Reset();
                    }
                    for (auto& [deviceIndex, materialTextureIndices] : m_materialTextureIndices)
                    {
                        materialTextureIndices.Reset();
                    }

#if !USE_BINDLESS_SRG
                    m_meshBuffers.Reset();
                    m_materialTextures.Reset();
#endif
                }
            }

            m_meshInfoBufferNeedsUpdate = true;
            m_materialInfoBufferNeedsUpdate = true;
            m_indexListNeedsUpdate = true;
        }

        void RayTracingFeatureProcessor::SetMeshTransform(const AZ::Uuid& uuid, const AZ::Transform transform, const AZ::Vector3 nonUniformScale)
        {
            if (!m_rayTracingEnabled)
            {
                return;
            }

            AZStd::unique_lock<AZStd::mutex> lock(m_mutex);

            MeshMap::iterator itMesh = m_meshes.find(uuid);
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
                    for (auto& [deviceIndex, meshInfos] : m_meshInfos)
                    {
                        MeshInfo& meshInfo = meshInfos[subMeshIndex];
                        worldInvTranspose3x4.StoreToRowMajorFloat12(meshInfo.m_worldInvTranspose.data());
                    }
                }

                m_meshInfoBufferNeedsUpdate = true;
            }
        }

        void RayTracingFeatureProcessor::SetMeshReflectionProbe(const AZ::Uuid& uuid, const Mesh::ReflectionProbe& reflectionProbe)
        {
            if (!m_rayTracingEnabled)
            {
                return;
            }

            AZStd::unique_lock<AZStd::mutex> lock(m_mutex);

            MeshMap::iterator itMesh = m_meshes.find(uuid);
            if (itMesh != m_meshes.end())
            {
                Mesh& mesh = itMesh->second;

                // update the Mesh reflection probe data
                mesh.m_reflectionProbe = reflectionProbe;

                // update all of the subMeshes
                const Data::Instance<RPI::Image>& reflectionProbeCubeMap = reflectionProbe.m_reflectionProbeCubeMap;
                Matrix3x4 reflectionProbeModelToWorld3x4 = Matrix3x4::CreateFromTransform(mesh.m_reflectionProbe.m_modelToWorld);

                for (auto& subMeshIndex : mesh.m_subMeshIndices)
                {
                    SubMesh& subMesh = m_subMeshes[subMeshIndex];
                    uint32_t globalIndex = subMesh.m_globalIndex;

                    for (auto& [deviceIndex, materialInfos] : m_materialInfos)
                    {
                        MaterialInfo& materialInfo = materialInfos[globalIndex];

                        materialInfo.m_reflectionProbeCubeMapIndex = reflectionProbeCubeMap.get()
                            ? reflectionProbeCubeMap->GetImageView()->GetDeviceImageView(deviceIndex)->GetBindlessReadIndex()
                            : InvalidIndex;
                        if (materialInfo.m_reflectionProbeCubeMapIndex != InvalidIndex)
                        {
                            reflectionProbeModelToWorld3x4.StoreToRowMajorFloat12(materialInfo.m_reflectionProbeData.m_modelToWorld.data());
                            reflectionProbeModelToWorld3x4.GetInverseFull().StoreToRowMajorFloat12(materialInfo.m_reflectionProbeData.m_modelToWorldInverse.data());
                            mesh.m_reflectionProbe.m_outerObbHalfLengths.StoreToFloat3(materialInfo.m_reflectionProbeData.m_outerObbHalfLengths.data());
                            mesh.m_reflectionProbe.m_innerObbHalfLengths.StoreToFloat3(materialInfo.m_reflectionProbeData.m_innerObbHalfLengths.data());
                            materialInfo.m_reflectionProbeData.m_useReflectionProbe = true;
                            materialInfo.m_reflectionProbeData.m_useParallaxCorrection = mesh.m_reflectionProbe.m_useParallaxCorrection;
                            materialInfo.m_reflectionProbeData.m_exposure = mesh.m_reflectionProbe.m_exposure;
                        }
                        else
                        {
                            materialInfo.m_reflectionProbeData.m_useReflectionProbe = false;
                        }
                    }
                }

                m_materialInfoBufferNeedsUpdate = true;
            }
        }

        void RayTracingFeatureProcessor::SetMeshMaterials(const AZ::Uuid& uuid, const SubMeshMaterialVector& subMeshMaterials)
        {
            if (!m_rayTracingEnabled)
            {
                return;
            }

            AZStd::unique_lock<AZStd::mutex> lock(m_mutex);

            MeshMap::iterator itMesh = m_meshes.find(uuid);
            if (itMesh != m_meshes.end())
            {
                Mesh& mesh = itMesh->second;

                AZ_Assert(
                    subMeshMaterials.size() == mesh.m_subMeshIndices.size(),
                    "The size of subMeshes in SetMeshMaterial must be the same as in AddMesh");

                for (auto& subMeshIndex : mesh.m_subMeshIndices)
                {
                    const SubMesh& subMesh = m_subMeshes[subMeshIndex];
                    for (auto& [deviceIndex, materialInfos] : m_materialInfos)
                    {
                        ConvertMaterial(materialInfos[subMesh.m_globalIndex], subMeshMaterials[subMesh.m_subMeshIndex], deviceIndex);
                    }
                }

                m_materialInfoBufferNeedsUpdate = true;
                m_indexListNeedsUpdate = true;
            }
        }

        void RayTracingFeatureProcessor::Render(const RenderPacket&)
        {
            m_frameIndex++;
        }

        void RayTracingFeatureProcessor::BeginFrame()
        {
            if (m_updatedFrameIndex == m_frameIndex)
            {
                // Make sure the update is only called once per frame
                // When multiple devices are present a RayTracingAccelerationStructurePass is created per device
                // Thus this function is called once for each device
                return;
            }
            m_updatedFrameIndex = m_frameIndex;

            m_compactionQueryPool->BeginFrame(m_frameIndex);
            UpdateBlasInstances();

            if (m_tlasRevision != m_revision)
            {
                m_tlasRevision = m_revision;

                // create the TLAS descriptor
                RHI::RayTracingTlasDescriptor tlasDescriptor;
                RHI::RayTracingTlasDescriptor* tlasDescriptorBuild = tlasDescriptor.Build();

                uint32_t instanceIndex = 0;
                for (auto& subMesh : m_subMeshes)
                {
                    const auto& blasInstance =
                        m_blasInstanceMap.at(subMesh.m_blasInstanceId.first).m_subMeshes[subMesh.m_blasInstanceId.second];
                    auto& blas = blasInstance.m_compactBlas ? blasInstance.m_compactBlas : blasInstance.m_blas;
                    if (blas)
                    {
                        tlasDescriptorBuild->Instance()
                            ->InstanceID(instanceIndex)
                            ->InstanceMask(subMesh.m_mesh->m_instanceMask)
                            ->HitGroupIndex(0)
                            ->Blas(blas)
                            ->Transform(subMesh.m_mesh->m_transform)
                            ->NonUniformScale(subMesh.m_mesh->m_nonUniformScale)
                            ->Transparent(subMesh.m_material.m_irradianceColor.GetA() < 1.0f);
                    }

                    instanceIndex++;
                }

                unsigned proceduralHitGroupIndex = 1; // Hit group 0 is used for normal meshes
                AZStd::unordered_map<Name, unsigned> geometryTypeMap;
                geometryTypeMap.reserve(m_proceduralGeometryTypes.size());
                for (auto it = m_proceduralGeometryTypes.cbegin(); it != m_proceduralGeometryTypes.cend(); ++it)
                {
                    geometryTypeMap[it->m_name] = proceduralHitGroupIndex++;
                }

                for (const auto& proceduralGeometry : m_proceduralGeometry)
                {
                    tlasDescriptorBuild->Instance()
                        ->InstanceID(instanceIndex)
                        ->InstanceMask(proceduralGeometry.m_instanceMask)
                        ->HitGroupIndex(geometryTypeMap[proceduralGeometry.m_typeHandle->m_name])
                        ->Blas(proceduralGeometry.m_blas)
                        ->Transform(proceduralGeometry.m_transform)
                        ->NonUniformScale(proceduralGeometry.m_nonUniformScale);
                    instanceIndex++;
                }

                // create the TLAS buffers based on the descriptor
                RHI::Ptr<RHI::RayTracingTlas>& rayTracingTlas = m_tlas;
                rayTracingTlas->CreateBuffers(RHI::RHISystemInterface::Get()->GetRayTracingSupport(), &tlasDescriptor, *m_bufferPools);
            }

            // update and compile the RayTracingSceneSrg and RayTracingMaterialSrg
            // Note: the timing of this update is very important, it needs to be updated after the TLAS is allocated so it can
            // be set on the RayTracingSceneSrg for this frame, and the ray tracing mesh data in the RayTracingSceneSrg must
            // exactly match the TLAS.  Any mismatch in this data may result in a TDR.
            UpdateRayTracingSrgs();
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

            if (HasMeshGeometry())
            {
                UpdateMeshInfoBuffer();
            }
            if (HasProceduralGeometry())
            {
                UpdateProceduralGeometryInfoBuffer();
            }
            if (HasGeometry())
            {
                UpdateMaterialInfoBuffer();
                UpdateIndexLists();
            }

            UpdateRayTracingSceneSrg();
            UpdateRayTracingMaterialSrg();
        }

        const void RayTracingFeatureProcessor::MarkBlasInstanceForCompaction(int deviceIndex, Data::AssetId assetId)
        {
            AZStd::unique_lock lock(m_queueMutex);
            auto it = m_blasInstanceMap.find(assetId);
            if (RHI::Validation::IsEnabled())
            {
                if (it != m_blasInstanceMap.end())
                {
                    for ([[maybe_unused]] auto& subMeshInstance : it->second.m_subMeshes)
                    {
                        AZ_Assert(
                            subMeshInstance.m_compactionSizeQuery, "Enqueuing a Blas without an compaction size query for compaction");
                    }
                }
            }

            m_blasEnqueuedForCompact[assetId].m_frameIndex = static_cast<int>(m_frameIndex + RHI::Limits::Device::FrameCountMax);
            m_blasEnqueuedForCompact[assetId].m_deviceMask = RHI::SetBit(m_blasEnqueuedForCompact[assetId].m_deviceMask, deviceIndex);
        }

        const void RayTracingFeatureProcessor::MarkBlasInstanceAsCompactionEnqueued(int deviceIndex, Data::AssetId assetId)
        {
            AZStd::unique_lock lock(m_queueMutex);
            auto it = m_blasInstanceMap.find(assetId);
            if (RHI::Validation::IsEnabled())
            {
                if (it != m_blasInstanceMap.end())
                {
                    for ([[maybe_unused]] auto& subMeshInstance : it->second.m_subMeshes)
                    {
                        AZ_Assert(subMeshInstance.m_compactBlas, "Marking a Blas without a compacted Blas as enqueued for compaction");
                    }
                }
            }

            m_uncompactedBlasEnqueuedForDeletion[assetId].m_frameIndex =
                static_cast<int>(m_frameIndex + RHI::Limits::Device::FrameCountMax);
            m_uncompactedBlasEnqueuedForDeletion[assetId].m_deviceMask =
                RHI::SetBit(m_uncompactedBlasEnqueuedForDeletion[assetId].m_deviceMask, deviceIndex);
        }

        void RayTracingFeatureProcessor::UpdateBlasInstances()
        {
            bool changed = false;
            auto rpiDesc = RPI::RPISystemInterface::Get()->GetDescriptor();
            {
                uint32_t numModelBlasCreated = 0;
                uint32_t numCompactionQueriesEnqueued = 0;
                AZStd::unordered_set<Data::AssetId> toRemoveFromCreateList;
                for (auto assetId : m_blasToCreate)
                {
                    auto it = m_blasInstanceMap.find(assetId);
                    if (it == m_blasInstanceMap.end())
                    {
                        toRemoveFromCreateList.insert(assetId);
                        continue;
                    }
                    auto& instance = it->second;

                    {
                        int numSubmeshesWithCompactionQuery = 0;
                        for (auto& subMeshInstance : instance.m_subMeshes)
                        {
                            // create the BLAS object and store it in the BLAS list
                            if (RHI::CheckBitsAny(
                                    subMeshInstance.m_blasDescriptor.GetBuildFlags(),
                                    RHI::RayTracingAccelerationStructureBuildFlags::ENABLE_COMPACTION))
                            {
                                numSubmeshesWithCompactionQuery++;
                            }
                        }
                        if (numCompactionQueriesEnqueued + numSubmeshesWithCompactionQuery >
                            rpiDesc.m_rayTracingSystemDescriptor.m_rayTracingCompactionQueryPoolSize)
                        {
                            break;
                        }
                    }

                    for (auto& subMeshInstance : instance.m_subMeshes)
                    {
                        // create the BLAS object and store it in the BLAS list
                        RHI::Ptr<RHI::RayTracingBlas> rayTracingBlas = aznew RHI::RayTracingBlas;
                        if (RHI::CheckBitsAny(
                                subMeshInstance.m_blasDescriptor.GetBuildFlags(),
                                RHI::RayTracingAccelerationStructureBuildFlags::ENABLE_COMPACTION))
                        {
                            subMeshInstance.m_compactionSizeQuery = aznew RHI::RayTracingCompactionQuery;
                            m_compactionQueryPool->InitQuery(subMeshInstance.m_compactionSizeQuery.get());
                            numCompactionQueriesEnqueued++;
                        }
                        subMeshInstance.m_blas = rayTracingBlas;
                        // create the buffers from the BLAS descriptor
                        subMeshInstance.m_blas->CreateBuffers(
                            RHI::RHISystemInterface::Get()->GetRayTracingSupport(), &subMeshInstance.m_blasDescriptor, *m_bufferPools);
                    }

                    if (instance.m_isSkinnedMesh)
                    {
                        ++m_skinnedMeshCount;
                        m_skinnedBlasIds.insert(assetId);
                    }
                    else
                    {
                        RHI::MultiDeviceObject::IterateDevices(
                            RHI::RHISystemInterface::Get()->GetRayTracingSupport(),
                            [&](int deviceIndex)
                            {
                                m_blasToBuild[deviceIndex].insert(assetId);
                                return true;
                            });
                    }
                    toRemoveFromCreateList.insert(assetId);
                    changed = true;
                    numModelBlasCreated++;
                    if (rpiDesc.m_rayTracingSystemDescriptor.m_maxBlasCreatedPerFrame > 0 &&
                        numModelBlasCreated >= static_cast<uint32_t>(rpiDesc.m_rayTracingSystemDescriptor.m_maxBlasCreatedPerFrame))
                    {
                        break;
                    }
                }
                for (auto toRemove : toRemoveFromCreateList)
                {
                    m_blasToCreate.erase(toRemove);
                }
            }

            // Check which Blas are ready for compaction and create compacted acceleration structures for them
            {
                AZStd::unordered_set<Data::AssetId> toDelete;
                for (const auto& [assetId, frameEvent] : m_blasEnqueuedForCompact)
                {
                    if (frameEvent.m_frameIndex <= m_frameIndex)
                    {
                        auto it = m_blasInstanceMap.find(assetId);
                        if (it != m_blasInstanceMap.end())
                        {
                            // Limit the number of blas we enqueue per frame to the size of the compaction query pool
                            for (int subMeshIdx = 0; subMeshIdx < it->second.m_subMeshes.size(); subMeshIdx++)
                            {
                                auto& subMeshInstance = it->second.m_subMeshes[subMeshIdx];
                                AZ_Assert(!subMeshInstance.m_compactBlas, "Trying to compact a Blas twice");
                                AZ_Assert(
                                    frameEvent.m_deviceMask == RHI::RHISystemInterface::Get()->GetRayTracingSupport(),
                                    "All device Blas of a SubMesh must be compacted in the same frame");
                                AZStd::unordered_map<int, uint64_t> sizes;
                                RHI::MultiDeviceObject::IterateDevices(
                                    frameEvent.m_deviceMask,
                                    [&](int deviceIndex)
                                    {
                                        sizes[deviceIndex] =
                                            subMeshInstance.m_compactionSizeQuery->GetDeviceRayTracingCompactionQuery(deviceIndex)
                                                ->GetResult();
                                        return true;
                                    });
                                subMeshInstance.m_compactBlas = aznew RHI::RayTracingBlas;
                                subMeshInstance.m_compactBlas->CreateCompactedBuffers(*subMeshInstance.m_blas, sizes, *m_bufferPools);
                                subMeshInstance.m_compactionSizeQuery = {};
                                changed = true;
                            }
                            RHI::MultiDeviceObject::IterateDevices(
                                RHI::RHISystemInterface::Get()->GetRayTracingSupport(),
                                [&, assetId = assetId](int deviceIndex)
                                {
                                    m_blasToCompact[deviceIndex].insert(assetId);
                                    return true;
                                });
                        }
                        toDelete.insert(assetId);
                    }
                }
                for (auto assetId : toDelete)
                {
                    m_blasEnqueuedForCompact.erase(assetId);
                }
            }

            // Check which uncompacted Blas can be deleted, and delete them
            {
                AZStd::unordered_set<Data::AssetId> toDelete;
                for (const auto& [assetId, frameEvent] : m_uncompactedBlasEnqueuedForDeletion)
                {
                    if (frameEvent.m_frameIndex <= m_frameIndex)
                    {
                        auto it = m_blasInstanceMap.find(assetId);
                        if (it != m_blasInstanceMap.end())
                        {
                            for (auto& subMeshInstance : it->second.m_subMeshes)
                            {
                                AZ_Assert(
                                    subMeshInstance.m_compactBlas, "Deleting a uncompacted Blas from a submesh without a compacted one");
                                // We assume here that all device Blas are handled at the same frame for all devices
                                subMeshInstance.m_blas = {};
                                changed = true;
                            }
                        }
                        toDelete.insert(assetId);
                    }
                }
                for (auto assetId : toDelete)
                {
                    m_uncompactedBlasEnqueuedForDeletion.erase(assetId);
                }
            }

            if (changed)
            {
                m_revision++;
            }
        }

        void RayTracingFeatureProcessor::UpdateMeshInfoBuffer()
        {
            if (m_meshInfoBufferNeedsUpdate)
            {
                AZStd::unordered_map<int, const void*> rawMeshInfos;

                for (auto& [deviceIndex, meshInfos] : m_meshInfos)
                {
                    rawMeshInfos[deviceIndex] = meshInfos.data();
                }

                size_t meshInfoByteCount = m_meshInfos.begin()->second.size() * sizeof(MeshInfo);
                m_meshInfoGpuBuffer.AdvanceCurrentBufferAndUpdateData(rawMeshInfos, meshInfoByteCount);
                m_meshInfoBufferNeedsUpdate = false;
            }
        }

        void RayTracingFeatureProcessor::UpdateProceduralGeometryInfoBuffer()
        {
            if (!m_proceduralGeometryInfoBufferNeedsUpdate)
            {
                return;
            }

            AZStd::unordered_map<int, AZStd::vector<uint32_t>> proceduralGeometryInfos;

            for (const auto& proceduralGeometry : m_proceduralGeometry)
            {
                for (auto& [deviceIndex, bindlessBufferIndex] : proceduralGeometry.m_typeHandle->m_bindlessBufferIndices)
                {
                    auto& proceduralGeometryInfo = proceduralGeometryInfos[deviceIndex];

                    if (proceduralGeometryInfo.empty())
                    {
                        proceduralGeometryInfo.reserve(m_proceduralGeometry.size() * 2);
                    }

                    proceduralGeometryInfo.push_back(bindlessBufferIndex);
                    proceduralGeometryInfo.push_back(proceduralGeometry.m_localInstanceIndex);
                }
            }

            AZStd::unordered_map<int, const void*> rawProceduralGeometryInfos;

            for (auto& [deviceIndex, proceduralGeometryInfo] : proceduralGeometryInfos)
            {
                rawProceduralGeometryInfos[deviceIndex] = proceduralGeometryInfo.data();
            }

            m_proceduralGeometryInfoGpuBuffer.AdvanceCurrentBufferAndUpdateData(
                rawProceduralGeometryInfos, m_proceduralGeometry.size() * 2 * sizeof(uint32_t));
            m_proceduralGeometryInfoBufferNeedsUpdate = false;
        }

        void RayTracingFeatureProcessor::UpdateMaterialInfoBuffer()
        {
            if (m_materialInfoBufferNeedsUpdate)
            {
                m_materialInfoGpuBuffer.AdvanceCurrentElement();
                m_materialInfoGpuBuffer.CreateOrResizeCurrentBufferWithElementCount<MaterialInfo>(
                    m_subMeshCount + m_proceduralGeometryMaterialInfos.begin()->second.size());
                m_materialInfoGpuBuffer.UpdateCurrentBufferData(m_materialInfos);
                m_materialInfoGpuBuffer.UpdateCurrentBufferData(m_proceduralGeometryMaterialInfos, m_subMeshCount);
                m_materialInfoBufferNeedsUpdate = false;
            }
        }

        void RayTracingFeatureProcessor::UpdateIndexLists()
        {
            if (m_indexListNeedsUpdate)
            {
#if !USE_BINDLESS_SRG
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

                m_meshBufferIndicesGpuBuffer.AdvanceCurrentBufferAndUpdateData(resolvedMeshBufferIndices);
#else
                AZStd::unordered_map<int, const void*> rawMeshData;

                for (auto& [deviceIndex, meshBufferIndices] : m_meshBufferIndices)
                {
                    rawMeshData[deviceIndex] = meshBufferIndices.GetIndexList().data();
                }

                size_t newMeshBufferIndicesByteCount = m_meshBufferIndices.begin()->second.GetIndexList().size() * sizeof(uint32_t);
                m_meshBufferIndicesGpuBuffer.AdvanceCurrentBufferAndUpdateData(rawMeshData, newMeshBufferIndicesByteCount);
#endif

#if !USE_BINDLESS_SRG
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

                m_materialTextureIndicesGpuBuffer.AdvanceCurrentBufferAndUpdateData(resolvedMaterialTextureIndices);
#else
                AZStd::unordered_map<int, const void*> rawMaterialData;

                for (auto& [deviceIndex, materialTextureIndices] : m_materialTextureIndices)
                {
                    rawMaterialData[deviceIndex] = materialTextureIndices.GetIndexList().data();
                }

                size_t newMaterialTextureIndicesByteCount = m_materialTextureIndices.begin()->second.GetIndexList().size() * sizeof(uint32_t);
                m_materialTextureIndicesGpuBuffer.AdvanceCurrentBufferAndUpdateData(rawMaterialData, newMaterialTextureIndicesByteCount);
#endif

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
            m_rayTracingSceneSrg->SetBufferView(bufferIndex, m_tlas->GetTlasBuffer()->BuildBufferView(bufferViewDescriptor).get());

            // directional lights
            const auto directionalLightFP = GetParentScene()->GetFeatureProcessor<DirectionalLightFeatureProcessor>();
            bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_directionalLights"));
            m_rayTracingSceneSrg->SetBufferView(
                bufferIndex,
                directionalLightFP->GetLightBuffer()->GetBufferView());

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_directionalLightCount"));
            m_rayTracingSceneSrg->SetConstant(constantIndex, directionalLightFP->GetLightCount());

            // simple point lights
            const auto simplePointLightFP = GetParentScene()->GetFeatureProcessor<SimplePointLightFeatureProcessor>();
            bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_simplePointLights"));
            m_rayTracingSceneSrg->SetBufferView(
                bufferIndex,
                simplePointLightFP->GetLightBuffer()->GetBufferView());

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_simplePointLightCount"));
            m_rayTracingSceneSrg->SetConstant(constantIndex, simplePointLightFP->GetLightCount());

            // simple spot lights
            const auto simpleSpotLightFP = GetParentScene()->GetFeatureProcessor<SimpleSpotLightFeatureProcessor>();
            bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_simpleSpotLights"));
            m_rayTracingSceneSrg->SetBufferView(
                bufferIndex,
                simpleSpotLightFP->GetLightBuffer()->GetBufferView());

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_simpleSpotLightCount"));
            m_rayTracingSceneSrg->SetConstant(constantIndex, simpleSpotLightFP->GetLightCount());

            // point lights (sphere)
            const auto pointLightFP = GetParentScene()->GetFeatureProcessor<PointLightFeatureProcessor>();
            bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_pointLights"));
            m_rayTracingSceneSrg->SetBufferView(
                bufferIndex,
                pointLightFP->GetLightBuffer()->GetBufferView());

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_pointLightCount"));
            m_rayTracingSceneSrg->SetConstant(constantIndex, pointLightFP->GetLightCount());

            // disk lights
            const auto diskLightFP = GetParentScene()->GetFeatureProcessor<DiskLightFeatureProcessor>();
            bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_diskLights"));
            m_rayTracingSceneSrg->SetBufferView(
                bufferIndex,
                diskLightFP->GetLightBuffer()->GetBufferView());

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_diskLightCount"));
            m_rayTracingSceneSrg->SetConstant(constantIndex, diskLightFP->GetLightCount());

            // capsule lights
            const auto capsuleLightFP = GetParentScene()->GetFeatureProcessor<CapsuleLightFeatureProcessor>();
            bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_capsuleLights"));
            m_rayTracingSceneSrg->SetBufferView(
                bufferIndex,
                capsuleLightFP->GetLightBuffer()->GetBufferView());

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_capsuleLightCount"));
            m_rayTracingSceneSrg->SetConstant(constantIndex, capsuleLightFP->GetLightCount());

            // quad lights
            const auto quadLightFP = GetParentScene()->GetFeatureProcessor<QuadLightFeatureProcessor>();
            bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_quadLights"));
            m_rayTracingSceneSrg->SetBufferView(
                bufferIndex,
                quadLightFP->GetLightBuffer()->GetBufferView());

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

            if (m_meshInfoGpuBuffer.IsCurrentBufferValid())
            {
                bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_meshInfo"));
                m_rayTracingSceneSrg->SetBufferView(bufferIndex, m_meshInfoGpuBuffer.GetCurrentBufferView());
            }

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_meshInfoCount"));
            m_rayTracingSceneSrg->SetConstant(constantIndex, m_subMeshCount);

            bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_meshBufferIndices"));
            m_rayTracingSceneSrg->SetBufferView(bufferIndex, m_meshBufferIndicesGpuBuffer.GetCurrentBufferView());

            if (m_proceduralGeometryInfoGpuBuffer.IsCurrentBufferValid())
            {
                bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_proceduralGeometryInfo"));
                m_rayTracingSceneSrg->SetBufferView(bufferIndex, m_proceduralGeometryInfoGpuBuffer.GetCurrentBufferView());
            }

#if !USE_BINDLESS_SRG
            RHI::ShaderInputBufferUnboundedArrayIndex bufferUnboundedArrayIndex = srgLayout->FindShaderInputBufferUnboundedArrayIndex(AZ::Name("m_meshBuffers"));
            m_rayTracingSceneSrg->SetBufferViewUnboundedArray(bufferUnboundedArrayIndex, m_meshBuffers.GetResourceList());
#endif
            m_rayTracingSceneSrg->Compile();
        }

        void RayTracingFeatureProcessor::UpdateRayTracingMaterialSrg()
        {
            const RHI::ShaderResourceGroupLayout* srgLayout = m_rayTracingMaterialSrg->GetLayout();
            RHI::ShaderInputBufferIndex bufferIndex;

            bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_materialInfo"));
            m_rayTracingMaterialSrg->SetBufferView(bufferIndex, m_materialInfoGpuBuffer.GetCurrentBufferView());

            bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_materialTextureIndices"));
            m_rayTracingMaterialSrg->SetBufferView(bufferIndex, m_materialTextureIndicesGpuBuffer.GetCurrentBufferView());

#if !USE_BINDLESS_SRG
            RHI::ShaderInputImageUnboundedArrayIndex textureUnboundedArrayIndex = srgLayout->FindShaderInputImageUnboundedArrayIndex(AZ::Name("m_materialTextures"));
            m_rayTracingMaterialSrg->SetImageViewUnboundedArray(textureUnboundedArrayIndex, m_materialTextures.GetResourceList());
#endif
            m_rayTracingMaterialSrg->Compile();
        }

        void RayTracingFeatureProcessor::OnRenderPipelineChanged([[maybe_unused]] RPI::RenderPipeline* renderPipeline, RPI::SceneNotification::RenderPipelineChangeType changeType)
        {
            if (!m_rayTracingEnabled)
            {
                return;
            }

            // determine which devices need RayTracingAccelerationStructurePasses and distribute multiple existing ones to the devices
            AZ::RPI::Pass* firstRayTracingAccelerationStructurePass{ nullptr };
            auto rayTracingDeviceMask{ RHI::RHISystemInterface::Get()->GetRayTracingSupport() };
            AZ::RHI::MultiDevice::DeviceMask devicesToAdd{ rayTracingDeviceMask };

            // only enable the RayTracingAccelerationStructurePass for each device on the first pipeline in this scene, this will avoid
            // multiple updates to the same AS
            AZ::RPI::PassFilter passFilter =
                AZ::RPI::PassFilter::CreateWithTemplateName(AZ::Name("RayTracingAccelerationStructurePassTemplate"), GetParentScene());
            AZ::RPI::PassSystemInterface::Get()->ForEachPass(
                passFilter,
                [&devicesToAdd, &firstRayTracingAccelerationStructurePass, &rayTracingDeviceMask](
                    AZ::RPI::Pass* pass) -> AZ::RPI::PassFilterExecutionFlow
                {
                    if (!firstRayTracingAccelerationStructurePass)
                    {
                        firstRayTracingAccelerationStructurePass = pass;
                    }

                    // we always set an invalid device index to the first available device
                    if (pass->GetDeviceIndex() == RHI::MultiDevice::InvalidDeviceIndex)
                    {
                        pass->SetDeviceIndex(az_ctz_u32(AZStd::to_underlying(rayTracingDeviceMask)));
                    }

                    auto mask = RHI::MultiDevice::DeviceMask(AZ_BIT(pass->GetDeviceIndex()));

                    // only have one RayTracingAccelerationStructurePass per device
                    pass->SetEnabled((mask & devicesToAdd) != RHI::MultiDevice::NoDevices);
                    devicesToAdd &= ~mask;

                    return AZ::RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                });

            // we only add the passes on the other devices if the pipeline contains one in the first place
            if (firstRayTracingAccelerationStructurePass && changeType != RPI::SceneNotification::RenderPipelineChangeType::Removed &&
                renderPipeline->FindFirstPass(firstRayTracingAccelerationStructurePass->GetName()))
            {
                // add passes for the remaining devices
                while (devicesToAdd != RHI::MultiDevice::NoDevices)
                {
                    auto deviceIndex{ az_ctz_u32(AZStd::to_underlying(devicesToAdd)) };

                    AZStd::shared_ptr<RPI::PassRequest> passRequest = AZStd::make_shared<RPI::PassRequest>();
                    passRequest->m_templateName = Name("RayTracingAccelerationStructurePassTemplate");
                    passRequest->m_passName = Name("RayTracingAccelerationStructurePass" + AZStd::to_string(deviceIndex));

                    AZStd::shared_ptr<RPI::PassData> passData = AZStd::make_shared<RPI::PassData>();
                    passData->m_deviceIndex = deviceIndex;
                    passRequest->m_passData = passData;

                    auto pass = RPI::PassSystemInterface::Get()->CreatePassFromRequest(passRequest.get());

                    renderPipeline->AddPassAfter(pass, firstRayTracingAccelerationStructurePass->GetName());

                    devicesToAdd &= RHI::MultiDevice::DeviceMask(~AZ_BIT(deviceIndex));
                }
            }
        }

        void RayTracingFeatureProcessor::RemoveBlasInstance(Data::AssetId id)
        {
            m_blasInstanceMap.erase(id);
            m_blasToCreate.erase(id);
            m_skinnedBlasIds.erase(id);
            for (auto& [deviceIndex, entries] : m_blasToBuild)
            {
                entries.erase(id);
            }
            for (auto& [deviceIndex, entries] : m_blasToCompact)
            {
                entries.erase(id);
            }
            m_blasEnqueuedForCompact.erase(id);
            m_uncompactedBlasEnqueuedForDeletion.erase(id);
        }

        AZ::RHI::RayTracingAccelerationStructureBuildFlags RayTracingFeatureProcessor::CreateRayTracingAccelerationStructureBuildFlags(bool isSkinnedMesh)
        {
            AZ::RHI::RayTracingAccelerationStructureBuildFlags buildFlags;
            if (isSkinnedMesh)
            {
                buildFlags = AZ::RHI::RayTracingAccelerationStructureBuildFlags::ENABLE_UPDATE | AZ::RHI::RayTracingAccelerationStructureBuildFlags::FAST_BUILD;
            }
            else
            {
                buildFlags = AZ::RHI::RayTracingAccelerationStructureBuildFlags::FAST_TRACE;

                auto rpiDesc = RPI::RPISystemInterface::Get()->GetDescriptor();
                if (rpiDesc.m_rayTracingSystemDescriptor.m_enableBlasCompaction)
                {
                    buildFlags = buildFlags | RHI::RayTracingAccelerationStructureBuildFlags::ENABLE_COMPACTION;
                }
            }

            return buildFlags;
        }

        void RayTracingFeatureProcessor::ConvertMaterial(MaterialInfo& materialInfo, const SubMeshMaterial& subMeshMaterial, int deviceIndex)
        {
            subMeshMaterial.m_baseColor.StoreToFloat4(materialInfo.m_baseColor.data());
            subMeshMaterial.m_emissiveColor.StoreToFloat4(materialInfo.m_emissiveColor.data());
            subMeshMaterial.m_irradianceColor.StoreToFloat4(materialInfo.m_irradianceColor.data());
            materialInfo.m_metallicFactor = subMeshMaterial.m_metallicFactor;
            materialInfo.m_roughnessFactor = subMeshMaterial.m_roughnessFactor;
            materialInfo.m_textureFlags = subMeshMaterial.m_textureFlags;

            if (materialInfo.m_textureStartIndex != InvalidIndex)
            {
                m_materialTextureIndices[deviceIndex].RemoveEntry(materialInfo.m_textureStartIndex);
#if !USE_BINDLESS_SRG
                m_materialTextures.RemoveResource(subMeshMaterial.m_baseColorImageView.get());
                m_materialTextures.RemoveResource(subMeshMaterial.m_normalImageView.get());
                m_materialTextures.RemoveResource(subMeshMaterial.m_metallicImageView.get());
                m_materialTextures.RemoveResource(subMeshMaterial.m_roughnessImageView.get());
                m_materialTextures.RemoveResource(subMeshMaterial.m_emissiveImageView.get());
#endif
            }

            materialInfo.m_textureStartIndex = m_materialTextureIndices[deviceIndex].AddEntry({
#if USE_BINDLESS_SRG
                subMeshMaterial.m_baseColorImageView.get() ? subMeshMaterial.m_baseColorImageView->GetDeviceImageView(deviceIndex)->GetBindlessReadIndex() : InvalidIndex,
                subMeshMaterial.m_normalImageView.get() ? subMeshMaterial.m_normalImageView->GetDeviceImageView(deviceIndex)->GetBindlessReadIndex() : InvalidIndex,
                subMeshMaterial.m_metallicImageView.get() ? subMeshMaterial.m_metallicImageView->GetDeviceImageView(deviceIndex)->GetBindlessReadIndex() : InvalidIndex,
                subMeshMaterial.m_roughnessImageView.get() ? subMeshMaterial.m_roughnessImageView->GetDeviceImageView(deviceIndex)->GetBindlessReadIndex() : InvalidIndex,
                subMeshMaterial.m_emissiveImageView.get() ? subMeshMaterial.m_emissiveImageView->GetDeviceImageView(deviceIndex)->GetBindlessReadIndex() : InvalidIndex
#else
                m_materialTextures.AddResource(subMeshMaterial.m_baseColorImageView.get()),
                m_materialTextures.AddResource(subMeshMaterial.m_normalImageView.get()),
                m_materialTextures.AddResource(subMeshMaterial.m_metallicImageView.get()),
                m_materialTextures.AddResource(subMeshMaterial.m_roughnessImageView.get()),
                m_materialTextures.AddResource(subMeshMaterial.m_emissiveImageView.get())
#endif
            });
        }
    } // namespace Render
}
