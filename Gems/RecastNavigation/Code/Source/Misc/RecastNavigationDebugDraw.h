/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <DebugDraw.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/std/containers/vector.h>
#include <RecastNavigation/RecastHelpers.h>
#include <RecastNavigation/RecastSmartPointer.h>

namespace RecastNavigation
{
    //! Recast library specific debug draw that captures and draws the various debug overlays.
    //! See @duDebugDraw for documentation of the methods.
    class RecastNavigationDebugDraw final : public duDebugDraw
    {
    public:
        explicit RecastNavigationDebugDraw(bool drawLines = false);

        //! duDebugDraw overrides...
        //! @{
        //! Not implemented
        void depthMask([[maybe_unused]] bool state) override {}
        //! Not implemented
        void texture([[maybe_unused]] bool state) override {}
        void begin(duDebugDrawPrimitives prim, float size = 1.0f) override;
        void vertex(const float* pos, unsigned int color) override;
        void vertex(float x, float y, float z, unsigned int color) override;
        void vertex(const float* pos, unsigned int color, const float* uv) override;
        void vertex(float x, float y, float z, unsigned int color, float u, float v) override;
        void end() override;
        //! @}

        //! Limit debug draw to a specified volume.
        void SetViewableAabb(const AZ::Aabb& viewAabb);

    protected:
        duDebugDrawPrimitives m_currentPrim = DU_DRAW_POINTS;

        //! Vertices with color information.
        AZStd::vector<AZStd::pair<AZ::Vector3, AZ::u32>> m_verticesToDraw;

        void AddVertex(float x, float y, float z, unsigned int color);

        //! Recast debug draw is quite noisy with lines, disabling them by default.
        bool m_drawLines = false;

        //! Only draw debug view within this volume.
        AZ::Aabb m_viewAabb = AZ::Aabb::CreateFromMinMax(
            -AZ::Vector3(AZ::Constants::FloatMax),
            AZ::Vector3(AZ::Constants::FloatMax));
    };
} // namespace RecastNavigation
