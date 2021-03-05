/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzFramework/Physics/Configuration/SystemConfiguration.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzPhysics
{
    AZ_CLASS_ALLOCATOR_IMPL(SystemConfiguration, AZ::SystemAllocator, 0);

    /*static*/ void SystemConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SystemConfiguration>()
                ->Version(2)
                ->Field("AutoManageSimulationUpdate", &SystemConfiguration::m_autoManageSimulationUpdate)
                ->Field("MaxTimestep", &SystemConfiguration::m_maxTimestep)
                ->Field("FixedTimeStep", &SystemConfiguration::m_fixedTimestep)
                ->Field("RaycastBufferSize", &SystemConfiguration::m_raycastBufferSize)
                ->Field("ShapecastBufferSize", &SystemConfiguration::m_shapecastBufferSize)
                ->Field("OverlapBufferSize", &SystemConfiguration::m_overlapBufferSize)
                ->Field("CollisionConfig", &SystemConfiguration::m_collisionConfig)
                ;
        }
    }
    
    bool SystemConfiguration::operator==(const SystemConfiguration& other) const
    {
        return m_autoManageSimulationUpdate == other.m_autoManageSimulationUpdate &&
            m_raycastBufferSize == other.m_raycastBufferSize &&
            m_shapecastBufferSize == other.m_shapecastBufferSize &&
            m_overlapBufferSize == other.m_overlapBufferSize &&
            AZ::IsClose(m_maxTimestep, other.m_maxTimestep) &&
            AZ::IsClose(m_fixedTimestep, other.m_fixedTimestep) &&
            m_collisionConfig == other.m_collisionConfig
            ;
    }

    bool SystemConfiguration::operator!=(const SystemConfiguration& other) const
    {
        return !(*this == other);
    }
}
