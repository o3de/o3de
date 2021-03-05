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

#include <AzFramework/Physics/RigidBody.h>
#include <AzFramework/Physics/ClassConverters.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace Physics
{
    float DefaultRigidBodyConfiguration::m_mass = 1.f;
    bool  DefaultRigidBodyConfiguration::m_computeInertiaTensor = false;
    float DefaultRigidBodyConfiguration::m_linearDamping = 0.05f;
    float DefaultRigidBodyConfiguration::m_angularDamping = 0.15f;
    float DefaultRigidBodyConfiguration::m_sleepMinEnergy = 0.5f;
    float DefaultRigidBodyConfiguration::m_maxAngularVelocity = 100.0f;

    void RigidBodyConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<RigidBodyConfiguration, WorldBodyConfiguration>()
                ->Version(3, &ClassConverters::RigidBodyVersionConverter)
                ->Field("Initial linear velocity", &Physics::RigidBodyConfiguration::m_initialLinearVelocity)
                ->Field("Initial angular velocity", &Physics::RigidBodyConfiguration::m_initialAngularVelocity)
                ->Field("Linear damping", &Physics::RigidBodyConfiguration::m_linearDamping)
                ->Field("Angular damping", &Physics::RigidBodyConfiguration::m_angularDamping)
                ->Field("Sleep threshold", &Physics::RigidBodyConfiguration::m_sleepMinEnergy)
                ->Field("Start Asleep", &Physics::RigidBodyConfiguration::m_startAsleep)
                ->Field("Interpolate Motion", &Physics::RigidBodyConfiguration::m_interpolateMotion)
                ->Field("Gravity Enabled", &Physics::RigidBodyConfiguration::m_gravityEnabled)
                ->Field("Simulated", &Physics::RigidBodyConfiguration::m_simulated)
                ->Field("Kinematic", &Physics::RigidBodyConfiguration::m_kinematic)
                ->Field("CCD Enabled", &Physics::RigidBodyConfiguration::m_ccdEnabled)
                ->Field("Compute Mass", &Physics::RigidBodyConfiguration::m_computeMass)
                ->Field("Mass", &Physics::RigidBodyConfiguration::m_mass)
                ->Field("Compute COM", &Physics::RigidBodyConfiguration::m_computeCenterOfMass)
                ->Field("Centre of mass offset", &RigidBodyConfiguration::m_centerOfMassOffset)
                ->Field("Compute inertia", &RigidBodyConfiguration::m_computeInertiaTensor)
                ->Field("Inertia tensor", &RigidBodyConfiguration::m_inertiaTensor)
                ->Field("Property Visibility Flags", &RigidBodyConfiguration::m_propertyVisibilityFlags)
                ->Field("Maximum Angular Velocity", &RigidBodyConfiguration::m_maxAngularVelocity)
                ->Field("Include All Shapes In Mass", &RigidBodyConfiguration::m_includeAllShapesInMassCalculation)
                ->Field("CCD Min Advance", &RigidBodyConfiguration::m_ccdMinAdvanceCoefficient)
                ->Field("CCD Friction", &RigidBodyConfiguration::m_ccdFrictionEnabled)
            ;
        }
    }

    AZ::Crc32 RigidBodyConfiguration::GetPropertyVisibility(PropertyVisibility property) const
    {
        return (m_propertyVisibilityFlags & property) != 0 ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    void RigidBodyConfiguration::SetPropertyVisibility(PropertyVisibility property, bool isVisible)
    {
        if (isVisible)
        {
            m_propertyVisibilityFlags |= property;
        }
        else
        {
            m_propertyVisibilityFlags &= ~property;
        }
    }

    AZ::Crc32 RigidBodyConfiguration::GetInitialVelocitiesVisibility() const
    {
        return GetPropertyVisibility(PropertyVisibility::InitialVelocities);
    }

    AZ::Crc32 RigidBodyConfiguration::GetInertiaSettingsVisibility() const
    {
        return GetPropertyVisibility(PropertyVisibility::InertiaProperties);
    }

    AZ::Crc32 RigidBodyConfiguration::GetInertiaVisibility() const
    {
        bool visible = ((m_propertyVisibilityFlags & PropertyVisibility::InertiaProperties) != 0) && !m_computeInertiaTensor;
        return visible ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    AZ::Crc32 RigidBodyConfiguration::GetCoMVisibility() const
    {
        bool visible = ((m_propertyVisibilityFlags & PropertyVisibility::InertiaProperties) != 0) && !m_computeCenterOfMass;
        return visible ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    AZ::Crc32 RigidBodyConfiguration::GetMassVisibility() const
    {
        bool visible = ((m_propertyVisibilityFlags & PropertyVisibility::InertiaProperties) != 0) && !m_computeMass;
        return visible ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    AZ::Crc32 RigidBodyConfiguration::GetDampingVisibility() const
    {
        return GetPropertyVisibility(PropertyVisibility::Damping);
    }

    AZ::Crc32 RigidBodyConfiguration::GetSleepOptionsVisibility() const
    {
        return GetPropertyVisibility(PropertyVisibility::SleepOptions);
    }

    AZ::Crc32 RigidBodyConfiguration::GetInterpolationVisibility() const
    {
        return GetPropertyVisibility(PropertyVisibility::Interpolation);
    }

    AZ::Crc32 RigidBodyConfiguration::GetGravityVisibility() const
    {
        return GetPropertyVisibility(PropertyVisibility::Gravity);
    }

    AZ::Crc32 RigidBodyConfiguration::GetKinematicVisibility() const
    {
        return GetPropertyVisibility(PropertyVisibility::Kinematic);
    }

    AZ::Crc32 RigidBodyConfiguration::GetCCDVisibility() const
    {
        return GetPropertyVisibility(PropertyVisibility::ContinuousCollisionDetection);
    }

    AZ::Crc32 RigidBodyConfiguration::GetMaxVelocitiesVisibility() const
    {
        return GetPropertyVisibility(PropertyVisibility::MaxVelocities);
    }

    Physics::MassComputeFlags RigidBodyConfiguration::GetMassComputeFlags() const
    {
        using Physics::MassComputeFlags;

        MassComputeFlags flags = MassComputeFlags::NONE;

        if (m_computeCenterOfMass)
        {
            flags = flags | MassComputeFlags::COMPUTE_COM;
        }

        if (m_computeInertiaTensor)
        {
            flags = flags | MassComputeFlags::COMPUTE_INERTIA;
        }

        if (m_computeMass)
        {
            flags = flags | MassComputeFlags::COMPUTE_MASS;
        }

        if (m_includeAllShapesInMassCalculation)
        {
            flags = flags | MassComputeFlags::INCLUDE_ALL_SHAPES;
        }

        return flags;
    }

    void RigidBodyConfiguration::SetMassComputeFlags(MassComputeFlags flags)
    {
        using Physics::MassComputeFlags;
        m_computeCenterOfMass = MassComputeFlags::COMPUTE_COM == (flags & MassComputeFlags::COMPUTE_COM);
        m_computeInertiaTensor = MassComputeFlags::COMPUTE_INERTIA == (flags & MassComputeFlags::COMPUTE_INERTIA);
        m_computeMass = MassComputeFlags::COMPUTE_MASS == (flags & MassComputeFlags::COMPUTE_MASS);
        m_includeAllShapesInMassCalculation =
            MassComputeFlags::INCLUDE_ALL_SHAPES == (flags & MassComputeFlags::INCLUDE_ALL_SHAPES);
    }

    bool RigidBodyConfiguration::IsCCDEnabled() const
    {
        return m_ccdEnabled;
    }

    RigidBody::RigidBody(const RigidBodyConfiguration& settings)
        : WorldBody(settings)
    {
    }
} // namespace Physics
