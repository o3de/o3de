/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/RigidBodyStatic.h>

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/utility/as_const.h>
#include <AzFramework/Physics/Configuration/StaticRigidBodyConfiguration.h>
#include <PxPhysicsAPI.h>
#include <Source/Utils.h>
#include <PhysX/Utils.h>
#include <Source/Shape.h>
#include <Include/PhysX/NativeTypeIdentifiers.h>
#include <PhysX/PhysXLocks.h>
#include <Common/PhysXSceneQueryHelpers.h>
#include <PhysX/MathConversion.h>

namespace PhysX
{
    AZ_CLASS_ALLOCATOR_IMPL(PhysX::StaticRigidBody, AZ::SystemAllocator);

    StaticRigidBody::StaticRigidBody(const AzPhysics::StaticRigidBodyConfiguration& configuration)
    {
        CreatePhysXActor(configuration);
    }

    StaticRigidBody::~StaticRigidBody()
    {
        //clean up the attached shapes
        {
            PHYSX_SCENE_WRITE_LOCK(m_pxStaticRigidBody->getScene());
            for (auto shape : m_shapes)
            {
                m_pxStaticRigidBody->detachShape(*shape->GetPxShape());
                shape->DetachedFromActor();
            }
        }
        m_shapes.clear();

        // Invalidate user data so it sets m_pxStaticRigidBody->userData to nullptr.
        // It's appropriate to do this as m_pxStaticRigidBody is a shared pointer and
        // technically it could survive m_actorUserData life's span.
        m_actorUserData.Invalidate();
    }

    void StaticRigidBody::CreatePhysXActor(const AzPhysics::StaticRigidBodyConfiguration& configuration)
    {
        if (m_pxStaticRigidBody != nullptr)
        {
            AZ_Warning("PhysX Static Rigid Body", false, "Trying to create PhysX static rigid actor when it's already created");
            return;
        }

        if (m_pxStaticRigidBody = PxActorFactories::CreatePxStaticRigidBody(configuration))
        {
            m_actorUserData = ActorData(m_pxStaticRigidBody.get());
            m_actorUserData.SetRigidBodyStatic(this);
            m_actorUserData.SetEntityId(configuration.m_entityId);

            m_debugName = configuration.m_debugName;
            m_pxStaticRigidBody->setName(m_debugName.c_str());
        }
    }

    void StaticRigidBody::AddShape(AZStd::shared_ptr<Physics::Shape> shape)
    {
        auto pxShape = AZStd::rtti_pointer_cast<PhysX::Shape>(shape);
        if (pxShape && pxShape->GetPxShape())
        {
            {
                PHYSX_SCENE_WRITE_LOCK(m_pxStaticRigidBody->getScene());
                m_pxStaticRigidBody->attachShape(*pxShape->GetPxShape());
            }
            pxShape->AttachedToActor(m_pxStaticRigidBody.get());
            m_shapes.push_back(pxShape);
        }
        else
        {
            AZ_Error("PhysX Rigid Body Static", false, "Trying to add an invalid shape.");
        }
    }

    AZStd::shared_ptr<Physics::Shape> StaticRigidBody::GetShape(AZ::u32 index)
    {
        AZStd::shared_ptr<const Physics::Shape> constShape = AZStd::as_const(*this).GetShape(index);
        return AZStd::const_pointer_cast<Physics::Shape>(constShape);
    }

    AZStd::shared_ptr<const Physics::Shape> StaticRigidBody::GetShape(AZ::u32 index) const
    {
        if (index >= m_shapes.size())
        {
            return nullptr;
        }
        return m_shapes[index];
    }

    AZ::u32 StaticRigidBody::GetShapeCount() const
    {
        return static_cast<AZ::u32>(m_shapes.size());
    }

    AZ::Transform StaticRigidBody::GetTransform() const
    {
        if (m_pxStaticRigidBody)
        {
            PHYSX_SCENE_READ_LOCK(m_pxStaticRigidBody->getScene());
            return PxMathConvert(m_pxStaticRigidBody->getGlobalPose());
        }
        return AZ::Transform::CreateIdentity();
    }

    void StaticRigidBody::SetTransform(const AZ::Transform & transform)
    {
        if (m_pxStaticRigidBody)
        {
            PHYSX_SCENE_WRITE_LOCK(m_pxStaticRigidBody->getScene());
            m_pxStaticRigidBody->setGlobalPose(PxMathConvert(transform));
        }
    }

    AZ::Vector3 StaticRigidBody::GetPosition() const
    {
        if (m_pxStaticRigidBody)
        {
            PHYSX_SCENE_READ_LOCK(m_pxStaticRigidBody->getScene());
            return PxMathConvert(m_pxStaticRigidBody->getGlobalPose().p);
        }
        return AZ::Vector3::CreateZero();
    }

    AZ::Quaternion StaticRigidBody::GetOrientation() const
    {
        if (m_pxStaticRigidBody)
        {
            PHYSX_SCENE_READ_LOCK(m_pxStaticRigidBody->getScene());
            return PxMathConvert(m_pxStaticRigidBody->getGlobalPose().q);
        }
        return  AZ::Quaternion::CreateZero();
    }

    AZ::Aabb StaticRigidBody::GetAabb() const
    {
        if (m_pxStaticRigidBody)
        {
            PHYSX_SCENE_READ_LOCK(m_pxStaticRigidBody->getScene());
            return PxMathConvert(m_pxStaticRigidBody->getWorldBounds(1.0f));
        }
        return AZ::Aabb::CreateNull();
    }

    AzPhysics::SceneQueryHit StaticRigidBody::RayCast(const AzPhysics::RayCastRequest& request)
    {
        return PhysX::SceneQueryHelpers::ClosestRayHitAgainstShapes(request, m_shapes, GetTransform());
    }

    AZ::EntityId StaticRigidBody::GetEntityId() const
    {
        return m_actorUserData.GetEntityId();
    }

    AZ::Crc32 StaticRigidBody::GetNativeType() const
    {
        return PhysX::NativeTypeIdentifiers::RigidBodyStatic;
    }

    void* StaticRigidBody::GetNativePointer() const
    {
        return m_pxStaticRigidBody.get();
    }
}
