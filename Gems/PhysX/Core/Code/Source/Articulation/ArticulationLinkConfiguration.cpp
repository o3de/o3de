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

namespace PhysX
{
    AZ_CLASS_ALLOCATOR_IMPL(ArticulationCacheConfiguration, AZ::SystemAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(ArticulationLinkConfiguration, AZ::SystemAllocator);

    physx::PxArticulationCacheFlags ArticulationCacheConfiguration::GetPxCacheFlags() const
    {
        physx::PxArticulationCacheFlags flags;

        if (m_jointVelocities)
        {
            flags |= physx::PxArticulationCacheFlag::eVELOCITY;
        }
        if (m_jointAccelerations)
        {
            flags |= physx::PxArticulationCacheFlag::eACCELERATION;
        }
        if (m_jointPositions)
        {
            flags |= physx::PxArticulationCacheFlag::ePOSITION;
        }
        if (m_jointForces)
        {
            flags |= physx::PxArticulationCacheFlag::eFORCE;
        }
        if (m_linkVelocities)
        {
            flags |= physx::PxArticulationCacheFlag::eLINK_VELOCITY;
        }
        if (m_linkAccelerations)
        {
            flags |= physx::PxArticulationCacheFlag::eLINK_ACCELERATION;
        }
        if (m_rootLinkTransform)
        {
            flags |= physx::PxArticulationCacheFlag::eROOT_TRANSFORM;
        }
        if (m_rootLinkVelocities)
        {
            flags |= physx::PxArticulationCacheFlag::eROOT_VELOCITIES;
        }
        if (m_linkIncomingJointForces)
        {
            flags |= physx::PxArticulationCacheFlag::eLINK_INCOMING_JOINT_FORCE;
        }
        if (m_jointTargetPositions)
        {
            flags |= physx::PxArticulationCacheFlag::eJOINT_TARGET_POSITIONS;
        }
        if (m_jointTargetVelocities)
        {
            flags |= physx::PxArticulationCacheFlag::eJOINT_TARGET_VELOCITIES;
        }
        if (m_linkForces)
        {
            flags |= physx::PxArticulationCacheFlag::eLINK_FORCE;
        }
        if (m_linkTorques)
        {
            flags |= physx::PxArticulationCacheFlag::eLINK_TORQUE;
        }
        return flags;
    }

    AZ_CLASS_ALLOCATOR_IMPL(ArticulationJointMotorProperties, AZ::SystemAllocator);

    void ArticulationCacheConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ArticulationCacheConfiguration>()
                ->Version(1)
                ->Field("Joint Velocities", &ArticulationCacheConfiguration::m_jointVelocities)
                ->Field("Joint Accelerations", &ArticulationCacheConfiguration::m_jointAccelerations)
                ->Field("Joint Positions", &ArticulationCacheConfiguration::m_jointPositions)
                ->Field("Joint Forces", &ArticulationCacheConfiguration::m_jointForces)
                ->Field("Link Velocities", &ArticulationCacheConfiguration::m_linkVelocities)
                ->Field("Link Accelerations", &ArticulationCacheConfiguration::m_linkAccelerations)
                ->Field("Root Link Transform", &ArticulationCacheConfiguration::m_rootLinkTransform)
                ->Field("Root Link Velocities", &ArticulationCacheConfiguration::m_rootLinkVelocities)
                ->Field("Link Incoming Joint Forces", &ArticulationCacheConfiguration::m_linkIncomingJointForces)
                ->Field("Joint Target Positions", &ArticulationCacheConfiguration::m_jointTargetPositions)
                ->Field("Joint Target Velocities", &ArticulationCacheConfiguration::m_jointTargetVelocities)
                ->Field("Link Forces", &ArticulationCacheConfiguration::m_linkForces)
                ->Field("Link Torques", &ArticulationCacheConfiguration::m_linkTorques)
                ;

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<ArticulationCacheConfiguration>("PhysX Articulation Cache Configuration", "")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationCacheConfiguration::m_jointVelocities,
                        "Include Joint Velocities",
                        "The articulation joint DOF velocities.\n"
                        "N = getDofs()")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationCacheConfiguration::m_jointAccelerations,
                        "Include Joint Accelerations",
                        "The articulation joint DOF Accelerations.\n"
                        "N = getDofs()")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationCacheConfiguration::m_jointPositions,
                        "Include Joint Positions",
                        "The articulation joint DOF Positions.\n"
                        "N = getDofs()")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationCacheConfiguration::m_jointForces,
                        "Include Joint Forces",
                        "The articulation joint DOF Forces.\n"
                        "N = getDofs()")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationCacheConfiguration::m_linkVelocities,
                        "Include Link Spacial Velocities",
                        "Link Spacial Velocity.\n"
                        "N = getNbLinks()")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationCacheConfiguration::m_linkAccelerations,
                        "Include Link Accelerations",
                        "Link Classical Acceleration.\n"
                        "N = getNbLinks()")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationCacheConfiguration::m_rootLinkTransform,
                        "Include Root Link Transform",
                        "Root Link Transform.\n"
                        "N = 1")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationCacheConfiguration::m_rootLinkVelocities,
                        "Include Link Accelerations",
                        "The root link velocities (read/write) and accelerations (read).\n"
                        "N = 1")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationCacheConfiguration::m_linkIncomingJointForces,
                        "Include Link Incoming Joint Forces",
                        "The total force transmitted from the parent link to this link.\n"
                        "N = getNbLinks()")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationCacheConfiguration::m_jointTargetPositions,
                        "Include Joint Target Positions",
                        "The articulation joint drive target positions.\n"
                        "N = getDofs()")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationCacheConfiguration::m_jointTargetVelocities,
                        "Include Root Link Transform",
                        "The articulation joint drive target velocities.\n"
                        "N = getDofs()")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationCacheConfiguration::m_linkForces,
                        "Include Link Forces",
                        "An external force applied to the link’s center of mass.\n"
                        "N = getNbLinks()")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationCacheConfiguration::m_linkTorques,
                        "Include Link Torques",
                        "An external torque applied to the link.\n"
                        "N = getNbLinks()");
            }
        }
    }

    void ArticulationJointMotorProperties::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ArticulationJointMotorProperties>()
                ->Version(1)
                ->Field("UseMotor", &ArticulationJointMotorProperties::m_useMotor)
                ->Field("ForceLimit", &ArticulationJointMotorProperties::m_driveForceLimit)
                ->Field("Stiffness", &ArticulationJointMotorProperties::m_driveStiffness)
                ->Field("Damping", &ArticulationJointMotorProperties::m_driveDamping);


            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<PhysX::ArticulationJointMotorProperties>("PhysX Joint Motor Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &PhysX::ArticulationJointMotorProperties::m_useMotor, "Use Motor", "Enable motor in the joint.")
                    ->DataElement(0, &PhysX::ArticulationJointMotorProperties::m_driveForceLimit, "Force Limit Value", "Sets force limit value.")
                    ->DataElement(0, &PhysX::ArticulationJointMotorProperties::m_driveStiffness, "Stiffness Value", "Stiffness of motor servo.")
                    ->DataElement(0, &PhysX::ArticulationJointMotorProperties::m_driveDamping, "Damping Value", "Damping of motor servo.");
            }
        }
    }

    void ArticulationLinkConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ArticulationLinkConfiguration, AzPhysics::SimulatedBodyConfiguration>()
                ->Version(1)
                ->Field("Fixed Base", &ArticulationLinkConfiguration::m_isFixedBase)
                ->Field("Self Collide", &ArticulationLinkConfiguration::m_selfCollide)
                ->Field("Mass", &ArticulationLinkConfiguration::m_mass)
                ->Field("Gravity Enabled", &ArticulationLinkConfiguration::m_gravityEnabled)
                ->Field("Linear damping", &ArticulationLinkConfiguration::m_linearDamping)
                ->Field("Angular damping", &ArticulationLinkConfiguration::m_angularDamping)
                ->Field("Sleep threshold", &ArticulationLinkConfiguration::m_sleepMinEnergy)
                ->Field("Start Asleep", &ArticulationLinkConfiguration::m_startAsleep)
                ->Field("Centre of mass offset", &ArticulationLinkConfiguration::m_centerOfMassOffset)
                ->Field("Maximum Angular Velocity", &ArticulationLinkConfiguration::m_maxAngularVelocity)
                ->Field("SolverPositionIterations", &ArticulationLinkConfiguration::m_solverPositionIterations)
                ->Field("SolverVelocityIterations", &ArticulationLinkConfiguration::m_solverVelocityIterations)
                ->Field("Articulation Joint Type", &ArticulationLinkConfiguration::m_articulationJointType)
                ->Field("Local Position", &ArticulationLinkConfiguration::m_localPosition)
                ->Field("Local Rotation", &ArticulationLinkConfiguration::m_localRotation)
                ->Field("Fix Joint Location", &ArticulationLinkConfiguration::m_fixJointLocation)
                ->Field("Auto-calculate Lead frame", &ArticulationLinkConfiguration::m_autoCalculateLeadFrame)
                ->Field("Lead Local Position", &ArticulationLinkConfiguration::m_leadLocalPosition)
                ->Field("Lead Local Rotation", &ArticulationLinkConfiguration::m_LeadLocalRotation)
                ->Field("Display Joint Setup", &ArticulationLinkConfiguration::m_displayJointSetup)
                ->Field("Is Limited", &ArticulationLinkConfiguration::m_isLimited)
                ->Field("Linear Limit Lower", &ArticulationLinkConfiguration::m_linearLimitLower)
                ->Field("Linear Limit Upper", &ArticulationLinkConfiguration::m_linearLimitUpper)
                ->Field("Angular Limit Negative", &ArticulationLinkConfiguration::m_angularLimitNegative)
                ->Field("Angular Limit Positive", &ArticulationLinkConfiguration::m_angularLimitPositive)
                ->Field("Motor configuration", &ArticulationLinkConfiguration::m_motorConfiguration)
                ->Field("Armature", &ArticulationLinkConfiguration::m_armature)
                ->Field("Friction", &ArticulationLinkConfiguration::m_jointFriction)
                ->Field("Sensor Configuration", &ArticulationLinkConfiguration::m_sensorConfigs)
                ->Field("Offset", &ArticulationLinkConfiguration::m_offset);
                ->Field("Cache Configuration", &ArticulationLinkConfiguration::m_articulationCacheConfig);
        }
    }

    bool ArticulationLinkConfiguration::IsNotRootArticulation() const
    {
        return !m_isRootArticulation;
    }

    bool ArticulationLinkConfiguration::HingePropertiesVisible() const
    {
        return m_articulationJointType == ArticulationJointType::Hinge && IsNotRootArticulation();
    }

    bool ArticulationLinkConfiguration::PrismaticPropertiesVisible() const
    {
        return m_articulationJointType == ArticulationJointType::Prismatic && IsNotRootArticulation();
    }

    bool ArticulationLinkConfiguration::IsSingleDofJointType() const
    {
        return HingePropertiesVisible() || PrismaticPropertiesVisible();
    }
} // namespace PhysX
