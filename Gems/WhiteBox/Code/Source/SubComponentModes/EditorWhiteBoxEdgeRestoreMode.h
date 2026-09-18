/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

        static void RegisterActionUpdaters();
        static void RegisterActions();
        static void BindActionsToModes(const AZStd::string& modeIdentifier);
        static void BindActionsToMenus();

        // Submode interface
        void Refresh();
        //! Handle Escape. Edge restore has no drag or numeric state of its own, so it never
        //! consumes the key - Escape leaves component mode as usual.
        bool HandleEscape() { return false; }

        //! Edge restore has no drag state of its own.
        bool CancelActiveDrag() { return false; }

        AZStd::vector<AzToolsFramework::ActionOverride> PopulateActions(
            const AZ::EntityComponentIdPair& entityComponentIdPair);
        void Display(
            const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Transform& worldFromLocal,
            const IntersectionAndRenderData& renderData, const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay);
        bool HandleMouseInteraction(const ModeMouseInteraction& mouse);

    private:
        AZStd::optional<EdgeIntersection> m_edgeIntersection; //!< The hovered edge if one exists.
        AZStd::optional<VertexIntersection> m_vertexIntersection; //!< The hovered vertex if one exists.
        Api::EdgeHandles m_edgeHandlesBeingRestored; //!< The edge handles currently attempting to be restored.

        //! Scratch storage for the batched debug lines drawn each frame, kept so the steady state
        //! does not reallocate. Contents are meaningless between Display calls.
        AZStd::vector<AZ::Vector3> m_lineBuffer;
    };
} // namespace WhiteBox
