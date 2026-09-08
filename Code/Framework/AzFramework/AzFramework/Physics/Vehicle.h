/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Matrix3x3.h>

#include <AzFramework/Physics/Shape.h>

#include <AzFramework/Physics/Configuration/RigidBodyConfiguration.h>
#include <AzFramework/AzFrameworkAPI.h>

namespace AZ
{
    class ReflectContext;
}

namespace Physics
{
    class Wheel;
    struct WheelConfiguration;

    //! Configuration for an anti-roll bar, which is a spring between two wheels.
    struct AZF_API AntiRollBarConfiguration
    {
        AZ_TYPE_INFO(AntiRollBarConfiguration, "{B2054DA9-8CCD-4839-B7D2-F72FF82FD6FC}");
        AZ_CLASS_ALLOCATOR(AntiRollBarConfiguration, AZ::SystemAllocator);

        size_t m_leftWheel; //!< The index of the left wheel connected to the anti-roll bar.
        size_t m_rightWheel; //!< The index of the right wheel connected to the anti-roll bar.
        float m_stiffness; //!< The stiffness of the anti-roll bar, which controls how much it resists body roll. Higher values will result in less body roll, but can also lead to a harsher ride and reduced traction.
    };

    struct AZF_API DriveModelConfiguration
    {
        AZ_RTTI(DriveModelConfiguration, "{AE13D49A-EB7E-419C-A58F-EFAB658BE10F}");
        AZ_CLASS_ALLOCATOR(DriveModelConfiguration, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);
        virtual ~DriveModelConfiguration() = default;
    };

    struct AZF_API EngineDriveModelConfiguration
        : public DriveModelConfiguration
    {
        AZ_RTTI(EngineDriveModelConfiguration, "{D90C23AC-B1E9-4C57-83F1-A5F3FF373DAE}", DriveModelConfiguration);
        AZ_CLASS_ALLOCATOR(EngineDriveModelConfiguration, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);
        virtual ~EngineDriveModelConfiguration() = default;

        // Engine configuration parameters
        float m_moi; //!< Moment of inertia of the engine, which affects how quickly it can change speed.
        float m_minRotationSpeed; //!< Rotation speed of the engine when idling (rad/s). The engine will not produce torque below this rotation speed.
        float m_maxRotationSpeed; //!< Maximum rotation speed of the engine (rad/s), above which it will not produce more torque.
        float m_peakTorque; //!< The maximum torque produced by the engine at the optimal rotation speed.
        AZStd::vector<AZ::Vector2> m_torqueCurve; //!< A curve defining how the torque produced by the engine changes with rotation speed. The x-axis is the rotation speed, and the y-axis is the torque multiplier (where 1.0 means full torque, 0.5 means half torque, etc.). The curve should be defined such that the point with the highest y value corresponds to the optimal rotation speed for producing peak torque.

        // Transmission configuration parameters
        AZStd::vector<float> m_gearRatios; //!< The gear ratios for each forward gear. The length of this vector determines the number of forward gears.
        size_t m_neutralGearIndex; //!< The index of the neutral gear in the gear ratios vector. This is typically 1 (the only reverse gear is m_gearRatios[0]), but can be set to a different value if desired.
        float m_switchTime; //!< The time it takes to switch gears, during which the engine does not produce torque.
    };

    //! Configuration for a vehicle, which includes settings for the chassis rigid body,
    //! as well as settings specific to vehicles such as wheel configurations and drive model configuration.
    //! This configuration is used to create a Vehicle.
    struct AZF_API VehicleConfiguration
        : AzPhysics::RigidBodyConfiguration
    {
        AZ_CLASS_ALLOCATOR(VehicleConfiguration, AZ::SystemAllocator);
        AZ_RTTI(VehicleConfiguration, "{2FEAF593-4EE2-4044-8C1F-F600B82AE9C1}", AzPhysics::RigidBodyConfiguration);

        virtual ~VehicleConfiguration() = default;

        static void Reflect(AZ::ReflectContext* context);

        //! Up direction for the vehicle, Z positive by default
        AZ::Vector3 m_upDirection = AZ::Vector3::CreateAxisZ();

        //! Forward direction for the vehicle, Y positive by default
        AZ::Vector3 m_forwardDirection = AZ::Vector3::CreateAxisY();
        
        //! The configuration for each wheel on the vehicle.
        //!The number of wheels is determined by the number of entries in this vector.
        AZStd::vector<AZStd::shared_ptr<WheelConfiguration>> m_wheels;
        
        //! The configuration for each anti-roll bars on the vehicle.
        AZStd::vector<AntiRollBarConfiguration> m_antiRollBars; //!< The configuration for each anti-roll bar on the vehicle.
        AZStd::shared_ptr<DriveModelConfiguration> m_driveModel; //!< The configuration for the vehicle's drive model, which controls how the vehicle responds to inputs
    };
    
    class AZF_API Vehicle
        : public AzPhysics::SimulatedBody
    {
    public:
        AZ_CLASS_ALLOCATOR(Vehicle, AZ::SystemAllocator);
        AZ_RTTI(Vehicle, "{A5DC77D5-175E-44C8-AD0E-211F818BA708}", AzPhysics::SimulatedBody);

        // Common vehicle controls
        virtual const float GetBrake() const = 0;
        virtual void SetBrake(const float brake) = 0;

        virtual const float GetHandbrake() const = 0;
        virtual void SetHandbrake(const float handbrake) = 0;

        virtual const float GetThrottle() const = 0;
        virtual void SetThrottle(const float throttle) = 0;

        virtual const float GetSteeringAngle() const = 0;
        virtual void SetSteeringAngle(const float steeringAngle) = 0;

        // Wheels
        virtual Wheel* GetWheel(const size_t wheelIndex) const = 0;
        virtual const size_t GetWheelCount() const = 0;

        // Rigid body
        virtual AzPhysics::RigidBody* GetRigidBody() const = 0;
    };
}

