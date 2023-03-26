/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/Articulation/ArticulationLinkConfiguration.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Physics/Shape.h>
#include <Source/EditorRigidBodyComponent.h>
#include <Source/EditorStaticRigidBodyComponent.h>

namespace PhysX
{
    namespace Internal
    {
        // Visibility functions.
        AZ::Crc32 GetPropertyVisibility(AZ::u16 flags, ArticulationLinkConfiguration::PropertyVisibility property)
        {
            return (flags & property) != 0 ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
        }

        bool RigidBodyVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() <= 1)
            {
                const int elementIndex = classElement.FindElement(AZ_CRC_CE("Centre of mass offset"));

                if (elementIndex >= 0)
                {
                    AZ::Vector3 existingCenterOfMassOffset;
                    AZ::SerializeContext::DataElementNode& centerOfMassElement = classElement.GetSubElement(elementIndex);
                    const bool found = centerOfMassElement.GetData<AZ::Vector3>(existingCenterOfMassOffset);

                    if (found && !existingCenterOfMassOffset.IsZero())
                    {
                        // An existing center of mass (COM) offset value was specified for this rigid body.
                        // Version 2 includes a new m_computeCenterOfMass boolean flag to specify the automatic calculation of COM.
                        // In this case set m_computeCenterOfMass to false so that the existing center of mass offset value is utilized
                        // correctly.
                        const int idx = classElement.AddElement<bool>(context, "Compute COM");
                        if (idx != -1)
                        {
                            if (!classElement.GetSubElement(idx).SetData<bool>(context, false))
                            {
                                return false;
                            }
                        }
                    }
                }
            }

            if (classElement.GetVersion() <= 2)
            {
                const int elementIndex = classElement.FindElement(AZ_CRC_CE("Mass"));

                if (elementIndex >= 0)
                {
                    float existingMass = 0;
                    AZ::SerializeContext::DataElementNode& massElement = classElement.GetSubElement(elementIndex);
                    const bool found = massElement.GetData<float>(existingMass);

                    if (found && existingMass > 0)
                    {
                        // Keeping the existing mass and disabling auto-compute of the mass for this rigid body.
                        // Version 3 includes a new m_computeMass boolean flag to specify the automatic calculation of mass.
                        const int idx = classElement.AddElement<bool>(context, "Compute Mass");
                        if (idx != -1)
                        {
                            if (!classElement.GetSubElement(idx).SetData<bool>(context, false))
                            {
                                return false;
                            }
                        }
                    }
                }
            }

            if (classElement.GetVersion() <= 3)
            {
                classElement.RemoveElementByName(AZ_CRC_CE("Property Visibility Flags"));
            }

            if (classElement.GetVersion() <= 4)
            {
                classElement.RemoveElementByName(AZ_CRC_CE("Simulated"));
            }

            return true;
        }
    } // namespace Internal

    const float ArticulationLinkConfiguration::s_breakageMax = 10000000.0f;
    const float ArticulationLinkConfiguration::s_breakageMin = 0.01f;

    AZ_CLASS_ALLOCATOR_IMPL(ArticulationLinkConfiguration, AZ::SystemAllocator);

    void ArticulationLinkConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ArticulationLinkConfiguration>()
                ->Version(1)
                ->Field("Kinematic", &ArticulationLinkConfiguration::m_kinematic)
                ->Field("Initial linear velocity", &ArticulationLinkConfiguration::m_initialLinearVelocity)
                ->Field("Initial angular velocity", &ArticulationLinkConfiguration::m_initialAngularVelocity)
                ->Field("Linear damping", &ArticulationLinkConfiguration::m_linearDamping)
                ->Field("Angular damping", &ArticulationLinkConfiguration::m_angularDamping)
                ->Field("Sleep threshold", &ArticulationLinkConfiguration::m_sleepMinEnergy)
                ->Field("Start Asleep", &ArticulationLinkConfiguration::m_startAsleep)
                ->Field("Interpolate Motion", &ArticulationLinkConfiguration::m_interpolateMotion)
                ->Field("Gravity Enabled", &ArticulationLinkConfiguration::m_gravityEnabled)
                ->Field("CCD Enabled", &ArticulationLinkConfiguration::m_ccdEnabled)
                ->Field("Compute Mass", &ArticulationLinkConfiguration::m_computeMass)
                ->Field("Lock Linear X", &ArticulationLinkConfiguration::m_lockLinearX)
                ->Field("Lock Linear Y", &ArticulationLinkConfiguration::m_lockLinearY)
                ->Field("Lock Linear Z", &ArticulationLinkConfiguration::m_lockLinearZ)
                ->Field("Lock Angular X", &ArticulationLinkConfiguration::m_lockAngularX)
                ->Field("Lock Angular Y", &ArticulationLinkConfiguration::m_lockAngularY)
                ->Field("Lock Angular Z", &ArticulationLinkConfiguration::m_lockAngularZ)
                ->Field("Mass", &ArticulationLinkConfiguration::m_mass)
                ->Field("Compute COM", &ArticulationLinkConfiguration::m_computeCenterOfMass)
                ->Field("Centre of mass offset", &ArticulationLinkConfiguration::m_centerOfMassOffset)
                ->Field("Compute inertia", &ArticulationLinkConfiguration::m_computeInertiaTensor)
                ->Field("Inertia tensor", &ArticulationLinkConfiguration::m_inertiaTensor)
                ->Field("Maximum Angular Velocity", &ArticulationLinkConfiguration::m_maxAngularVelocity)
                ->Field("Include All Shapes In Mass", &ArticulationLinkConfiguration::m_includeAllShapesInMassCalculation)
                ->Field("CCD Min Advance", &ArticulationLinkConfiguration::m_ccdMinAdvanceCoefficient)
                ->Field("CCD Friction", &ArticulationLinkConfiguration::m_ccdFrictionEnabled)
                ->Field("Open PhysX Configuration", &ArticulationLinkConfiguration::m_configButton)
                ->Field("Local Position", &ArticulationLinkConfiguration::m_localPosition)
                ->Field("Local Rotation", &ArticulationLinkConfiguration::m_localRotation)
                ->Field("Parent Entity", &ArticulationLinkConfiguration::m_leadEntity)
                ->Field("Child Entity", &ArticulationLinkConfiguration::m_followerEntity)
                ->Field("Breakable", &ArticulationLinkConfiguration::m_breakable)
                ->Field("Maximum Force", &ArticulationLinkConfiguration::m_forceMax)
                ->Field("Maximum Torque", &ArticulationLinkConfiguration::m_torqueMax)
                ->Field("Display Debug", &ArticulationLinkConfiguration::m_displayJointSetup)
                ->Field("Select Lead on Snap", &ArticulationLinkConfiguration::m_selectLeadOnSnap)
                ->Field("Self Collide", &ArticulationLinkConfiguration::m_selfCollide)
                ->Field("SolverPositionIterations", &ArticulationLinkConfiguration::m_solverPositionIterations)
                ->Field("SolverVelocityIterations", &ArticulationLinkConfiguration::m_solverVelocityIterations)
                ->Field("ArticulationJointType", &ArticulationLinkConfiguration::m_articulationJointType)
                ->Field("Root Articulation", &ArticulationLinkConfiguration::m_isRootArticulation);
        }
    }

    AzPhysics::MassComputeFlags ArticulationLinkConfiguration::GetMassComputeFlags() const
    {
        AzPhysics::MassComputeFlags flags = AzPhysics::MassComputeFlags::NONE;

        if (m_computeCenterOfMass)
        {
            flags = flags | AzPhysics::MassComputeFlags::COMPUTE_COM;
        }

        if (m_computeInertiaTensor)
        {
            flags = flags | AzPhysics::MassComputeFlags::COMPUTE_INERTIA;
        }

        if (m_computeMass)
        {
            flags = flags | AzPhysics::MassComputeFlags::COMPUTE_MASS;
        }

        if (m_includeAllShapesInMassCalculation)
        {
            flags = flags | AzPhysics::MassComputeFlags::INCLUDE_ALL_SHAPES;
        }

        return flags;
    }

    void ArticulationLinkConfiguration::SetMassComputeFlags(AzPhysics::MassComputeFlags flags)
    {
        m_computeCenterOfMass = AzPhysics::MassComputeFlags::COMPUTE_COM == (flags & AzPhysics::MassComputeFlags::COMPUTE_COM);
        m_computeInertiaTensor = AzPhysics::MassComputeFlags::COMPUTE_INERTIA == (flags & AzPhysics::MassComputeFlags::COMPUTE_INERTIA);
        m_computeMass = AzPhysics::MassComputeFlags::COMPUTE_MASS == (flags & AzPhysics::MassComputeFlags::COMPUTE_MASS);
        m_includeAllShapesInMassCalculation =
            AzPhysics::MassComputeFlags::INCLUDE_ALL_SHAPES == (flags & AzPhysics::MassComputeFlags::INCLUDE_ALL_SHAPES);
    }

    bool ArticulationLinkConfiguration::IsCcdEnabled() const
    {
        return m_ccdEnabled;
    }

    bool ArticulationLinkConfiguration::CcdReadOnly() const
    {
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            return !physicsSystem->GetDefaultSceneConfiguration().m_enableCcd || m_kinematic;
        }
        return m_kinematic;
    }

    AZ::Crc32 ArticulationLinkConfiguration::GetInitialVelocitiesVisibility() const
    {
        return Internal::GetPropertyVisibility(
            m_propertyVisibilityFlags, ArticulationLinkConfiguration::PropertyVisibility::InitialVelocities);
    }

    AZ::Crc32 ArticulationLinkConfiguration::GetInertiaSettingsVisibility() const
    {
        return Internal::GetPropertyVisibility(
            m_propertyVisibilityFlags, ArticulationLinkConfiguration::PropertyVisibility::InertiaProperties);
    }

    AZ::Crc32 ArticulationLinkConfiguration::GetInertiaVisibility() const
    {
        bool visible = ((m_propertyVisibilityFlags & ArticulationLinkConfiguration::PropertyVisibility::InertiaProperties) != 0) &&
            !m_computeInertiaTensor;
        return visible ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    AZ::Crc32 ArticulationLinkConfiguration::GetCoMVisibility() const
    {
        bool visible = ((m_propertyVisibilityFlags & ArticulationLinkConfiguration::PropertyVisibility::InertiaProperties) != 0) &&
            !m_computeCenterOfMass;
        return visible ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    AZ::Crc32 ArticulationLinkConfiguration::GetMassVisibility() const
    {
        bool visible =
            ((m_propertyVisibilityFlags & ArticulationLinkConfiguration::PropertyVisibility::InertiaProperties) != 0) && !m_computeMass;
        return visible ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    AZ::Crc32 ArticulationLinkConfiguration::GetDampingVisibility() const
    {
        return Internal::GetPropertyVisibility(m_propertyVisibilityFlags, ArticulationLinkConfiguration::PropertyVisibility::Damping);
    }

    AZ::Crc32 ArticulationLinkConfiguration::GetSleepOptionsVisibility() const
    {
        return Internal::GetPropertyVisibility(m_propertyVisibilityFlags, ArticulationLinkConfiguration::PropertyVisibility::SleepOptions);
    }

    AZ::Crc32 ArticulationLinkConfiguration::GetInterpolationVisibility() const
    {
        return Internal::GetPropertyVisibility(m_propertyVisibilityFlags, ArticulationLinkConfiguration::PropertyVisibility::Interpolation);
    }

    AZ::Crc32 ArticulationLinkConfiguration::GetGravityVisibility() const
    {
        return Internal::GetPropertyVisibility(m_propertyVisibilityFlags, ArticulationLinkConfiguration::PropertyVisibility::Gravity);
    }

    AZ::Crc32 ArticulationLinkConfiguration::GetKinematicVisibility() const
    {
        return Internal::GetPropertyVisibility(m_propertyVisibilityFlags, ArticulationLinkConfiguration::PropertyVisibility::Kinematic);
    }

    AZ::Crc32 ArticulationLinkConfiguration::GetCcdVisibility() const
    {
        return Internal::GetPropertyVisibility(
            m_propertyVisibilityFlags, ArticulationLinkConfiguration::PropertyVisibility::ContinuousCollisionDetection);
    }

    AZ::Crc32 ArticulationLinkConfiguration::GetMaxVelocitiesVisibility() const
    {
        return Internal::GetPropertyVisibility(m_propertyVisibilityFlags, ArticulationLinkConfiguration::PropertyVisibility::MaxVelocities);
    }

    JointGenericProperties ArticulationLinkConfiguration::ToGenericProperties() const
    {
        JointGenericProperties::GenericJointFlag flags = JointGenericProperties::GenericJointFlag::None;
        if (m_breakable)
        {
            flags |= JointGenericProperties::GenericJointFlag::Breakable;
        }
        if (m_selfCollide)
        {
            flags |= JointGenericProperties::GenericJointFlag::SelfCollide;
        }

        return JointGenericProperties(flags, m_forceMax, m_torqueMax);
    }

    JointComponentConfiguration ArticulationLinkConfiguration::ToGameTimeConfig() const
    {
        AZ::Vector3 localRotation(m_localRotation);

        return JointComponentConfiguration(
            AZ::Transform::CreateFromQuaternionAndTranslation(AZ::Quaternion::CreateFromEulerAnglesDegrees(localRotation), m_localPosition),
            m_leadEntity,
            m_followerEntity);
    }

    void ArticulationLinkConfiguration::SetIsRootArticulation(bool value)
    {
        m_isRootArticulation = value;
    }
} // namespace PhysX
