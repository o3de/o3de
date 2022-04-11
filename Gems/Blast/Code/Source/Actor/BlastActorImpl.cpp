/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Actor/BlastActorImpl.h>
#include <Actor/ShapesProvider.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Transform.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Physics/RigidBodyBus.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Utils.h>
#include <AzFramework/Physics/Components/SimulatedBodyComponentBus.h>
#include <Blast/BlastActor.h>
#include <Family/BlastFamily.h>
#include <NvBlastExtPxAsset.h>
#include <NvBlastTkActor.h>
#include <NvBlastTkAsset.h>
#include <NvBlastTypes.h>
#include <PhysX/ColliderComponentBus.h>
#include <PhysX/ComponentTypeIds.h>
#include <PhysX/MathConversion.h>
#include <PhysX/SystemComponentBus.h>

namespace Blast
{
    BlastActorImpl::BlastActorImpl(const BlastActorDesc& desc)
        : m_family(*desc.m_family)
        , m_tkActor(*desc.m_tkActor)
        , m_entity(desc.m_entity)
        , m_chunkIndices(desc.m_chunkIndices)
        , m_isLeafChunk(desc.m_isLeafChunk)
        , m_isStatic(desc.m_isStatic)
        , m_physicsMaterialId(desc.m_physicsMaterialId)
        , m_parentLinearVelocity(desc.m_parentLinearVelocity)
        , m_parentCenterOfMass(desc.m_parentCenterOfMass)
        , m_bodyConfiguration(desc.m_bodyConfiguration)
        , m_scale(desc.m_scale)
    {
        // Store pointer to ourselves in the blast toolkit actor's userData
        m_tkActor.userData = this;

        m_shapesProvider = AZStd::make_unique<ShapesProvider>(m_entity->GetId(), desc.m_bodyConfiguration);
    }

    BlastActorImpl::~BlastActorImpl()
    {
        m_tkActor.userData = nullptr;
    }

    void BlastActorImpl::Spawn()
    {
        // Add shapes for each of the visible chunks
        AddShapes(m_chunkIndices, m_family.GetPxAsset(), m_physicsMaterialId);

        m_entity->Init();
        m_entity->Activate();

        auto transform = AZ::Transform::CreateFromQuaternionAndTranslation(
            m_bodyConfiguration.m_orientation, m_bodyConfiguration.m_position);
        transform.MultiplyByUniformScale(m_scale);

        AZ::TransformBus::Event(m_entity->GetId(), &AZ::TransformInterface::SetWorldTM, transform);

        // Set initial velocities if we're not static
        if (!m_isStatic)
        {
            AzPhysics::RigidBody* rigidBody = nullptr;
            Physics::RigidBodyRequestBus::EventResult(
                rigidBody, m_entity->GetId(), &Physics::RigidBodyRequests::GetRigidBody);
            rigidBody->SetTransform(transform);
            const AZ::Vector3 com = rigidBody->GetTransform().TransformPoint(rigidBody->GetCenterOfMassLocal());
            const AZ::Vector3 linearVelocity =
                m_parentLinearVelocity + m_bodyConfiguration.m_initialAngularVelocity.Cross(com - m_parentCenterOfMass);
            Physics::RigidBodyRequestBus::Event(
                m_entity->GetId(), &Physics::RigidBodyRequests::SetLinearVelocity, linearVelocity);
            Physics::RigidBodyRequestBus::Event(
                m_entity->GetId(), &Physics::RigidBodyRequests::SetAngularVelocity,
                m_bodyConfiguration.m_initialAngularVelocity);
        }
    }

    void BlastActorImpl::AddShapes(
        const AZStd::vector<uint32_t>& chunkIndices, const Nv::Blast::ExtPxAsset& asset,
        const Physics::MaterialId& material)
    {
        const Nv::Blast::ExtPxChunk* pxChunks = asset.getChunks();
        const Nv::Blast::ExtPxSubchunk* pxSubchunks = asset.getSubchunks();
        const uint32_t chunkCount = asset.getChunkCount();
        const uint32_t subchunkCount = asset.getSubchunkCount();

        AZ_Assert(pxChunks, "Received asset with a null chunk array.");
        AZ_Assert(pxSubchunks, "Received asset with a null subchunk array.");
        if (!pxChunks || !pxSubchunks)
        {
            return;
        }

        for (uint32_t chunkId : chunkIndices)
        {
            AZ_Assert(chunkId < chunkCount, "Out of bounds access to the BlastPxActor's PxChunks.");
            if (chunkId >= chunkCount)
            {
                continue;
            }

            const Nv::Blast::ExtPxChunk& chunk = pxChunks[chunkId];
            for (uint32_t i = 0; i < chunk.subchunkCount; i++)
            {
                const uint32_t subchunkIndex = chunk.firstSubchunkIndex + i;
                AZ_Assert(subchunkIndex < subchunkCount, "Out of bounds access to the BlastPxActor's PxSubchunks.");
                if (subchunkIndex >= subchunkCount)
                {
                    continue;
                }

                auto& subchunk = pxSubchunks[subchunkIndex];
                AZ::Transform transform = PxMathConvert(subchunk.transform);
                auto colliderConfiguration = CalculateColliderConfiguration(transform, material);

                Physics::NativeShapeConfiguration shapeConfiguration;
                shapeConfiguration.m_nativeShapePtr =
                    reinterpret_cast<void*>(const_cast<physx::PxConvexMeshGeometry*>(&subchunk.geometry)->convexMesh);
                shapeConfiguration.m_nativeShapeScale = AZ::Vector3(m_scale);

                AZStd::shared_ptr<Physics::Shape> shape = AZ::Interface<Physics::SystemRequests>::Get()->CreateShape(
                    colliderConfiguration, shapeConfiguration);

                AZ_Assert(shape, "Failed to create Shape for BlastActor");

                m_shapesProvider->AddShape(shape);
            }
        }
    }

    Physics::ColliderConfiguration BlastActorImpl::CalculateColliderConfiguration(
        const AZ::Transform& transform, Physics::MaterialId material)
    {
        auto& actorConfiguration = m_family.GetActorConfiguration();
        Physics::ColliderConfiguration colliderConfiguration;
        colliderConfiguration.m_position = transform.GetTranslation();
        colliderConfiguration.m_rotation = transform.GetRotation();
        colliderConfiguration.m_isExclusive = true;
        colliderConfiguration.m_materialSelection.SetMaterialId(material);
        colliderConfiguration.m_collisionGroupId = actorConfiguration.m_collisionGroupId;
        colliderConfiguration.m_collisionLayer = actorConfiguration.m_collisionLayer;
        colliderConfiguration.m_isInSceneQueries = actorConfiguration.m_isInSceneQueries;
        colliderConfiguration.m_isSimulated = actorConfiguration.m_isSimulated;
        colliderConfiguration.m_tag = actorConfiguration.m_tag;

        return colliderConfiguration;
    }

    AZ::Transform BlastActorImpl::GetTransform() const
    {
        return GetSimulatedBody()->GetTransform();
    }

    const BlastFamily& BlastActorImpl::GetFamily() const
    {
        return m_family;
    }

    const Nv::Blast::TkActor& BlastActorImpl::GetTkActor() const
    {
        return m_tkActor;
    }

    AzPhysics::SimulatedBody* BlastActorImpl::GetSimulatedBody()
    {
        AzPhysics::SimulatedBody* worldBody = nullptr;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(
            worldBody, m_entity->GetId(), &AzPhysics::SimulatedBodyComponentRequests::GetSimulatedBody);
        return worldBody;
    }

    const AzPhysics::SimulatedBody* BlastActorImpl::GetSimulatedBody() const
    {
        AzPhysics::SimulatedBody* worldBody = nullptr;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(
            worldBody, m_entity->GetId(), &AzPhysics::SimulatedBodyComponentRequests::GetSimulatedBody);
        return worldBody;
    }

    const AZStd::vector<uint32_t>& BlastActorImpl::GetChunkIndices() const
    {
        return m_chunkIndices;
    }

    bool BlastActorImpl::IsStatic() const
    {
        return m_isStatic;
    }

    const AZ::Entity* BlastActorImpl::GetEntity() const
    {
        return m_entity.get();
    }

    void BlastActorImpl::Damage(const NvBlastDamageProgram& program, NvBlastExtProgramParams* programParams)
    {
        m_tkActor.damage(program, programParams);
    }
} // namespace Blast
