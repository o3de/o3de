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

#include <AzFramework/Physics/Configuration/ApiJointConfiguration.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzPhysics
{
    AZ_CLASS_ALLOCATOR_IMPL(ApiJointConfiguration, AZ::SystemAllocator, 0);

    void ApiJointConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            // serializeContext->ClassDeprecate("WorldBodyConfiguration", "{6EEB377C-DC60-4E10-AF12-9626C0763B2D}", &Internal::DeprecateWorldBodyConfiguration);
            serializeContext->Class<ApiJointConfiguration>()
                ->Version(1)
                ->Field("name", &ApiJointConfiguration::m_debugName)
                ->Field("ParentLocalRotation", &ApiJointConfiguration::m_parentLocalRotation)
                ->Field("ParentLocalPosition", &ApiJointConfiguration::m_parentLocalPosition)
                ->Field("ChildLocalRotation", &ApiJointConfiguration::m_childLocalRotation)
                ->Field("ChildLocalPosition", &ApiJointConfiguration::m_childLocalPosition)
                ->Field("startSimulationEnabled", &ApiJointConfiguration::m_startSimulationEnabled)
                ;
        }
    }
}
