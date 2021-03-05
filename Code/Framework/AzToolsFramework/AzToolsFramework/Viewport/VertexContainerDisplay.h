/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
    /// Utility functions for rendering vertex container indices.
    namespace VertexContainerDisplay
    {
        extern const float DefaultVertexTextSize;
        extern const AZ::Color DefaultVertexTextColor;
        extern const AZ::Vector3 DefaultVertexTextOffset;

        /// Displays all vertex container indices as text at the position of each vertex when selected
        template<typename Vertex>
        void DisplayVertexContainerIndices(
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AZ::FixedVertices<Vertex>& vertices,
            const AZ::Transform& transform,
            bool selected, float textSize = DefaultVertexTextSize,
            const AZ::Color& textColor = DefaultVertexTextColor,
            const AZ::Vector3& textOffset = DefaultVertexTextOffset);
    }
} // namespace AzToolsFramework