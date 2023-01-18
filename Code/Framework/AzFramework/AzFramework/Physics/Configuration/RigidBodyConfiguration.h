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

namespace AZ
{
    class ReflectContext;
}

namespace AzPhysics
{
    //! Configuration used to Add Rigid bodies to a Scene.
    struct RigidBodyConfiguration
        : public AzPhysics::SimulatedBodyConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_RTTI(RigidBodyConfiguration, "{ACFA8900-8530-4744-AF00-AA533C868A8E}", AzPhysics::SimulatedBodyConfiguration);
        static void Reflect(AZ::ReflectContext* context);

        RigidBodyConfiguration() = default;

        MassComputeFlags GetMassComputeFlags() const;
        void SetMassComputeFlags(MassComputeFlags flags);

        bool IsCcdEnabled() const;
        bool CcdReadOnly() const;
        AZStd::string_view GetCcdTooltip() const;
        AZStd::string_view GetKinematicTooltip() const;

        // Basic initial settings.
        AZ::Vector3 m_initialLinearVelocity = AZ::Vector3::CreateZero();
        AZ::Vector3 m_initialAngularVelocity = AZ::Vector3::CreateZero();
        AZ::Vector3 m_centerOfMassOffset = AZ::Vector3::CreateZero();

        // Simulation parameters.
        float m_mass = 1.0f;
        AZ::Matrix3x3 m_inertiaTensor = AZ::Matrix3x3::CreateIdentity();
        float m_linearDamping = 0.05f;
        float m_angularDamping = 0.15f;
        float m_sleepMinEnergy = 0.005f;
        float m_maxAngularVelocity = 100.0f;

        bool m_startAsleep = false;
        bool m_interpolateMotion = false;
        bool m_gravityEnabled = true;
        bool m_kinematic = false;
        bool m_ccdEnabled = false; //!< Whether continuous collision detection is enabled.
        float m_ccdMinAdvanceCoefficient = 0.15f; //!< Coefficient affecting how granularly time is subdivided in CCD.
        bool m_ccdFrictionEnabled = false; //!< Whether friction is applied when resolving CCD collisions.
        bool m_configButton = false; //!< Required for button positioning in edit context, otherwise unused.

        bool m_computeCenterOfMass = true;
        bool m_computeInertiaTensor = true;
        bool m_computeMass = true;

        // Flags to restrict motion along specific world-space axes.
        bool m_lockLinearX = false;
        bool m_lockLinearY = false;
        bool m_lockLinearZ = false;

        // Flags to restrict rotation around specific world-space axes.
        bool m_lockAngularX = false;
        bool m_lockAngularY = false;
        bool m_lockAngularZ = false;

        //! If set, non-simulated shapes will also be included in the mass properties calculation.
        bool m_includeAllShapesInMassCalculation = false;

        //! Variant to support multiple having the system creating the Shape(s) or just providing the Shape(s) that have been created externally.
        //! See ShapeVariantData for more information.
        ShapeVariantData m_colliderAndShapeData;

        //Visibility helpers for use in the Editor when reflected. RagdollNodeConfiguration also uses these.
        enum PropertyVisibility : AZ::u16
        {
            InitialVelocities = 1 << 0, //!< Whether the initial linear and angular velocities are visible.
            InertiaProperties = 1 << 1, //!< Whether the whole category of inertia properties (mass, compute inertia, inertia tensor etc) is visible.
            Damping = 1 << 2, //!< Whether linear and angular damping are visible.
            SleepOptions = 1 << 3, //!< Whether the sleep threshold and start asleep options are visible.
            Interpolation = 1 << 4, //!< Whether the interpolation option is visible.
            Gravity = 1 << 5, //!< Whether the effected by gravity option is visible.
            Kinematic = 1 << 6, //!< Whether the option to make the body kinematic is visible.
            ContinuousCollisionDetection = 1 << 7, //!< Whether the option to enable continuous collision detection is visible.
            MaxVelocities = 1 << 8 //!< Whether upper limits on velocities are visible.
        };

        AZ::Crc32 GetInitialVelocitiesVisibility() const;
        AZ::Crc32 GetInertiaSettingsVisibility() const;
        AZ::Crc32 GetInertiaVisibility() const;
        AZ::Crc32 GetMassVisibility() const;
        AZ::Crc32 GetCoMVisibility() const;
        AZ::Crc32 GetDampingVisibility() const;
        AZ::Crc32 GetSleepOptionsVisibility() const;
        AZ::Crc32 GetInterpolationVisibility() const;
        AZ::Crc32 GetGravityVisibility() const;
        AZ::Crc32 GetKinematicVisibility() const;
        AZ::Crc32 GetCcdVisibility() const;
        AZ::Crc32 GetMaxVelocitiesVisibility() const;

        AZ::u16 m_propertyVisibilityFlags = (std::numeric_limits<AZ::u16>::max)();
    };
}
