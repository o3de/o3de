/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "AzCore/Math/Color.h"
#include "EditorWhiteBoxComponentMode.h"
#include "EditorWhiteBoxComponentModeTypes.h"
#include "Viewport/WhiteBoxManipulatorViews.h"

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <Manipulators/TranslationManipulators.h>

namespace AZ
{
    class Transform;
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

    class TransformMode
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        using SelectedTransformModifier = AZStd::variant<AZStd::monostate>;

        TransformMode(const AZ::EntityComponentIdPair& entityComponentIdPair);
        ~TransformMode();

        void Refresh();
        AZStd::vector<AzToolsFramework::ActionOverride> PopulateActions(const AZ::EntityComponentIdPair& entityComponentIdPair);
        void Display(
            const AZ::EntityComponentIdPair& entityComponentIdPair,
            const AZ::Transform& worldFromLocal,
            const IntersectionAndRenderData& renderData,
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay);
        void SetTransformMode(WhiteBox::TransformType type);

        bool HandleMouseInteraction(
            const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction,
            const AZ::EntityComponentIdPair& entityComponentIdPair,
            const AZStd::optional<EdgeIntersection>& edgeIntersection,
            const AZStd::optional<PolygonIntersection>& polygonIntersection,
            const AZStd::optional<VertexIntersection>& vertexIntersection);

    private:
        void DrawFace(AzFramework::DebugDisplayRequests& debugDisplay, WhiteBoxMesh* mesh, const Api::PolygonHandle& polygon, const AZ::Color& color );
        void DrawOutline(AzFramework::DebugDisplayRequests& debugDisplay, WhiteBoxMesh* mesh, const Api::PolygonHandle& polygon, const AZ::Color& color );

        void CreateTranslationManipulators();
        void CreateRotationManipulators();
        void CreateScaleManipulators();

        void RefreshManipulator();
        void DestroyManipulators();

        AZ::EntityComponentIdPair m_entityComponentIdPair; //!< The entity and component id this modifier is associated with.

        SelectedTransformModifier m_transform;
        
        AZStd::shared_ptr<AzToolsFramework::Manipulators> m_manipulator = nullptr;
        AZStd::variant<PolygonIntersection, AZStd::monostate> m_selection = AZStd::monostate{};
        AZStd::optional<PolygonIntersection> m_polygonIntersection = AZStd::nullopt;

        WhiteBox::TransformType m_transformType;
    };

} // namespace WhiteBox
