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

#include <AzFramework/Physics/Configuration/CollisionConfiguration.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzPhysics
{
    AZ_CLASS_ALLOCATOR_IMPL(CollisionConfiguration, AZ::SystemAllocator, 0);

    void CollisionConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CollisionConfiguration>()
                ->Version(1)
                ->Field("Layers", &CollisionConfiguration::m_collisionLayers)
                ->Field("Groups", &CollisionConfiguration::m_collisionGroups)
                ;
        }
    }

    bool CollisionConfiguration::operator==(const CollisionConfiguration& other) const
    {
        return m_collisionLayers == other.m_collisionLayers &&
            m_collisionGroups == other.m_collisionGroups
            ;
    }

    bool CollisionConfiguration::operator!=(const CollisionConfiguration& other) const
    {
        return !(*this == other);
    }
}

