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

#include <PhysX_precompiled.h>

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <PxPhysicsAPI.h>
#include <Source/RigidBodyStatic.h>
#include <Source/Utils.h>
#include <PhysX/Utils.h>
#include <Source/Shape.h>
#include <Include/PhysX/NativeTypeIdentifiers.h>
#include <AzFramework/Physics/World.h>
#include <PhysX/PhysXLocks.h> 

namespace PhysX
{
    RigidBodyStatic::RigidBodyStatic(const Physics::WorldBodyConfiguration& configuration)
    {
        CreatePxActor(configuration);
    }

    RigidBodyStatic::~RigidBodyStatic()
    {
        //clean up the attached shapes
        {
            PHYSX_SCENE_WRITE_LOCK(m_staticRigidBody->getScene());
            for (auto shape : m_shapes)
            {
                m_staticRigidBody->detachShape(*shape->GetPxShape());
                shape->DetachedFromActor();
            }
        }
        m_shapes.clear();
    }

    void RigidBodyStatic::CreatePxActor(const Physics::WorldBodyConfiguration& configuration)
    {
        if (auto staticBody = PxActorFactories::CreatePxStaticRigidBody(configuration))
        {
            m_staticRigidBody = AZStd::shared_ptr<physx::PxRigidStatic>(staticBody, [](auto& actor)
            {
                PxActorFactories::ReleaseActor(actor);
            });
            m_actorUserData = PhysX::ActorData(m_staticRigidBody.get());
            m_actorUserData.SetRigidBodyStatic(this);
            m_actorUserData.SetEntityId(configuration.m_entityId);
            m_debugName = configuration.m_debugName;
            m_staticRigidBody->setName(m_debugName.c_str());
        }
    }

    void RigidBodyStatic::AddShape(const AZStd::shared_ptr<Physics::Shape>& shape)
    {
        auto pxShape = AZStd::rtti_pointer_cast<PhysX::Shape>(shape);
        if (pxShape && pxShape->GetPxShape())
        {
            {
                PHYSX_SCENE_WRITE_LOCK(m_staticRigidBody->getScene());
                m_staticRigidBody->attachShape(*pxShape->GetPxShape());
            }
            pxShape->AttachedToActor(m_staticRigidBody.get());
            m_shapes.push_back(pxShape);
        }
        else
        {
            AZ_Error("PhysX Rigid Body Static", false, "Trying to add an invalid shape.");
        }
    }

    AZStd::shared_ptr<Physics::Shape> RigidBodyStatic::GetShape(AZ::u32 index)
    {
        if (index >= m_shapes.size())
        {
            return nullptr;
        }
        return m_shapes[index];
    }

    AZ::u32 RigidBodyStatic::GetShapeCount()
    {
        return static_cast<AZ::u32>(m_shapes.size());
    }

    // Physics::WorldBody
    Physics::World* RigidBodyStatic::GetWorld() const
    {
        return m_staticRigidBody ? Utils::GetUserData(m_staticRigidBody->getScene()) : nullptr;
    }

    AZ::Transform RigidBodyStatic::GetTransform() const
    {
        if (m_staticRigidBody)
        {
            PHYSX_SCENE_READ_LOCK(m_staticRigidBody->getScene());
            return PxMathConvert(m_staticRigidBody->getGlobalPose());
        }
        return AZ::Transform::CreateIdentity();
    }

    void RigidBodyStatic::SetTransform(const AZ::Transform & transform)
    {
        if (m_staticRigidBody)
        {
            PHYSX_SCENE_WRITE_LOCK(m_staticRigidBody->getScene());
            m_staticRigidBody->setGlobalPose(PxMathConvert(transform));
        }
    }

    AZ::Vector3 RigidBodyStatic::GetPosition() const
    {
        if (m_staticRigidBody)
        {
            PHYSX_SCENE_READ_LOCK(m_staticRigidBody->getScene());
            return PxMathConvert(m_staticRigidBody->getGlobalPose().p);
        }
        return AZ::Vector3::CreateZero();
    }

    AZ::Quaternion RigidBodyStatic::GetOrientation() const
    {
        if (m_staticRigidBody)
        {
            PHYSX_SCENE_READ_LOCK(m_staticRigidBody->getScene());
            return PxMathConvert(m_staticRigidBody->getGlobalPose().q);
        }
        return  AZ::Quaternion::CreateZero();
    }

    AZ::Aabb RigidBodyStatic::GetAabb() const
    {
        if (m_staticRigidBody)
        {
            PHYSX_SCENE_READ_LOCK(m_staticRigidBody->getScene());
            return PxMathConvert(m_staticRigidBody->getWorldBounds(1.0f));
        }
        return AZ::Aabb::CreateNull();
    }

    Physics::RayCastHit RigidBodyStatic::RayCast(const Physics::RayCastRequest& request)
    {
        return PhysX::Utils::RayCast::ClosestRayHitAgainstShapes(request, m_shapes, GetTransform());
    }

    AZ::EntityId RigidBodyStatic::GetEntityId() const
    {
        return m_actorUserData.GetEntityId();
    }

    AZ::Crc32 RigidBodyStatic::GetNativeType() const
    {
        return PhysX::NativeTypeIdentifiers::RigidBodyStatic;
    }

    void* RigidBodyStatic::GetNativePointer() const
    {
        return m_staticRigidBody.get();
    }

    void RigidBodyStatic::AddToWorld(Physics::World& world)
    {
        physx::PxScene* scene = static_cast<physx::PxScene*>(world.GetNativePointer());

        if (!scene)
        {
            AZ_Error("RigidBodyStatic", false, "Tried to add body to invalid world.");
            return;
        }

        if (!m_staticRigidBody)
        {
            AZ_Error("RigidBodyStatic", false, "Tried to add invalid PhysX body to world.");
            return;
        }

        PHYSX_SCENE_WRITE_LOCK(scene);
        scene->addActor(*m_staticRigidBody);
    }

    void RigidBodyStatic::RemoveFromWorld(Physics::World& world)
    {
        physx::PxScene* scene = static_cast<physx::PxScene*>(world.GetNativePointer());
        if (!scene)
        {
            AZ_Error("RigidBodyStatic", false, "Tried to remove body from invalid world.");
            return;
        }

        if (!m_staticRigidBody)
        {
            AZ_Error("RigidBodyStatic", false, "Tried to remove invalid PhysX body from world.");
            return;
        }

        PHYSX_SCENE_WRITE_LOCK(scene);
        scene->removeActor(*m_staticRigidBody);
    }
}
