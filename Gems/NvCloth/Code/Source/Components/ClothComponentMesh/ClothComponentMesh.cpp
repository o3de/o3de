/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Math/PackedVector3.h>
#include <AzCore/Math/PackedVector4.h>

#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/SkinnedMesh/SkinnedMeshOverrideBus.h>
#include <Atom/RHI/RHIUtils.h>

#include <NvCloth/IClothSystem.h>
#include <NvCloth/IFabricCooker.h>
#include <NvCloth/IClothConfigurator.h>
#include <NvCloth/ITangentSpaceHelper.h>

#include <Components/ClothComponentMesh/ActorClothColliders.h>
#include <Components/ClothComponentMesh/ActorClothSkinning.h>
#include <Components/ClothComponentMesh/ClothConstraints.h>
#include <Components/ClothComponentMesh/ClothDebugDisplay.h>
#include <Components/ClothComponentMesh/ClothComponentMesh.h>

#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/WindBus.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>
#include <Atom/RPI.Reflect/Buffer/BufferAssetView.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/Model/ModelLodAsset.h>

namespace NvCloth
{
    AZ_CVAR(float, cloth_DistanceToTeleport, 0.5f, nullptr, AZ::ConsoleFunctorFlags::Null,
        "The amount of meters the entity has to move in a frame to consider it a teleport for cloth.");

    AZ_CVAR(float, cloth_SecondsToDelaySimulationOnActorSpawned, 0.25f, nullptr, AZ::ConsoleFunctorFlags::Null,
        "The amount of time in seconds the cloth simulation will be delayed to avoid sudden impulses when actors are spawned.");

    // Helper class to map an RPI buffer from a buffer asset view.
    template<typename T>
    class MappedBuffer
    {
    public:
        MappedBuffer(
            const AZ::RPI::BufferAssetView* bufferAssetView,
            const size_t expectedElementCount,
            const AZ::RHI::Format expectedElementFormat);
        ~MappedBuffer();

        const AZStd::unordered_map<int, T*>& GetBuffer()
        {
            return m_buffer;
        }

    private:
        AZ::Data::Instance<AZ::RPI::Buffer> m_rpiBuffer;
        AZStd::unordered_map<int, T*> m_buffer;
    };

    template<typename T>
    MappedBuffer<T>::MappedBuffer(
        const AZ::RPI::BufferAssetView* bufferAssetView,
        [[maybe_unused]] const size_t expectedElementCount,
        [[maybe_unused]] const AZ::RHI::Format expectedElementFormat)
    {
        if (!bufferAssetView)
        {
            return;
        }

        const AZ::RHI::BufferViewDescriptor& bufferViewDescriptor = bufferAssetView->GetBufferViewDescriptor();
        AZ_Assert(bufferViewDescriptor.m_elementCount == expectedElementCount,
            "Unexpected buffer size: expected is %d but descriptor's is %d", expectedElementCount, bufferViewDescriptor.m_elementCount);
        AZ_Assert(bufferViewDescriptor.m_elementSize == sizeof(T),
            "Unexpected buffer element size: expected is %d but descriptor's is %d", sizeof(T), bufferViewDescriptor.m_elementSize);
        AZ_Assert(bufferViewDescriptor.m_elementFormat == expectedElementFormat,
            "Unexpected buffer format: expected is %d but descriptor's is %d", expectedElementFormat, bufferViewDescriptor.m_elementFormat);

        const AZ::Data::Asset<AZ::RPI::BufferAsset>& bufferAsset = bufferAssetView->GetBufferAsset();
        m_rpiBuffer = AZ::RPI::Buffer::FindOrCreate(bufferAsset);
        if (m_rpiBuffer == nullptr)
        {
            AZ_Error("ClothComponentMesh", false,
                "Failed to find or create RPI buffer from buffer asset '%s'", bufferAsset.GetHint().c_str());
            return;
        }

        const uint64_t byteCount = aznumeric_cast<uint64_t>(bufferViewDescriptor.m_elementCount) * aznumeric_cast<uint64_t>(bufferViewDescriptor.m_elementSize);
        const uint64_t byteOffset = aznumeric_cast<uint64_t>(bufferViewDescriptor.m_elementOffset) * aznumeric_cast<uint64_t>(bufferViewDescriptor.m_elementSize);

        auto data = m_rpiBuffer->Map(byteCount, byteOffset);
        for(auto [deviceIndex, buffer] : data)
        {
            m_buffer[deviceIndex] = static_cast<T*>(buffer);
        }
    }

    template<typename T>
    MappedBuffer<T>::~MappedBuffer()
    {
        if (!m_buffer.empty())
        {
            m_rpiBuffer->Unmap();
        }
    }

    ClothComponentMesh::ClothComponentMesh(AZ::EntityId entityId, const ClothConfiguration& config)
        : m_preSimulationEventHandler(
            [this](ClothId clothId, float deltaTime)
            {
                this->OnPreSimulation(clothId, deltaTime);
            })
        , m_postSimulationEventHandler(
            [this](ClothId clothId, float deltaTime, const AZStd::vector<SimParticleFormat>& updatedParticles)
            {
                this->OnPostSimulation(clothId, deltaTime, updatedParticles);
            })
    {
        Setup(entityId, config);
    }

    ClothComponentMesh::~ClothComponentMesh()
    {
        TearDown();
    }

    void ClothComponentMesh::UpdateConfiguration(AZ::EntityId entityId, const ClothConfiguration& config)
    {
        if (m_entityId != entityId ||
            m_config.m_meshNode != config.m_meshNode ||
            m_config.m_removeStaticTriangles != config.m_removeStaticTriangles)
        {
            Setup(entityId, config);
        }
        else if (m_cloth)
        {
            m_config = config;
            ApplyConfigurationToCloth();

            // Update the cloth constraints parameters
            m_clothConstraints->SetMotionConstraintMaxDistance(m_config.m_motionConstraintsMaxDistance);
            m_clothConstraints->SetBackstopMaxRadius(m_config.m_backstopRadius);
            m_clothConstraints->SetBackstopMaxOffsets(m_config.m_backstopBackOffset, m_config.m_backstopFrontOffset);
            UpdateSimulationConstraints();

            // Subscribe to WindNotificationsBus only if custom wind velocity flag is not set
            if (m_config.IsUsingWindBus())
            {
                Physics::WindNotificationsBus::Handler::BusConnect();
            }
            else
            {
                Physics::WindNotificationsBus::Handler::BusDisconnect();
            }
        }
    }

    void ClothComponentMesh::Setup(AZ::EntityId entityId, const ClothConfiguration& config)
    {
        TearDown();

        m_entityId = entityId;
        m_config = config;

        if (!CreateCloth())
        {
            TearDown();
            return;
        }

        // Initialize render data
        m_renderDataBufferIndex = 0;
        {
            auto& renderData = GetRenderData();
            renderData.m_particles = m_meshClothInfo.m_particles;
            renderData.m_tangents = m_meshClothInfo.m_tangents;
            renderData.m_bitangents = m_meshClothInfo.m_bitangents;
            renderData.m_normals = m_meshClothInfo.m_normals;
        }
        UpdateRenderData(m_cloth->GetParticles());
        // Copy the first initialized element to the rest of the buffer
        for (AZ::u32 i = 1; i < RenderDataBufferSize; ++i)
        {
            m_renderDataBuffer[i] = m_renderDataBuffer[0];
        }

        // It will return a valid instance if it's an actor with cloth colliders in it.
        m_actorClothColliders = ActorClothColliders::Create(m_entityId);

        // It will return a valid instance if it's an actor with skinning data.
        m_actorClothSkinning = ActorClothSkinning::Create(
            m_entityId,
            m_meshNodeInfo,
            m_meshClothInfo.m_particles,
            m_cloth->GetParticles().size(),
            m_meshRemappedVertices);
        m_timeClothSkinningUpdates = 0.0f;

        // Turn off GPU skinning for any sub-meshes simulated by the cloth component
        DisableSkinning();

        m_clothConstraints = ClothConstraints::Create(
            m_meshClothInfo.m_motionConstraints,
            m_config.m_motionConstraintsMaxDistance,
            m_meshClothInfo.m_backstopData,
            m_config.m_backstopRadius,
            m_config.m_backstopBackOffset,
            m_config.m_backstopFrontOffset,
            m_cloth->GetParticles(),
            m_cloth->GetInitialIndices(),
            m_meshRemappedVertices);
        AZ_Assert(m_clothConstraints, "Failed to create cloth constraints");
        UpdateSimulationConstraints();

#ifndef RELEASE
        m_clothDebugDisplay = AZStd::make_unique<ClothDebugDisplay>(this);
#endif

        AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);
        AZ::TickBus::Handler::BusConnect();
        m_cloth->ConnectPreSimulationEventHandler(m_preSimulationEventHandler);
        m_cloth->ConnectPostSimulationEventHandler(m_postSimulationEventHandler);

        if (m_config.IsUsingWindBus())
        {
            Physics::WindNotificationsBus::Handler::BusConnect();
        }
    }

    void ClothComponentMesh::TearDown()
    {
        if (m_cloth)
        {
            Physics::WindNotificationsBus::Handler::BusDisconnect();
            AZ::TickBus::Handler::BusDisconnect();
            AZ::TransformNotificationBus::Handler::BusDisconnect();
            m_preSimulationEventHandler.Disconnect();
            m_postSimulationEventHandler.Disconnect();

            AZ::Interface<IClothSystem>::Get()->RemoveCloth(m_cloth);
            AZ::Interface<IClothSystem>::Get()->DestroyCloth(m_cloth);

            // Re-enable skinning for any sub-meshes that were previously skinned by the cloth component
            EnableSkinning();
        }
        m_entityId.SetInvalid();
        m_renderDataBuffer = {};
        m_meshRemappedVertices.clear();
        m_meshNodeInfo = {};
        m_meshClothInfo = {};
        m_actorClothColliders.reset();
        m_actorClothSkinning.reset();
        m_clothConstraints.reset();
        m_motionConstraints.clear();
        m_separationConstraints.clear();
        m_clothDebugDisplay.reset();
    }

    void ClothComponentMesh::OnPreSimulation(
        [[maybe_unused]] ClothId clothId,
        float deltaTime)
    {
        AZ_PROFILE_FUNCTION(Cloth);

        UpdateSimulationCollisions();

        if (m_actorClothSkinning)
        {
            UpdateSimulationSkinning(deltaTime);

            UpdateSimulationConstraints();
        }
    }

    void ClothComponentMesh::OnPostSimulation(
        [[maybe_unused]] ClothId clothId,
        [[maybe_unused]] float deltaTime,
        const AZStd::vector<SimParticleFormat>& updatedParticles)
    {
        AZ_PROFILE_FUNCTION(Cloth);

        // Next buffer index of the render data
        m_renderDataBufferIndex = (m_renderDataBufferIndex + 1) % RenderDataBufferSize;

        UpdateRenderData(updatedParticles);
    }

    void ClothComponentMesh::OnTransformChanged([[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
    {
        // At the moment there is no way to distinguish "move" from "teleport".
        // As a workaround we will consider a teleport if the position has changed considerably.
        bool teleport = (m_worldPosition.GetDistance(world.GetTranslation()) >= cloth_DistanceToTeleport);

        if (teleport)
        {
            TeleportCloth(world);
        }
        else
        {
            MoveCloth(world);
        }
    }

    void ClothComponentMesh::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        CopyRenderDataToModel();
    }

    int ClothComponentMesh::GetTickOrder()
    {
        return AZ::TICK_PRE_RENDER;
    }

    void ClothComponentMesh::OnGlobalWindChanged()
    {
        m_cloth->GetClothConfigurator()->SetWindVelocity(GetWindBusVelocity());
    }

    void ClothComponentMesh::OnWindChanged([[maybe_unused]] const AZ::Aabb& aabb)
    {
        OnGlobalWindChanged();
    }

    ClothComponentMesh::RenderData& ClothComponentMesh::GetRenderData()
    {
        return const_cast<RenderData&>(
            static_cast<const ClothComponentMesh&>(*this).GetRenderData());
    }

    const ClothComponentMesh::RenderData& ClothComponentMesh::GetRenderData() const
    {
        return m_renderDataBuffer[m_renderDataBufferIndex];
    }

    void ClothComponentMesh::UpdateSimulationCollisions()
    {
        if (m_actorClothColliders)
        {
            AZ_PROFILE_FUNCTION(Cloth);

            m_actorClothColliders->Update();

            const auto& spheres = m_actorClothColliders->GetSpheres();
            m_cloth->GetClothConfigurator()->SetSphereColliders(spheres);

            const auto& capsuleIndices = m_actorClothColliders->GetCapsuleIndices();
            m_cloth->GetClothConfigurator()->SetCapsuleColliders(capsuleIndices);
        }
    }

    void ClothComponentMesh::UpdateSimulationSkinning(float deltaTime)
    {
        if (m_actorClothSkinning)
        {
            AZ_PROFILE_FUNCTION(Cloth);

            m_actorClothSkinning->UpdateSkinning();

            // Since component activation order is not trivial, the actor's pose might not be updated
            // immediately. Because of this cloth will receive a sudden impulse when changing from
            // T pose to animated pose. To avoid this undesired effect we will override cloth simulation during
            // a short amount of time.
            m_timeClothSkinningUpdates += deltaTime;

            // While the actor is not visible the skinned joints are not updated. Then when
            // it becomes visible the jump to the new skinned positions causes a sudden
            // impulse to cloth simulation. To avoid this undesired effect we will override cloth simulation during
            // a short amount of time.
            m_actorClothSkinning->UpdateActorVisibility();
            if (!m_actorClothSkinning->WasActorVisible() &&
                m_actorClothSkinning->IsActorVisible())
            {
                m_timeClothSkinningUpdates = 0.0f;
            }

            if (m_timeClothSkinningUpdates <= cloth_SecondsToDelaySimulationOnActorSpawned)
            {
                // Update skinning for all particles and apply it to cloth 
                AZStd::vector<SimParticleFormat> particles = m_cloth->GetParticles();
                m_actorClothSkinning->ApplySkinning(m_cloth->GetInitialParticles(), particles);
                m_cloth->SetParticles(AZStd::move(particles));
                m_cloth->DiscardParticleDelta();
            }
        }
    }

    void ClothComponentMesh::UpdateSimulationConstraints()
    {
        AZ_PROFILE_FUNCTION(Cloth);

        m_motionConstraints = m_clothConstraints->GetMotionConstraints();
        m_separationConstraints = m_clothConstraints->GetSeparationConstraints();

        if (m_actorClothSkinning)
        {
            m_actorClothSkinning->ApplySkinning(m_clothConstraints->GetMotionConstraints(), m_motionConstraints);
            m_actorClothSkinning->ApplySkinning(m_clothConstraints->GetSeparationConstraints(), m_separationConstraints);
        }

        m_cloth->GetClothConfigurator()->SetMotionConstraints(m_motionConstraints);
        if (!m_separationConstraints.empty())
        {
            m_cloth->GetClothConfigurator()->SetSeparationConstraints(m_separationConstraints);
        }
    }

    void ClothComponentMesh::UpdateRenderData(const AZStd::vector<SimParticleFormat>& particles)
    {
        AZ_PROFILE_FUNCTION(Cloth);

        if (!m_cloth)
        {
            return;
        }

        auto& renderData = GetRenderData();

        if (m_actorClothSkinning)
        {
            // Apply skinning to the non-simulated part of the mesh.
            m_actorClothSkinning->ApplySkinningOnNonSimulatedVertices(m_meshClothInfo, renderData);
        }

        // Calculate normals of the cloth particles (simplified mesh).
        AZStd::vector<AZ::Vector3> normals;
        [[maybe_unused]] bool normalsCalculated =
            AZ::Interface<ITangentSpaceHelper>::Get()->CalculateNormals(particles, m_cloth->GetInitialIndices(), normals);
        AZ_Assert(normalsCalculated, "Cloth component mesh failed to calculate normals.");

        // Copy particles and normals to render data.
        // Since cloth's vertices were welded together,
        // the full mesh will result in smooth normals.
        for (size_t index = 0; index < m_meshRemappedVertices.size(); ++index)
        {
            const int remappedIndex = m_meshRemappedVertices[index];
            if (remappedIndex >= 0)
            {
                renderData.m_particles[index] = particles[remappedIndex];

                // For static particles only use the updated normal when indicated in the configuration.
                const bool useSimulatedClothParticleNormal =
                    m_meshClothInfo.m_particles[index].GetW() != 0.0f ||
                    m_config.m_updateNormalsOfStaticParticles;
                if (useSimulatedClothParticleNormal)
                {
                    renderData.m_normals[index] = normals[remappedIndex];
                }
            }
        }

        // Calculate tangents and bitangents for the full mesh.
        [[maybe_unused]] bool tangentsAndBitangentsCalculated =
            AZ::Interface<ITangentSpaceHelper>::Get()->CalculateTangentsAndBitagents(
                renderData.m_particles, m_meshClothInfo.m_indices,
                m_meshClothInfo.m_uvs, renderData.m_normals,
                renderData.m_tangents, renderData.m_bitangents);
        AZ_Assert(tangentsAndBitangentsCalculated, "Cloth component mesh failed to calculate tangents and bitangents.");
    }

    void ClothComponentMesh::CopyRenderDataToModel()
    {
        AZ_PROFILE_FUNCTION(Cloth);

        // Previous buffer index of the render data
        const AZ::u32 previousBufferIndex = (m_renderDataBufferIndex + RenderDataBufferSize - 1) % RenderDataBufferSize;

        // Workaround to sync debug drawing with cloth rendering as
        // the Entity Debug Display Bus renders on the next frame.
        const bool isDebugDrawEnabled = m_clothDebugDisplay && m_clothDebugDisplay->IsDebugDrawEnabled();
        const RenderData& renderData = (isDebugDrawEnabled)
            ? m_renderDataBuffer[previousBufferIndex]
            : m_renderDataBuffer[m_renderDataBufferIndex];

        const auto& renderParticles = renderData.m_particles;
        const auto& renderNormals = renderData.m_normals;
        const auto& renderTangents = renderData.m_tangents;
        const auto& renderBitangents = renderData.m_bitangents;

        // Since Atom has a 1:1 relation with between ModelAsset buffers and Model buffers,
        // internally it created a new asset for the model instance. So it's important to
        // get the asset from the model when we want to write to them, instead of getting the
        // ModelAsset directly from the bus (which returns the original asset shared by all entities).
        AZ::Data::Instance<AZ::RPI::Model> model;
        AZ::Render::MeshComponentRequestBus::EventResult(model, m_entityId, &AZ::Render::MeshComponentRequestBus::Events::GetModel);
        if (!model)
        {
            return;
        }

        AZ::Data::Asset<AZ::RPI::ModelAsset> modelAsset = model->GetModelAsset();
        if (!modelAsset.IsReady())
        {
            return;
        }

        if (modelAsset->GetLodCount() < m_meshNodeInfo.m_lodLevel)
        {
            AZ_Error("ClothComponentMesh", false,
                "Unable to access lod %d from model asset '%s' as it only has %d lod levels.",
                m_meshNodeInfo.m_lodLevel,
                modelAsset.GetHint().c_str(),
                modelAsset->GetLodCount());
            return;
        }

        const auto modelLodAssets = modelAsset->GetLodAssets();
        const AZ::Data::Asset<AZ::RPI::ModelLodAsset> modelLodAsset = modelLodAssets[m_meshNodeInfo.m_lodLevel];
        if (!modelLodAsset.GetId().IsValid())
        {
            AZ_Error("ClothComponentMesh", false,
                "Model asset '%s' returns an invalid lod asset '%s' (lod level %d).",
                modelAsset.GetHint().c_str(),
                modelLodAsset.GetHint().c_str(),
                m_meshNodeInfo.m_lodLevel);
            return;
        }

        const AZ::Name positionSemantic("POSITION");
        const AZ::Name normalSemantic("NORMAL");
        const AZ::Name tangentSemantic("TANGENT");
        const AZ::Name bitangentSemantic("BITANGENT");

        // For each submesh...
        for (const auto& subMeshInfo : m_meshNodeInfo.m_subMeshes)
        {
            if (modelLodAsset->GetMeshes().size() < subMeshInfo.m_primitiveIndex)
            {
                AZ_Error("ClothComponentMesh", false,
                    "Unable to access submesh %d from lod asset '%s' as it only has %d submeshes.",
                    subMeshInfo.m_primitiveIndex,
                    modelAsset.GetHint().c_str(),
                    modelLodAsset->GetMeshes().size());
                continue;
            }
            
            const auto subMeshes = modelLodAsset->GetMeshes();
            const AZ::RPI::ModelLodAsset::Mesh& subMesh = subMeshes[subMeshInfo.m_primitiveIndex];

            const int numVertices = subMeshInfo.m_numVertices;
            const int firstVertex = subMeshInfo.m_verticesFirstIndex;
            if (subMesh.GetVertexCount() != static_cast<uint32_t>(numVertices))
            {
                AZ_Error("ClothComponentMesh", false,
                    "Render mesh to be modified doesn't have the same number of vertices (%d) as the cloth's submesh (%d).",
                    subMesh.GetVertexCount(),
                    numVertices);
                continue;
            }
            AZ_Assert(firstVertex >= 0, "Invalid first vertex index %d", firstVertex);
            AZ_Assert((firstVertex + numVertices) <= static_cast<int>(renderParticles.size()),
                "Submesh number of vertices (%d) reaches outside the particles (%zu)", (firstVertex + numVertices), renderParticles.size());

            MappedBuffer<AZ::PackedVector3f> destVertices(subMesh.GetSemanticBufferAssetView(positionSemantic), numVertices, AZ::RHI::Format::R32G32B32_FLOAT);
            MappedBuffer<AZ::PackedVector3f> destNormals(subMesh.GetSemanticBufferAssetView(normalSemantic), numVertices, AZ::RHI::Format::R32G32B32_FLOAT);
            MappedBuffer<AZ::PackedVector4f> destTangents(subMesh.GetSemanticBufferAssetView(tangentSemantic), numVertices, AZ::RHI::Format::R32G32B32A32_FLOAT);
            MappedBuffer<AZ::PackedVector3f> destBitangents(subMesh.GetSemanticBufferAssetView(bitangentSemantic), numVertices, AZ::RHI::Format::R32G32B32_FLOAT);

            const auto& destVerticesData = destVertices.GetBuffer();
            const auto& destNormalsData = destNormals.GetBuffer();
            const auto& destTangentsData = destTangents.GetBuffer();
            const auto& destBitangentsData = destBitangents.GetBuffer();

            if (!destVerticesData.empty())
            {
                AZ_Error("ClothComponentMesh", AZ::RHI::IsNullRHI(),
                    "Invalid vertex position buffer obtained from the render mesh to be modified.");
                continue;
            }

            for (size_t index = 0; index < numVertices; ++index)
            {
                const int renderVertexIndex = static_cast<int>(firstVertex + index);

                const SimParticleFormat& renderParticle = renderParticles[renderVertexIndex];

                for(auto& [deviceIndex, destVerticesBuffer] : destVerticesData)
                {
                    destVerticesBuffer[index].Set(
                        renderParticle.GetX(),
                        renderParticle.GetY(),
                        renderParticle.GetZ());
                }

                if (!destNormalsData.empty())
                {
                    const AZ::Vector3& renderNormal = renderNormals[renderVertexIndex];
                    for(auto& [deviceIndex, destNormalsBuffer] : destNormalsData)
                    {
                        destNormalsBuffer[index].Set(
                            renderNormal.GetX(),
                            renderNormal.GetY(),
                            renderNormal.GetZ());
                    }
                }

                if (!destTangentsData.empty())
                {
                    const AZ::Vector3& renderTangent = renderTangents[renderVertexIndex];
                    for(auto& [deviceIndex, destTangentsBuffer] : destTangentsData)
                    {
                        destTangentsBuffer[index].Set(
                            renderTangent.GetX(),
                            renderTangent.GetY(),
                            renderTangent.GetZ(),
                            -1.0f); // Shader function ConstructTBN inverts w to change bitangent sign, but the bitangents passed are already corrected, so passing -1.0 to counteract.
                    }
                }

                if (!destBitangentsData.empty())
                {
                    const AZ::Vector3& renderBitangent = renderBitangents[renderVertexIndex];
                    for(auto& [deviceIndex, destBitangentsBuffer] : destBitangentsData)
                    {
                        destBitangentsBuffer[index].Set(
                            renderBitangent.GetX(),
                            renderBitangent.GetY(),
                            renderBitangent.GetZ());
                    }
                }
            }
        }
    }

    bool ClothComponentMesh::CreateCloth()
    {
        AZStd::unique_ptr<AssetHelper> assetHelper = AssetHelper::CreateAssetHelper(m_entityId);
        if (!assetHelper)
        {
            return false;
        }

        // Obtain cloth mesh info
        bool clothInfoObtained = assetHelper->ObtainClothMeshNodeInfo(m_config.m_meshNode,
            m_meshNodeInfo, m_meshClothInfo);
        if (!clothInfoObtained)
        {
            return false;
        }

        // Generate a simplified mesh for simulation
        AZStd::vector<SimParticleFormat> meshSimplifiedParticles;
        AZStd::vector<SimIndexType> meshSimplifiedIndices;
        AZ::Interface<IFabricCooker>::Get()->SimplifyMesh(
            m_meshClothInfo.m_particles, m_meshClothInfo.m_indices,
            meshSimplifiedParticles, meshSimplifiedIndices,
            m_meshRemappedVertices,
            m_config.m_removeStaticTriangles);
        if (meshSimplifiedParticles.empty() ||
            meshSimplifiedIndices.empty())
        {
            return false;
        }

        // Cook Fabric
        AZStd::optional<FabricCookedData> cookedData =
            AZ::Interface<IFabricCooker>::Get()->CookFabric(meshSimplifiedParticles, meshSimplifiedIndices);
        if (!cookedData)
        {
            return false;
        }

        // Create cloth instance
        m_cloth = AZ::Interface<IClothSystem>::Get()->CreateCloth(meshSimplifiedParticles, *cookedData);
        if (!m_cloth)
        {
            return false;
        }

        // Set initial Position and Rotation
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, m_entityId, &AZ::TransformInterface::GetWorldTM);
        TeleportCloth(transform);

        ApplyConfigurationToCloth();

        // Add cloth to default solver to be simulated
        AZ::Interface<IClothSystem>::Get()->AddCloth(m_cloth);

        return true;
    }

    void ClothComponentMesh::ApplyConfigurationToCloth()
    {
        IClothConfigurator* clothConfig = m_cloth->GetClothConfigurator();

        // Mass
        clothConfig->SetMass(m_config.m_mass);

        // Gravity and scale
        if (m_config.IsUsingWorldBusGravity())
        {
            AZ::Vector3 gravity = AzPhysics::DefaultGravity;
            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
            {
                AzPhysics::SceneHandle defaultScene = sceneInterface->GetSceneHandle(AzPhysics::DefaultPhysicsSceneName);
                if (defaultScene != AzPhysics::InvalidSceneHandle)
                {
                    gravity = sceneInterface->GetGravity(defaultScene);
                }
            }
            clothConfig->SetGravity(gravity * m_config.m_gravityScale);
        }
        else
        {
            clothConfig->SetGravity(m_config.m_customGravity * m_config.m_gravityScale);
        }

        // Stiffness Frequency
        clothConfig->SetStiffnessFrequency(m_config.m_stiffnessFrequency);

        // Motion constraints parameters
        clothConfig->SetMotionConstraintsScale(m_config.m_motionConstraintsScale);
        clothConfig->SetMotionConstraintsBias(m_config.m_motionConstraintsBias);
        clothConfig->SetMotionConstraintsStiffness(m_config.m_motionConstraintsStiffness);

        // Damping parameters
        clothConfig->SetDamping(m_config.m_damping);
        clothConfig->SetDampingLinearDrag(m_config.m_linearDrag);
        clothConfig->SetDampingAngularDrag(m_config.m_angularDrag);

        // Inertia parameters
        clothConfig->SetLinearInertia(m_config.m_linearInteria);
        clothConfig->SetAngularInertia(m_config.m_angularInteria);
        clothConfig->SetCentrifugalInertia(m_config.m_centrifugalInertia);

        // Wind parameters
        if (m_config.IsUsingWindBus())
        {
            clothConfig->SetWindVelocity(GetWindBusVelocity());
        }
        else
        {
            clothConfig->SetWindVelocity(m_config.m_windVelocity);
        }
        clothConfig->SetWindDragCoefficient(m_config.m_airDragCoefficient);
        clothConfig->SetWindLiftCoefficient(m_config.m_airLiftCoefficient);
        clothConfig->SetWindFluidDensity(m_config.m_fluidDensity);

        // Collision parameters
        clothConfig->SetCollisionFriction(m_config.m_collisionFriction);
        clothConfig->SetCollisionMassScale(m_config.m_collisionMassScale);
        clothConfig->EnableContinuousCollision(m_config.m_continuousCollisionDetection);
        clothConfig->SetCollisionAffectsStaticParticles(m_config.m_collisionAffectsStaticParticles);

        // Self Collision parameters
        clothConfig->SetSelfCollisionDistance(m_config.m_selfCollisionDistance);
        clothConfig->SetSelfCollisionStiffness(m_config.m_selfCollisionStiffness);

        // Tether Constraints parameters
        clothConfig->SetTetherConstraintStiffness(m_config.m_tetherConstraintStiffness);
        clothConfig->SetTetherConstraintScale(m_config.m_tetherConstraintScale);

        // Quality parameters
        clothConfig->SetSolverFrequency(m_config.m_solverFrequency);
        clothConfig->SetAcceleationFilterWidth(m_config.m_accelerationFilterIterations);

        // Fabric Phases
        clothConfig->SetVerticalPhaseConfig(
            m_config.m_verticalStiffness,
            m_config.m_verticalStiffnessMultiplier,
            m_config.m_verticalCompressionLimit,
            m_config.m_verticalStretchLimit);
        clothConfig->SetHorizontalPhaseConfig(
            m_config.m_horizontalStiffness,
            m_config.m_horizontalStiffnessMultiplier,
            m_config.m_horizontalCompressionLimit,
            m_config.m_horizontalStretchLimit);
        clothConfig->SetBendingPhaseConfig(
            m_config.m_bendingStiffness,
            m_config.m_bendingStiffnessMultiplier,
            m_config.m_bendingCompressionLimit,
            m_config.m_bendingStretchLimit);
        clothConfig->SetShearingPhaseConfig(
            m_config.m_shearingStiffness,
            m_config.m_shearingStiffnessMultiplier,
            m_config.m_shearingCompressionLimit,
            m_config.m_shearingStretchLimit);
    }

    void ClothComponentMesh::MoveCloth(const AZ::Transform& worldTransform)
    {
        m_worldPosition = worldTransform.GetTranslation();

        m_cloth->GetClothConfigurator()->SetTransform(worldTransform);

        if (m_config.IsUsingWindBus())
        {
            // Wind velocity is affected by world position
            m_cloth->GetClothConfigurator()->SetWindVelocity(GetWindBusVelocity());
        }
    }

    void ClothComponentMesh::TeleportCloth(const AZ::Transform& worldTransform)
    {
        MoveCloth(worldTransform);

        // By clearing inertia the cloth won't be affected by the sudden translation caused when teleporting the entity.
        m_cloth->GetClothConfigurator()->ClearInertia();
    }

    AZ::Vector3 ClothComponentMesh::GetWindBusVelocity()
    {
        const Physics::WindRequests* windRequests = AZ::Interface<Physics::WindRequests>::Get();
        if (windRequests)
        {
            const AZ::Vector3 globalWind = windRequests->GetGlobalWind();
            const AZ::Vector3 localWind = windRequests->GetWind(m_worldPosition);
            return globalWind + localWind;
        }
        return AZ::Vector3::CreateZero();
    }

    void ClothComponentMesh::EnableSkinning() const
    {
        if (m_actorClothSkinning)
        {
            for (const auto& subMeshInfo : m_meshNodeInfo.m_subMeshes)
            {
                AZ::Render::SkinnedMeshOverrideRequestBus::Event(
                    m_entityId,
                    &AZ::Render::SkinnedMeshOverrideRequestBus::Events::EnableSkinning,
                        aznumeric_cast<uint32_t>(m_meshNodeInfo.m_lodLevel), aznumeric_cast<uint32_t>(subMeshInfo.m_primitiveIndex));
            }
        }
    }

    void ClothComponentMesh::DisableSkinning() const
    {
        if (m_actorClothSkinning)
        {
            for (const auto& subMeshInfo : m_meshNodeInfo.m_subMeshes)
            {
                AZ::Render::SkinnedMeshOverrideRequestBus::Event(
                    m_entityId,
                    &AZ::Render::SkinnedMeshOverrideRequestBus::Events::DisableSkinning,
                        aznumeric_cast<uint32_t>(m_meshNodeInfo.m_lodLevel), aznumeric_cast<uint32_t>(subMeshInfo.m_primitiveIndex));
            }
        }
    }
} // namespace NvCloth
