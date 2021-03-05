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

#include "EditorWhiteBoxComponentModeTypes.h"

namespace AZ
{
    class EntityComponentIdPair;
}

namespace AzFramework
{
    struct ViewportInfo;
}

namespace AzToolsFramework
{
    namespace ViewportInteraction
    {
        struct MouseInteractionEvent;
    }

    struct ActionOverride;
} // namespace AzToolsFramework

namespace WhiteBox
{
    struct IntersectionAndRenderData;

    //! The mode where 'mesh' edges can be promoted/restored to 'user' edges so the user can interact
    //! with them again or form new polygons to manipulate.
    class EdgeRestoreMode
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        EdgeRestoreMode() = default;
        EdgeRestoreMode(EdgeRestoreMode&&) = default;
        EdgeRestoreMode& operator=(EdgeRestoreMode&&) = default;
        ~EdgeRestoreMode() = default;

        // Submode interface
        void Refresh();
        AZStd::vector<AzToolsFramework::ActionOverride> PopulateActions(
            const AZ::EntityComponentIdPair& entityComponentIdPair);
        void Display(
            const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Transform& worldFromLocal,
            const IntersectionAndRenderData& renderData, const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay);
        bool HandleMouseInteraction(
            const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction,
            const AZ::EntityComponentIdPair& entityComponentIdPair,
            const AZStd::optional<EdgeIntersection>& edgeIntersection,
            const AZStd::optional<PolygonIntersection>& polygonIntersection,
            const AZStd::optional<VertexIntersection>& vertexIntersection);

    private:
        AZStd::optional<EdgeIntersection> m_edgeIntersection; //!< The hovered edge if one exists.
        AZStd::optional<VertexIntersection> m_vertexIntersection; //!< The hovered vertex if one exists.
        Api::EdgeHandles m_edgeHandlesBeingRestored; //!< The edge handles currently attempting to be restored.
    };
} // namespace WhiteBox
