/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "VertexContainerDisplay.h"

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

namespace AzToolsFramework
{
    namespace VertexContainerDisplay
    {
        const float DefaultVertexTextSize = 1.5f;
        const AZ::Color DefaultVertexTextColor = AZ::Color(1.0f, 1.0f, 1.0f, 1.0f);
        const AZ::Vector3 DefaultVertexTextOffset = AZ::Vector3(0.0f, 0.0f, -0.1f);

        void DisplayVertexContainerIndex(
            AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Vector3& position, const size_t index, const float textSize)
        {
            AZStd::string indexFormat = AZStd::string::format("[%zu]", index);
            debugDisplay.DrawTextLabel(position, textSize, indexFormat.c_str(), true);
        }

        template<typename Vertex>
        void DisplayVertexContainerIndices(
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AZ::FixedVertices<Vertex>& vertices,
            const AZ::Transform& transform,
            const AZ::Vector3& nonUniformScale,
            const bool selected,
            const float textSize,
            const AZ::Color& textColor,
            const AZ::Vector3& textOffset)
        {
            if (!selected)
            {
                return;
            }

            debugDisplay.SetColor(textColor);
            for (size_t vertIndex = 0; vertIndex < vertices.Size(); ++vertIndex)
            {
                Vertex vertex;
                if (vertices.GetVertex(vertIndex, vertex))
                {
                    DisplayVertexContainerIndex(
                        debugDisplay, transform.TransformPoint(nonUniformScale * (AdaptVertexOut(vertex) + textOffset)), vertIndex,
                        textSize);
                }
            }
        }
    } // namespace VertexContainerDisplay

    // explicit template instantiations
    template void VertexContainerDisplay::DisplayVertexContainerIndices(
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AZ::FixedVertices<AZ::Vector2>& vertices,
        const AZ::Transform& transform,
        const AZ::Vector3& nonUniformScale,
        bool selected,
        float textSize,
        const AZ::Color& textColor,
        const AZ::Vector3& textOffset);
    template void VertexContainerDisplay::DisplayVertexContainerIndices(
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AZ::FixedVertices<AZ::Vector3>& vertices,
        const AZ::Transform& transform,
        const AZ::Vector3& nonUniformScale,
        bool selected,
        float textSize,
        const AZ::Color& textColor,
        const AZ::Vector3& textOffset);
} // namespace AzToolsFramework
