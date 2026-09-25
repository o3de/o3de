/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector2.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/utils.h>
#include <WhiteBox/WhiteBoxToolApi.h>
#include "Util/WhiteBoxUvOps.h"

#include <QImage>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QWidget>

namespace WhiteBox
{
    //! The UVs of a polygon selection, grouped the way an editor moves them.
    struct UvModel
    {
        //! Face corners that share a mesh vertex and a UV move as one point, so islands stay stitched.
        struct Vertex
        {
            AZ::Vector2 m_uv;
            AZStd::vector<Api::HalfedgeHandle> m_corners;
            int m_meshVertex = -1; //!< The mesh vertex every corner here sits on; Sew joins points that share it.
        };
        struct Edge
        {
            AZ::u32 m_from = 0;
            AZ::u32 m_to = 0;
            bool m_border = false; //!< A polygon edge; false for the diagonals inside a polygon.
        };

        AZStd::vector<Vertex> m_vertices;
        AZStd::vector<Edge> m_edges;
        AZStd::vector<AZStd::array<AZ::u32, 3>> m_triangles;
        Api::FaceHandles m_faces; //!< The face each triangle belongs to.
        AZStd::vector<AZStd::array<Api::HalfedgeHandle, 3>> m_triangleCorners; //!< Each triangle's own corners.
        size_t m_signature = 0; //!< Changes with the selected faces, not with their UVs.
    };

    //! Build the model for the selected polygons; stale handles are skipped. Detached corners (halfedge indices) form
    //! their own points even where they share a vertex and UV with others, so a split holds until they are moved.
    UvModel BuildUvModel(
        const WhiteBoxMesh& whiteBox, const Api::PolygonHandles& polygons, const AZStd::unordered_set<int>& detached = {});

    //! The 2D view of UvModel: pan, zoom, select points or islands, and move, rotate or scale them.
    //! V runs down the view, as rows do in a texture, so (0, 0) is the top left of the unit square.
    class WhiteBoxUvCanvas : public QWidget
    {
    public:
        enum class Tool
        {
            Move,
            Rotate,
            Scale
        };

        explicit WhiteBoxUvCanvas(QWidget* parent = nullptr);

        //! Selection carries over by corner, so it survives the model being rebuilt after an edit.
        void SetModel(UvModel model);
        const UvModel& Model() const { return m_model; }
        bool IsDragging() const { return m_drag != Drag::None; }

        void SetTool(Tool tool) { m_tool = tool; }
        void FrameAll();
        void FrameSelection();
        void SelectAll();
        void SelectNone();
        void InvertSelection();
        //! Every island that holds a selected point.
        void SelectLinked();
        //! Add, or drop, one step of neighbours along UV edges.
        void GrowSelection();
        void ShrinkSelection();
        //! Follow polygon edges on from each selected edge, straight on through each point, until the line turns or ends.
        //! @return false when no polygon edge has both ends selected.
        bool SelectLoop();
        //! Points on island boundaries: edges only one face uses in UV space. Limited to islands with a selection, if any.
        void SelectBorder();

        //! Faces whose every corner is selected; empty when no face is.
        Api::FaceHandles SelectedFaces() const;
        //! Mesh vertices under selected points, once each.
        AZStd::vector<int> SelectedMeshVertices() const;
        //! Polygon edges with both ends selected, as mesh vertex pairs.
        AZStd::vector<AZStd::pair<int, int>> SelectedMeshEdges() const;
        bool HasSelection() const { return !m_selectedCorners.empty(); }

        //! Drawn tiled behind the UVs, full strength on the unit square; a null image shows the checker.
        void SetBackground(const QImage& image);
        //! Faces to highlight because the viewport cursor is over them.
        void SetHoveredFaces(const Api::FaceHandles& faces);
        //! The selected faces, or every face in view when none is.
        Api::FaceHandles TargetFaces() const;
        const AZStd::unordered_set<int>& DetachedCorners() const { return m_detached; }
        //! Tear the selected faces' corners away from their unselected neighbours; the caller rebuilds the model.
        //! @return how many faces were split off (zero with no face selected).
        size_t SplitSelectedFaces();
        //! Join selected points that sit on the same mesh vertex at their average UV, as one finished edit.
        //! @return how many points were moved.
        size_t SewSelected();

        //! Apply a UV transform to the selection (every point when nothing is selected) as one finished edit.
        void TransformTargets(const AZStd::function<AZ::Vector2(const AZ::Vector2&, const AZ::Vector2& centre)>& transform);
        //! Scale and move the selection (or everything) uniformly so its bounds fill the unit square.
        void FitTargetsToUnitSquare();

        //! Receives every UV change; final is set once a gesture ends and should become one undo step.
        AZStd::function<void(const AZStd::vector<UvChange>& changes, bool final)> m_onEdit;
        //! Receives a one-line description of what the canvas is showing or doing.
        AZStd::function<void(const QString& message)> m_onStatus;

    protected:
        void paintEvent(QPaintEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;
        void wheelEvent(QWheelEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;

    private:
        enum class Drag
        {
            None,
            Pan,
            Box,
            Transform
        };

        QPointF ToScreen(const AZ::Vector2& uv) const;
        AZ::Vector2 ToUv(const QPointF& screen) const;
        void Frame(const AZStd::vector<AZ::u32>& vertices);
        //! The point under the cursor within a few pixels, or -1.
        int PickVertex(const QPointF& screen) const;
        bool IsSelected(AZ::u32 vertex) const;
        void SetSelected(AZ::u32 vertex, bool selected);
        //! Selected points, or every point when nothing is selected.
        AZStd::vector<AZ::u32> Targets() const;
        AZ::Vector2 Centre(const AZStd::vector<AZ::u32>& vertices) const;
        //! Every point joined to the given one through shared triangles.
        AZStd::vector<AZ::u32> Island(AZ::u32 vertex) const;
        //! Points joined by an edge; polygon edges only when asked, which leaves out the diagonals inside polygons.
        AZStd::vector<AZStd::vector<AZ::u32>> Neighbours(bool polygonEdgesOnly) const;
        //! Replace the selection with exactly these points, then redraw and report.
        void SelectPoints(const AZStd::vector<bool>& selected);
        AZStd::vector<bool> SelectionMask() const;
        //! Move the targets to new UVs and report them.
        void Apply(const AZStd::vector<AZ::u32>& targets, const AZStd::vector<AZ::Vector2>& uvs, bool final);
        void UpdateTransformDrag(const QPointF& screen, bool snap, bool final);
        void Report();

        UvModel m_model;
        AZStd::unordered_set<int> m_selectedCorners; //!< Halfedge indices; a point is selected when any of its corners is.
        AZStd::unordered_set<int> m_detached; //!< Halfedge indices split off by SplitSelectedFaces.
        QImage m_background;
        AZStd::unordered_set<int> m_hoveredFaces; //!< Face indices under the viewport cursor.

        QPointF m_originOnScreen = QPointF(40.0, 40.0); //!< Where UV (0, 0) is drawn.
        double m_pixelsPerUnit = 256.0;
        bool m_framed = false;

        Tool m_tool = Tool::Move;
        Drag m_drag = Drag::None;
        QPointF m_pressScreen;
        QPointF m_lastScreen;
        QRect m_box;
        AZStd::vector<AZ::u32> m_dragTargets;
        AZStd::vector<AZ::Vector2> m_dragStartUvs;
        AZ::Vector2 m_dragPivot = AZ::Vector2::CreateZero();
        bool m_dragMoved = false; //!< Changes went out mid-drag, so the release must close them with a final one.
    };
} // namespace WhiteBox
