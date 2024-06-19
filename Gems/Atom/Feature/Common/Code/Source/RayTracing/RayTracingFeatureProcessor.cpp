/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RayTracing/RayTracingFeatureProcessor.h>
#include <RayTracing/RayTracingPass.h>
#include <Atom/Feature/TransformService/TransformServiceFeatureProcessor.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
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
                    ->Version(1);
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
            uint32_t bindlessBufferIndex)
        {
            ProceduralGeometryTypeHandle geometryTypeHandle;

            {
                ProceduralGeometryType proceduralGeometryType;
                proceduralGeometryType.m_name = AZ::Name(name);
                proceduralGeometryType.m_intersectionShader = intersectionShader;
                proceduralGeometryType.m_intersectionShaderName = AZ::Name(intersectionShaderName);
                proceduralGeometryType.m_bindlessBufferIndex = bindlessBufferIndex;

                AZStd::unique_lock<AZStd::mutex> lock(m_mutex);
                geometryTypeHandle = m_proceduralGeometryTypes.insert(proceduralGeometryType);
            }

            m_proceduralGeometryTypeRevision++;
            return geometryTypeHandle;
        }

        void RayTracingFeatureProcessor::SetProceduralGeometryTypeBindlessBufferIndex(
            ProceduralGeometryTypeWeakHandle geometryTypeHandle, uint32_t bindlessBufferIndex)
        {
            if (!m_rayTracingEnabled)
            {
                return;
            }

            geometryTypeHandle->m_bindlessBufferIndex = bindlessBufferIndex;
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

            RHI::Ptr<RHI::RayTracingBlas> rayTracingBlas = AZ::RHI::RayTracingBlas::CreateRHIRayTracingBlas();
            RHI::Ptr<RHI::Device> device = RHI::RHISystemInterface::Get()->GetDevice();
            RHI::RayTracingBlasDescriptor blasDescriptor;
            blasDescriptor.Build()
                ->AABB(aabb)
                ;
            rayTracingBlas->CreateBuffers(*device, &blasDescriptor, *m_bufferPools);

            ProceduralGeometry proceduralGeometry;
            proceduralGeometry.m_uuid = uuid;
            proceduralGeometry.m_typeHandle = geometryTypeHandle;
            proceduralGeometry.m_aabb = aabb;
            proceduralGeometry.m_instanceMask = static_cast<uint32_t>(instanceMask);
            proceduralGeometry.m_blas = rayTracingBlas;
            proceduralGeometry.m_localInstanceIndex = localInstanceIndex;

            MeshBlasInstance meshBlasInstance;
            meshBlasInstance.m_count = 1;
            meshBlasInstance.m_subMeshes.push_back(SubMeshBlasInstance{ rayTracingBlas });

            MaterialInfo materialInfo;
            ConvertMaterial(materialInfo, material);

            AZStd::unique_lock<AZStd::mutex> lock(m_mutex);

            m_proceduralGeometryLookup.emplace(uuid, m_proceduralGeometry.size());
            m_proceduralGeometry.push_back(proceduralGeometry);
            m_proceduralGeometryMaterialInfos.push_back(materialInfo);
            m_blasInstanceMap.emplace(Data::AssetId(uuid), meshBlasInstance);
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

            AZStd::unique_lock<AZStd::mutex> lock(m_mutex);

            if (auto it = m_proceduralGeometryLookup.find(uuid); it != m_proceduralGeometryLookup.end())
            {
                ConvertMaterial(m_proceduralGeometryMaterialInfos[it->second], material);
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

            if (materialInfoIndex < m_proceduralGeometryMaterialInfos.size() - 1)
            {
                m_proceduralGeometryLookup[m_proceduralGeometry.back().m_uuid] = m_proceduralGeometryLookup[uuid];
                m_proceduralGeometry[materialInfoIndex] = m_proceduralGeometry.back();
                m_proceduralGeometryMaterialInfos[materialInfoIndex] = m_proceduralGeometryMaterialInfos.back();
            }
            m_proceduralGeometry.pop_back();
            m_proceduralGeometryMaterialInfos.pop_back();

            m_blasInstanceMap.erase(uuid);
            m_proceduralGeometryLookup.erase(uuid);

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

            RHI::Ptr<RHI::Device> device = RHI::RHISystemInterface::Get()->GetDevice();

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
                m_meshInfos.emplace_back();
                m_materialInfos.emplace_back();
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
                if (mesh.m_isSkinnedMesh)
                {
                    ++m_skinnedMeshCount;
                }
            }
            else
            {
                itMeshBlasInstance->second.m_count++;
            }

            // create the BLAS buffers for each sub-mesh, or re-use existing BLAS objects if they were already created.
            // Note: all sub-meshes must either create new BLAS objects or re-use existing ones, otherwise it's an error (it's the same model in both cases)
            // Note: the buffer is just reserved here, the BLAS is built in the RayTracingAccelerationStructurePass
            // Note: the build flags are set to be the same for each BLAS created for the mesh
            RHI::RayTracingAccelerationStructureBuildFlags buildFlags = CreateRayTracingAccelerationStructureBuildFlags(mesh.m_isSkinnedMesh);
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
                        ->BuildFlags(buildFlags)
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
                MeshInfo& meshInfo = m_meshInfos[subMesh.m_globalIndex];
                MaterialInfo& materialInfo = m_materialInfos[subMesh.m_globalIndex];

                ConvertMaterial(materialInfo, subMesh.m_material);

                worldInvTranspose3x4.StoreToRowMajorFloat12(meshInfo.m_worldInvTranspose.data());
                meshInfo.m_bufferFlags = subMesh.m_bufferFlags;

                AZ_Assert(subMesh.m_indexShaderBufferView.get(), "RayTracing Mesh IndexBuffer cannot be null");
                AZ_Assert(subMesh.m_positionShaderBufferView.get(), "RayTracing Mesh PositionBuffer cannot be null");
                AZ_Assert(subMesh.m_normalShaderBufferView.get(), "RayTracing Mesh NormalBuffer cannot be null");

                // add mesh buffers
                meshInfo.m_bufferStartIndex = m_meshBufferIndices.AddEntry(
                {
#if USE_BINDLESS_SRG
                    subMesh.m_indexShaderBufferView.get() ? subMesh.m_indexShaderBufferView->GetBindlessReadIndex() : InvalidIndex,
                    subMesh.m_positionShaderBufferView.get() ? subMesh.m_positionShaderBufferView->GetBindlessReadIndex() : InvalidIndex,
                    subMesh.m_normalShaderBufferView.get() ? subMesh.m_normalShaderBufferView->GetBindlessReadIndex() : InvalidIndex,
                    subMesh.m_tangentShaderBufferView.get() ? subMesh.m_tangentShaderBufferView->GetBindlessReadIndex() : InvalidIndex,
                    subMesh.m_bitangentShaderBufferView.get() ? subMesh.m_bitangentShaderBufferView->GetBindlessReadIndex() : InvalidIndex,
                    subMesh.m_uvShaderBufferView.get() ? subMesh.m_uvShaderBufferView->GetBindlessReadIndex() : InvalidIndex
#else
                    m_meshBuffers.AddResource(subMesh.m_indexShaderBufferView.get()),
                    m_meshBuffers.AddResource(subMesh.m_positionShaderBufferView.get()),
                    m_meshBuffers.AddResource(subMesh.m_normalShaderBufferView.get()),
                    m_meshBuffers.AddResource(subMesh.m_tangentShaderBufferView.get()),
                    m_meshBuffers.AddResource(subMesh.m_bitangentShaderBufferView.get()),
                    m_meshBuffers.AddResource(subMesh.m_uvShaderBufferView.get())
#endif
                });

                meshInfo.m_indexByteOffset = subMesh.m_indexBufferView.GetByteOffset();
                meshInfo.m_positionByteOffset = subMesh.m_positionVertexBufferView.GetByteOffset();
                meshInfo.m_normalByteOffset = subMesh.m_normalVertexBufferView.GetByteOffset();
                meshInfo.m_tangentByteOffset = subMesh.m_tangentShaderBufferView ? subMesh.m_tangentVertexBufferView.GetByteOffset() : 0;
                meshInfo.m_bitangentByteOffset = subMesh.m_bitangentShaderBufferView ? subMesh.m_bitangentVertexBufferView.GetByteOffset() : 0;
                meshInfo.m_uvByteOffset = subMesh.m_uvShaderBufferView ? subMesh.m_uvVertexBufferView.GetByteOffset() : 0;

                // add reflection probe data
                if (mesh.m_reflectionProbe.m_reflectionProbeCubeMap.get())
                {
                    materialInfo.m_reflectionProbeCubeMapIndex = mesh.m_reflectionProbe.m_reflectionProbeCubeMap->GetImageView()->GetBindlessReadIndex();
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
                    MeshInfo& meshInfo = m_meshInfos[subMeshIndex];
                    worldInvTranspose3x4.StoreToRowMajorFloat12(meshInfo.m_worldInvTranspose.data());
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
                uint32_t reflectionProbeCubeMapIndex = reflectionProbeCubeMap.get() ? reflectionProbeCubeMap->GetImageView()->GetBindlessReadIndex() : InvalidIndex;
                Matrix3x4 reflectionProbeModelToWorld3x4 = Matrix3x4::CreateFromTransform(mesh.m_reflectionProbe.m_modelToWorld);

                for (auto& subMeshIndex : mesh.m_subMeshIndices)
                {
                    SubMesh& subMesh = m_subMeshes[subMeshIndex];
                    uint32_t globalIndex = subMesh.m_globalIndex;
                    MaterialInfo& materialInfo = m_materialInfos[globalIndex];

                    materialInfo.m_reflectionProbeCubeMapIndex = reflectionProbeCubeMapIndex;
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
                    ConvertMaterial(m_materialInfos[subMesh.m_globalIndex], subMeshMaterials[subMesh.m_subMeshIndex]);
                }

                m_materialInfoBufferNeedsUpdate = true;
                m_indexListNeedsUpdate = true;
            }
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

        void RayTracingFeatureProcessor::UpdateMeshInfoBuffer()
        {
            if (m_meshInfoBufferNeedsUpdate)
            {
                m_meshInfoGpuBuffer.AdvanceCurrentBufferAndUpdateData(m_meshInfos);
                m_meshInfoBufferNeedsUpdate = false;
            }
        }

        void RayTracingFeatureProcessor::UpdateProceduralGeometryInfoBuffer()
        {
            if (!m_proceduralGeometryInfoBufferNeedsUpdate)
            {
                return;
            }

            AZStd::vector<uint32_t> proceduralGeometryInfo;
            proceduralGeometryInfo.reserve(m_proceduralGeometry.size() * 2);
            for (const auto& proceduralGeometry : m_proceduralGeometry)
            {
                proceduralGeometryInfo.push_back(proceduralGeometry.m_typeHandle->m_bindlessBufferIndex);
                proceduralGeometryInfo.push_back(proceduralGeometry.m_localInstanceIndex);
            }

            m_proceduralGeometryInfoGpuBuffer.AdvanceCurrentBufferAndUpdateData(proceduralGeometryInfo);
            m_proceduralGeometryInfoBufferNeedsUpdate = false;
        }

        void RayTracingFeatureProcessor::UpdateMaterialInfoBuffer()
        {
            if (m_materialInfoBufferNeedsUpdate)
            {
                m_materialInfoGpuBuffer.AdvanceCurrentElement();
                m_materialInfoGpuBuffer.CreateOrResizeCurrentBufferWithElementCount<MaterialInfo>(
                    m_subMeshCount + m_proceduralGeometryMaterialInfos.size());
                m_materialInfoGpuBuffer.UpdateCurrentBufferData(m_materialInfos);
                m_materialInfoGpuBuffer.UpdateCurrentBufferData(m_proceduralGeometryMaterialInfos, m_materialInfos.size());

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
                m_meshBufferIndicesGpuBuffer.AdvanceCurrentBufferAndUpdateData(m_meshBufferIndices.GetIndexList());
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
                m_materialTextureIndicesGpuBuffer.AdvanceCurrentBufferAndUpdateData(m_materialTextureIndices.GetIndexList());
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

            // only enable the RayTracingAccelerationStructurePass on the first pipeline in this scene, this will avoid multiple updates to the same AS
            bool enabled = true;
            if (changeType == RPI::SceneNotification::RenderPipelineChangeType::Added
                || changeType == RPI::SceneNotification::RenderPipelineChangeType::Removed)
            {
                AZ::RPI::PassFilter passFilter = AZ::RPI::PassFilter::CreateWithPassName(AZ::Name("RayTracingAccelerationStructurePass"), GetParentScene());
                AZ::RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [&enabled](AZ::RPI::Pass* pass) -> AZ::RPI::PassFilterExecutionFlow
                    {
                        pass->SetEnabled(enabled);
                        enabled = false;

                        return AZ::RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                    });
            }
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
            }

            return buildFlags;
        }

        void RayTracingFeatureProcessor::ConvertMaterial(MaterialInfo& materialInfo, const SubMeshMaterial& subMeshMaterial)
        {
            subMeshMaterial.m_baseColor.StoreToFloat4(materialInfo.m_baseColor.data());
            subMeshMaterial.m_emissiveColor.StoreToFloat4(materialInfo.m_emissiveColor.data());
            subMeshMaterial.m_irradianceColor.StoreToFloat4(materialInfo.m_irradianceColor.data());
            materialInfo.m_metallicFactor = subMeshMaterial.m_metallicFactor;
            materialInfo.m_roughnessFactor = subMeshMaterial.m_roughnessFactor;
            materialInfo.m_textureFlags = subMeshMaterial.m_textureFlags;

            if (materialInfo.m_textureStartIndex != InvalidIndex)
            {
                m_materialTextureIndices.RemoveEntry(materialInfo.m_textureStartIndex);
#if !USE_BINDLESS_SRG
                m_materialTextures.RemoveResource(subMesh.m_baseColorImageView.get());
                m_materialTextures.RemoveResource(subMesh.m_normalImageView.get());
                m_materialTextures.RemoveResource(subMesh.m_metallicImageView.get());
                m_materialTextures.RemoveResource(subMesh.m_roughnessImageView.get());
                m_materialTextures.RemoveResource(subMesh.m_emissiveImageView.get());
#endif
            }

            materialInfo.m_textureStartIndex = m_materialTextureIndices.AddEntry({
#if USE_BINDLESS_SRG
                subMeshMaterial.m_baseColorImageView.get() ? subMeshMaterial.m_baseColorImageView->GetBindlessReadIndex() : InvalidIndex,
                subMeshMaterial.m_normalImageView.get() ? subMeshMaterial.m_normalImageView->GetBindlessReadIndex() : InvalidIndex,
                subMeshMaterial.m_metallicImageView.get() ? subMeshMaterial.m_metallicImageView->GetBindlessReadIndex() : InvalidIndex,
                subMeshMaterial.m_roughnessImageView.get() ? subMeshMaterial.m_roughnessImageView->GetBindlessReadIndex() : InvalidIndex,
                subMeshMaterial.m_emissiveImageView.get() ? subMeshMaterial.m_emissiveImageView->GetBindlessReadIndex() : InvalidIndex
#else
                m_materialTextures.AddResource(subMeshMaterial.m_baseColorImageView.get()),
                m_materialTextures.AddResource(subMeshMaterial.m_normalImageView.get()),
                m_materialTextures.AddResource(subMeshMaterial.m_metallicImageView.get()),
                m_materialTextures.AddResource(subMeshMaterial.m_roughnessImageView.get()),
                m_materialTextures.AddResource(subMeshMaterial.m_emissiveImageView.get())
#endif
            });
        }
    }
}
