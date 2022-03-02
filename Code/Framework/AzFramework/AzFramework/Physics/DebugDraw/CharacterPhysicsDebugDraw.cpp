/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/DebugDraw/CharacterPhysicsDebugDraw.h>
#include <AzFramework/Physics/Common/PhysicsJoint.h>
#include <AzCore/Interface/Interface.h>

namespace Physics
{
    void CharacterPhysicsDebugDraw::JointLimitRenderBuffers::Clear()
    {
        m_vertexBuffer.clear();
        m_indexBuffer.clear();
        m_lineBuffer.clear();
        m_lineValidityBuffer.clear();
    }

    void CharacterPhysicsDebugDraw::RenderColliders(
        AzFramework::DebugDisplayRequests* debugDisplay,
        const Physics::CharacterColliderNodeConfiguration& nodeConfig,
        NodeDebugDrawDataFunction nodeDebugDrawDataFunction,
        const ColorSettings& colorSettings)
    {
        NodeDebugDrawData nodeDebugDrawData = nodeDebugDrawDataFunction(nodeConfig);
        if (!nodeDebugDrawData.m_valid)
        {
            return;
        }

        const AZ::Color colliderColor = nodeDebugDrawData.m_selected ? colorSettings.m_selectedColor : colorSettings.m_defaultColor;
        debugDisplay->DepthTestOff();
        debugDisplay->SetColor(colliderColor);

        const AzPhysics::ShapeColliderPairList& colliders = nodeConfig.m_shapes;
        for (const auto& collider : colliders)
        {
            const AZ::Transform colliderOffsetTransform =
                AZ::Transform::CreateFromQuaternionAndTranslation(collider.first->m_rotation, collider.first->m_position);
            const AZ::Transform colliderGlobalTransform = nodeDebugDrawData.m_worldTransform * colliderOffsetTransform;

            const AZ::TypeId colliderType = collider.second->RTTI_GetType();
            if (colliderType == azrtti_typeid<Physics::SphereShapeConfiguration>())
            {
                auto* sphere = static_cast<Physics::SphereShapeConfiguration*>(collider.second.get());
                debugDisplay->DrawWireSphere(colliderGlobalTransform.GetTranslation(), sphere->m_radius);
            }
            else if (colliderType == azrtti_typeid<Physics::CapsuleShapeConfiguration>())
            {
                auto* capsule = static_cast<Physics::CapsuleShapeConfiguration*>(collider.second.get());
                debugDisplay->DrawWireCapsule(
                    colliderGlobalTransform.GetTranslation(), colliderGlobalTransform.GetBasisZ(), capsule->m_radius,
                    capsule->m_height);
            }
            else if (colliderType == azrtti_typeid<Physics::BoxShapeConfiguration>())
            {
                auto* box = static_cast<Physics::BoxShapeConfiguration*>(collider.second.get());
                debugDisplay->DrawWireOBB(
                    colliderGlobalTransform.GetTranslation(), colliderGlobalTransform.GetBasisX(),
                    colliderGlobalTransform.GetBasisY(), colliderGlobalTransform.GetBasisZ(), 0.5f * box->m_dimensions);
            }
        }
    }

    void CharacterPhysicsDebugDraw::RenderColliders(
        AzFramework::DebugDisplayRequests* debugDisplay,
        const Physics::CharacterColliderConfiguration* colliderConfig,
        NodeDebugDrawDataFunction nodeDebugDrawDataFunction,
        const ColorSettings& colorSettings)
    {
        for (const Physics::CharacterColliderNodeConfiguration& nodeConfig : colliderConfig->m_nodes)
        {
            RenderColliders(debugDisplay, nodeConfig, nodeDebugDrawDataFunction, colorSettings);
        }
    }

    void CharacterPhysicsDebugDraw::RenderJointFrame(
        AzFramework::DebugDisplayRequests* debugDisplay,
        const RagdollNodeConfiguration& ragdollNodeConfig,
        const JointDebugDrawData& jointDebugDrawData,
        const ColorSettings& colorSettings)
    {
        const AZ::Transform jointChildWorldSpaceTransform = jointDebugDrawData.m_childWorldTransform *
            AZ::Transform::CreateFromQuaternion(ragdollNodeConfig.m_jointConfig->m_childLocalRotation);
        const AZ::Vector3 dir = jointChildWorldSpaceTransform.GetBasisX();

        debugDisplay->SetColor(colorSettings.m_selectedColor);
        debugDisplay->DrawArrow(jointChildWorldSpaceTransform.GetTranslation(), jointChildWorldSpaceTransform.GetTranslation() + dir, 0.1f);
    };

    void CharacterPhysicsDebugDraw::RenderJointLimit(
        AzFramework::DebugDisplayRequests* debugDisplay,
        const RagdollNodeConfiguration& ragdollNodeConfig,
        const JointDebugDrawData& jointDebugDrawData,
        const ColorSettings& colorSettings)
    {
        m_jointLimitRenderBuffers.Clear();

        if (auto* jointHelpers = AZ::Interface<AzPhysics::JointHelpersInterface>::Get())
        {
            jointHelpers->GenerateJointLimitVisualizationData(
                *ragdollNodeConfig.m_jointConfig, jointDebugDrawData.m_parentModelSpaceOrientation,
                jointDebugDrawData.m_childModelSpaceOrientation, s_scale, s_angularSubdivisions, s_radialSubdivisions,
                m_jointLimitRenderBuffers.m_vertexBuffer, m_jointLimitRenderBuffers.m_indexBuffer,
                m_jointLimitRenderBuffers.m_lineBuffer, m_jointLimitRenderBuffers.m_lineValidityBuffer);
        }

        const size_t numLineBufferEntries = m_jointLimitRenderBuffers.m_lineBuffer.size();
        if (m_jointLimitRenderBuffers.m_lineValidityBuffer.size() * 2 != numLineBufferEntries)
        {
            AZ_ErrorOnce(
                "Ragdoll Debug Draw", false, "Unexpected buffer size in joint limit visualization for node %s",
                ragdollNodeConfig.m_debugName.c_str());
            return;
        }

        for (size_t i = 0; i < numLineBufferEntries; i += 2)
        {
            const AZ::Color& lineColor =
                m_jointLimitRenderBuffers.m_lineValidityBuffer[i / 2] ? colorSettings.m_selectedColor : colorSettings.m_errorColor;
            debugDisplay->DepthTestOff();
            debugDisplay->DrawLine(
                jointDebugDrawData.m_parentWorldTransform.TransformPoint(m_jointLimitRenderBuffers.m_lineBuffer[i]),
                jointDebugDrawData.m_parentWorldTransform.TransformPoint(m_jointLimitRenderBuffers.m_lineBuffer[i + 1]),
                lineColor.GetAsVector4(), lineColor.GetAsVector4());
        }
    }

    void CharacterPhysicsDebugDraw::RenderJointLimits(AzFramework::DebugDisplayRequests* debugDisplay,
        const Physics::RagdollConfiguration& ragdollConfig,
        JointDebugDrawDataFunction jointDebugDrawDataFunction,
        const ColorSettings& colorSettings)
    {
        for (const auto& ragdollNodeConfig : ragdollConfig.m_nodes)
        {
            JointDebugDrawData jointDebugDrawData = jointDebugDrawDataFunction(ragdollNodeConfig);
            if (!jointDebugDrawData.m_valid || !jointDebugDrawData.m_visible)
            {
                continue;
            }

            RenderJointLimit(debugDisplay, ragdollNodeConfig, jointDebugDrawData, colorSettings);
            RenderJointFrame(debugDisplay, ragdollNodeConfig, jointDebugDrawData, colorSettings);
        }
    }
} // namespace Physics
