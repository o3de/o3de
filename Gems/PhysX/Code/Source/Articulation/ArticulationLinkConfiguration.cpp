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
    AZ_CLASS_ALLOCATOR_IMPL(ArticulationSensorConfiguration, AZ::SystemAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(ArticulationLinkConfiguration, AZ::SystemAllocator);

    void ArticulationSensorConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ArticulationSensorConfiguration>()
                ->Version(1)
                ->Field("Local Position", &ArticulationSensorConfiguration::m_localPosition)
                ->Field("Local Rotation", &ArticulationSensorConfiguration::m_localRotation)
                ->Field("Include Forward Dynamics Forces", &ArticulationSensorConfiguration::m_includeForwardDynamicsForces)
                ->Field("Include Constraint Solver Forces", &ArticulationSensorConfiguration::m_includeConstraintSolverForces)
                ->Field("Use World Frame", &ArticulationSensorConfiguration::m_useWorldFrame);

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<ArticulationSensorConfiguration>("PhysX Articulation Sensor Configuration", "")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationSensorConfiguration::m_localPosition,
                        "Local Position",
                        "The local position of the sensor relative to the articulation link")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationSensorConfiguration::m_localRotation,
                        "Local Rotation",
                        "The local rotation of the sensor relative to the articulation link")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationSensorConfiguration::m_includeForwardDynamicsForces,
                        "Include Forward Dynamics Forces",
                        "Whether the output reported by the sensor should include forward dynamics forces")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationSensorConfiguration::m_includeConstraintSolverForces,
                        "Include Constraint Solver Forces",
                        "Whether the output reported by the sensor should include constraint solver forces")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationSensorConfiguration::m_useWorldFrame,
                        "Use World Frame",
                        "If true, the output will be reported in world space, otherwise in the local space of the sensor");
            }
        }
    }

    void ArticulationLinkConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ArticulationLinkConfiguration, AzPhysics::SimulatedBodyConfiguration>()
                ->Version(2)
                ->Field("Fixed Base", &ArticulationLinkConfiguration::m_isFixedBase)
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
                ->Field("Self Collide", &ArticulationLinkConfiguration::m_selfCollide)
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
                ->Field("Sensor Configurations", &ArticulationLinkConfiguration::m_sensorConfigs);
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
