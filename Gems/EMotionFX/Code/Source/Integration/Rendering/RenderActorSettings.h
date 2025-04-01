/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Color.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ::Render
{
    // RenderActorSettings is a subset of RenderOptions. The goal is eventually move those actor render related settings out of render options since
    // it will be shared between main editor and animation editor.
    class RenderActorSettings
    {
    public:
        AZ_RTTI(RenderActorSettings, "{240BDFE2-D7F5-4927-A8CA-D2945E41AFFD}");
        AZ_CLASS_ALLOCATOR(RenderActorSettings, AZ::SystemAllocator)

        virtual ~RenderActorSettings() = default;

        float m_vertexNormalsScale = 1.0f;
        float m_faceNormalsScale = 1.0f;
        float m_tangentsScale = 1.0f;

        //! Use the vertex normals to scale the wireframe a bit to avoid Z-fighting when rendering.
        //! Scale the normal by the m_wireframeScale to push the wireframe a bit above the solide mesh rendering.
        //! Additionally the character bounds will be taken into account, so this is a relative-to the character size value.
        float m_wireframeScale = 0.1f;

        float m_nodeOrientationScale = 1.0f;

        bool m_enabledNodeBasedAabb = true;
        bool m_enabledMeshBasedAabb = true;
        bool m_enabledStaticBasedAabb = true;

        AZ::Color m_hitDetectionColliderColor{0.44f, 0.44f, 0.44f, 1.0f};
        AZ::Color m_selectedHitDetectionColliderColor{ 0.09f, 0.30f, 0.55f, 1.0f };
        AZ::Color m_hoveredHitDetectionColliderColor{ 0.75f, 0.84f, 0.96f, 1.0f };
        AZ::Color m_ragdollColliderColor{ 0.44f, 0.44f, 0.44f, 1.0f };
        AZ::Color m_selectedRagdollColliderColor{ 0.67f, 0.43f, 0.03f, 1.0f };
        AZ::Color m_violatedRagdollColliderColor{ 1.0f, 0.0f, 0.0f, 1.0f };
        AZ::Color m_hoveredRagdollColliderColor{ 0.98f, 0.81f, 0.52f, 1.0f };
        AZ::Color m_violatedJointLimitColor{ 1.0f, 0.0f, 0.0f, 1.0f };
        AZ::Color m_clothColliderColor{ 0.44f, 0.44f, 0.44f, 1.0f };
        AZ::Color m_selectedClothColliderColor{ 0.26f,0.0f, 0.99f, 1.0f };
        AZ::Color m_hoveredClothColliderColor{ 0.89f, 0.86f, 1.0f, 1.0f };
        AZ::Color m_simulatedObjectColliderColor{ 0.44f, 0.44f, 0.44f, 1.0f };
        AZ::Color m_selectedSimulatedObjectColliderColor{ 0.87f, 0.0f, 0.70f, 1.0f };
        AZ::Color m_hoveredSimulatedObjectColliderColor{ 1.0f, 0.81f, 0.96f, 1.0f };

        AZ::Color m_vertexNormalsColor{ 0.0f, 1.0f, 0.0f, 1.0f };
        AZ::Color m_faceNormalsColor{ 0.5f, 0.5f, 1.0f, 1.0f };
        AZ::Color m_tangentsColor{ 1.0f, 0.0f, 0.0f, 1.0f };
        AZ::Color m_mirroredBitangentsColor{ 1.0f, 1.0f, 0.0f, 1.0f };
        AZ::Color m_bitangentsColor{ 1.0f, 1.0f, 1.0f, 1.0f };
        AZ::Color m_wireframeColor{ 0.0f, 0.0f, 0.0f, 1.0f };
        AZ::Color m_lineSkeletonColor{ 0.27f, 0.8f, 0.0f, 1.0f };
        AZ::Color m_skeletonColor{ 0.19f, 0.58f, 0.19f, 1.0f };
        AZ::Color m_jointNameColor{ 1.0f, 1.0f, 1.0f, 1.0f };

        AZ::Color m_nodeAABBColor{ 1.0f, 0.0f, 0.0f, 1.0f };
        AZ::Color m_meshAABBColor{ 0.0f, 0.0f, 0.7f, 1.0f };
        AZ::Color m_staticAABBColor{ 0.0f, 0.7f, 0.7f, 1.0f };
        AZ::Color m_trajectoryHeadColor{ 0.230f, 0.580f, 0.980, 1.0f };
        AZ::Color m_trajectoryPathColor{ 0.184f, 0.494f, 0.866f, 1.0f };
    };
} // namespace AZ::Render
