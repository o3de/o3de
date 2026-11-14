/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "EditorWhiteBoxAuthoringBus.h"

#include <AzCore/std/functional.h>

namespace WhiteBox
{
    struct WhiteBoxMesh;
    class EditorWhiteBoxComponent;

    //! Answers the authoring bus for one White Box component. Kept out of the component so the Python / MCP surface lives
    //! in one place; it only uses the component's public pane API and commits edits the way the modeling ops do.
    class EditorWhiteBoxAuthoring : public EditorWhiteBoxAuthoringRequestBus::Handler
    {
    public:
        explicit EditorWhiteBoxAuthoring(EditorWhiteBoxComponent& component);
        ~EditorWhiteBoxAuthoring();
        EditorWhiteBoxAuthoring(const EditorWhiteBoxAuthoring&) = delete;
        EditorWhiteBoxAuthoring& operator=(const EditorWhiteBoxAuthoring&) = delete;

        // EditorWhiteBoxAuthoringRequestBus overrides ...

        // ---- Overview -----------------------------------------------------------------------------------------------
        AZStd::string Describe() override;

        // ---- Layers -------------------------------------------------------------------------------------------------
        int GetLayerCount() override;
        int GetActiveLayer() override;
        bool SetActiveLayer(int index) override;
        int AddLayer() override;
        int AddShapeLayer(int shape) override;
        bool DeleteActiveLayer() override;
        int DuplicateActiveLayer() override;
        bool MoveLayer(int from, int to) override;
        AuthoringShapeParams GetShapeParams(int layer) override;
        AuthoringResult SetShapeParams(int layer, const AuthoringShapeParams& params) override;
        AuthoringResult BakeShapeLayer(int layer) override;
        AuthoringLayerSettings GetLayerSettings(int layer) override;
        AuthoringResult SetLayerSettings(int layer, const AuthoringLayerSettings& settings) override;
        AuthoringResult ApplyLayerTransform() override;
        AuthoringResult ApplyLayerModifiers() override;

        // ---- Mesh queries (active layer) ----------------------------------------------------------------------------
        AZ::u64 GetVertexCount() override;
        AZ::u64 GetFaceCount() override;
        AZStd::vector<int> GetVertexIds() override;
        AZStd::vector<AZ::Vector3> GetVertexPositions() override;
        AZStd::vector<int> GetFaceIds() override;
        AZStd::vector<int> GetFaceVertexIds() override;
        AZStd::vector<AuthoringPolygonInfo> GetPolygons() override;
        AZStd::vector<AuthoringEdgeInfo> GetEdges(bool includeInterior) override;

        // ---- Mesh authoring (active layer) --------------------------------------------------------------------------
        AuthoringResult SetMeshFromTriangles(
            const AZStd::vector<AZ::Vector3>& positions, const AZStd::vector<AZ::u32>& indices) override;
        AuthoringResult AddPolygon(const AZStd::vector<AZ::Vector3>& outline) override;
        AuthoringResult ClearMesh() override;
        AuthoringResult SetVertexPositions(
            const AZStd::vector<int>& vertexIds, const AZStd::vector<AZ::Vector3>& positions) override;
        AuthoringResult TranslateVertices(const AZStd::vector<int>& vertexIds, const AZ::Vector3& offset) override;
        AuthoringResult ExtrudePolygons(const AZStd::vector<int>& polygonIds, float distance) override;
        AuthoringResult InsetPolygons(const AZStd::vector<int>& polygonIds, float fraction) override;
        AuthoringResult TranslatePolygons(const AZStd::vector<int>& polygonIds, float distance) override;
        AuthoringResult BevelEdges(
            const AZStd::vector<int>& edgeIds, float width, int segments, float profile) override;
        AuthoringResult Bridge(const AZStd::vector<int>& polygonIds, const AZStd::vector<int>& edgeIds) override;
        AuthoringResult FillHole(const AZStd::vector<int>& edgeIds) override;
        AuthoringResult DeletePolygons(const AZStd::vector<int>& polygonIds) override;
        AuthoringResult MergePolygons(const AZStd::vector<int>& polygonIds) override;
        AuthoringResult WeldVertices(const AZStd::vector<int>& vertexIds, bool atLastVertex) override;
        AuthoringResult InsertEdgeLoops(int edgeId, int count, float slide) override;
        AuthoringResult SubdividePolygons(const AZStd::vector<int>& polygonIds) override;
        AuthoringResult ConnectVertices(const AZStd::vector<int>& vertexIds) override;
        AuthoringResult InsertVertexOnEdge(int edgeId, float fraction) override;
        AuthoringResult FlipEdge(int edgeId) override;
        AuthoringResult DetachPolygonsToLayer(const AZStd::vector<int>& polygonIds) override;
        AuthoringResult RepairMesh() override;

        // ---- Surface (materials, paint, smoothing, UVs) -------------------------------------------------------------
        AuthoringResult SetPolygonMaterial(
            const AZStd::vector<int>& polygonIds, const AZStd::string& materialPath) override;
        AuthoringResult SetPolygonPaint(const AZStd::vector<int>& polygonIds, const AZ::Color& color) override;
        AuthoringResult SetPolygonSmoothing(const AZStd::vector<int>& polygonIds, AZ::u32 groups, int edit) override;
        AuthoringResult AutoSmooth(const AZStd::vector<int>& polygonIds, float angleDegrees) override;
        AuthoringResult SetPolygonUvProjection(
            const AZStd::vector<int>& polygonIds, int mode, float scaleU, float scaleV, float offsetU,
            float offsetV, float rotationDegrees) override;
        AuthoringResult FitPolygonUvs(const AZStd::vector<int>& polygonIds) override;
        AuthoringResult RecalculateUvs() override;

        // ---- Entity settings (the pane's cards) ---------------------------------------------------------------------
        AuthoringDisplaySettings GetDisplaySettings() override;
        AuthoringResult SetDisplaySettings(const AuthoringDisplaySettings& settings) override;
        AuthoringBooleanSettings GetBooleanSettings() override;
        AuthoringResult SetBooleanSettings(const AuthoringBooleanSettings& settings) override;
        AuthoringResult ApplyBoolean() override;
        AuthoringResult RefreshGlobalBooleans() override;
        AuthoringDrawSettings GetDrawSettings() override;
        AuthoringResult SetDrawSettings(const AuthoringDrawSettings& settings) override;

        // ---- Voxel stamp, collision and export ----------------------------------------------------------------------
        AuthoringResult SetVoxelCells(const AZStd::vector<AZ::Vector3>& cellMins, bool filled) override;
        AuthoringResult ClearCubeStamp() override;
        AuthoringResult AddCollision() override;
        AuthoringResult ExportObj(const AZStd::string& filePath) override;

    private:
        //! Bake a parametric layer, store the working mesh and publish the change, as the pane's modeling ops do.
        void CommitMeshEdit();
        //! Tell the White Box pane to re-read the component; it only watches the viewport and its own widgets, so a
        //! change made through the bus would otherwise leave it showing the old values.
        void RefreshPane();
        //! Mesh the next edit acts on; null when the entity has no editable mesh.
        WhiteBoxMesh* EditableMesh() const;
        //! Run an edit of the active layer's mesh as one undo step; a successful edit is committed and published.
        AuthoringResult EditMesh(const char* undoLabel, const AZStd::function<AuthoringResult(WhiteBoxMesh&)>& edit);

        EditorWhiteBoxComponent& m_component;
    };
} // namespace WhiteBox
