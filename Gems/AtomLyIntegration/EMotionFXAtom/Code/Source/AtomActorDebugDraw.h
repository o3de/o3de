/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/Math/Color.h>
#include <AzFramework/Font/FontInterface.h>
#include <Integration/Rendering/RenderFlag.h>
#include <Integration/Rendering/RenderActorInstance.h>
#include <Atom/RPI.Public/ViewportContext.h>

namespace AzFramework
{
    class DebugDisplayRequests;
}

namespace EMotionFX
{
    class Mesh;
    class ActorInstance;
}

namespace AZ::RPI
{
    class AuxGeomDraw;
    class AuxGeomFeatureProcessorInterface;
}

namespace AZ::Render
{
    // Ultility class for atom debug render on actor
    class AtomActorDebugDraw
    {
    public:
        AtomActorDebugDraw(AZ::EntityId entityId);

        void DebugDraw(const EMotionFX::ActorRenderFlags& renderFlags, EMotionFX::ActorInstance* instance);

    private:
        float CalculateBoneScale(EMotionFX::ActorInstance* actorInstance, EMotionFX::Node* node);
        float CalculateScaleMultiplier(EMotionFX::ActorInstance* instance) const;
        void PrepareForMesh(EMotionFX::Mesh* mesh, const AZ::Transform& worldTM);
        AzFramework::DebugDisplayRequests* GetDebugDisplay(AzFramework::ViewportId viewportId);

        void RenderAABB(EMotionFX::ActorInstance* instance,
            bool enableNodeAabb,
            const AZ::Color& nodeAabbColor,
            bool enableMeshAabb,
            const AZ::Color& meshAabbColor,
            bool enableStaticAabb,
            const AZ::Color& staticAabbColor);
        void RenderLineSkeleton(AzFramework::DebugDisplayRequests* debugDisplay,
            EMotionFX::ActorInstance* instance,
            const AZ::Color& skeletonColor) const;
        void RenderSkeleton(AzFramework::DebugDisplayRequests* debugDisplay,
            EMotionFX::ActorInstance* instance,
            const AZ::Color& color);
        void RenderEMFXDebugDraw(EMotionFX::ActorInstance* instance);
        void RenderNormals(
            EMotionFX::Mesh* mesh,
            const AZ::Transform& worldTM,
            bool vertexNormals,
            bool faceNormals,
            float vertexNormalsScale,
            float faceNormalsScale,
            float scaleMultiplier,
            const AZ::Color& vertexNormalsColor,
            const AZ::Color& faceNormalsColor);
        void RenderTangents(
            EMotionFX::Mesh* mesh, const AZ::Transform& worldTM, float tangentsScale, float scaleMultiplier,
            const AZ::Color& tangentsColor, const AZ::Color& mirroredBitangentsColor, const AZ::Color& bitangentsColor);
        void RenderWireframe(EMotionFX::Mesh* mesh, const AZ::Transform& worldTM,
            float scale, const AZ::Color& color);
        void RenderJointNames(EMotionFX::ActorInstance* actorInstance, RPI::ViewportContextPtr viewportContext, const AZ::Color& jointNameColor);
        void RenderNodeOrientations(EMotionFX::ActorInstance* actorInstance, AzFramework::DebugDisplayRequests* debugDisplay, float scale = 1.0f);
        void RenderLineAxis(
            AzFramework::DebugDisplayRequests* debugDisplay,
            AZ::Transform worldTM,      //!< The world space transformation matrix to visualize. */
            float size,                 //!< The size value in units is used to control the scaling of the axis. */
            bool selected,              //!< Set to true if you want to render the axis using the selection color. */
            bool renderAxisName = false);

        void RenderColliders(AzFramework::DebugDisplayRequests* debugDisplay,
            const AzPhysics::ShapeColliderPairList& colliders,
            const EMotionFX::ActorInstance* actorInstance,
            const EMotionFX::Node* node,
            const AZ::Color& colliderColor) const;

        void RenderColliders(AzFramework::DebugDisplayRequests* debugDisplay,
            EMotionFX::PhysicsSetup::ColliderConfigType colliderConfigType,
            EMotionFX::ActorInstance* actorInstance,
            const AZ::Color& defaultColor,
            const AZ::Color& selectedColor) const;

        void RenderJointFrame(AzFramework::DebugDisplayRequests* debugDisplay,
            const AzPhysics::JointConfiguration& configuration,
            const EMotionFX::ActorInstance* actorInstance,
            const EMotionFX::Node* node,
            const AZ::Color& color) const;

        // Ragdoll
        void RenderJointLimit(AzFramework::DebugDisplayRequests* debugDisplay,
            const AzPhysics::JointConfiguration& configuration,
            const EMotionFX::ActorInstance* actorInstance,
            const EMotionFX::Node* node,
            const EMotionFX::Node* parentNode,
            const AZ::Color& regularColor,
            const AZ::Color& violatedColor);
        void RenderRagdoll(AzFramework::DebugDisplayRequests* debugDisplay,
            EMotionFX::ActorInstance* actorInstance,
            bool renderColliders,
            bool renderJointLimits);

        EMotionFX::Mesh* m_currentMesh = nullptr; //!< A pointer to the mesh whose world space positions are in the pre-calculated positions buffer.
                                                  //!< NULL in case we haven't pre-calculated any positions yet.
        AZStd::vector<AZ::Vector3> m_worldSpacePositions; //!< The buffer used to store world space positions for rendering normals
                                                          //!< tangents and the wireframe.

        static constexpr float BaseFontSize = 0.7f;
        const Vector3 TopRightBorderPadding = AZ::Vector3(-40.0f, 22.0f, 0.0f);
        const AZ::Color SelectedColor = AZ::Color{ 1.0f, 0.67f, 0.0f, 1.0f };

        RPI::AuxGeomFeatureProcessorInterface* m_auxGeomFeatureProcessor = nullptr;
        AZStd::vector<AZ::Vector3> m_auxVertices;
        AZStd::vector<AZ::Color> m_auxColors;
        EntityId m_entityId;

        // Joint limits
        static constexpr float s_scale = 0.1f;
        static constexpr AZ::u32 s_angularSubdivisions = 32;
        static constexpr AZ::u32 s_radialSubdivisions = 2;

        struct JointLimitRenderData
        {
            AZStd::vector<AZ::Vector3>  m_vertexBuffer;
            AZStd::vector<AZ::u32>      m_indexBuffer;
            AZStd::vector<AZ::Vector3>  m_lineBuffer;
            AZStd::vector<bool>         m_lineValidityBuffer;

            void Clear();
        };
        JointLimitRenderData m_jointLimitRenderData;

        AzFramework::TextDrawParameters m_drawParams;
        AzFramework::FontDrawInterface* m_fontDrawInterface = nullptr;
    };
}
