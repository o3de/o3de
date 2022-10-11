/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Physics/Character.h>
#include <AzFramework/Physics/Ragdoll.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

namespace Physics
{
    //! Provides debug drawing for character physics configurations, such as colliders and joint limits.
    class CharacterPhysicsDebugDraw
    {
    public:
        static constexpr float s_scale = 0.1f;
        static constexpr AZ::u32 s_angularSubdivisions = 32;
        static constexpr AZ::u32 s_radialSubdivisions = 2;

        //! Location and visibility data etc required to debug draw a CharacterColliderNodeConfiguration.
        struct NodeDebugDrawData
        {
            AZ::Transform m_worldTransform = AZ::Transform::CreateIdentity();
            AZ::Vector3 m_scale = AZ::Vector3::CreateOne();
            bool m_selected = false;
            bool m_hovered = false;
            bool m_valid = false;
        };

        //! Location and visibility data etc required to debug draw a JointConfiguration.
        struct JointDebugDrawData
        {
            AZ::Quaternion m_childModelSpaceOrientation = AZ::Quaternion::CreateIdentity();
            AZ::Quaternion m_parentModelSpaceOrientation = AZ::Quaternion::CreateIdentity();
            AZ::Transform m_childWorldTransform = AZ::Transform::CreateIdentity();
            AZ::Transform m_parentWorldTransform = AZ::Transform::CreateIdentity();
            bool m_selected = false;
            bool m_visible = false;
            bool m_valid = false;
        };

        //! Buffers to store vertices, line colors etc for debug drawing a joint limit.
        struct JointLimitRenderBuffers
        {
            AZStd::vector<AZ::Vector3> m_vertexBuffer;
            AZStd::vector<AZ::u32> m_indexBuffer;
            AZStd::vector<AZ::Vector3> m_lineBuffer;
            AZStd::vector<bool> m_lineValidityBuffer;

            void Clear();
        };

        //! Color settings for character physics debug drawing, such as default and selected colors.
        struct ColorSettings
        {
            AZ::Color m_defaultColor;
            AZ::Color m_selectedColor;
            AZ::Color m_hoveredColor;
            AZ::Color m_errorColor;
        };

        //! Function allowing callers to specify where and how to debug draw a CharacterColliderNodeConfiguration.
        using NodeDebugDrawDataFunction = AZStd::function<NodeDebugDrawData(const CharacterColliderNodeConfiguration& colliderNodeConfig)>;

        //! Debug draw all the colliders attached to a single CharacterColliderNodeConfiguration.
        void RenderColliders(
            AzFramework::DebugDisplayRequests* debugDisplay,
            const Physics::CharacterColliderNodeConfiguration& nodeConfig,
            NodeDebugDrawDataFunction nodeDebugDrawDataFunction,
            const ColorSettings& colorSettings,
            uint32_t invalidShapeBitArray = 0);

        //! Debug draw all the colliders for an entire CharacterColliderConfiguration.
        void RenderColliders(
            AzFramework::DebugDisplayRequests* debugDisplay,
            const Physics::CharacterColliderConfiguration* colliderConfig,
            NodeDebugDrawDataFunction nodeDebugDrawDataFunction,
            const ColorSettings& colorSettings);

        //! Debug draw all the colliders for an entire ragdoll
        void RenderRagdollColliders(
            AzFramework::DebugDisplayRequests* debugDisplay,
            const Physics::CharacterColliderConfiguration* colliderConfig,
            NodeDebugDrawDataFunction nodeDebugDrawDataFunction,
            const ParentIndices& parentIndices,
            const ColorSettings& colorSettings);

        using JointDebugDrawDataFunction = AZStd::function<JointDebugDrawData(const RagdollNodeConfiguration& ragdollNodeConfig)>;

        //! Debug draw the joint frame for a single RagdollNodeConfiguration.
        void RenderJointFrame(
            AzFramework::DebugDisplayRequests* debugDisplay,
            const RagdollNodeConfiguration& ragdollNodeConfig,
            const JointDebugDrawData& jointDebugDrawData,
            const ColorSettings& colorSettings);

        //! Debug draw the joint limit for a single RagdollNodeConfiguration.
        void RenderJointLimit(
            AzFramework::DebugDisplayRequests* debugDisplay,
            const RagdollNodeConfiguration& ragdollNodeConfig,
            const JointDebugDrawData& jointDebugDrawData,
            const ColorSettings& colorSettings);

        //! Debug draw the joint limits for an entire RagdollConfiguration.
        void RenderJointLimits(
            AzFramework::DebugDisplayRequests* debugDisplay,
            const Physics::RagdollConfiguration& ragdollConfig,
            JointDebugDrawDataFunction jointDebugDrawDataFunction,
            const ColorSettings& colorSettings);
    private:
        JointLimitRenderBuffers m_jointLimitRenderBuffers;
    };
} // namespace Physics
