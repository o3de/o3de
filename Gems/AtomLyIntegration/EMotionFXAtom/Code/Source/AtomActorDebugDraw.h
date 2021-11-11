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
#include <Integration/Rendering/RenderFlag.h>
#include <Integration/Rendering/RenderActorInstance.h>

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

        void DebugDraw(const EMotionFX::ActorRenderFlagBitset& renderFlags, EMotionFX::ActorInstance* instance);

    private:

        float CalculateScaleMultiplier(EMotionFX::ActorInstance* instance) const;
        void PrepareForMesh(EMotionFX::Mesh* mesh, const AZ::Transform& worldTM);
        void RenderAABB(EMotionFX::ActorInstance* instance, const AZ::Color& aabbColor);
        void RenderSkeleton(EMotionFX::ActorInstance* instance, const AZ::Color& skeletonColor);
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
        void RenderWireframe(EMotionFX::Mesh* mesh, const AZ::Transform& worldTM, float wireframeScale, float scaleMultiplier,
            const AZ::Color& wireframeColor);

        EMotionFX::Mesh* m_currentMesh = nullptr; /**< A pointer to the mesh whose world space positions are in the pre-calculated positions buffer.
                                           NULL in case we haven't pre-calculated any positions yet. */
        AZStd::vector<AZ::Vector3> m_worldSpacePositions; /**< The buffer used to store world space positions for rendering normals
                                                          tangents and the wireframe. */

        RPI::AuxGeomFeatureProcessorInterface* m_auxGeomFeatureProcessor = nullptr;
        AZStd::vector<AZ::Vector3> m_auxVertices;
        AZStd::vector<AZ::Color> m_auxColors;
    };
}
