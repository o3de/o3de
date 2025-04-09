/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "EditorWhiteBoxComponentMode.h"
#include "EditorWhiteBoxComponentModeTypes.h"
#include "Viewport/WhiteBoxManipulatorViews.h"

#include <AzCore/Math/Quaternion.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <SubComponentModes/EditorWhiteBoxTransformModeBus.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace AZ
{
    class Color;
    class Transform;
    class EntityComponentIdPair;
} // namespace AZ

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
        : public EditorWhiteBoxTransformModeRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        constexpr static const char* const ManipulatorModeClusterTranslateTooltip = "Switch to translate mode";
        constexpr static const char* const ManipulatorModeClusterRotateTooltip = "Switch to rotate mode";
        constexpr static const char* const ManipulatorModeClusterScaleTooltip = "Switch to scale mode";

        using IntersectionSelection = AZStd::variant<PolygonIntersection, EdgeIntersection, VertexIntersection, AZStd::monostate>;

        TransformMode(const AZ::EntityComponentIdPair& entityComponentIdPair);
        ~TransformMode();

        static void RegisterActionUpdaters();
        static void RegisterActions();
        static void BindActionsToModes(const AZStd::string& modeIdentifier);
        static void BindActionsToMenus();

        void Refresh();
        AZStd::vector<AzToolsFramework::ActionOverride> PopulateActions(const AZ::EntityComponentIdPair& entityComponentIdPair);
        void Display(
            const AZ::EntityComponentIdPair& entityComponentIdPair,
            const AZ::Transform& worldFromLocal,
            const IntersectionAndRenderData& renderData,
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay);

        bool HandleMouseInteraction(
            const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction,
            const AZ::EntityComponentIdPair& entityComponentIdPair,
            const AZStd::optional<EdgeIntersection>& edgeIntersection,
            const AZStd::optional<PolygonIntersection>& polygonIntersection,
            const AZStd::optional<VertexIntersection>& vertexIntersection);

        // EditorWhiteBoxTransformModeRequestBus overrides ...
        void ChangeTransformType(TransformType subModeType) override;

    private:
        //! shared data that is used between the different transformation modes Translation/Rotation/Scale.
        struct VertexTransformSelection
        {
            AZ::Vector3 m_localPosition = AZ::Vector3::CreateZero();
            AZ::Quaternion m_localRotation = AZ::Quaternion::CreateIdentity();
            AZStd::vector<AZ::Vector3> m_vertexPositions;
            Api::VertexHandles m_vertexHandles;
            IntersectionSelection m_selection;
        };

        void CreateTranslationManipulators();
        void CreateRotationManipulators();
        void CreateScaleManipulators();
        void UpdateTransformHandles(WhiteBoxMesh* mesh);
        void RefreshManipulator();
        void DestroyManipulators();

        AZ::EntityComponentIdPair m_entityComponentIdPair; //!< The entity and component id this modifier is associated with.

        AZStd::shared_ptr<AzToolsFramework::Manipulators> m_manipulator = nullptr;
        AZStd::shared_ptr<VertexTransformSelection> m_whiteBoxSelection = nullptr;

        AZStd::optional<PolygonIntersection> m_polygonIntersection = AZStd::nullopt;
        AZStd::optional<EdgeIntersection> m_edgeIntersection = AZStd::nullopt;
        AZStd::optional<VertexIntersection> m_vertexIntersection = AZStd::nullopt;

        TransformType m_transformType = TransformType::Translation;
        AzToolsFramework::ViewportUi::ClusterId m_transformClusterId;
        AzToolsFramework::ViewportUi::ButtonId m_transformTranslateButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_transformRotateButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_transformScaleButtonId;

        AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler m_transformSelectionHandler;
    };

} // namespace WhiteBox
