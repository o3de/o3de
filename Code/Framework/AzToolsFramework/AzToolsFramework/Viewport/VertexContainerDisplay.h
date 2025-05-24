/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/AzToolsFrameworkAPI.h>

#include <AzCore/Math/Color.h>
#include <AzCore/Math/VertexContainerInterface.h>
#include <AzToolsFramework/Viewport/VertexContainerDisplayDefaults.h>

namespace AzFramework
{
    class DebugDisplayRequests;
}

namespace AzToolsFramework
{
    //! Utility functions for rendering vertex container indices.
    namespace VertexContainerDisplay
    {
        //! Displays all vertex container indices as text at the position of each vertex when selected
        template<typename Vertex>
        void DisplayVertexContainerIndices(
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AZ::FixedVertices<Vertex>& vertices,
            const AZ::Transform& transform,
            const AZ::Vector3& nonUniformScale,
            bool selected,
            float textSize = AZ_DEFAULT_VERTEX_TEXT_SIZE,
            const AZ::Color& textColor = AZ_DEFAULT_VERTEX_TEXT_COLOR,
            const AZ::Vector3& textOffset = AZ_DEFAULT_VERTEX_TEXT_OFFSET);
    } // namespace VertexContainerDisplay


    extern template void VertexContainerDisplay::DisplayVertexContainerIndices(
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AZ::FixedVertices<AZ::Vector2>& vertices,
        const AZ::Transform& transform,
        const AZ::Vector3& nonUniformScale,
        bool selected,
        float textSize,
        const AZ::Color& textColor,
        const AZ::Vector3& textOffset);
    extern template void VertexContainerDisplay::DisplayVertexContainerIndices(
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AZ::FixedVertices<AZ::Vector3>& vertices,
        const AZ::Transform& transform,
        const AZ::Vector3& nonUniformScale,
        bool selected,
        float textSize,
        const AZ::Color& textColor,
        const AZ::Vector3& textOffset);

} // namespace AzToolsFramework
