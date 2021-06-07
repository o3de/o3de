/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzFramework/Physics/Configuration/ApiJointConfiguration.h>

namespace PhysX
{
    struct D6ApiJointLimitConfiguration
        : public AzPhysics::ApiJointConfiguration
    {
        AZ_CLASS_ALLOCATOR(D6ApiJointLimitConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(D6ApiJointLimitConfiguration, "{88E067B4-21E8-4FFA-9142-6C52605B704C}", AzPhysics::ApiJointConfiguration);
        static void Reflect(AZ::ReflectContext* context);

        float m_swingLimitY = 45.0f; ///< Maximum angle in degrees from the Y axis of the joint frame.
        float m_swingLimitZ = 45.0f; ///< Maximum angle in degrees from the Z axis of the joint frame.
        float m_twistLimitLower = -45.0f; ///< Lower limit in degrees for rotation about the X axis of the joint frame.
        float m_twistLimitUpper = 45.0f; ///< Upper limit in degrees for rotation about the X axis of the joint frame.
    };

    //! Properties that are common for several types of joints.
    struct ApiJointGenericProperties
    {
        enum class GenericApiJointFlag : AZ::u16
        {
            None = 0,
            Breakable = 1,
            SelfCollide = 1 << 1
        };

        AZ_CLASS_ALLOCATOR(ApiJointGenericProperties, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(ApiJointGenericProperties, "{6CB15399-24F6-4F03-AAEF-1AE013B683E0}");
        static void Reflect(AZ::ReflectContext* context);

        ApiJointGenericProperties() = default;
        ApiJointGenericProperties(GenericApiJointFlag flags, float forceMax, float torqueMax);

        bool IsFlagSet(GenericApiJointFlag flag) const; ///< Returns if a particular flag is set as a bool.

        /// Flags that indicates if joint is breakable, self-colliding, etc. 
        /// Converting joint between breakable/non-breakable at game time is allowed.
        GenericApiJointFlag m_flags = GenericApiJointFlag::None;
        float m_forceMax = 1.0f; ///< Max force joint can tolerate before breaking.
        float m_torqueMax = 1.0f; ///< Max torque joint can tolerate before breaking.
    };
    AZ_DEFINE_ENUM_BITWISE_OPERATORS(PhysX::ApiJointGenericProperties::GenericApiJointFlag)

    struct ApiJointLimitProperties
    {
        AZ_CLASS_ALLOCATOR(ApiJointLimitProperties, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(ApiJointLimitProperties, "{31F941CB-6699-48BB-B12D-61874B52B984}");
        static void Reflect(AZ::ReflectContext* context);

        ApiJointLimitProperties() = default;
        ApiJointLimitProperties(
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

    struct FixedApiJointConfiguration : public AzPhysics::ApiJointConfiguration 
    {
        AZ_CLASS_ALLOCATOR(FixedApiJointConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(FixedApiJointConfiguration, "{9BCB368B-8D71-4928-B231-0225907E3BD9}", AzPhysics::ApiJointConfiguration);
        static void Reflect(AZ::ReflectContext* context);

        ApiJointGenericProperties m_genericProperties;
    };

    struct BallApiJointConfiguration : public AzPhysics::ApiJointConfiguration 
    {
        AZ_CLASS_ALLOCATOR(BallApiJointConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(BallApiJointConfiguration, "{C2DE2479-B752-469D-BE05-900CD2CD8481}", AzPhysics::ApiJointConfiguration);
        static void Reflect(AZ::ReflectContext* context);

        ApiJointGenericProperties m_genericProperties;
        ApiJointLimitProperties m_limitProperties;
    };
    
    struct HingeApiJointConfiguration : public AzPhysics::ApiJointConfiguration 
    {
        AZ_CLASS_ALLOCATOR(HingeApiJointConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(HingeApiJointConfiguration, "{FB04198E-0BA5-45C2-8343-66DA28ED45EA}", AzPhysics::ApiJointConfiguration);
        static void Reflect(AZ::ReflectContext* context);

        ApiJointGenericProperties m_genericProperties;
        ApiJointLimitProperties m_limitProperties;
    };
} // namespace PhysX
