/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/Common/PhysicsJoint.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/PhysicsSystem.h>

namespace AzPhysics
{
    AZ_CLASS_ALLOCATOR_IMPL(Joint, AZ::SystemAllocator);

    void Joint::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azdynamic_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AzPhysics::Joint>()
                ->Version(1)
                ->Field("SceneOwner", &Joint::m_sceneOwner)
                ->Field("JointHandle", &Joint::m_jointHandle)
                ;
        }
    }
}
