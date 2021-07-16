/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Color.h>
#include <AzCore/Math/VertexContainerInterface.h>

namespace AzFramework
{
    class DebugDisplayRequests;
}

namespace AzToolsFramework
{
    //! Utility functions for rendering vertex container indices.
    namespace VertexContainerDisplay
    {
        extern const float DefaultVertexTextSize;
        extern const AZ::Color DefaultVertexTextColor;
        extern const AZ::Vector3 DefaultVertexTextOffset;

        //! Displays all vertex container indices as text at the position of each vertex when selected
        template<typename Vertex>
        void DisplayVertexContainerIndices(
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AZ::FixedVertices<Vertex>& vertices,
            const AZ::Transform& transform,
            const AZ::Vector3& nonUniformScale,
            bool selected,
            float textSize = DefaultVertexTextSize,
            const AZ::Color& textColor = DefaultVertexTextColor,
            const AZ::Vector3& textOffset = DefaultVertexTextOffset);
    } // namespace VertexContainerDisplay
} // namespace AzToolsFramework
