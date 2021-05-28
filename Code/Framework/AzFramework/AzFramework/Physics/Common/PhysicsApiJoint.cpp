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

#include <AzFramework/Physics/Common/PhysicsApiJoint.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/PhysicsSystem.h>

namespace AzPhysics
{
    AZ_CLASS_ALLOCATOR_IMPL(ApiJoint, AZ::SystemAllocator, 0);

    void ApiJoint::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azdynamic_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AzPhysics::ApiJoint>()
                ->Version(1)
                ->Field("SceneOwner", &ApiJoint::m_sceneOwner)
                ->Field("JointHandle", &ApiJoint::m_jointHandle)
                ;
        }
    }
}
