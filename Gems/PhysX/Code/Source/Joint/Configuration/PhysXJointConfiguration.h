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
    class D6ApiJointLimitConfiguration
        : public AzPhysics::ApiJointConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(D6ApiJointLimitConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(D6ApiJointLimitConfiguration, "{88E067B4-21E8-4FFA-9142-6C52605B704C}", AzPhysics::ApiJointConfiguration);
        static void Reflect(AZ::ReflectContext* context);

        float m_swingLimitY = 45.0f; ///< Maximum angle in degrees from the Y axis of the joint frame.
        float m_swingLimitZ = 45.0f; ///< Maximum angle in degrees from the Z axis of the joint frame.
        float m_twistLimitLower = -45.0f; ///< Lower limit in degrees for rotation about the X axis of the joint frame.
        float m_twistLimitUpper = 45.0f; ///< Upper limit in degrees for rotation about the X axis of the joint frame.
    };

    class GenericApiJointConfiguration : public AzPhysics::ApiJointConfiguration
    {
    public:
        enum class GenericApiJointFlag : AZ::u16
        {
            None = 0,
            Breakable = 1,
            SelfCollide = 1 << 1
        };

        AZ_CLASS_ALLOCATOR(GenericApiJointConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(GenericApiJointConfiguration, "{6CB15399-24F6-4F03-AAEF-1AE013B683E0}", AzPhysics::ApiJointConfiguration);
        static void Reflect(AZ::ReflectContext* context);

        bool GetFlag(GenericApiJointFlag flag) const; ///< Returns if a particular flag is set as a bool.

        GenericApiJointFlag m_flags = GenericApiJointFlag::None; ///< Flags that indicates if joint is breakable, self-colliding, etc.. Converting joint between breakable/non-breakable at game time is allowed.
        float m_forceMax = 1.0f; ///< Max force joint can tolerate before breaking.
        float m_torqueMax = 1.0f; ///< Max torque joint can tolerate before breaking.
    };
    AZ_DEFINE_ENUM_BITWISE_OPERATORS(PhysX::GenericApiJointConfiguration::GenericApiJointFlag)

    class FixedApiJointConfiguration : public GenericApiJointConfiguration 
    {
    public:
        AZ_CLASS_ALLOCATOR(FixedApiJointConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(FixedApiJointConfiguration, "{9BCB368B-8D71-4928-B231-0225907E3BD9}", GenericApiJointConfiguration);
        static void Reflect(AZ::ReflectContext* context);
    };
} // namespace PhysX
