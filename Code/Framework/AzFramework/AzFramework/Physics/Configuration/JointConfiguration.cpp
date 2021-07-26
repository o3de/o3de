/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/Configuration/JointConfiguration.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzPhysics
{
    AZ_CLASS_ALLOCATOR_IMPL(JointConfiguration, AZ::SystemAllocator, 0);

    void JointConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<JointConfiguration>()
                ->Version(1)
                ->Field("Name", &JointConfiguration::m_debugName)
                ->Field("ParentLocalRotation", &JointConfiguration::m_parentLocalRotation)
                ->Field("ParentLocalPosition", &JointConfiguration::m_parentLocalPosition)
                ->Field("ChildLocalRotation", &JointConfiguration::m_childLocalRotation)
                ->Field("ChildLocalPosition", &JointConfiguration::m_childLocalPosition)
                ->Field("StartSimulationEnabled", &JointConfiguration::m_startSimulationEnabled)
                ;
        }
    }
}
