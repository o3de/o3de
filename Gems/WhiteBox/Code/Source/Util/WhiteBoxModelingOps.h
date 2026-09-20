/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/std/string/string.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace WhiteBox
{
    //! The Transform-mode modeling operations, shared by the viewport cluster and the floating windows.
    namespace ModelingOps
    {
        //! What an operation did, for whichever surface invoked it to display.
        struct Result
        {
            bool m_success = false;
            AZStd::string m_message; //!< The error when it failed, otherwise what just happened.
        };

        //! The Transform-mode selection, read once and shared by the questions below.
        struct Selection
        {
            Api::PolygonHandles m_polygons;
            Api::EdgeHandles m_edges;
            Api::VertexHandles m_vertices;
            bool m_editable = false;  //!< There is a component with a mesh to act on.
            bool m_liveBevel = false; //!< A bevel is already live, so another cannot start.
        };
        Selection CurrentSelection(const AZ::EntityComponentIdPair& entityComponentIdPair);

        //! Whether each operation would do anything with the selection (drives the buttons' enabled state).
        bool CanBridge(const Selection& selection);
        bool CanWeld(const Selection& selection);
        bool CanLoopCut(const Selection& selection);
        bool CanBevel(const Selection& selection);
        bool CanSelectEdgePattern(const Selection& selection);
        //! A typed amount only applies to polygons. Edges extrude by dragging with the latch on.
        bool CanExtrudeInset(const Selection& selection);

        Result ExtrudeInset(const AZ::EntityComponentIdPair& pair, float amount, bool inset);
        bool HasLiveBevel(const AZ::EntityComponentIdPair& entityComponentIdPair);
        //! Selection only: expands all current edge seeds, without baking or editing geometry.
        Result SelectEdgePattern(const AZ::EntityComponentIdPair& entityComponentIdPair, bool ring);

        //! Connect the selection: two open boundary edges, or two facing polygons.
        Result Bridge(const AZ::EntityComponentIdPair& entityComponentIdPair);
        //! Merge the selected vertices into one, at the selection centre or at the last one selected.
        Result Weld(const AZ::EntityComponentIdPair& entityComponentIdPair, bool atLastVertex);
        //! Enter the interactive loop cut. Switches to Transform sub-mode first if something else is active.
        Result BeginLoopCut(const AZ::EntityComponentIdPair& entityComponentIdPair);
        //! Start a live bevel with the current parameters; a polygon region bevels its perimeter only.
        Result BeginBevel(const AZ::EntityComponentIdPair& entityComponentIdPair);
    } // namespace ModelingOps
} // namespace WhiteBox
