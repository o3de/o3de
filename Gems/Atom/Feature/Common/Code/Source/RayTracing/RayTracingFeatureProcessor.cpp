/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/Mesh/MeshFeatureProcessorInterface.h>
#include <Atom/Feature/RayTracing/RayTracingPass.h>
#include <Atom/Feature/TransformService/TransformServiceFeatureProcessorInterface.h>
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
            m_meshFeatureProcessor = GetParentScene()->GetFeatureProcessor<MeshFeatureProcessorInterface>();

            // initialize the ray tracing buffer pools
            m_bufferPools = aznew RHI::RayTracingBufferPools;
            m_bufferPools->Init(deviceMask);

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
            const FallbackPBR::MaterialParameters& material,
            RHI::RayTracingAccelerationStructureInstanceInclusionMask instanceMask,
            uint32_t localInstanceIndex)
        {
            if (!m_rayTracingEnabled)
            {
                return;
            }

            RHI::Ptr<AZ::RHI::RayTracingBlas> rayTracingBlas = aznew AZ::RHI::RayTracingBlas;
            RHI::RayTracingBlasDescriptor blasDescriptor;
            blasDescriptor.m_aabb = aabb;
            rayTracingBlas->CreateBuffers(m_deviceMask, &blasDescriptor, *m_bufferPools);

            ProceduralGeometry proceduralGeometry;
            proceduralGeometry.m_uuid = uuid;
            // acquire an empty meshInfo - entry
            proceduralGeometry.m_meshInfoHandle = m_meshFeatureProcessor->AcquireMeshInfoEntry();
            proceduralGeometry.m_typeHandle = geometryTypeHandle;
            proceduralGeometry.m_aabb = aabb;
            proceduralGeometry.m_instanceMask = static_cast<uint32_t>(instanceMask);
            proceduralGeometry.m_blas = rayTracingBlas;
            proceduralGeometry.m_localInstanceIndex = localInstanceIndex;

            // Update the meshinfo-entry for the procedural mesh
            m_meshFeatureProcessor->UpdateMeshInfoEntry(
                proceduralGeometry.m_meshInfoHandle,
                [](MeshInfoEntry* entry)
                {
                    // enable all lighting channels for the procedural mesh.
                    entry->m_lightingChannels = AZStd::numeric_limits<uint32_t>::max();
                    return true;
                });

            // create a FallbackPBR material entry for the empty meshInfo entry
            m_meshFeatureProcessor->UpdateFallbackPBRMaterialEntry(
                proceduralGeometry.m_meshInfoHandle,
                [&material](FallbackPBR::MaterialEntry* entry)
                {
                    entry->m_materialParameters = material;
                    return true;
                });

            MeshBlasInstance meshBlasInstance;
            meshBlasInstance.m_count = 1;
            SubMeshBlasInstance subMeshBlasInstance;
            subMeshBlasInstance.m_blas = rayTracingBlas;
            meshBlasInstance.m_subMeshes.push_back(AZStd::move(subMeshBlasInstance));

            AZStd::unique_lock<AZStd::mutex> lock(m_mutex);

            m_proceduralGeometryLookup.emplace(uuid, m_proceduralGeometry.size());
            m_proceduralGeometry.push_back(proceduralGeometry);

            m_blasInstanceMap.emplace(Data::AssetId(uuid), meshBlasInstance);

            RHI::MultiDeviceObject::IterateDevices(
                m_deviceMask,
                [&](int deviceIndex)
                {
                    m_blasToBuild[deviceIndex].insert(Data::AssetId(uuid));
                    return true;
                });

            geometryTypeHandle->m_instanceCount++;

            m_revision++;
            m_proceduralGeometryInfoBufferNeedsUpdate = true;
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

        void RayTracingFeatureProcessor::RemoveProceduralGeometry(const Uuid& uuid)
        {
            if (!m_rayTracingEnabled)
            {
                return;
            }

            AZStd::unique_lock<AZStd::mutex> lock(m_mutex);

            size_t materialInfoIndex = m_proceduralGeometryLookup[uuid];
            m_proceduralGeometry[materialInfoIndex].m_typeHandle->m_instanceCount--;

            m_meshFeatureProcessor->ReleaseMeshInfoEntry(m_proceduralGeometry[materialInfoIndex].m_meshInfoHandle);

            if (materialInfoIndex < m_proceduralGeometry.size() - 1)
            {
                m_proceduralGeometryLookup[m_proceduralGeometry.back().m_uuid] = m_proceduralGeometryLookup[uuid];
                m_proceduralGeometry[materialInfoIndex] = m_proceduralGeometry.back();
            }

            m_proceduralGeometry.pop_back();

            m_proceduralGeometryLookup.erase(uuid);
            RemoveBlasInstance(uuid);

            m_revision++;
            m_proceduralGeometryInfoBufferNeedsUpdate = true;
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

                auto rpiDesc = RPI::RPISystemInterface::Get()->GetDescriptor();
                if (mesh.m_subMeshIndices.size() > rpiDesc.m_rayTracingSystemDescriptor.m_rayTracingCompactionQueryPoolSize)
                {
                    AZ_Warning(
                        "RaytracingFeatureProcessor",
                        false,
                        "CompactionQueryPool is not large enough for model %s.\n"
                        "Pool size: %d\n"
                        "Num meshes in model: %d\n"
                        "Raytracing Acceleration Structure Compaction will be disabled for this model\n"
                        "Consider increasing the size of the pool through the registry setting "
                        "O3DE/Atom/RPI/Initialization/RayTracingSystemDescriptor/RayTracingCompactionQueryPoolSize",
                        mesh.m_assetId.ToFixedString().c_str(),
                        rpiDesc.m_rayTracingSystemDescriptor.m_rayTracingCompactionQueryPoolSize,
                        mesh.m_subMeshIndices.size());
                    buildFlags = buildFlags & ~RHI::RayTracingAccelerationStructureBuildFlags::ENABLE_COMPACTION;
                }

                for (uint32_t subMeshIndex = 0; subMeshIndex < mesh.m_subMeshIndices.size(); ++subMeshIndex)
                {
                    const SubMesh& subMesh = m_subMeshes[mesh.m_subMeshIndices[subMeshIndex]];
                    auto& meshInfoEntry = m_meshFeatureProcessor->GetMeshInfoEntry(subMesh.m_meshInfoHandle);
                    auto positionIt = meshInfoEntry->m_meshBuffers.find(RHI::ShaderSemantic{ AZ::Name{ "POSITION" } });
                    if (positionIt == meshInfoEntry->m_meshBuffers.end())
                    {
                        // mesh has no position buffer?
                        continue;
                    }
                    auto& position = positionIt->second;

                    auto indexBuffer = meshInfoEntry->m_indexBuffer;

                    SubMeshBlasInstance subMeshBlasInstance;

                    RHI::RayTracingBlasDescriptor& blasDescriptor = subMeshBlasInstance.m_blasDescriptor;
                    blasDescriptor.m_buildFlags = buildFlags;

                    RHI::RayTracingGeometry& blasGeometry = blasDescriptor.m_geometries.emplace_back();
                    blasGeometry.m_vertexFormat = position.m_vertexFormat;
                    blasGeometry.m_vertexBuffer = position.m_streamBufferView;
                    blasGeometry.m_indexBuffer = indexBuffer.m_indexBufferView;

                    itMeshBlasInstance->second.m_subMeshes.push_back(subMeshBlasInstance);
                }
                m_blasToCreate.insert(mesh.m_assetId);
            }
            else
            {
                itMeshBlasInstance->second.m_count++;
            }
            AZ_Error(
                "RaytracingFeatureProcessor",
                itMeshBlasInstance->second.m_subMeshes.size() == mesh.m_subMeshIndices.size(),
                "AddMesh: The number of submeshes given does match the number of submeshes in the mesh (%d vs %d)",
                itMeshBlasInstance->second.m_subMeshes.size(),
                mesh.m_subMeshIndices.size());

            for (uint32_t subMeshIndex = 0; subMeshIndex < mesh.m_subMeshIndices.size(); ++subMeshIndex)
            {
                m_subMeshes[mesh.m_subMeshIndices[subMeshIndex]].m_blasInstanceId = { mesh.m_assetId, subMeshIndex };
            }

            m_revision++;
            m_subMeshCount += aznumeric_cast<uint32_t>(subMeshes.size());
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

                    if (globalIndex < m_subMeshes.size() - 1)
                    {
                        // the subMesh we're removing is in the middle of the global lists, remove by swapping the last element to its
                        // position in the list
                        m_subMeshes[globalIndex] = m_subMeshes.back();
                        // update the global index for the swapped subMesh
                        m_subMeshes[globalIndex].m_globalIndex = globalIndex;

                        // update the global index in the parent Mesh' subMesh list
                        Mesh* swappedSubMeshParent = m_subMeshes[globalIndex].m_mesh;
                        uint32_t swappedSubMeshIndex = m_subMeshes[globalIndex].m_subMeshIndex;
                        swappedSubMeshParent->m_subMeshIndices[swappedSubMeshIndex] = globalIndex;
                    }

                    m_subMeshes.pop_back();

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
                    m_blasInstanceMap.clear();

                }
            }
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
            }
        }

        void RayTracingFeatureProcessor::Render(const RenderPacket&)
        {
            m_frameIndex++;
        }

        void RayTracingFeatureProcessor::BeginFrame(int deviceIndex)
        {
            if (deviceIndex == RHI::MultiDevice::InvalidDeviceIndex)
            {
                deviceIndex = RHI::MultiDevice::DefaultDeviceIndex;
            }
            bool updatedDeviceMask = false;
            if (!RHI::CheckBit(m_deviceMask, deviceIndex))
            {
                for (auto& [assetId, blasInstance] : m_blasInstanceMap)
                {
                    m_blasToCreate.insert(assetId);
                }
                m_deviceMask = RHI::SetBit(m_deviceMask, deviceIndex);
                updatedDeviceMask = true;
                m_revision++;

                // Make sure the map entries are present so we don't have a race condition in MarkBlasInstance*
                m_uncompactedBlasEnqueuedForDeletion.insert(deviceIndex);
                m_blasEnqueuedForCompact.insert(deviceIndex);
            }

            if (m_updatedFrameIndex == m_frameIndex)
            {
                if (!updatedDeviceMask)
                {
                    // Make sure the update is only called once per frame
                    // When multiple devices are present a RayTracingAccelerationStructurePass is created per device
                    // Thus this function is called once for each device
                    return;
                }
            }
            else
            {
                m_compactionQueryPool->BeginFrame(m_frameIndex);
            }
            m_updatedFrameIndex = m_frameIndex;

            UpdateBlasInstances();

            if (m_tlasRevision != m_revision)
            {
                m_tlasRevision = m_revision;

                // create the TLAS descriptor
                AZStd::unordered_map<int, RHI::DeviceRayTracingTlasDescriptor> tlasDescriptor;
                RHI::MultiDeviceObject::IterateDevices(
                    m_deviceMask,
                    [&](int deviceIndex)
                    {
                        // Create all device descriptors. This is needed if no Blas instances are present
                        tlasDescriptor[deviceIndex];
                        return true;
                    });

                int32_t maxInstanceId = 0;

                for (auto& subMesh : m_subMeshes)
                {
                    RHI::MultiDeviceObject::IterateDevices(
                        m_deviceMask,
                        [&](int deviceIndex)
                        {
                            auto meshIt = m_blasInstanceMap.find(subMesh.m_blasInstanceId.first);
                            if (meshIt == m_blasInstanceMap.end())
                            {
                                return false;
                            }
                            if (subMesh.m_blasInstanceId.second >= meshIt->second.m_subMeshes.size())
                            {
                                return false;
                            }
                            const auto& blasInstance = meshIt->second.m_subMeshes[subMesh.m_blasInstanceId.second];
                            RHI::RayTracingBlas* blas = blasInstance.m_compactBlas.get();
                            if (blas == nullptr || !RHI::CheckBit(blas->GetDeviceMask(), deviceIndex))
                            {
                                blas = blasInstance.m_blas.get();
                                if (blas && !RHI::CheckBit(blas->GetDeviceMask(), deviceIndex))
                                {
                                    // This might happen if the number of BLAS created per frame is limited
                                    blas = nullptr;
                                }
                            }
                            if (blas)
                            {
                                RHI::DeviceRayTracingTlasInstance& tlasInstance = tlasDescriptor[deviceIndex].m_instances.emplace_back();
                                tlasInstance.m_instanceID = subMesh.m_meshInfoHandle.GetIndex();
                                tlasInstance.m_instanceMask = subMesh.m_mesh->m_instanceMask;
                                tlasInstance.m_hitGroupIndex = 0;
                                tlasInstance.m_blas = blas->GetDeviceRayTracingBlas(deviceIndex);
                                tlasInstance.m_transform = subMesh.m_mesh->m_transform;
                                tlasInstance.m_nonUniformScale = subMesh.m_mesh->m_nonUniformScale;
                                // TODO: tlasInstance.m_transparent = subMesh.m_material.m_irradianceColor.GetA() < 1.0f;
                                maxInstanceId = AZStd::max(maxInstanceId, subMesh.m_meshInfoHandle.GetIndex());
                            }
                            return true;
                        });
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
                    RHI::MultiDeviceObject::IterateDevices(
                        m_deviceMask,
                        [&](int deviceIndex)
                        {
                            RHI::DeviceRayTracingTlasInstance& tlasInstance = tlasDescriptor[deviceIndex].m_instances.emplace_back();
                            tlasInstance.m_instanceID = maxInstanceId++;
                            tlasInstance.m_instanceMask = proceduralGeometry.m_instanceMask;
                            tlasInstance.m_hitGroupIndex = geometryTypeMap[proceduralGeometry.m_typeHandle->m_name];
                            tlasInstance.m_blas = proceduralGeometry.m_blas->GetDeviceRayTracingBlas(deviceIndex);
                            tlasInstance.m_transform = proceduralGeometry.m_transform;
                            tlasInstance.m_nonUniformScale = proceduralGeometry.m_nonUniformScale;
                            return true;
                        });
                }

                // create the TLAS buffers based on the descriptor
                RHI::Ptr<RHI::RayTracingTlas>& rayTracingTlas = m_tlas;
                rayTracingTlas->CreateBuffers(m_deviceMask, tlasDescriptor, *m_bufferPools);
            }

            // update and compile the RayTracingSceneSrg and RayTracingMaterialSrg
            // Note: the timing of this update is very important, it needs to be updated after the TLAS is allocated so it can
            // be set on the RayTracingSceneSrg for this frame, and the ray tracing mesh data in the RayTracingSceneSrg must
            // exactly match the TLAS.  Any mismatch in this data may result in a TDR.
            UpdateRayTracingSrgs();
        }

        uint32_t RayTracingFeatureProcessor::GetBuiltRevision(int deviceIndex) const
        {
            auto it = m_builtRevisions.find(deviceIndex);
            if (it != m_builtRevisions.end())
            {
                return it->second;
            }
            else
            {
                return 0;
            }
        }

        void RayTracingFeatureProcessor::SetBuiltRevision(int deviceIndex, uint32_t revision)
        {
            m_builtRevisions[deviceIndex] = revision;
        }

        void RayTracingFeatureProcessor::UpdateRayTracingSrgs()
        {
            AZ_PROFILE_SCOPE(AzRender, "RayTracingFeatureProcessor::UpdateRayTracingSrgs");

            if (!m_tlas->GetTlasBuffer())
            {
                return;
            }

            if (m_rayTracingSceneSrg->IsQueuedForCompile())
            {
                //[GFX TODO][ATOM-14792] AtomSampleViewer: Reset scene and feature processors before switching to sample
                return;
            }

            // lock the mutex to protect the mesh and BLAS lists
            AZStd::unique_lock<AZStd::mutex> lock(m_mutex);
            if (HasProceduralGeometry())
            {
                UpdateProceduralGeometryInfoBuffer();
            }
            UpdateRayTracingSceneSrg();
        }

        const void RayTracingFeatureProcessor::MarkBlasInstanceForCompaction(int deviceIndex, Data::AssetId assetId)
        {
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

            m_blasEnqueuedForCompact[deviceIndex][assetId].m_frameIndex =
                static_cast<int>(m_frameIndex + RHI::Limits::Device::FrameCountMax);
        }

        const void RayTracingFeatureProcessor::MarkBlasInstanceAsCompactionEnqueued(int deviceIndex, Data::AssetId assetId)
        {
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

            m_uncompactedBlasEnqueuedForDeletion[deviceIndex][assetId].m_frameIndex =
                static_cast<int>(m_frameIndex + RHI::Limits::Device::FrameCountMax);
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
                                    subMeshInstance.m_blasDescriptor.m_buildFlags,
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

                    RHI::MultiDevice::DeviceMask createdOnDevices{};
                    for (auto& subMeshInstance : instance.m_subMeshes)
                    {
                        // create the BLAS object and store it in the BLAS list
                        if (RHI::CheckBitsAny(
                                subMeshInstance.m_blasDescriptor.m_buildFlags,
                                RHI::RayTracingAccelerationStructureBuildFlags::ENABLE_COMPACTION))
                        {
                            if (subMeshInstance.m_compactionSizeQuery)
                            {
                                RHI::MultiDeviceObject::IterateDevices(
                                    m_deviceMask & ~subMeshInstance.m_compactionSizeQuery->GetDeviceMask(),
                                    [&](int deviceIndex)
                                    {
                                        m_compactionQueryPool->AddDeviceToQuery(deviceIndex, subMeshInstance.m_compactionSizeQuery.get());
                                        return true;
                                    });
                            }
                            else
                            {
                                subMeshInstance.m_compactionSizeQuery = aznew RHI::RayTracingCompactionQuery;
                                m_compactionQueryPool->InitQuery(m_deviceMask, subMeshInstance.m_compactionSizeQuery.get());
                            }
                            numCompactionQueriesEnqueued++;
                        }

                        if (subMeshInstance.m_blas)
                        {
                            createdOnDevices = m_deviceMask & ~subMeshInstance.m_blas->GetDeviceMask();
                            RHI::MultiDeviceObject::IterateDevices(
                                createdOnDevices,
                                [&](int deviceIndex)
                                {
                                    subMeshInstance.m_blas->AddDevice(deviceIndex, *m_bufferPools);
                                    return true;
                                });
                        }
                        else
                        {
                            subMeshInstance.m_blas = aznew RHI::RayTracingBlas;
                            subMeshInstance.m_blas->CreateBuffers(m_deviceMask, &subMeshInstance.m_blasDescriptor, *m_bufferPools);
                            createdOnDevices = m_deviceMask;
                        }
                    }

                    if (instance.m_isSkinnedMesh)
                    {
                        if (createdOnDevices ==
                            m_deviceMask) // If it's not the full device mask, a new device was added, not a new blas instance
                        {
                            ++m_skinnedMeshCount;
                            m_skinnedBlasIds.insert(assetId);
                        }
                    }
                    else if (createdOnDevices != RHI::MultiDevice::NoDevices)
                    {
                        RHI::MultiDeviceObject::IterateDevices(
                            createdOnDevices,
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
                for (auto& toRemove : toRemoveFromCreateList)
                {
                    m_blasToCreate.erase(toRemove);
                }
            }

            // Check which Blas are ready for compaction and create compacted acceleration structures for them
            for (auto& [deviceIndex, blasEnqueuedForCompact] : m_blasEnqueuedForCompact)
            {
                AZStd::unordered_set<Data::AssetId> toDelete;
                for (const auto& [assetId, frameEvent] : blasEnqueuedForCompact)
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
                                AZ_Assert(
                                    !subMeshInstance.m_compactBlas ||
                                        !RHI::CheckBit(subMeshInstance.m_compactBlas->GetDeviceMask(), deviceIndex),
                                    "Trying to compact a Blas twice");
                                auto deviceMask = RHI::SetBit(RHI::MultiDevice::DeviceMask{}, deviceIndex);
                                if (subMeshInstance.m_compactBlas)
                                {
                                    auto size =
                                        subMeshInstance.m_compactionSizeQuery->GetDeviceRayTracingCompactionQuery(deviceIndex)->GetResult();
                                    subMeshInstance.m_compactBlas->AddDeviceCompacted(
                                        deviceIndex, *subMeshInstance.m_blas, size, *m_bufferPools);
                                }
                                else
                                {
                                    AZStd::unordered_map<int, uint64_t> sizes;
                                    sizes[deviceIndex] =
                                        subMeshInstance.m_compactionSizeQuery->GetDeviceRayTracingCompactionQuery(deviceIndex)->GetResult();
                                    subMeshInstance.m_compactBlas = aznew RHI::RayTracingBlas;
                                    subMeshInstance.m_compactBlas->CreateCompactedBuffers(
                                        deviceMask, *subMeshInstance.m_blas, sizes, *m_bufferPools);
                                }
                                if (RHI::ResetBits(subMeshInstance.m_compactionSizeQuery->GetDeviceMask(), deviceMask) ==
                                    RHI::MultiDevice::DeviceMask{})
                                {
                                    subMeshInstance.m_compactionSizeQuery = {};
                                }
                                else
                                {
                                    m_compactionQueryPool->RemoveDeviceFromQuery(deviceIndex, subMeshInstance.m_compactionSizeQuery.get());
                                }
                                changed = true;
                            }
                            m_blasToCompact[deviceIndex].insert(assetId);
                        }
                        toDelete.insert(assetId);
                    }
                }
                for (auto& assetId : toDelete)
                {
                    blasEnqueuedForCompact.erase(assetId);
                }
            }

            // Check which uncompacted Blas can be deleted, and delete them
            for (auto& [deviceIndex, uncompactedBlasEnqueuedForDeletion] : m_uncompactedBlasEnqueuedForDeletion)
            {
                AZStd::unordered_set<Data::AssetId> toDelete;
                for (const auto& [assetId, frameEvent] : uncompactedBlasEnqueuedForDeletion)
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
                                if (subMeshInstance.m_blas->GetDeviceMask() == RHI::SetBit(RHI::MultiDevice::NoDevices, deviceIndex))
                                {
                                    subMeshInstance.m_blas = {};
                                }
                                else
                                {
                                    subMeshInstance.m_blas->RemoveDevice(deviceIndex);
                                }
                                changed = true;
                            }
                        }
                        toDelete.insert(assetId);
                    }
                }
                for (auto& assetId : toDelete)
                {
                    uncompactedBlasEnqueuedForDeletion.erase(assetId);
                }
            }

            if (changed)
            {
                m_revision++;
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

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_blasMeshCount"));
            m_rayTracingSceneSrg->SetConstant(constantIndex, m_subMeshCount);

            if (m_proceduralGeometryInfoGpuBuffer.IsCurrentBufferValid())
            {
                bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_proceduralGeometryInfo"));
                m_rayTracingSceneSrg->SetBufferView(bufferIndex, m_proceduralGeometryInfoGpuBuffer.GetCurrentBufferView());
            }

            m_rayTracingSceneSrg->Compile();
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
            for (auto& [deviceIndex, blasEnqueuedForCompact] : m_blasEnqueuedForCompact)
            {
                blasEnqueuedForCompact.erase(id);
            }
            for (auto& [deviceIndex, uncompactedBlasEnqueuedForDeletion] : m_uncompactedBlasEnqueuedForDeletion)
            {
                uncompactedBlasEnqueuedForDeletion.erase(id);
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

                auto rpiDesc = RPI::RPISystemInterface::Get()->GetDescriptor();
                if (rpiDesc.m_rayTracingSystemDescriptor.m_enableBlasCompaction)
                {
                    buildFlags = buildFlags | RHI::RayTracingAccelerationStructureBuildFlags::ENABLE_COMPACTION;
                }
            }

            return buildFlags;
        }
    } // namespace Render
}
