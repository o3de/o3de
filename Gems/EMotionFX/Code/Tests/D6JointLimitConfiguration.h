/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Physics/Configuration/JointConfiguration.h>

namespace EMotionFX
{
    // Add so that RagdollNodeInspectorPlugin::PhysXCharactersGemAvailable() will return the correct value
    // We duplicated the D6JointLimitConfiguration because it doesn't exist in the test environment.
    struct D6JointLimitConfiguration
        : public AzPhysics::JointConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(D6JointLimitConfiguration, AZ::SystemAllocator);
        // This uses the same uuid as the production D6JointLimitConfiguration.
        AZ_RTTI(D6JointLimitConfiguration, "{88E067B4-21E8-4FFA-9142-6C52605B704C}", AzPhysics::JointConfiguration);

        static void Reflect(AZ::ReflectContext* context);

        float m_swingLimitY = 45.0f; ///< Maximum angle in degrees from the Y axis of the joint frame.
        float m_swingLimitZ = 45.0f; ///< Maximum angle in degrees from the Z axis of the joint frame.
        float m_twistLimitLower = -45.0f; ///< Lower limit in degrees for rotation about the X axis of the joint frame.
        float m_twistLimitUpper = 45.0f; ///< Upper limit in degrees for rotation about the X axis of the joint frame.
    };
} // namespace EMotionFX
