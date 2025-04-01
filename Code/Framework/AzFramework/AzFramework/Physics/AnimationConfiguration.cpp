/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/AnimationConfiguration.h>
#include <AzCore/Serialization/EditContext.h>


namespace Physics
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimationConfiguration, AZ::SystemAllocator)

    void AnimationConfiguration::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<AnimationConfiguration>()
                ->Version(3)
                ->Field("hitDetectionConfig", &AnimationConfiguration::m_hitDetectionConfig)
                ->Field("ragdollConfig", &AnimationConfiguration::m_ragdollConfig)
                ->Field("clothConfig", &AnimationConfiguration::m_clothConfig)
                ->Field("simulatedObjectColliderConfig", &AnimationConfiguration::m_simulatedObjectColliderConfig)
            ;
        }
    }
} // Physics
