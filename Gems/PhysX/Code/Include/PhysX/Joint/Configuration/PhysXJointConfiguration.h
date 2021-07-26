/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Physics/Configuration/JointConfiguration.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace PhysX
{
    struct D6JointLimitConfiguration
        : public AzPhysics::JointConfiguration
    {
        AZ_CLASS_ALLOCATOR(D6JointLimitConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(D6JointLimitConfiguration, "{88E067B4-21E8-4FFA-9142-6C52605B704C}", AzPhysics::JointConfiguration);
        static void Reflect(AZ::ReflectContext* context);

        float m_swingLimitY = 45.0f; ///< Maximum angle in degrees from the Y axis of the joint frame.
        float m_swingLimitZ = 45.0f; ///< Maximum angle in degrees from the Z axis of the joint frame.
        float m_twistLimitLower = -45.0f; ///< Lower limit in degrees for rotation about the X axis of the joint frame.
        float m_twistLimitUpper = 45.0f; ///< Upper limit in degrees for rotation about the X axis of the joint frame.
    };

    //! Properties that are common for several types of joints.
    struct JointGenericProperties
    {
        enum class GenericJointFlag : AZ::u16
        {
            None = 0,
            Breakable = 1,
            SelfCollide = 1 << 1
        };

        AZ_CLASS_ALLOCATOR(JointGenericProperties, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(JointGenericProperties, "{6CB15399-24F6-4F03-AAEF-1AE013B683E0}");
        static void Reflect(AZ::ReflectContext* context);

        JointGenericProperties() = default;
        JointGenericProperties(GenericJointFlag flags, float forceMax, float torqueMax);

        bool IsFlagSet(GenericJointFlag flag) const; ///< Returns if a particular flag is set as a bool.

        /// Flags that indicates if joint is breakable, self-colliding, etc. 
        /// Converting joint between breakable/non-breakable at game time is allowed.
        GenericJointFlag m_flags = GenericJointFlag::None;
        float m_forceMax = 1.0f; ///< Max force joint can tolerate before breaking.
        float m_torqueMax = 1.0f; ///< Max torque joint can tolerate before breaking.
    };
    AZ_DEFINE_ENUM_BITWISE_OPERATORS(PhysX::JointGenericProperties::GenericJointFlag)

    struct JointLimitProperties
    {
        AZ_CLASS_ALLOCATOR(JointLimitProperties, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(JointLimitProperties, "{31F941CB-6699-48BB-B12D-61874B52B984}");
        static void Reflect(AZ::ReflectContext* context);

        JointLimitProperties() = default;
        JointLimitProperties(
            bool isLimited, bool isSoftLimit, 
            float damping, float limitFirst, float limitSecond, float stiffness, float tolerance);

        bool m_isLimited = true; ///< Specifies if limits are applied to the joint constraints. E.g. if the swing angles are limited.
        bool m_isSoftLimit = false; ///< If limit is soft, spring and damping are used, otherwise tolerance is used. Converting between soft/hard limit at game time is allowed.
        float m_damping = 20.0f; ///< The damping strength of the drive, the force proportional to the velocity error. Used if limit is soft.
        float m_limitFirst = 45.0f; ///< Positive angle limit in the case of twist angle limits, Y-axis swing limit in the case of cone limits.
        float m_limitSecond = 45.0f; ///< Negative angle limit in the case of twist angle limits, Z-axis swing limit in the case of cone limits.
        float m_stiffness = 100.0f; ///< The spring strength of the drive, the force proportional to the position error. Used if limit is soft.
        float m_tolerance = 0.1f; ///< Distance from the joint at which limits becomes enforced. Used if limit is hard.
    };

    struct FixedJointConfiguration : public AzPhysics::JointConfiguration 
    {
        AZ_CLASS_ALLOCATOR(FixedJointConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(FixedJointConfiguration, "{9BCB368B-8D71-4928-B231-0225907E3BD9}", AzPhysics::JointConfiguration);
        static void Reflect(AZ::ReflectContext* context);

        JointGenericProperties m_genericProperties;
    };

    struct BallJointConfiguration : public AzPhysics::JointConfiguration 
    {
        AZ_CLASS_ALLOCATOR(BallJointConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(BallJointConfiguration, "{C2DE2479-B752-469D-BE05-900CD2CD8481}", AzPhysics::JointConfiguration);
        static void Reflect(AZ::ReflectContext* context);

        JointGenericProperties m_genericProperties;
        JointLimitProperties m_limitProperties;
    };
    
    struct HingeJointConfiguration : public AzPhysics::JointConfiguration 
    {
        AZ_CLASS_ALLOCATOR(HingeJointConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(HingeJointConfiguration, "{FB04198E-0BA5-45C2-8343-66DA28ED45EA}", AzPhysics::JointConfiguration);
        static void Reflect(AZ::ReflectContext* context);

        JointGenericProperties m_genericProperties;
        JointLimitProperties m_limitProperties;
    };
} // namespace PhysX
