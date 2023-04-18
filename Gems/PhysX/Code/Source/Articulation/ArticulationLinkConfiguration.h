/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>
#include <AzFramework/Physics/Configuration/SimulatedBodyConfiguration.h>

#include <PhysX/ArticulationTypes.h>
#include <PhysX/Joint/Configuration/PhysXJointConfiguration.h>
#include <Source/JointComponent.h>

namespace AZ
{
    class ReflectContext;
}

namespace PhysX
{
    //! Configuration used to describe force/torque sensors attached to articulation links.
    struct ArticulationSensorConfiguration
    {
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_TYPE_INFO(ArticulationSensorConfiguration, "{83960469-C92D-405D-B12E-EB235BCFFECA}");
        static void Reflect(AZ::ReflectContext* context);

        ArticulationSensorConfiguration() = default;

        AZ::Vector3 m_localPosition = AZ::Vector3::CreateZero(); //!< Position of the sensor relative to its link.
        AZ::Vector3 m_localRotation = AZ::Vector3::CreateZero(); //!< Euler angle rotation of the sensor relative to its link.
        bool m_includeForwardDynamicsForces = true; //!< Whether the output reported by the sensor should include forward dynamics forces.
        bool m_includeConstraintSolverForces = true; //!< Whether the output reported by the sensor should include constraint solver forces.
        bool m_useWorldFrame = false; //!< If true, the output will be reported in world space, otherwise in the local space of the sensor.
    };

    //! Configuration of the articulation joint motor
    struct ArticulationJointMotorProperties
    {
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_TYPE_INFO(ArticulationJointMotorProperties, "{44AAC055-73A0-4835-B9E9-A9E233B2C54A}");
        static void Reflect(AZ::ReflectContext* context);

        ArticulationJointMotorProperties() = default;

        bool m_useMotor = false; //!< Enables joint actuation.
        float m_driveForceLimit = 1.0f; //!< Force/torque limit applied by motor.
        float m_driveStiffness = 1.0f; //!< Stiffness of motor servo.
        float m_driveDamping = 1.0f; //!< Damping of motor servo.
    };

    //! Configuration used to Add Articulations to a Scene.
    struct ArticulationLinkConfiguration : public AzPhysics::SimulatedBodyConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_RTTI(ArticulationLinkConfiguration, "{56268154-037A-4BB2-A7EE-E1E76B7D2F8E}", AzPhysics::SimulatedBodyConfiguration);
        static void Reflect(AZ::ReflectContext* context);

        ArticulationLinkConfiguration() = default;
        virtual ~ArticulationLinkConfiguration() = default;

        // Articulation specific configuration

        bool m_isFixedBase = false;
        bool m_selfCollide = false;

        // Rigid Body configuration

        // Basic initial settings.
        AZ::Vector3 m_centerOfMassOffset = AZ::Vector3::CreateZero();

        // Simulation parameters.
        float m_mass = 1.0f;
        float m_linearDamping = 0.05f;
        float m_angularDamping = 0.15f;
        float m_sleepMinEnergy = 0.005f; // Relevant to the root link only
        float m_maxAngularVelocity = 100.0f;

        bool m_startAsleep = false;
        bool m_gravityEnabled = true;

        // PhysX specific Rigid Body configuration

        AZ::u8 m_solverPositionIterations = 4; //!< Higher values can improve stability at the cost of performance.
        AZ::u8 m_solverVelocityIterations = 1; //!< Higher values can improve stability at the cost of performance.

        // Joint configuration

        ArticulationJointType m_articulationJointType = ArticulationJointType::Fix;
        bool m_fixJointLocation = false;

        bool m_isLimited = true; //!< Indicates if this joint has limits, e.g. maximum swing angles.
        float m_linearLimitLower = -1.0f;
        float m_linearLimitUpper = 1.0f;
        float m_angularLimitPositive = 45.0f;
        float m_angularLimitNegative = -45.0f;

        float m_jointFriction = 0.f;
        AZ::Vector3 m_armature = AZ::Vector3::CreateZero();

        ArticulationJointMotorProperties m_motorConfiguration;

        AZ::Vector3 m_localPosition = AZ::Vector3::CreateZero();
        AZ::Vector3 m_localRotation =
            AZ::Vector3::CreateZero(); //!< Local rotation angles about X, Y, Z axes in degrees, relative to follower body.

        // These properties are not exposed to the edit context. They are used when creating editor Articulation components programmatically.
        bool m_autoCalculateLeadFrame = true;
        AZ::Vector3 m_leadLocalPosition = AZ::Vector3::CreateZero(); 
        AZ::Vector3 m_LeadLocalRotation =
            AZ::Vector3::CreateZero(); //!< Local rotation angles about X, Y, Z axes in degrees, relative to lead body.

        AZStd::vector<ArticulationSensorConfiguration> m_sensorConfigs;

        enum class DisplaySetupState : AZ::u8
        {
            Never = 0,
            Selected,
            Always
        };
        DisplaySetupState m_displayJointSetup = DisplaySetupState::Selected;

        // This is only used to control the visibility attribute in the edit context. This is not part of the edit context.
        bool m_isRootArticulation = false;

        bool IsNotRootArticulation() const;
        bool HingePropertiesVisible() const;
        bool PrismaticPropertiesVisible() const;
        bool IsSingleDofJointType() const;
    };
} // namespace PhysX
