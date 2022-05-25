/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Plugins/Ragdoll/PhysicsSetupManipulators.h>
#include <AzFramework/Physics/Character.h>

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

    void PhysicsSetupManipulatorsBase::SetViewportId(AZ::s32 viewportId)
    {
        m_viewportId = viewportId;
    }
} // namespace EMotionFX
