/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <DebugDraw.h>
#include <AzCore/Math/Color.h>
#include <AzCore/std/containers/vector.h>
#include <Components/RecastHelpers.h>
#include <Components/RecastSmartPointer.h>

namespace RecastNavigation
{
    class RecastNavigationDebugDraw final : public duDebugDraw
    {
    public:
        ~RecastNavigationDebugDraw() override = default;

        void depthMask(bool state) override;

        void texture(bool state) override;

        void begin(duDebugDrawPrimitives prim, float size = 1.0f) override;

        void vertex(const float* pos, unsigned int color) override;

        void vertex(const float x, const float y, const float z, unsigned int color) override;

        void vertex(const float* pos, unsigned int color, const float* uv) override;

        void vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v) override;

        void end() override;

        void SetColor(const AZ::Color& color);

    protected:
        void AddVertex(float x, float y, float z, unsigned int color);

        AZ::Color m_currentColor{ 1.F, 1, 1, 1 };

        duDebugDrawPrimitives m_currentPrim = DU_DRAW_POINTS;

        AZStd::vector<AZStd::pair<AZ::Vector3, AZ::u32>> m_verticesToDraw;
    };
} // namespace RecastNavigation
