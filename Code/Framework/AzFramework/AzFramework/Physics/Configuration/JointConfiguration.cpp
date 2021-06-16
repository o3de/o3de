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
