/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/std/optional.h>
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
        bool CanFillHole(const Selection& selection);
        bool CanMergePolygons(const Selection& selection);
        bool CanSelectLinked(const Selection& selection);
        bool CanSelectCoplanar(const Selection& selection);
        bool CanDeletePolygon(const Selection& selection);
        bool CanWeld(const Selection& selection);
        bool CanLoopCut(const Selection& selection);
        bool CanBevel(const Selection& selection);
        bool CanSelectEdgePattern(const Selection& selection);
        bool CanGrowShrink(const Selection& selection);
        bool CanDetach(const Selection& selection);
        bool CanConnectVertices(const Selection& selection);
        bool CanProjectUvs(const Selection& selection);
        bool CanSubdivide(const Selection& selection);
        bool CanSelectSimilar(const Selection& selection);
        //! A typed amount only applies to polygons. Edges extrude by dragging with the latch on.
        bool CanExtrudeInset(const Selection& selection);

        Result ExtrudeInset(const AZ::EntityComponentIdPair& pair, float amount, bool inset);
        bool HasLiveBevel(const AZ::EntityComponentIdPair& entityComponentIdPair);
        //! Selection only: grows edges into a loop or ring, or quads into a face strip.
        Result SelectEdgePattern(const AZ::EntityComponentIdPair& entityComponentIdPair, bool ring);

        //! Connect the selection: two open boundary edges, or two facing polygons.
        Result Bridge(const AZ::EntityComponentIdPair& entityComponentIdPair);
        //! Close the single planar hole bordered by the selected open edges, and select the new cap.
        Result FillHole(const AZ::EntityComponentIdPair& entityComponentIdPair);
        //! Merge the selected polygons into one by hiding every border they share.
        Result MergePolygons(const AZ::EntityComponentIdPair& entityComponentIdPair);
        //! Selection only: grow whatever is selected to the whole shell it is joined to.
        Result SelectLinked(const AZ::EntityComponentIdPair& entityComponentIdPair);
        //! Selection only: grow the polygon selection across neighbours that face the same way.
        Result SelectCoplanar(const AZ::EntityComponentIdPair& entityComponentIdPair);
        //! Delete the selected polygons but keep their vertices, so the opening can be refilled.
        Result DeletePolygon(const AZ::EntityComponentIdPair& entityComponentIdPair);
        //! Selection only: grow or shrink whatever is selected by one step of adjacency.
        Result GrowSelection(const AZ::EntityComponentIdPair& entityComponentIdPair);
        Result ShrinkSelection(const AZ::EntityComponentIdPair& entityComponentIdPair);
        //! Selection only: carry the selection over to another element type, falling back to touching when nothing is enclosed.
        Result ConvertSelection(const AZ::EntityComponentIdPair& entityComponentIdPair, Api::SelectionElement target, bool touching);
        //! Move the selected polygons into a new layer directly above, with the same transform and settings.
        Result DetachToLayer(const AZ::EntityComponentIdPair& entityComponentIdPair);
        //! Split polygons along straight edges between the selected vertices.
        Result ConnectVertices(const AZ::EntityComponentIdPair& entityComponentIdPair);
        //! Enter the interactive Insert Vertex tool, or leave it when it is already running.
        Result BeginInsertVertex(const AZ::EntityComponentIdPair& entityComponentIdPair);
        //! UV projection of the selected polygons; the first selected polygon's settings, or none without a polygon.
        AZStd::optional<Api::UvProjection> SelectedUvProjection(const AZ::EntityComponentIdPair& entityComponentIdPair);
        Result ApplyUvProjection(const AZ::EntityComponentIdPair& entityComponentIdPair, const Api::UvProjection& projection);
        //! Keep each selected polygon's mode and rotation, and scale and offset its texture to span it once.
        Result FitUvProjection(const AZ::EntityComponentIdPair& entityComponentIdPair);
        //! Mean tiling of the selected polygons, or none without a polygon.
        AZStd::optional<float> SelectedTexelDensity(const AZ::EntityComponentIdPair& entityComponentIdPair);
        //! Give every selected polygon the same repeats per metre so textures read at one scale across them.
        Result NormalizeTexelDensity(const AZ::EntityComponentIdPair& entityComponentIdPair, float repeatsPerMetre);
        //! Remember the first selected polygon's projection; Paste applies it to whatever is selected then, on any White Box.
        Result CopyUvProjection(const AZ::EntityComponentIdPair& entityComponentIdPair);
        Result PasteUvProjection(const AZ::EntityComponentIdPair& entityComponentIdPair);
        bool HasCopiedUvProjection();
        //! Split each selected polygon into quads meeting at its centre, and select them.
        Result Subdivide(const AZ::EntityComponentIdPair& entityComponentIdPair);
        //! Selection only: add every element like the selected ones; polygons compare by similarBy, edges by length, vertices by edge count.
        Result SelectSimilar(const AZ::EntityComponentIdPair& entityComponentIdPair, Api::SimilarBy similarBy);
        //! Smoothing bits every selected polygon has (m_all) and that any has (m_any); none without a polygon.
        struct SmoothingState
        {
            AZ::u32 m_all = 0;
            AZ::u32 m_any = 0;
        };
        AZStd::optional<SmoothingState> SelectedSmoothingGroups(const AZ::EntityComponentIdPair& entityComponentIdPair);
        Result EditSmoothingGroups(const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::u32 groups, Api::SmoothingEdit edit);
        //! Group the selected polygons (or the whole layer with none selected) so creases sharper than the angle stay hard.
        Result AutoSmooth(const AZ::EntityComponentIdPair& entityComponentIdPair, float angleDegrees);
        //! Merge the selected vertices into one, at the selection centre or at the last one selected.
        Result Weld(const AZ::EntityComponentIdPair& entityComponentIdPair, bool atLastVertex);
        //! Enter the interactive loop cut. Switches to Transform sub-mode first if something else is active.
        Result BeginLoopCut(const AZ::EntityComponentIdPair& entityComponentIdPair);
        //! Start a live bevel with the current parameters; a polygon region bevels its perimeter only.
        Result BeginBevel(const AZ::EntityComponentIdPair& entityComponentIdPair);
    } // namespace ModelingOps
} // namespace WhiteBox
