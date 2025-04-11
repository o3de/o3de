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
#include <AzFramework/Physics/DebugDraw/CharacterPhysicsDebugDraw.h>
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
        AZ_CLASS_ALLOCATOR(AtomActorDebugDraw, AZ::SystemAllocator)

        struct TrajectoryPathParticle
        {
            EMotionFX::Transform m_worldTm;
        };

        struct TrajectoryTracePath
        {
            AZ_CLASS_ALLOCATOR(AtomActorDebugDraw::TrajectoryTracePath, AZ::SystemAllocator)
            AZStd::vector<TrajectoryPathParticle> m_traceParticles;
            const EMotionFX::ActorInstance* m_actorInstance = nullptr;
            float m_timePassed = 0.0f;
        };

        AtomActorDebugDraw(AZ::EntityId entityId);

        void UpdateActorInstance(EMotionFX::ActorInstance* actorInstance, float deltaTime);
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
        void RenderTrajectoryPath(
            AzFramework::DebugDisplayRequests* debugDisplay,
            const EMotionFX::ActorInstance* actorInstance,
            const AZ::Color& headColor,
            const AZ::Color& pathColor);
        void RenderRootMotion(
            AzFramework::DebugDisplayRequests* debugDisplay,
            const EMotionFX::ActorInstance* actorInstance,
            const AZ::Color& rootColor);
        // Return a non-owning trajectory path pointer.
        TrajectoryTracePath* FindTrajectoryPath(const EMotionFX::ActorInstance* actorInstance);
        EMotionFX::Mesh* m_currentMesh = nullptr; //!< A pointer to the mesh whose world space positions are in the pre-calculated positions buffer.
                                                  //!< NULL in case we haven't pre-calculated any positions yet.
        AZStd::vector<AZ::Vector3> m_worldSpacePositions; //!< The buffer used to store world space positions for rendering normals
                                                          //!< tangents and the wireframe.

        //! Checks if a joint is selected or hovered and updates the color appropriately.
        AZ::Color GetModifiedColor(
            const AZ::Color& color,
            size_t jointIndex,
            const AZStd::unordered_set<size_t>* cachedSelectedJointIndices,
            size_t cachedHoveredJointIndex) const;

        static constexpr float BaseFontSize = 0.7f;
        const Vector3 TopRightBorderPadding = AZ::Vector3(-40.0f, 22.0f, 0.0f);
        const AZ::Color SelectedColor = AZ::Color{ 1.0f, 0.67f, 0.0f, 1.0f };
        const AZ::Color HoveredColor = AZ::Color{ 0.78f, 1.0f, 0.67f, 1.0f };

        RPI::AuxGeomFeatureProcessorInterface* m_auxGeomFeatureProcessor = nullptr;
        AZStd::vector<AZ::Vector3> m_auxVertices;
        AZStd::vector<AZ::Color> m_auxColors;
        EntityId m_entityId;

        Physics::CharacterPhysicsDebugDraw m_characterPhysicsDebugDraw;
        // Motion extraction paths
        AZStd::vector<AZStd::unique_ptr<TrajectoryTracePath>> m_trajectoryTracePaths;

        AzFramework::TextDrawParameters m_drawParams;
        AzFramework::FontDrawInterface* m_fontDrawInterface = nullptr;
    };
}
