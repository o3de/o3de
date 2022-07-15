/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Plugins/Ragdoll/PhysicsSetupManipulators.h>
#include <AzFramework/Physics/Character.h>
#include <AzFramework/Physics/Configuration/JointConfiguration.h>

namespace EMotionFX
{
    bool PhysicsSetupManipulatorData::HasColliders() const
    {
        return m_valid && m_colliderNodeConfiguration && !m_colliderNodeConfiguration->m_shapes.empty();
    }

    bool PhysicsSetupManipulatorData::HasCapsuleCollider() const
    {
        return HasColliders() && m_colliderNodeConfiguration->m_shapes[0].second->GetShapeType() == Physics::ShapeType::Capsule;
    }

    bool PhysicsSetupManipulatorData::HasJointLimit() const
    {
        return m_valid && m_jointConfiguration;
    }

    AZ::Transform PhysicsSetupManipulatorData::GetJointParentFrameWorld() const
    {
        const AZ::Quaternion& parentWorldRotation = m_parentWorldTransform.GetRotation();
        const AZ::Vector3& childWorldTranslation = m_nodeWorldTransform.GetTranslation();
        return AZ::Transform::CreateFromQuaternionAndTranslation(parentWorldRotation, childWorldTranslation) *
            AZ::Transform::CreateFromQuaternion(m_jointConfiguration->m_parentLocalRotation);
    }

    void PhysicsSetupManipulatorsBase::SetViewportId(AZ::s32 viewportId)
    {
        m_viewportId = viewportId;
    }
} // namespace EMotionFX
