/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Math/Aabb.h>

#include <AzFramework/Physics/WindBus.h>

#include <NvCloth/ICloth.h>

#include <Components/ClothConfiguration.h>

#include <Utils/AssetHelper.h>

namespace NvCloth
{
    class ActorClothColliders;
    class ActorClothSkinning;
    class ClothConstraints;
    class ClothDebugDisplay;

    //! Class that applies cloth simulation to Static Meshes and Actors
    //! by reading their data and modifying the render nodes in real time.
    class ClothComponentMesh
        : public AZ::TransformNotificationBus::Handler
        , public AZ::TickBus::Handler
        , public Physics::WindNotificationsBus::Handler
    {
    public:
        AZ_RTTI(ClothComponentMesh, "{15A0F10C-6248-4CE4-A6FD-0E2D8AFCFEE8}");

        ClothComponentMesh(AZ::EntityId entityId, const ClothConfiguration& config);
        ~ClothComponentMesh();

        AZ_DISABLE_COPY_MOVE(ClothComponentMesh);

        // Rendering data.
        // It stores the tangent space information of each vertex, which is calculated every frame.
        struct RenderData
        {
            AZStd::vector<SimParticleFormat> m_particles;
            AZStd::vector<AZ::Vector3> m_tangents;
            AZStd::vector<AZ::Vector3> m_bitangents;
            AZStd::vector<AZ::Vector3> m_normals;
        };

        const RenderData& GetRenderData() const;
        RenderData& GetRenderData();

        void UpdateConfiguration(AZ::EntityId entityId, const ClothConfiguration& config);

        void CopyRenderDataToModel();

    protected:
        // Functions used to setup and tear down cloth component mesh
        void Setup(AZ::EntityId entityId, const ClothConfiguration& config);
        void TearDown();

        // ICloth notifications
        void OnPreSimulation(ClothId clothId, float deltaTime);
        void OnPostSimulation(ClothId clothId, float deltaTime, const AZStd::vector<SimParticleFormat>& updatedParticles);

        // AZ::TransformNotificationBus::Handler overrides ...
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // AZ::TickBus::Handler overrides ...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        int GetTickOrder() override;

        // Physics::WindNotificationsBus::Handler overrides ...
        void OnGlobalWindChanged() override;
        void OnWindChanged(const AZ::Aabb& aabb) override;

    private:
        void UpdateSimulationCollisions();
        void UpdateSimulationSkinning(float deltaTime);
        void UpdateSimulationConstraints();
        void UpdateRenderData(const AZStd::vector<SimParticleFormat>& particles);

        bool CreateCloth();
        void ApplyConfigurationToCloth();
        void MoveCloth(const AZ::Transform& worldTransform);
        void TeleportCloth(const AZ::Transform& worldTransform);

        void EnableSkinning() const;
        void DisableSkinning() const;

        AZ::Vector3 GetWindBusVelocity();

        // Entity Id of the cloth component
        AZ::EntityId m_entityId;

        // Current position in world space
        AZ::Vector3 m_worldPosition;

        // Configuration parameters for cloth simulation
        ClothConfiguration m_config;

        // Instance of cloth simulation
        ICloth* m_cloth = nullptr;

        // Cloth event handlers
        ICloth::PreSimulationEvent::Handler m_preSimulationEventHandler;
        ICloth::PostSimulationEvent::Handler m_postSimulationEventHandler;

        // Use a double buffer of render data to always have access to the previous frame's data.
        // The previous frame's data is used to workaround that debug draw is one frame delayed.
        static const AZ::u32 RenderDataBufferSize = 2;
        AZ::u32 m_renderDataBufferIndex = 0;
        AZStd::array<RenderData, RenderDataBufferSize> m_renderDataBuffer;

        // Vertex mapping between full mesh and simplified mesh used in cloth simulation.
        // Negative elements means the vertex has been removed.
        AZStd::vector<int> m_meshRemappedVertices;

        // Information to map the simulation particles to render mesh nodes.
        MeshNodeInfo m_meshNodeInfo;

        // Original cloth information from the mesh.
        MeshClothInfo m_meshClothInfo;

        // Cloth Colliders from the character
        AZStd::unique_ptr<ActorClothColliders> m_actorClothColliders;

        // Cloth Skinning from the character
        AZStd::unique_ptr<ActorClothSkinning> m_actorClothSkinning;
        float m_timeClothSkinningUpdates = 0.0f;

        // Cloth Constraints
        AZStd::unique_ptr<ClothConstraints> m_clothConstraints;
        AZStd::vector<AZ::Vector4> m_motionConstraints;
        AZStd::vector<AZ::Vector4> m_separationConstraints;

        AZStd::unique_ptr<ClothDebugDisplay> m_clothDebugDisplay;
        friend class ClothDebugDisplay; // Give access to data to draw debug information
    };
} // namespace NvCloth
