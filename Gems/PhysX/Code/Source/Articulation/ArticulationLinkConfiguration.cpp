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
    AZ_CLASS_ALLOCATOR_IMPL(ArticulationLinkConfiguration, AZ::SystemAllocator);

    void ArticulationLinkConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ArticulationLinkConfiguration>()
                ->Version(2)
                ->Field("Root Articulation", &ArticulationLinkConfiguration::m_isRootArticulation)
                ->Field("Mass", &ArticulationLinkConfiguration::m_mass)
                ->Field("Gravity Enabled", &ArticulationLinkConfiguration::m_gravityEnabled)
                ->Field("Fixed Base", &ArticulationLinkConfiguration::m_isFixedBase)
                ->Field("Local Position", &ArticulationLinkConfiguration::m_localPosition)
                ->Field("Local Rotation", &ArticulationLinkConfiguration::m_localRotation)
                ->Field("Initial linear velocity", &ArticulationLinkConfiguration::m_initialLinearVelocity)
                ->Field("Initial angular velocity", &ArticulationLinkConfiguration::m_initialAngularVelocity)
                ->Field("Linear damping", &ArticulationLinkConfiguration::m_linearDamping)
                ->Field("Angular damping", &ArticulationLinkConfiguration::m_angularDamping)
                ->Field("Sleep threshold", &ArticulationLinkConfiguration::m_sleepMinEnergy)
                ->Field("Start Asleep", &ArticulationLinkConfiguration::m_startAsleep)
                ->Field("Centre of mass offset", &ArticulationLinkConfiguration::m_centerOfMassOffset)
                ->Field("Maximum Angular Velocity", &ArticulationLinkConfiguration::m_maxAngularVelocity)
                ->Field("Articulation Joint Type", &ArticulationLinkConfiguration::m_articulationJointType)
                //->Field("Angular Limit", &ArticulationLinkConfiguration::m_angularLimit)
                //->Field("Linear Limit", &ArticulationLinkConfiguration::m_linearLimit)
                ->Field("Motor configuration", &ArticulationLinkConfiguration::m_motorConfiguration)
                ->Field("Fix Joint Location", &ArticulationLinkConfiguration::m_fixJointLocation)
                ->Field("Select Lead on Snap", &ArticulationLinkConfiguration::m_selectLeadOnSnap)
                ->Field("Self Collide", &ArticulationLinkConfiguration::m_selfCollide)
                ->Field("SolverPositionIterations", &ArticulationLinkConfiguration::m_solverPositionIterations)
                ->Field("SolverVelocityIterations", &ArticulationLinkConfiguration::m_solverVelocityIterations);
        }
    }
} // namespace PhysX
