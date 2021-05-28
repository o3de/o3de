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

#include <AzFramework/Physics/AnimationConfiguration.h>
#include <AzCore/Serialization/EditContext.h>


namespace Physics
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimationConfiguration, AZ::SystemAllocator, 0)

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
