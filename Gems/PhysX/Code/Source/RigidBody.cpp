/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PhysX_precompiled.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzFramework/Physics/Utils.h>
#include <AzFramework/Physics/Configuration/RigidBodyConfiguration.h>
#include <PhysX/NativeTypeIdentifiers.h>
#include <PhysX/MathConversion.h>
#include <Source/RigidBody.h>
#include <Source/Utils.h>
#include <PhysX/Utils.h>
#include <Source/Shape.h>
#include <extensions/PxRigidBodyExt.h>
#include <PxPhysicsAPI.h>
#include <PhysX/PhysXLocks.h>
#include <Common/PhysXSceneQueryHelpers.h>

namespace PhysX
{
    void RigidBody::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<RigidBody>()
                ->Version(1)
            ;
        }
    }

    RigidBody::RigidBody(const AzPhysics::RigidBodyConfiguration& configuration)
        : m_startAsleep(configuration.m_startAsleep)
    {
        CreatePhysXActor(configuration);
    }

    RigidBody::~RigidBody()
    {
        //clean up the attached shapes
        if(m_pxRigidActor)
        {
            PHYSX_SCENE_WRITE_LOCK(m_pxRigidActor->getScene());
            for (auto shape : m_shapes)
            {
                m_pxRigidActor->detachShape(*shape->GetPxShape());
                shape->DetachedFromActor();
            }
        }
        m_shapes.clear();

        // Invalidate user data so it sets m_pxRigidActor->userData to nullptr.
        // It's appropriate to do this as m_pxRigidActor is a shared pointer and
        // techniqucally it could survive m_actorUserData life's spam.
        m_actorUserData.Invalidate();
    }

    void RigidBody::CreatePhysXActor(const AzPhysics::RigidBodyConfiguration& configuration)
    {
        if (m_pxRigidActor != nullptr)
        {
            AZ_Warning("PhysX Rigid Body", false, "Trying to create PhysX rigid actor when it's already created");
            return;
        }

        if (m_pxRigidActor = PxActorFactories::CreatePxRigidBody(configuration))
        {
            m_actorUserData = ActorData(m_pxRigidActor.get());
            m_actorUserData.SetRigidBody(this);
            m_actorUserData.SetEntityId(configuration.m_entityId);

            SetName(configuration.m_debugName);
            SetGravityEnabled(configuration.m_gravityEnabled);
            SetCCDEnabled(configuration.m_ccdEnabled);
            SetKinematic(configuration.m_kinematic);

            if (configuration.m_customUserData)
            {
                SetUserData(configuration.m_customUserData);
            }
        }
    }

    void RigidBody::AddShape(AZStd::shared_ptr<Physics::Shape> shape)
    {
        if (!m_pxRigidActor || !shape)
        {
            return;
        }

        auto pxShape = AZStd::rtti_pointer_cast<PhysX::Shape>(shape);

        if (!pxShape)
        {
            AZ_Error("PhysX Rigid Body", false, "Trying to add a shape of unknown type. Name: %s", GetName().c_str());
            return;
        }

        if (!pxShape->GetPxShape())
        {
            AZ_Error("PhysX Rigid Body", false, "Trying to add a shape with no valid PxShape. Name: %s", GetName().c_str());
            return;
        }

        if (pxShape->GetPxShape()->getGeometryType() == physx::PxGeometryType::eTRIANGLEMESH && !IsKinematic())
        {
            AZ_Error("PhysX", false, "Cannot use triangle mesh geometry on a dynamic object: %s", GetName().c_str());
            return;
        }

        {
            PHYSX_SCENE_WRITE_LOCK(m_pxRigidActor->getScene());
            m_pxRigidActor->attachShape(*pxShape->GetPxShape());
        }

        pxShape->AttachedToActor(m_pxRigidActor.get());
        m_shapes.push_back(pxShape);
    }

    void RigidBody::RemoveShape(AZStd::shared_ptr<Physics::Shape> shape)
    {
        if (m_pxRigidActor == nullptr)
        {
            AZ_Warning("PhysX::RigidBody", false, "Trying to remove shape from rigid body with no actor");
            return;
        }

        auto pxShape = AZStd::rtti_pointer_cast<PhysX::Shape>(shape);
        if (!pxShape)
        {
            AZ_Warning("PhysX::RigidBody", false, "Trying to remove shape of unknown type", GetName().c_str());
            return;
        }

        const auto found = AZStd::find(m_shapes.begin(), m_shapes.end(), shape);
        if (found == m_shapes.end())
        {
            AZ_Warning("PhysX::RigidBody", false, "Shape has not been attached to this rigid body: %s", GetName().c_str());
            return;
        }

        {
            PHYSX_SCENE_WRITE_LOCK(m_pxRigidActor->getScene());
            m_pxRigidActor->detachShape(*pxShape->GetPxShape());
        }
        pxShape->DetachedFromActor();
        m_shapes.erase(found);
    }

    void RigidBody::UpdateMassProperties(AzPhysics::MassComputeFlags flags, const AZ::Vector3* centerOfMassOffsetOverride, const AZ::Matrix3x3* inertiaTensorOverride, const float* massOverride)
    {
        // Input validation
        bool computeCenterOfMass = AzPhysics::MassComputeFlags::COMPUTE_COM == (flags & AzPhysics::MassComputeFlags::COMPUTE_COM);
        AZ_Assert(computeCenterOfMass || centerOfMassOffsetOverride,
            "UpdateMassProperties: MassComputeFlags::COMPUTE_COM is not set but COM offset is not specified");
        computeCenterOfMass = computeCenterOfMass || !centerOfMassOffsetOverride;

        bool computeInertiaTensor = AzPhysics::MassComputeFlags::COMPUTE_INERTIA == (flags & AzPhysics::MassComputeFlags::COMPUTE_INERTIA);
        AZ_Assert(computeInertiaTensor || inertiaTensorOverride,
            "UpdateMassProperties: MassComputeFlags::COMPUTE_INERTIA is not set but inertia tensor is not specified");
        computeInertiaTensor = computeInertiaTensor || !inertiaTensorOverride;

        bool computeMass = AzPhysics::MassComputeFlags::COMPUTE_MASS == (flags & AzPhysics::MassComputeFlags::COMPUTE_MASS);
        AZ_Assert(computeMass || massOverride,
            "UpdateMassProperties: MassComputeFlags::COMPUTE_MASS is not set but mass is not specified");
        computeMass = computeMass || !massOverride;

        AZ::u32 shapesCount = GetShapeCount();

        // Basic cases when we don't need to compute anything
        if (shapesCount == 0 || flags == AzPhysics::MassComputeFlags::NONE)
        {
            if (massOverride)
            {
                SetMass(*massOverride);
            }

            if (inertiaTensorOverride)
            {
                SetInertia(*inertiaTensorOverride);
            }

            if (centerOfMassOffsetOverride)
            {
                SetCenterOfMassOffset(*centerOfMassOffsetOverride);
            }
            return;
        }

        // Setup center of mass offset pointer for PxRigidBodyExt::updateMassAndInertia function
        AZStd::optional<physx::PxVec3> optionalComOverride;
        if (!computeCenterOfMass && centerOfMassOffsetOverride)
        {
            optionalComOverride = PxMathConvert(*centerOfMassOffsetOverride);
        }

        const physx::PxVec3* massLocalPose = optionalComOverride.has_value() ? &optionalComOverride.value() : nullptr;

        bool includeAllShapesInMassCalculation =
            AzPhysics::MassComputeFlags::INCLUDE_ALL_SHAPES == (flags & AzPhysics::MassComputeFlags::INCLUDE_ALL_SHAPES);

        // Handle the case when we don't compute mass
        if (!computeMass)
        {
            {
                PHYSX_SCENE_WRITE_LOCK(m_pxRigidActor->getScene());
                physx::PxRigidBodyExt::setMassAndUpdateInertia(*m_pxRigidActor, *massOverride, massLocalPose,
                    includeAllShapesInMassCalculation);
            }

            if (!computeInertiaTensor)
            {
                SetInertia(*inertiaTensorOverride);
            }

            return;
        }

        // Handle the cases when mass should be computed from density
        if (shapesCount == 1)
        {
            AZStd::shared_ptr<Physics::Shape> shape = GetShape(0);
            float density = shape->GetMaterial()->GetDensity();

            PHYSX_SCENE_WRITE_LOCK(m_pxRigidActor->getScene());
            physx::PxRigidBodyExt::updateMassAndInertia(*m_pxRigidActor, density, massLocalPose,
                includeAllShapesInMassCalculation);
        }
        else
        {
            AZStd::vector<float> densities(shapesCount);
            for (AZ::u32 i = 0; i < shapesCount; ++i)
            {
                densities[i] = GetShape(i)->GetMaterial()->GetDensity();
            }

            PHYSX_SCENE_WRITE_LOCK(m_pxRigidActor->getScene());
            physx::PxRigidBodyExt::updateMassAndInertia(*m_pxRigidActor, densities.data(),
                shapesCount, massLocalPose, includeAllShapesInMassCalculation);
        }

        // Set the overrides if provided.
        // Note: We don't set the center of mass here because it was already provided
        // to PxRigidBodyExt::updateMassAndInertia above
        if (!computeInertiaTensor)
        {
            SetInertia(*inertiaTensorOverride);
        }
    }

    AZ::u32 RigidBody::GetShapeCount()
    {
        return static_cast<AZ::u32>(m_shapes.size());
    }

    AZStd::shared_ptr<Physics::Shape> RigidBody::GetShape(AZ::u32 index)
    {
        if (index >= m_shapes.size())
        {
            return nullptr;
        }
        return m_shapes[index];
    }

    AZ::Vector3 RigidBody::GetCenterOfMassWorld() const
    {
        return m_pxRigidActor ? GetTransform().TransformPoint(GetCenterOfMassLocal()) : AZ::Vector3::CreateZero();
    }

    AZ::Vector3 RigidBody::GetCenterOfMassLocal() const
    {
        if (m_pxRigidActor)
        {
            PHYSX_SCENE_READ_LOCK(m_pxRigidActor->getScene());
            return PxMathConvert(m_pxRigidActor->getCMassLocalPose().p);
        }
        return AZ::Vector3::CreateZero();
    }

    AZ::Matrix3x3 RigidBody::GetInverseInertiaWorld() const
    {
        if (m_pxRigidActor)
        {
            PHYSX_SCENE_READ_LOCK(m_pxRigidActor->getScene());
            AZ::Vector3 inverseInertiaDiagonal = PxMathConvert(m_pxRigidActor->getMassSpaceInvInertiaTensor());
            AZ::Matrix3x3 rotationToWorld = AZ::Matrix3x3::CreateFromQuaternion(PxMathConvert(m_pxRigidActor->getGlobalPose().q.getConjugate()));
            return Physics::Utils::InverseInertiaLocalToWorld(inverseInertiaDiagonal, rotationToWorld);
        }

        return AZ::Matrix3x3::CreateZero();
    }

    AZ::Matrix3x3 RigidBody::GetInverseInertiaLocal() const
    {
        if (m_pxRigidActor)
        {
            PHYSX_SCENE_READ_LOCK(m_pxRigidActor->getScene());
            physx::PxVec3 inverseInertiaDiagonal = m_pxRigidActor->getMassSpaceInvInertiaTensor();
            return AZ::Matrix3x3::CreateDiagonal(PxMathConvert(inverseInertiaDiagonal));
        }
        return AZ::Matrix3x3::CreateZero();
    }

    float RigidBody::GetMass() const
    {
        if (m_pxRigidActor)
        {
            PHYSX_SCENE_READ_LOCK(m_pxRigidActor->getScene());
            return m_pxRigidActor->getMass();
        }
        return 0.0f;
    }

    float RigidBody::GetInverseMass() const
    {
        if (m_pxRigidActor)
        {
            PHYSX_SCENE_READ_LOCK(m_pxRigidActor->getScene());
            return m_pxRigidActor->getInvMass();
        }
        return 0.0f;
    }

    void RigidBody::SetMass(float mass)
    {
        if (m_pxRigidActor)
        {
            PHYSX_SCENE_WRITE_LOCK(m_pxRigidActor->getScene());
            m_pxRigidActor->setMass(mass);
        }
    }

    void RigidBody::SetCenterOfMassOffset(const AZ::Vector3& comOffset)
    {
        if (m_pxRigidActor)
        {
            PHYSX_SCENE_WRITE_LOCK(m_pxRigidActor->getScene());
            m_pxRigidActor->setCMassLocalPose(physx::PxTransform(PxMathConvert(Utils::Sanitize(comOffset))));
        }
    }

    void RigidBody::UpdateComputedCenterOfMass()
    {
        if (m_pxRigidActor)
        {
            physx::PxU32 shapeCount = 0;
            {
                PHYSX_SCENE_READ_LOCK(m_pxRigidActor->getScene());
                shapeCount = m_pxRigidActor->getNbShapes();
            }
            if (shapeCount > 0)
            {
                AZStd::vector<physx::PxShape*> shapes;
                shapes.resize(shapeCount);

                {
                    PHYSX_SCENE_READ_LOCK(m_pxRigidActor->getScene());
                    m_pxRigidActor->getShapes(&shapes[0], shapeCount);
                }

                shapes.erase(AZStd::remove_if(shapes.begin()
                    , shapes.end()
                    , [](const physx::PxShape* shape)
                      { 
                        return shape->getFlags() & physx::PxShapeFlag::eTRIGGER_SHAPE;
                      })
                    , shapes.end());
                shapeCount = static_cast<physx::PxU32>(shapes.size());

                if (shapeCount == 0)
                {
                    SetZeroCenterOfMass();
                    return;
                }

                const auto properties = physx::PxRigidBodyExt::computeMassPropertiesFromShapes(&shapes[0], shapeCount);
                const physx::PxTransform computedCenterOfMass(properties.centerOfMass);
                {
                    PHYSX_SCENE_WRITE_LOCK(m_pxRigidActor->getScene());
                    m_pxRigidActor->setCMassLocalPose(computedCenterOfMass);
                }
            }
            else
            {
                SetZeroCenterOfMass();
            }
        }
    }

    void RigidBody::SetInertia(const AZ::Matrix3x3& inertia)
    {
        if (m_pxRigidActor)
        {
            PHYSX_SCENE_WRITE_LOCK(m_pxRigidActor->getScene());
            m_pxRigidActor->setMassSpaceInertiaTensor(PxMathConvert(inertia.RetrieveScale()));
        }
    }

    void RigidBody::ComputeInertia()
    {
        if (m_pxRigidActor)
        {
            PHYSX_SCENE_WRITE_LOCK(m_pxRigidActor->getScene());
            auto localPose = m_pxRigidActor->getCMassLocalPose().p;
            physx::PxRigidBodyExt::setMassAndUpdateInertia(*m_pxRigidActor, m_pxRigidActor->getMass(), &localPose);
        }
    }

    AZ::Vector3 RigidBody::GetLinearVelocity() const
    {
        if (m_pxRigidActor)
        {
            PHYSX_SCENE_READ_LOCK(m_pxRigidActor->getScene());
            return PxMathConvert(m_pxRigidActor->getLinearVelocity());
        }
        return AZ::Vector3::CreateZero();
    }

    void RigidBody::SetLinearVelocity(const AZ::Vector3& velocity)
    {
        if (m_pxRigidActor)
        {
            PHYSX_SCENE_WRITE_LOCK(m_pxRigidActor->getScene());
            m_pxRigidActor->setLinearVelocity(PxMathConvert(Utils::Sanitize(velocity)));
        }
    }

    AZ::Vector3 RigidBody::GetAngularVelocity() const
    {
        if (m_pxRigidActor)
        {
            PHYSX_SCENE_READ_LOCK(m_pxRigidActor->getScene());
            return PxMathConvert(m_pxRigidActor->getAngularVelocity());
        }
        return AZ::Vector3::CreateZero();
    }

    void RigidBody::SetAngularVelocity(const AZ::Vector3& angularVelocity)
    {
        if (m_pxRigidActor)
        {
            PHYSX_SCENE_WRITE_LOCK(m_pxRigidActor->getScene());
            m_pxRigidActor->setAngularVelocity(PxMathConvert(Utils::Sanitize(angularVelocity)));
        }
    }

    AZ::Vector3 RigidBody::GetLinearVelocityAtWorldPoint(const AZ::Vector3& worldPoint) const
    {
        return m_pxRigidActor ?
               GetLinearVelocity() + GetAngularVelocity().Cross(worldPoint - GetCenterOfMassWorld()) :
               AZ::Vector3::CreateZero();
    }

    void RigidBody::ApplyLinearImpulse(const AZ::Vector3& impulse)
    {
        if (m_pxRigidActor)
        {
            physx::PxScene* scene = m_pxRigidActor->getScene();
            if (!scene)
            {
                AZ_Warning("PhysX Rigid Body", false, "ApplyLinearImpulse is only valid if the rigid body has been added to a scene. Name: %s", GetName().c_str());
                return;
            }

            if (IsKinematic())
            {
                AZ_Warning("PhysX Rigid Body", false, "ApplyLinearImpulse is only valid if the rigid body is not kinematic. Name: %s", GetName().c_str());
                return;
            }
            PHYSX_SCENE_WRITE_LOCK(scene);
            m_pxRigidActor->addForce(PxMathConvert(Utils::Sanitize(impulse)), physx::PxForceMode::eIMPULSE);
        }
    }

    void RigidBody::ApplyLinearImpulseAtWorldPoint(const AZ::Vector3& impulse, const AZ::Vector3& worldPoint)
    {
        if (m_pxRigidActor)
        {
            if (IsKinematic())
            {
                AZ_Warning("PhysX Rigid Body", false, "ApplyLinearImpulseAtWorldPoint is only valid if the rigid body is not kinematic. Name: %s", GetName().c_str());
                return;
            }
            PHYSX_SCENE_WRITE_LOCK(m_pxRigidActor->getScene());
            physx::PxRigidBodyExt::addForceAtPos(*m_pxRigidActor, PxMathConvert(Utils::Sanitize(impulse)),
                PxMathConvert(Utils::Sanitize(worldPoint)), physx::PxForceMode::eIMPULSE);
        }
    }

    void RigidBody::ApplyAngularImpulse(const AZ::Vector3& angularImpulse)
    {
        if (m_pxRigidActor)
        {
            physx::PxScene* scene = m_pxRigidActor->getScene();
            if (!scene)
            {
                AZ_Warning("PhysX Rigid Body", false, "ApplyAngularImpulse is only valid if the rigid body has been added to a scene. Name: %s", GetName().c_str());
                return;
            }

            if (IsKinematic())
            {
                AZ_Warning("PhysX Rigid Body", false, "ApplyAngularImpulse is only valid if the rigid body is not kinematic. Name: %s", GetName().c_str());
                return;
            }

            PHYSX_SCENE_WRITE_LOCK(scene);
            m_pxRigidActor->addTorque(PxMathConvert(Utils::Sanitize(angularImpulse)), physx::PxForceMode::eIMPULSE);
        }
    }

    void RigidBody::SetKinematic(bool isKinematic)
    {
        if (m_pxRigidActor)
        {
            PHYSX_SCENE_WRITE_LOCK(m_pxRigidActor->getScene());
            m_pxRigidActor->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, isKinematic);
        }
    }

    bool RigidBody::IsKinematic() const
    {
        bool result = false;

        if (m_pxRigidActor)
        {
            PHYSX_SCENE_READ_LOCK(m_pxRigidActor->getScene());
            auto rigidBodyFlags = m_pxRigidActor->getRigidBodyFlags();
            result = rigidBodyFlags.isSet(physx::PxRigidBodyFlag::eKINEMATIC);
        }

        return result;
    }

    void RigidBody::SetKinematicTarget(const AZ::Transform& targetTransform)
    {
        if (IsKinematic())
        {
            PHYSX_SCENE_WRITE_LOCK(m_pxRigidActor->getScene());
            m_pxRigidActor->setKinematicTarget(PxMathConvert(targetTransform));
        }
        else
        {
            AZ_Error("PhysX Rigid Body", false, "SetKinematicTarget is only valid if rigid body is kinematic. Name: %s", GetName().c_str());
        }
    }

    bool RigidBody::IsGravityEnabled() const
    {
        if (m_pxRigidActor)
        {
            PHYSX_SCENE_READ_LOCK(m_pxRigidActor->getScene());
            return m_pxRigidActor->getActorFlags().isSet(physx::PxActorFlag::eDISABLE_GRAVITY) == false;
        }
        return false;
    }

    void RigidBody::SetGravityEnabled(bool enabled)
    {
        {
            PHYSX_SCENE_WRITE_LOCK(m_pxRigidActor->getScene());
            m_pxRigidActor->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, enabled == false);
        }
        if (enabled)
        {
            ForceAwake();
        }
    }

    void RigidBody::SetSimulationEnabled(bool enabled)
    {
        if (m_pxRigidActor)
        {
            PHYSX_SCENE_WRITE_LOCK(m_pxRigidActor->getScene());
            m_pxRigidActor->setActorFlag(physx::PxActorFlag::eDISABLE_SIMULATION, enabled == false);
        }
    }

    void RigidBody::SetCCDEnabled(bool enabled)
    {
        PHYSX_SCENE_WRITE_LOCK(m_pxRigidActor->getScene());
        m_pxRigidActor->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_CCD, enabled);
    }

    AZ::Transform RigidBody::GetTransform() const
    {
        if (m_pxRigidActor)
        {
            PHYSX_SCENE_READ_LOCK(m_pxRigidActor->getScene());
            return PxMathConvert(m_pxRigidActor->getGlobalPose());
        }
        return AZ::Transform::CreateIdentity();
    }

    void RigidBody::SetTransform(const AZ::Transform& transform)
    {
        if (m_pxRigidActor)
        {
            PHYSX_SCENE_WRITE_LOCK(m_pxRigidActor->getScene());
            m_pxRigidActor->setGlobalPose(PxMathConvert(transform));
        }
    }

    AZ::Vector3 RigidBody::GetPosition() const
    {
        if (m_pxRigidActor)
        {
            PHYSX_SCENE_READ_LOCK(m_pxRigidActor->getScene());
            return PxMathConvert(m_pxRigidActor->getGlobalPose().p);
        }
        return AZ::Vector3::CreateZero();
    }

    AZ::Quaternion RigidBody::GetOrientation() const
    {
        if (m_pxRigidActor)
        {
            PHYSX_SCENE_READ_LOCK(m_pxRigidActor->getScene());
            return PxMathConvert(m_pxRigidActor->getGlobalPose().q);
        }
        return  AZ::Quaternion::CreateZero();
    }

    AZ::Aabb RigidBody::GetAabb() const
    {
        if (m_pxRigidActor)
        {
            PHYSX_SCENE_READ_LOCK(m_pxRigidActor->getScene());
            return PxMathConvert(m_pxRigidActor->getWorldBounds(1.0f));
        }
        return  AZ::Aabb::CreateNull();
    }

    AZ::EntityId RigidBody::GetEntityId() const
    {
        return m_actorUserData.GetEntityId();
    }

    AzPhysics::SceneQueryHit RigidBody::RayCast(const AzPhysics::RayCastRequest& request)
    {
        return PhysX::SceneQueryHelpers::ClosestRayHitAgainstShapes(request, m_shapes, GetTransform());
    }

    // Physics::ReferenceBase
    AZ::Crc32 RigidBody::GetNativeType() const
    {
        return PhysX::NativeTypeIdentifiers::RigidBody;
    }

    void* RigidBody::GetNativePointer() const
    {
        return m_pxRigidActor.get();
    }

    // Not in API but needed to support PhysicsComponentBus
    float RigidBody::GetLinearDamping() const
    {
        if (m_pxRigidActor)
        {
            PHYSX_SCENE_READ_LOCK(m_pxRigidActor->getScene());
            return m_pxRigidActor->getLinearDamping();
        }
        return 0.0f;
    }

    void RigidBody::SetLinearDamping(float damping)
    {
        if (damping < 0.0f)
        {
            AZ_Warning("PhysX Rigid Body", false, "Negative linear damping value (%6.4e). Name: %s", damping, GetName().c_str());
            return;
        }
        if (m_pxRigidActor)
        {
            PHYSX_SCENE_WRITE_LOCK(m_pxRigidActor->getScene());
            m_pxRigidActor->setLinearDamping(damping);
        }
    }

    float RigidBody::GetAngularDamping() const
    {
        if (m_pxRigidActor)
        {
            PHYSX_SCENE_READ_LOCK(m_pxRigidActor->getScene());
            return m_pxRigidActor->getAngularDamping();
        }
        return 0.0f;
    }

    void RigidBody::SetAngularDamping(float damping)
    {
        if (damping < 0.0f)
        {
            AZ_Warning("PhysX Rigid Body", false, "Negative angular damping value (%6.4e). Name: %s", damping, GetName().c_str());
            return;
        }

        if (m_pxRigidActor)
        {
            PHYSX_SCENE_WRITE_LOCK(m_pxRigidActor->getScene());
            m_pxRigidActor->setAngularDamping(damping);
        }
    }

    bool RigidBody::IsAwake() const
    {
        if (m_pxRigidActor)
        {
            PHYSX_SCENE_READ_LOCK(m_pxRigidActor->getScene());
            return !m_pxRigidActor->isSleeping();
        }
        return false;
    }

    void RigidBody::ForceAsleep()
    {
        if (m_pxRigidActor) //<- Rigid body must be in a scene, otherwise putToSleep will crash
        {
            physx::PxScene* scene = m_pxRigidActor->getScene();
            if (scene)
            {
                PHYSX_SCENE_WRITE_LOCK(scene);
                m_pxRigidActor->putToSleep();
            }
        }
    }

    void RigidBody::ForceAwake()
    {
        if (m_pxRigidActor) //<- Rigid body must be in a scene, otherwise wakeUp will crash
        {
            physx::PxScene* scene = m_pxRigidActor->getScene();
            if (scene)
            {
                PHYSX_SCENE_WRITE_LOCK(scene);
                m_pxRigidActor->wakeUp();
            }
        }
    }

    float RigidBody::GetSleepThreshold() const
    {
        if (m_pxRigidActor)
        {
            PHYSX_SCENE_READ_LOCK(m_pxRigidActor->getScene());
            return m_pxRigidActor->getSleepThreshold();
        }
        return 0.0f;
    }

    void RigidBody::SetSleepThreshold(float threshold)
    {
        if (threshold < 0.0f)
        {
            AZ_Warning("PhysX Rigid Body", false, "Negative sleep threshold value (%6.4e). Name: %s", threshold, GetName().c_str());
            return;
        }

        if (m_pxRigidActor)
        {
            PHYSX_SCENE_WRITE_LOCK(m_pxRigidActor->getScene());
            m_pxRigidActor->setSleepThreshold(threshold);
        }
    }

    void RigidBody::SetName(const AZStd::string& entityName)
    {
        m_name = entityName;

        if (m_pxRigidActor)
        {
            PHYSX_SCENE_WRITE_LOCK(m_pxRigidActor->getScene());
            m_pxRigidActor->setName(m_name.c_str());
        }
    }

    const AZStd::string& RigidBody::GetName() const
    {
        return m_name;
    }

    void RigidBody::SetZeroCenterOfMass()
    {
        if (m_pxRigidActor)
        {
            PHYSX_SCENE_WRITE_LOCK(m_pxRigidActor->getScene());
            m_pxRigidActor->setCMassLocalPose(physx::PxTransform(PxMathConvert(AZ::Vector3::CreateZero())));
        }
    }
}
