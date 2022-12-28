/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/RigidBody.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/utility/as_const.h>
#include <AzCore/Math/MathStringConversions.h>
#include <AzFramework/Physics/Utils.h>
#include <AzFramework/Physics/Configuration/RigidBodyConfiguration.h>
#include <PhysX/NativeTypeIdentifiers.h>
#include <PhysX/MathConversion.h>
#include <Source/Utils.h>
#include <PhysX/Utils.h>
#include <Source/Shape.h>
#include <extensions/PxRigidBodyExt.h>
#include <PxPhysicsAPI.h>
#include <PhysX/PhysXLocks.h>
#include <PhysX/Material/PhysXMaterial.h>
#include <Common/PhysXSceneQueryHelpers.h>

namespace PhysX
{
    namespace
    {
        const AZ::Vector3 DefaultCenterOfMass = AZ::Vector3::CreateZero();
        const float DefaultMass = 1.0f;
        const AZ::Matrix3x3 DefaultInertiaTensor = AZ::Matrix3x3::CreateIdentity();

        bool IsSimulationShape(const physx::PxShape& pxShape)
        {
            return (pxShape.getFlags() & physx::PxShapeFlag::eSIMULATION_SHAPE);
        }

        bool CanShapeComputeMassProperties(const physx::PxShape& pxShape)
        {
            // Note: List based on computeMassAndInertia function in ExtRigidBodyExt.cpp file in PhysX.
            const physx::PxGeometryType::Enum geometryType = pxShape.getGeometryType();
            return geometryType == physx::PxGeometryType::eSPHERE
                || geometryType == physx::PxGeometryType::eBOX
                || geometryType == physx::PxGeometryType::eCAPSULE
                || geometryType == physx::PxGeometryType::eCONVEXMESH;
        }
    }

    void RigidBodyConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PhysX::RigidBodyConfiguration>()
                ->Version(1)
                ->Field("SolverPositionIterations", &PhysX::RigidBodyConfiguration::m_solverPositionIterations)
                ->Field("SolverVelocityIterations", &PhysX::RigidBodyConfiguration::m_solverVelocityIterations)
                ;

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<PhysX::RigidBodyConfiguration>("PhysX-specific Rigid Body Configuration",
                    "Additional Rigid Body settings specific to PhysX.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &PhysX::RigidBodyConfiguration::m_solverPositionIterations,
                        "Solver Position Iterations",
                        "Higher values can improve stability at the cost of performance.")
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->Attribute(AZ::Edit::Attributes::Max, 255)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &PhysX::RigidBodyConfiguration::m_solverVelocityIterations,
                        "Solver Velocity Iterations",
                        "Higher values can improve stability at the cost of performance.")
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->Attribute(AZ::Edit::Attributes::Max, 255)
                    ;
            }
        }
    }

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

    void RigidBody::UpdateMassProperties(AzPhysics::MassComputeFlags flags, const AZ::Vector3& centerOfMassOffsetOverride, const AZ::Matrix3x3& inertiaTensorOverride, const float massOverride)
    {
        const bool computeCenterOfMass = AzPhysics::MassComputeFlags::COMPUTE_COM == (flags & AzPhysics::MassComputeFlags::COMPUTE_COM);
        const bool computeInertiaTensor = AzPhysics::MassComputeFlags::COMPUTE_INERTIA == (flags & AzPhysics::MassComputeFlags::COMPUTE_INERTIA);
        const bool computeMass = AzPhysics::MassComputeFlags::COMPUTE_MASS == (flags & AzPhysics::MassComputeFlags::COMPUTE_MASS);
        const bool needsCompute = computeCenterOfMass || computeInertiaTensor || computeMass;
        const bool includeAllShapesInMassCalculation = AzPhysics::MassComputeFlags::INCLUDE_ALL_SHAPES == (flags & AzPhysics::MassComputeFlags::INCLUDE_ALL_SHAPES);

        // Basic case where all properties are set directly.
        if (!needsCompute)
        {
            SetCenterOfMassOffset(centerOfMassOffsetOverride);
            SetMass(massOverride);
            SetInertia(inertiaTensorOverride);
            return;
        }

        // If there are no shapes then set the properties directly without computing anything.
        if (m_shapes.empty())
        {
            SetCenterOfMassOffset(computeCenterOfMass ? DefaultCenterOfMass  : centerOfMassOffsetOverride);
            SetMass(computeMass ? DefaultMass : massOverride);
            SetInertia(computeInertiaTensor ? DefaultInertiaTensor : inertiaTensorOverride);
            return;
        }

        auto cannotComputeMassProperties = [this, includeAllShapesInMassCalculation]
        {
            PHYSX_SCENE_READ_LOCK(m_pxRigidActor->getScene());
            return AZStd::any_of(m_shapes.cbegin(), m_shapes.cend(),
                [includeAllShapesInMassCalculation](const AZStd::shared_ptr<PhysX::Shape>& shape)
                {
                    const physx::PxShape& pxShape = *shape->GetPxShape();
                    const bool includeShape = includeAllShapesInMassCalculation || IsSimulationShape(pxShape);

                    return includeShape && !CanShapeComputeMassProperties(pxShape);
                });
        };

        // If contains shapes that cannot compute mass properties (triangle mesh,
        // plane or heightfield) then default values will be used.
        if (cannotComputeMassProperties())
        {
            AZ_Warning("RigidBody", !computeCenterOfMass,
                "Rigid body '%s' cannot compute COM because it contains triangle mesh, plane or heightfield shapes, it will default to %s.",
                GetName().c_str(), AZStd::to_string(DefaultCenterOfMass).c_str());
            AZ_Warning("RigidBody", !computeMass,
                "Rigid body '%s' cannot compute Mass because it contains triangle mesh, plane or heightfield shapes, it will default to %0.1f.",
                GetName().c_str(), DefaultMass);
            AZ_Warning("RigidBody", !computeInertiaTensor,
                "Rigid body '%s' cannot compute Inertia because it contains triangle mesh, plane or heightfield shapes, it will default to %s.",
                GetName().c_str(), AZStd::to_string(DefaultInertiaTensor.RetrieveScale()).c_str());

            SetCenterOfMassOffset(computeCenterOfMass ? DefaultCenterOfMass : centerOfMassOffsetOverride);
            SetMass(computeMass ? DefaultMass : massOverride);
            SetInertia(computeInertiaTensor ? DefaultInertiaTensor : inertiaTensorOverride);
            return;
        }

        // Center of mass needs to be considered first since
        // it's needed when computing mass and inertia.
        if (computeCenterOfMass)
        {
            // Compute Center of Mass
            UpdateCenterOfMass(includeAllShapesInMassCalculation);
        }
        else
        {
            SetCenterOfMassOffset(centerOfMassOffsetOverride);
        }
        const physx::PxVec3 pxCenterOfMass = PxMathConvert(GetCenterOfMassLocal());

        if (computeMass)
        {
            // Gather material densities from all shapes,
            // mass computation is based on them.
            AZStd::vector<float> densities;
            densities.reserve(m_shapes.size());
            for (const auto& shape : m_shapes)
            {
                const auto& physxMaterials = shape->GetPhysXMaterials();
                AZ_Assert(!physxMaterials.empty(), "Shape with no materials");
                densities.emplace_back(physxMaterials[0]->GetDensity());
            }

            // Compute Mass + Inertia
            {
                PHYSX_SCENE_WRITE_LOCK(m_pxRigidActor->getScene());
                physx::PxRigidBodyExt::updateMassAndInertia(*m_pxRigidActor,
                    densities.data(), static_cast<AZ::u32>(densities.size()),
                    &pxCenterOfMass, includeAllShapesInMassCalculation);
            }

            // There is no physx function to only compute the mass without
            // computing the inertia. So now that both have been computed
            // we can override the inertia if it's suppose to use a
            // specific value set by the user.
            if (!computeInertiaTensor)
            {
                SetInertia(inertiaTensorOverride);
            }
        }
        else
        {
            if (computeInertiaTensor)
            {
                // Set Mass + Compute Inertia
                PHYSX_SCENE_WRITE_LOCK(m_pxRigidActor->getScene());
                physx::PxRigidBodyExt::setMassAndUpdateInertia(*m_pxRigidActor, massOverride, 
                    &pxCenterOfMass, includeAllShapesInMassCalculation);
            }
            else
            {
                SetMass(massOverride);
                SetInertia(inertiaTensorOverride);
            }
        }
    }

    AZ::u32 RigidBody::GetShapeCount() const 
    {
        return static_cast<AZ::u32>(m_shapes.size());
    }

    AZStd::shared_ptr<Physics::Shape> RigidBody::GetShape(AZ::u32 index)
    {
        AZStd::shared_ptr<const Physics::Shape> constShape = AZStd::as_const(*this).GetShape(index);
        return AZStd::const_pointer_cast<Physics::Shape>(constShape);
    }

    AZStd::shared_ptr<const Physics::Shape> RigidBody::GetShape(AZ::u32 index) const
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

    AZ::Matrix3x3 RigidBody::GetInertiaWorld() const
    {
        if (m_pxRigidActor)
        {
            PHYSX_SCENE_READ_LOCK(m_pxRigidActor->getScene());
            AZ::Vector3 inertiaDiagonal = PxMathConvert(m_pxRigidActor->getMassSpaceInertiaTensor());
            AZ::Matrix3x3 rotationToWorld = AZ::Matrix3x3::CreateFromQuaternion(PxMathConvert(m_pxRigidActor->getGlobalPose().q.getConjugate()));
            return Physics::Utils::DiagonalMatrixLocalToWorld(inertiaDiagonal, rotationToWorld);
        }

        return AZ::Matrix3x3::CreateZero();
    }

    AZ::Matrix3x3 RigidBody::GetInertiaLocal() const
    {
        if (m_pxRigidActor)
        {
            PHYSX_SCENE_READ_LOCK(m_pxRigidActor->getScene());
            physx::PxVec3 inertiaDiagonal = m_pxRigidActor->getMassSpaceInertiaTensor();
            return AZ::Matrix3x3::CreateDiagonal(PxMathConvert(inertiaDiagonal));
        }

        return AZ::Matrix3x3::CreateZero();
    }

    AZ::Matrix3x3 RigidBody::GetInverseInertiaWorld() const
    {
        if (m_pxRigidActor)
        {
            PHYSX_SCENE_READ_LOCK(m_pxRigidActor->getScene());
            AZ::Vector3 inverseInertiaDiagonal = PxMathConvert(m_pxRigidActor->getMassSpaceInvInertiaTensor());
            AZ::Matrix3x3 rotationToWorld = AZ::Matrix3x3::CreateFromQuaternion(PxMathConvert(m_pxRigidActor->getGlobalPose().q.getConjugate()));
            return Physics::Utils::DiagonalMatrixLocalToWorld(inverseInertiaDiagonal, rotationToWorld);
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

    void RigidBody::UpdateCenterOfMass(bool includeAllShapesInMassCalculation)
    {
        if (m_shapes.empty())
        {
            SetCenterOfMassOffset(DefaultCenterOfMass);
            return;
        }

        AZStd::vector<const physx::PxShape*> pxShapes;
        pxShapes.reserve(m_shapes.size());
        {
            // Filter shapes in the same way that updateMassAndInertia function does.
            PHYSX_SCENE_READ_LOCK(m_pxRigidActor->getScene());
            for (const auto& shape : m_shapes)
            {
                const physx::PxShape& pxShape = *shape->GetPxShape();
                const bool includeShape = includeAllShapesInMassCalculation || IsSimulationShape(pxShape);

                if (includeShape && CanShapeComputeMassProperties(pxShape))
                {
                    pxShapes.emplace_back(&pxShape);
                }
            }
        }

        if (pxShapes.empty())
        {
            SetCenterOfMassOffset(DefaultCenterOfMass);
            return;
        }

        const physx::PxMassProperties pxMassProperties = [this, &pxShapes]
        {
            // Note: PhysX computeMassPropertiesFromShapes function does not use densities
            //       to compute the shape's masses, which are needed to calculate the center of mass.
            //       This differs from updateMassAndInertia function, which uses material density values.
            //       So the masses used during center of mass calculation do not match the masses
            //       used during mass/inertia calculation. This is an inconsistency in PhysX.
            PHYSX_SCENE_READ_LOCK(m_pxRigidActor->getScene());
            return physx::PxRigidBodyExt::computeMassPropertiesFromShapes(pxShapes.data(), static_cast<physx::PxU32>(pxShapes.size()));
        }();

        SetCenterOfMassOffset(PxMathConvert(pxMassProperties.centerOfMass));
    }

    void RigidBody::SetInertia(const AZ::Matrix3x3& inertia)
    {
        if (m_pxRigidActor)
        {
            PHYSX_SCENE_WRITE_LOCK(m_pxRigidActor->getScene());
            m_pxRigidActor->setMassSpaceInertiaTensor(PxMathConvert(inertia.RetrieveScale()));
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
            if (!isKinematic)
            {
                PHYSX_SCENE_READ_LOCK(m_pxRigidActor->getScene());

                // check if any of the shapes on the rigid body would prevent switching to dynamic
                const bool allShapesCanComputeMassProperties = AZStd::all_of(
                    m_shapes.begin(),
                    m_shapes.end(),
                    [](const AZStd::shared_ptr<PhysX::Shape>& shape)
                    {
                        return CanShapeComputeMassProperties(*shape->GetPxShape());
                    });
                if (!allShapesCanComputeMassProperties)
                {
                    AZ_Warning(
                        "PhysX Rigid Body",
                        false,
                        "Cannot set kinematic to false, because body has triangle mesh, plane or heightfield shapes attached. Name: %s",
                        GetName().c_str());
                    return;
                }
            }

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
}
