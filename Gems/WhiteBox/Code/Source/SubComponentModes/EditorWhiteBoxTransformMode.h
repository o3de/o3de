
#pragma once

#include "EditorWhiteBoxComponentModeTypes.h"

namespace WhiteBox 
{
    struct IntersectionAndRenderData;

    class TransformMode 
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

            TransformMode() = default;
            TransformMode(TransformMode&&) = default;
            TransformMode& operator=(TransformMode&&) = default;
            ~TransformMode() = default;

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

    };

}