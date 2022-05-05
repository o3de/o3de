/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Plugins/Ragdoll/RagdollColliderTranslationManipulators.h>
#include <AzCore/Debug/Trace.h>
#include <AzFramework/Physics/Character.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>

namespace EMotionFX
{
    RagdollColliderTranslationManipulators::RagdollColliderTranslationManipulators()
        : m_translationManipulators(AzToolsFramework::TranslationManipulators::Dimensions::Three, AZ::Transform::CreateIdentity(), AZ::Vector3::CreateOne())
    {
    }

    void RagdollColliderTranslationManipulators::Setup(RagdollManipulatorData& ragdollManipulatorData)
    {
        m_ragdollManipulatorData = ragdollManipulatorData;

        if (!m_ragdollManipulatorData.m_valid || !m_ragdollManipulatorData.m_colliderNodeConfiguration ||
            m_ragdollManipulatorData.m_colliderNodeConfiguration->m_shapes.empty())
        {
            return;
        }

        m_translationManipulators.SetSpace(m_ragdollManipulatorData.m_nodeWorldTransform);
        m_translationManipulators.SetLocalPosition(m_ragdollManipulatorData.m_colliderNodeConfiguration->m_shapes[0].first->m_position);
        m_translationManipulators.Register(EMStudio::g_animManipulatorManagerId);
        AzToolsFramework::ConfigureTranslationManipulatorAppearance3d(&m_translationManipulators);

        m_translationManipulators.InstallLinearManipulatorMouseMoveCallback([this](
            const AzToolsFramework::LinearManipulator::Action& action)
            {
                OnManipulatorMoved(action.m_start.m_localPosition, action.m_current.m_localPositionOffset);
            });

        m_translationManipulators.InstallPlanarManipulatorMouseMoveCallback([this](
            const AzToolsFramework::PlanarManipulator::Action& action)
            {
                OnManipulatorMoved(action.m_start.m_localPosition, action.m_current.m_localOffset);
            });

        m_translationManipulators.InstallSurfaceManipulatorMouseMoveCallback([this](
            const AzToolsFramework::SurfaceManipulator::Action& action)
            {
                OnManipulatorMoved(action.m_start.m_localPosition, action.m_current.m_localOffset);
            });
    }

    void RagdollColliderTranslationManipulators::Refresh(RagdollManipulatorData& ragdollManipulatorData)
    {
        (void)ragdollManipulatorData;
    }

    void RagdollColliderTranslationManipulators::Teardown()
    {
        m_translationManipulators.Unregister();
    }

    void RagdollColliderTranslationManipulators::ResetValues(RagdollManipulatorData& ragdollManipulatorData)
    {
        (void)ragdollManipulatorData;
    }

    void RagdollColliderTranslationManipulators::OnManipulatorMoved(const AZ::Vector3& startPosition, const AZ::Vector3& offset)
    {
        const float scale = AZ::GetMax(AZ::MinTransformScale, m_ragdollManipulatorData.m_nodeWorldTransform.GetUniformScale());
        const AZ::Vector3 newPosition = startPosition + offset / scale;
        m_ragdollManipulatorData.m_colliderNodeConfiguration->m_shapes[0].first->m_position = newPosition;
        m_translationManipulators.SetLocalPosition(newPosition);
    }
} // namespace EMotionFX
