/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/Configuration/RigidBodyConfiguration.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/PhysicsSystem.h>

namespace AzPhysics
{
    namespace Internal
    {
        // Visibility functions.
        AZ::Crc32 GetPropertyVisibility(AZ::u16 flags, RigidBodyConfiguration::PropertyVisibility property)
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
                        // In this case set m_computeCenterOfMass to false so that the existing center of mass offset value is utilized correctly.
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
    }

    static constexpr AZStd::string_view KinematicDescription =
        "Determines how the movement/position of the rigid body is controlled.";

    static constexpr AZStd::string_view KinematicDescriptionReadOnly =
        "Determines how the movement/position of the rigid body is controlled. \n"
        "<b>The rigid body cannot be set as Kinematic if CCD is enabled, disable CCD to allow changes to be made.</b>";

    static constexpr AZStd::string_view CcdDescription =
        "When active, the rigid body has continuous collision detection (CCD). Use this to ensure accurate "
        "collision detection, particularly for fast moving rigid bodies. CCD must be activated in the global PhysX "
        "configuration.";

    static constexpr AZStd::string_view CcdDescriptionReadOnly =
        "When active, the rigid body has continuous collision detection (CCD). Use this to ensure accurate "
        "collision detection, particularly for fast moving rigid bodies. CCD must be activated in the global PhysX "
        "configuration.\n <b>CCD cannot be enabled if the rigid body is Kinematic, set the rigid body as Dynamic "
        "to allow changes to be made.</b>";

    AZ_CLASS_ALLOCATOR_IMPL(RigidBodyConfiguration, AZ::SystemAllocator);

    void RigidBodyConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<RigidBodyConfiguration, AzPhysics::SimulatedBodyConfiguration>()
                ->Version(5, &Internal::RigidBodyVersionConverter)
                ->Field("Kinematic", &RigidBodyConfiguration::m_kinematic)
                ->Field("Initial linear velocity", &RigidBodyConfiguration::m_initialLinearVelocity)
                ->Field("Initial angular velocity", &RigidBodyConfiguration::m_initialAngularVelocity)
                ->Field("Linear damping", &RigidBodyConfiguration::m_linearDamping)
                ->Field("Angular damping", &RigidBodyConfiguration::m_angularDamping)
                ->Field("Sleep threshold", &RigidBodyConfiguration::m_sleepMinEnergy)
                ->Field("Start Asleep", &RigidBodyConfiguration::m_startAsleep)
                ->Field("Interpolate Motion", &RigidBodyConfiguration::m_interpolateMotion)
                ->Field("Gravity Enabled", &RigidBodyConfiguration::m_gravityEnabled)
                ->Field("CCD Enabled", &RigidBodyConfiguration::m_ccdEnabled)
                ->Field("Compute Mass", &RigidBodyConfiguration::m_computeMass)
                ->Field("Lock Linear X", &RigidBodyConfiguration::m_lockLinearX)
                ->Field("Lock Linear Y", &RigidBodyConfiguration::m_lockLinearY)
                ->Field("Lock Linear Z", &RigidBodyConfiguration::m_lockLinearZ)
                ->Field("Lock Angular X", &RigidBodyConfiguration::m_lockAngularX)
                ->Field("Lock Angular Y", &RigidBodyConfiguration::m_lockAngularY)
                ->Field("Lock Angular Z", &RigidBodyConfiguration::m_lockAngularZ)
                ->Field("Mass", &RigidBodyConfiguration::m_mass)
                ->Field("Compute COM", &RigidBodyConfiguration::m_computeCenterOfMass)
                ->Field("Centre of mass offset", &RigidBodyConfiguration::m_centerOfMassOffset)
                ->Field("Compute inertia", &RigidBodyConfiguration::m_computeInertiaTensor)
                ->Field("Inertia tensor", &RigidBodyConfiguration::m_inertiaTensor)
                ->Field("Maximum Angular Velocity", &RigidBodyConfiguration::m_maxAngularVelocity)
                ->Field("Include All Shapes In Mass", &RigidBodyConfiguration::m_includeAllShapesInMassCalculation)
                ->Field("CCD Min Advance", &RigidBodyConfiguration::m_ccdMinAdvanceCoefficient)
                ->Field("CCD Friction", &RigidBodyConfiguration::m_ccdFrictionEnabled)
                ->Field("Open PhysX Configuration", &RigidBodyConfiguration::m_configButton)
                ;
        }
    }

    MassComputeFlags RigidBodyConfiguration::GetMassComputeFlags() const
    {
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
        m_computeCenterOfMass = MassComputeFlags::COMPUTE_COM == (flags & MassComputeFlags::COMPUTE_COM);
        m_computeInertiaTensor = MassComputeFlags::COMPUTE_INERTIA == (flags & MassComputeFlags::COMPUTE_INERTIA);
        m_computeMass = MassComputeFlags::COMPUTE_MASS == (flags & MassComputeFlags::COMPUTE_MASS);
        m_includeAllShapesInMassCalculation =
            MassComputeFlags::INCLUDE_ALL_SHAPES == (flags & MassComputeFlags::INCLUDE_ALL_SHAPES);
    }

    bool RigidBodyConfiguration::IsCcdEnabled() const
    {
        return m_ccdEnabled;
    }

    bool RigidBodyConfiguration::CcdReadOnly() const
    {
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            return !physicsSystem->GetDefaultSceneConfiguration().m_enableCcd || m_kinematic;
        }
        return m_kinematic;
    }

    AZStd::string_view RigidBodyConfiguration::GetCcdTooltip() const
    {
        return m_kinematic ? CcdDescriptionReadOnly : CcdDescription;
    }

    AZStd::string_view RigidBodyConfiguration::GetKinematicTooltip() const
    {
        return m_ccdEnabled ? KinematicDescriptionReadOnly : KinematicDescription;
    }

    AZ::Crc32 RigidBodyConfiguration::GetInitialVelocitiesVisibility() const
    {
        return Internal::GetPropertyVisibility(m_propertyVisibilityFlags, RigidBodyConfiguration::PropertyVisibility::InitialVelocities);
    }

    AZ::Crc32 RigidBodyConfiguration::GetInertiaSettingsVisibility() const
    {
        return Internal::GetPropertyVisibility(m_propertyVisibilityFlags, RigidBodyConfiguration::PropertyVisibility::InertiaProperties);
    }

    AZ::Crc32 RigidBodyConfiguration::GetInertiaVisibility() const
    {
        bool visible = ((m_propertyVisibilityFlags & RigidBodyConfiguration::PropertyVisibility::InertiaProperties) != 0) && !m_computeInertiaTensor;
        return visible ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    AZ::Crc32 RigidBodyConfiguration::GetCoMVisibility() const
    {
        bool visible = ((m_propertyVisibilityFlags & RigidBodyConfiguration::PropertyVisibility::InertiaProperties) != 0) && !m_computeCenterOfMass;
        return visible ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    AZ::Crc32 RigidBodyConfiguration::GetMassVisibility() const
    {
        bool visible = ((m_propertyVisibilityFlags & RigidBodyConfiguration::PropertyVisibility::InertiaProperties) != 0) && !m_computeMass;
        return visible ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    AZ::Crc32 RigidBodyConfiguration::GetDampingVisibility() const
    {
        return Internal::GetPropertyVisibility(m_propertyVisibilityFlags, RigidBodyConfiguration::PropertyVisibility::Damping);
    }

    AZ::Crc32 RigidBodyConfiguration::GetSleepOptionsVisibility() const
    {
        return Internal::GetPropertyVisibility(m_propertyVisibilityFlags, RigidBodyConfiguration::PropertyVisibility::SleepOptions);
    }

    AZ::Crc32 RigidBodyConfiguration::GetInterpolationVisibility() const
    {
        return Internal::GetPropertyVisibility(m_propertyVisibilityFlags, RigidBodyConfiguration::PropertyVisibility::Interpolation);
    }

    AZ::Crc32 RigidBodyConfiguration::GetGravityVisibility() const
    {
        return Internal::GetPropertyVisibility(m_propertyVisibilityFlags, RigidBodyConfiguration::PropertyVisibility::Gravity);
    }

    AZ::Crc32 RigidBodyConfiguration::GetKinematicVisibility() const
    {
        return Internal::GetPropertyVisibility(m_propertyVisibilityFlags, RigidBodyConfiguration::PropertyVisibility::Kinematic);
    }

    AZ::Crc32 RigidBodyConfiguration::GetCcdVisibility() const
    {
        return Internal::GetPropertyVisibility(m_propertyVisibilityFlags, RigidBodyConfiguration::PropertyVisibility::ContinuousCollisionDetection);
    }

    AZ::Crc32 RigidBodyConfiguration::GetMaxVelocitiesVisibility() const
    {
        return Internal::GetPropertyVisibility(m_propertyVisibilityFlags, RigidBodyConfiguration::PropertyVisibility::MaxVelocities);
    }
}
