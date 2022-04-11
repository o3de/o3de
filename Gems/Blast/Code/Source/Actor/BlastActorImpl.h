/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Actor/ShapesProvider.h>
#include <Blast/BlastActor.h>
#include <Actor/BlastActorDesc.h>
#include <PhysX/ColliderComponentBus.h>

namespace Nv::Blast
{
    class ExtPxAsset;
} // namespace Nv::Blast

namespace Blast
{
    //! Provides the glue between Blast actors and the PhysX actors they manipulate.
    class BlastActorImpl : public BlastActor
    {
    public:
        AZ_CLASS_ALLOCATOR(BlastActorImpl, AZ::SystemAllocator, 0);

        BlastActorImpl(const BlastActorDesc& desc);
        ~BlastActorImpl();

        void Spawn();

        void Damage(const NvBlastDamageProgram& program, NvBlastExtProgramParams* programParams) override;

        const BlastFamily& GetFamily() const override;
        const Nv::Blast::TkActor& GetTkActor() const override;
        AZ::Transform GetTransform() const override;

        const AZ::Entity* GetEntity() const override;
        const AZStd::vector<uint32_t>& GetChunkIndices() const override;
        bool IsStatic() const override;

        AzPhysics::SimulatedBody* GetSimulatedBody() override;
        const AzPhysics::SimulatedBody* GetSimulatedBody() const override;

    protected:
        //! We want to be able to override this function for testing purposes, because
        //! ColliderConfiguration::SetMaterialLibrary has dependency on AssetManager being alive. This forced us to have
        //! a separate Spawn method that invokes CalculateColliderConfiguration, since if it's invoked from
        //! constructor it does not call override method.
        virtual Physics::ColliderConfiguration CalculateColliderConfiguration(
            const AZ::Transform& transform, Physics::MaterialId material);

        //! Static function to add shapes, based on a list of indices that references chunks in a Blast asset.
        //! @param chunkIndices The indices of chunks in the asset parameter that will instantiate shapes.
        //! @param asset The Blast asset that stores chunk data based on indices.
        //! @param material The PhysX material to use to create shapes.
        void AddShapes(
            const AZStd::vector<uint32_t>& chunkIndices, const Nv::Blast::ExtPxAsset& asset,
            const Physics::MaterialId& material);

        const BlastFamily& m_family;
        Nv::Blast::TkActor& m_tkActor;
        AZStd::unique_ptr<ShapesProvider> m_shapesProvider;

        AZStd::shared_ptr<AZ::Entity> m_entity;
        AZStd::vector<uint32_t> m_chunkIndices;
        bool m_isLeafChunk;
        bool m_isStatic;

        // Stored from BlastActorDescription, because we can't use them right away in constructor
        Physics::MaterialId m_physicsMaterialId;
        AZ::Vector3 m_parentLinearVelocity = AZ::Vector3::CreateZero();
        AZ::Vector3 m_parentCenterOfMass = AZ::Vector3::CreateZero();
        AzPhysics::RigidBodyConfiguration m_bodyConfiguration;
        float m_scale = 1.0f;
    };
} // namespace Blast
