/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/Configuration/StaticRigidBodyConfiguration.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/ShapeConfiguration.h>

namespace AzPhysics
{
    AZ_CLASS_ALLOCATOR_IMPL(StaticRigidBodyConfiguration, AZ::SystemAllocator);

    void StaticRigidBodyConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<StaticRigidBodyConfiguration, SimulatedBodyConfiguration>()
                ->Version(1)
                ->Field("ColliderAndShapeData", &StaticRigidBodyConfiguration::m_colliderAndShapeData);
                ;
        }
    }
}
