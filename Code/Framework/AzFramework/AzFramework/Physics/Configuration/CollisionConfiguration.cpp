/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/Configuration/CollisionConfiguration.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzPhysics
{
    AZ_CLASS_ALLOCATOR_IMPL(CollisionConfiguration, AZ::SystemAllocator);

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

