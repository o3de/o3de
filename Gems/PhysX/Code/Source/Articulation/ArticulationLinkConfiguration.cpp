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

namespace
{
    const float LocalRotationMax = 360.0f;
    const float LocalRotationMin = -360.0f;
} // namespace

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
                ->Field("SolverVelocityIterations", &ArticulationLinkConfiguration::m_solverVelocityIterations);
        }

        //if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        //{

        //    if (auto* editContext = serializeContext->GetEditContext())
        //    {
        //        editContext->Enum<ArticulationLinkConfiguration::DisplaySetupState>("Joint Display Setup State", "Options for displaying joint setup.")
        //            ->Value("Never", ArticulationLinkConfiguration::DisplaySetupState::Never)
        //            ->Value("Selected", ArticulationLinkConfiguration::DisplaySetupState::Selected)
        //            ->Value("Always", ArticulationLinkConfiguration::DisplaySetupState::Always);

        //        editContext->Class<PhysX::ArticulationLinkConfiguration>("PhysX Joint Configuration", "")
        //            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
        //            ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
        //            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
        //            ->DataElement(
        //                0, &PhysX::ArticulationLinkConfiguration::m_localPosition, "Local Position", "Local Position of joint, relative to its entity.")
        //            ->DataElement(
        //                0, &PhysX::ArticulationLinkConfiguration::m_localRotation, "Local Rotation", "Local Rotation of joint, relative to its entity.")
        //            ->Attribute(AZ::Edit::Attributes::Min, LocalRotationMin)
        //            ->Attribute(AZ::Edit::Attributes::Max, LocalRotationMax)
        //            ->DataElement(0, &PhysX::ArticulationLinkConfiguration::m_leadEntity, "Lead Entity", "Parent entity associated with joint.")
        //            //->Attribute(AZ::Edit::Attributes::ChangeNotify, &ArticulationLinkConfiguration::ValidateLeadEntityId)
        //            ->DataElement(
        //                0,
        //                &PhysX::ArticulationLinkConfiguration::m_selfCollide,
        //                "Lead-Follower Collide",
        //                "When active, the lead and follower pair will collide with each other.")
        //            ->DataElement(
        //                AZ::Edit::UIHandlers::ComboBox,
        //                &PhysX::ArticulationLinkConfiguration::m_displayJointSetup,
        //                "Display Setup in Viewport",
        //                "Never = Not shown."
        //                "Select = Show setup display when entity is selected."
        //                "Always = Always show setup display.")
        //            ->Attribute(AZ::Edit::Attributes::ReadOnly, &ArticulationLinkConfiguration::IsInComponentMode)
        //            ->DataElement(
        //                0,
        //                &PhysX::ArticulationLinkConfiguration::m_selectLeadOnSnap,
        //                "Select Lead on Snap",
        //                "Select lead entity on snap to position in component mode.")
        //            ->DataElement(
        //                0, &PhysX::ArticulationLinkConfiguration::m_breakable, "Breakable", "Joint is breakable when force or torque exceeds limit.")
        //            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
        //            ->Attribute(AZ::Edit::Attributes::ReadOnly, &ArticulationLinkConfiguration::IsInComponentMode)
        //            ->DataElement(
        //                0, &PhysX::ArticulationLinkConfiguration::m_forceMax, "Maximum Force", "Amount of force joint can withstand before breakage.")
        //            ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::m_breakable)
        //            ->Attribute(AZ::Edit::Attributes::Max, s_breakageMax)
        //            ->Attribute(AZ::Edit::Attributes::Min, s_breakageMin)
        //            ->DataElement(
        //                0,
        //                &PhysX::ArticulationLinkConfiguration::m_torqueMax,
        //                "Maximum Torque",
        //                "Amount of torque joint can withstand before breakage.")
        //            ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::m_breakable)
        //            ->Attribute(AZ::Edit::Attributes::Max, s_breakageMax)
        //            ->Attribute(AZ::Edit::Attributes::Min, s_breakageMin);
        //    }
        //}

        //if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        //{
        //    if (auto* editContext = serializeContext->GetEditContext())
        //    {
        //        editContext
        //            ->Class<PhysX::ArticulationLinkConfiguration>(
        //                "PhysX-specific Rigid Body Configuration", "Additional Rigid Body settings specific to PhysX.")
        //            ->DataElement(
        //                AZ::Edit::UIHandlers::Default,
        //                &PhysX::ArticulationLinkConfiguration::m_solverPositionIterations,
        //                "Solver Position Iterations",
        //                "Higher values can improve stability at the cost of performance.")
        //            ->Attribute(AZ::Edit::Attributes::Min, 1)
        //            ->Attribute(AZ::Edit::Attributes::Max, 255)
        //            ->DataElement(
        //                AZ::Edit::UIHandlers::Default,
        //                &PhysX::ArticulationLinkConfiguration::m_solverVelocityIterations,
        //                "Solver Velocity Iterations",
        //                "Higher values can improve stability at the cost of performance.")
        //            ->Attribute(AZ::Edit::Attributes::Min, 1)
        //            ->Attribute(AZ::Edit::Attributes::Max, 255);
        //    }
        //}
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
        m_includeAllShapesInMassCalculation = AzPhysics::MassComputeFlags::INCLUDE_ALL_SHAPES == (flags & AzPhysics::MassComputeFlags::INCLUDE_ALL_SHAPES);
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
        return Internal::GetPropertyVisibility(m_propertyVisibilityFlags, ArticulationLinkConfiguration::PropertyVisibility::InitialVelocities);
    }

    AZ::Crc32 ArticulationLinkConfiguration::GetInertiaSettingsVisibility() const
    {
        return Internal::GetPropertyVisibility(m_propertyVisibilityFlags, ArticulationLinkConfiguration::PropertyVisibility::InertiaProperties);
    }

    AZ::Crc32 ArticulationLinkConfiguration::GetInertiaVisibility() const
    {
        bool visible =
            ((m_propertyVisibilityFlags & ArticulationLinkConfiguration::PropertyVisibility::InertiaProperties) != 0) && !m_computeInertiaTensor;
        return visible ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    AZ::Crc32 ArticulationLinkConfiguration::GetCoMVisibility() const
    {
        bool visible =
            ((m_propertyVisibilityFlags & ArticulationLinkConfiguration::PropertyVisibility::InertiaProperties) != 0) && !m_computeCenterOfMass;
        return visible ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    AZ::Crc32 ArticulationLinkConfiguration::GetMassVisibility() const
    {
        bool visible = ((m_propertyVisibilityFlags & ArticulationLinkConfiguration::PropertyVisibility::InertiaProperties) != 0) && !m_computeMass;
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

    /*void ArticulationLinkConfiguration::SetLeadEntityId(AZ::EntityId leadEntityId)
    {
        m_leadEntity = leadEntityId;
        ValidateLeadEntityId();

        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
    }*/

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

    bool ArticulationLinkConfiguration::IsInComponentMode() const
    {
        return m_inComponentMode;
    }

    JointComponentConfiguration ArticulationLinkConfiguration::ToGameTimeConfig() const
    {
        AZ::Vector3 localRotation(m_localRotation);

        return JointComponentConfiguration(
            AZ::Transform::CreateFromQuaternionAndTranslation(AZ::Quaternion::CreateFromEulerAnglesDegrees(localRotation), m_localPosition),
            m_leadEntity,
            m_followerEntity);
    }

//    void ArticulationLinkConfiguration::ValidateLeadEntityId()
//    {
//        if (!m_leadEntity.IsValid())
//        {
//            return;
//        }
//
//        AZ::Entity* entity = nullptr;
//        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, m_leadEntity);
//        if (entity)
//        {
//            [[maybe_unused]] const bool leadEntityHasRigidActor =
//                (entity->FindComponent<PhysX::EditorRigidBodyComponent>() ||
//                 entity->FindComponent<PhysX::EditorStaticRigidBodyComponent>());
//
//            AZ_Warning(
//                "EditorJointComponent",
//                leadEntityHasRigidActor,
//                "Joints require either a dynamic or static rigid body on the lead entity. "
//                "Please add either a static or a dynamic rigid body component to entity %s",
//                entity->GetName().c_str());
//        }
//        else
//        {
//            AZStd::string followerEntityName;
//            if (m_followerEntity.IsValid())
//            {
//                AZ::ComponentApplicationBus::BroadcastResult(
//                    followerEntityName, &AZ::ComponentApplicationRequests::GetEntityName, m_followerEntity);
//            }
//
//            AZ_Warning(
//                "EditorJointComponent",
//                false,
//                "Cannot find instance of lead entity given its entity ID. Please check that joint in entity %s has valid lead entity.",
//                followerEntityName.c_str());
//        }
//    //}
//} // namespace AzPhysics
