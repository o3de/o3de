/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    class ReflectContext;
}

namespace WhiteBox
{
    // Reflected to Python (whitebox.authoring) for scripts and MCP. Ids are handle indices (a polygon's id is its
    // smallest face id) and stay valid until the next topology change. Enums travel as ints (see whitebox_author.py).

    //! Outcome of an authoring call. m_ids carries the new or changed element ids when the call produces any.
    struct AuthoringResult
    {
        AZ_TYPE_INFO(AuthoringResult, "{FEB4A28C-5F47-4955-A343-D388BC801C90}");

        bool m_success = false;
        AZStd::string m_message;
        AZStd::vector<int> m_ids;
    };

    //! A parametric shape layer's parameters, the same values the pane's shape card edits.
    struct AuthoringShapeParams
    {
        AZ_TYPE_INFO(AuthoringShapeParams, "{7D814EFB-CC71-4C67-A62D-97A0C256522C}");

        int m_shape = 0; //!< DrawShapeType.
        float m_width = 1.0f;
        float m_depth = 1.0f;
        float m_height = 1.0f;
        int m_sides = 4;
        int m_steps = 8;
        bool m_stepsByHeight = false;
        float m_stepHeight = 0.25f;
        float m_wallThickness = 0.15f;
        float m_cavityGap = 0.1f;
        bool m_floor = true;
        bool m_ceiling = false;
        bool m_doorFrame = true;
        float m_archHeight = 0.0f;
        float m_innerRadius = 0.5f;
        float m_sweepAngle = 270.0f;
        float m_holeRatio = 0.5f;
        int m_tubeSides = 12;
    };

    //! Per-layer settings: visibility, boolean combine, transform and the mirror / array modifiers.
    struct AuthoringLayerSettings
    {
        AZ_TYPE_INFO(AuthoringLayerSettings, "{C63EAC38-A5EB-4588-BF1F-EDCB0C7C3320}");

        AZStd::string m_name;
        bool m_visible = true;
        bool m_collision = true;
        AZ::Vector3 m_tint = AZ::Vector3::CreateOne();
        int m_combineMode = 0; //!< LayerCombineMode.
        bool m_invertNormals = false;
        bool m_edgesOnly = false;
        AZ::Vector3 m_position = AZ::Vector3::CreateZero();
        AZ::Vector3 m_rotation = AZ::Vector3::CreateZero(); //!< Euler degrees, XYZ.
        AZ::Vector3 m_scale = AZ::Vector3::CreateOne();
        bool m_mirrorX = false;
        bool m_mirrorY = false;
        bool m_mirrorZ = false;
        int m_arrayCount = 1;
        AZ::Vector3 m_arrayOffset = AZ::Vector3(2.0f, 0.0f, 0.0f);
    };

    //! One polygon of the active layer. Border positions and ids run counter-clockwise (several loops are flattened).
    struct AuthoringPolygonInfo
    {
        AZ_TYPE_INFO(AuthoringPolygonInfo, "{7C8E924A-D1A5-427C-BD57-03425B3242AF}");

        int m_id = -1;
        int m_faceCount = 0;
        AZ::Vector3 m_normal = AZ::Vector3::CreateZero();
        AZ::Vector3 m_center = AZ::Vector3::CreateZero();
        float m_area = 0.0f;
        AZStd::vector<int> m_vertexIds;
        AZStd::vector<AZ::Vector3> m_vertexPositions;
    };

    //! One edge of the active layer.
    struct AuthoringEdgeInfo
    {
        AZ_TYPE_INFO(AuthoringEdgeInfo, "{3A591CB7-3482-4C4F-AA82-418B46890EA0}");

        int m_id = -1;
        int m_vertexA = -1;
        int m_vertexB = -1;
        AZ::Vector3 m_start = AZ::Vector3::CreateZero();
        AZ::Vector3 m_end = AZ::Vector3::CreateZero();
        float m_length = 0.0f;
        bool m_boundary = false; //!< Only one face touches it, so a hole or an open border runs along it.
        bool m_interior = false; //!< A triangulation edge inside a polygon, not a visible border.
    };

    //! Display and material values from the pane's Display card.
    struct AuthoringDisplaySettings
    {
        AZ_TYPE_INFO(AuthoringDisplaySettings, "{9353EE43-D897-46C9-BA4F-BBDA01776377}");

        bool m_edgesOnly = false;
        bool m_useGlobalTint = true;
        AZ::Color m_tint = AZ::Color::CreateOne();
        bool m_useTexture = true;
        AZStd::string m_materialPath; //!< Product path of the entity's default material; empty is the built-in one.
        int m_csgSolver = 0;          //!< Api::CsgSolver.
        bool m_flipYZForExport = false;
    };

    //! The pane's Boolean card: the single-source boolean plus the scene-wide cutter flags.
    struct AuthoringBooleanSettings
    {
        AZ_TYPE_INFO(AuthoringBooleanSettings, "{087278F2-97CB-495B-8E85-37FA00805399}");

        AZ::EntityId m_sourceEntity;
        int m_operation = 1; //!< Api::BooleanOperation.
        bool m_live = false;
        bool m_affectActiveOnly = false;
        int m_sourceAfterApply = 0; //!< SourceAfterApply.
        bool m_excludeFromBoolean = false;
        bool m_booleanOthers = false; //!< This entity is a cutter that booleans every overlapping White Box.
        int m_cutterOperation = 1;    //!< Api::BooleanOperation the cutter applies.
    };

    //! The Draw Shape tool's settings. They only steer the interactive tool; scripts build shapes with shape layers.
    struct AuthoringDrawSettings
    {
        AZ_TYPE_INFO(AuthoringDrawSettings, "{B6B21F66-21F3-42BA-A5BD-C0FFB2ADB985}");

        int m_shape = 0; //!< DrawShapeType.
        int m_sides = 4;
        float m_holeRatio = 0.5f;
        int m_tubeSides = 12;
        bool m_carve = false;
        bool m_mergeUnion = false;
        bool m_polygonExtrude = false;
        bool m_unitCube = false;
        float m_unitCubeSize = 1.0f;
        bool m_unitCubeShowGrid = true;
        int m_stairSteps = 8;
        bool m_stairByHeight = false;
        float m_stairStepHeight = 0.25f;
        int m_stairRotation = 0;
    };

    //! Authoring requests for one White Box entity, addressed by entity id. Mesh calls act on the active layer in that
    //! layer's own space (entity-local while the layer transform is the identity) and each is one undo step.
    class EditorWhiteBoxAuthoringRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using BusIdType = AZ::EntityId;

        // ---- Overview -----------------------------------------------------------------------------------------------
        //! JSON: layers, active layer, mesh counts, bounds and the display / boolean / draw values.
        virtual AZStd::string Describe() = 0;

        // ---- Layers -------------------------------------------------------------------------------------------------
        virtual int GetLayerCount() = 0;
        virtual int GetActiveLayer() = 0;
        virtual bool SetActiveLayer(int index) = 0;
        //! Append an empty mesh layer and make it active; returns its index.
        virtual int AddLayer() = 0;
        //! Append a parametric shape layer (DrawShapeType) and make it active; returns its index.
        virtual int AddShapeLayer(int shape) = 0;
        virtual bool DeleteActiveLayer() = 0;
        //! Copy the active layer directly above itself; returns the copy's index.
        virtual int DuplicateActiveLayer() = 0;
        virtual bool MoveLayer(int from, int to) = 0;
        virtual AuthoringShapeParams GetShapeParams(int layer) = 0;
        //! Rebuilds the layer from the parameters; fails when the layer is not parametric.
        virtual AuthoringResult SetShapeParams(int layer, const AuthoringShapeParams& params) = 0;
        //! Freeze a parametric layer into an ordinary mesh so its vertices can be edited.
        virtual AuthoringResult BakeShapeLayer(int layer) = 0;
        virtual AuthoringLayerSettings GetLayerSettings(int layer) = 0;
        virtual AuthoringResult SetLayerSettings(int layer, const AuthoringLayerSettings& settings) = 0;
        //! Bake the active layer's position / rotation / scale into its vertices and reset them.
        virtual AuthoringResult ApplyLayerTransform() = 0;
        //! Bake the active layer's mirror and array into its mesh and switch them off.
        virtual AuthoringResult ApplyLayerModifiers() = 0;

        // ---- Mesh queries (active layer) ----------------------------------------------------------------------------
        virtual AZ::u64 GetVertexCount() = 0;
        virtual AZ::u64 GetFaceCount() = 0;
        virtual AZStd::vector<int> GetVertexIds() = 0;
        //! Parallel to GetVertexIds.
        virtual AZStd::vector<AZ::Vector3> GetVertexPositions() = 0;
        virtual AZStd::vector<int> GetFaceIds() = 0;
        //! Three vertex ids per face, parallel to GetFaceIds, wound counter-clockwise.
        virtual AZStd::vector<int> GetFaceVertexIds() = 0;
        virtual AZStd::vector<AuthoringPolygonInfo> GetPolygons() = 0;
        //! Polygon borders; pass true to include the triangulation edges inside polygons too.
        virtual AZStd::vector<AuthoringEdgeInfo> GetEdges(bool includeInterior) = 0;

        // ---- Mesh authoring (active layer) --------------------------------------------------------------------------
        //! Replace the layer's mesh with an indexed triangle list (welded, coplanar faces merged, CCW outward).
        virtual AuthoringResult SetMeshFromTriangles(
            const AZStd::vector<AZ::Vector3>& positions, const AZStd::vector<AZ::u32>& indices) = 0;
        //! Add one polygon from an ordered outline of at least three points; it is fan-triangulated, so keep it convex.
        //! Returns the new polygon id in m_ids.
        virtual AuthoringResult AddPolygon(const AZStd::vector<AZ::Vector3>& outline) = 0;
        virtual AuthoringResult ClearMesh() = 0;
        virtual AuthoringResult SetVertexPositions(
            const AZStd::vector<int>& vertexIds, const AZStd::vector<AZ::Vector3>& positions) = 0;
        virtual AuthoringResult TranslateVertices(const AZStd::vector<int>& vertexIds, const AZ::Vector3& offset) = 0;
        //! Push polygons out along their normals as new geometry (negative digs in). Returns the new polygon ids.
        virtual AuthoringResult ExtrudePolygons(const AZStd::vector<int>& polygonIds, float distance) = 0;
        //! Inset by a fraction of the polygon (0, 1). Returns the inner polygon ids.
        virtual AuthoringResult InsetPolygons(const AZStd::vector<int>& polygonIds, float fraction) = 0;
        //! Slide existing polygons along their normals without making new faces.
        virtual AuthoringResult TranslatePolygons(const AZStd::vector<int>& polygonIds, float distance) = 0;
        virtual AuthoringResult BevelEdges(const AZStd::vector<int>& edgeIds, float width, int segments, float profile) = 0;
        //! Connect two facing polygons or two open boundary edges (pass the other list empty).
        virtual AuthoringResult Bridge(const AZStd::vector<int>& polygonIds, const AZStd::vector<int>& edgeIds) = 0;
        //! Close the single planar hole bordered by the open edges; returns the cap polygon id.
        virtual AuthoringResult FillHole(const AZStd::vector<int>& edgeIds) = 0;
        //! Delete polygons but keep their vertices.
        virtual AuthoringResult DeletePolygons(const AZStd::vector<int>& polygonIds) = 0;
        virtual AuthoringResult MergePolygons(const AZStd::vector<int>& polygonIds) = 0;
        virtual AuthoringResult WeldVertices(const AZStd::vector<int>& vertexIds, bool atLastVertex) = 0;
        //! Cut count evenly spaced loops through the quad strip around the edge; slide in [-1, 1] shifts them.
        virtual AuthoringResult InsertEdgeLoops(int edgeId, int count, float slide) = 0;
        //! Split each polygon into quads meeting at its centre; returns the new polygon ids.
        virtual AuthoringResult SubdividePolygons(const AZStd::vector<int>& polygonIds) = 0;
        //! Split polygons along straight cuts between the vertices (in the order given, or two corners per polygon).
        virtual AuthoringResult ConnectVertices(const AZStd::vector<int>& vertexIds) = 0;
        //! Add a vertex on a polygon border edge at fraction (0, 1) from its first end; returns its id.
        virtual AuthoringResult InsertVertexOnEdge(int edgeId, float fraction) = 0;
        virtual AuthoringResult FlipEdge(int edgeId) = 0;
        //! Move polygons into a new layer directly above, which becomes active.
        virtual AuthoringResult DetachPolygonsToLayer(const AZStd::vector<int>& polygonIds) = 0;
        //! Weld coincident vertices and regroup coplanar faces so the mesh is a clean manifold.
        virtual AuthoringResult RepairMesh() = 0;

        // ---- Surface (materials, paint, smoothing, UVs) -------------------------------------------------------------
        //! Product path of a material, or empty to inherit the entity default; an empty polygon list means every polygon.
        virtual AuthoringResult SetPolygonMaterial(const AZStd::vector<int>& polygonIds, const AZStd::string& materialPath) = 0;
        //! Face paint colour; an alpha of zero clears it. An empty polygon list means every polygon.
        virtual AuthoringResult SetPolygonPaint(const AZStd::vector<int>& polygonIds, const AZ::Color& color) = 0;
        //! Smoothing group bitmask (bit n is group n + 1) with edit 0 = set, 1 = add, 2 = remove.
        virtual AuthoringResult SetPolygonSmoothing(const AZStd::vector<int>& polygonIds, AZ::u32 groups, int edit) = 0;
        //! Group neighbours within the angle so they shade smooth; an empty polygon list means every polygon.
        virtual AuthoringResult AutoSmooth(const AZStd::vector<int>& polygonIds, float angleDegrees) = 0;
        //! mode 0 = world, 1 = planar, 2 = manual; scale is repeats per metre.
        virtual AuthoringResult SetPolygonUvProjection(
            const AZStd::vector<int>& polygonIds, int mode, float scaleU, float scaleV, float offsetU, float offsetV,
            float rotationDegrees) = 0;
        //! Scale and offset each polygon's texture to span it exactly once.
        virtual AuthoringResult FitPolygonUvs(const AZStd::vector<int>& polygonIds) = 0;
        //! Recompute normals and planar UVs for the whole layer.
        virtual AuthoringResult RecalculateUvs() = 0;

        // ---- Entity settings (the pane's cards) ---------------------------------------------------------------------
        virtual AuthoringDisplaySettings GetDisplaySettings() = 0;
        virtual AuthoringResult SetDisplaySettings(const AuthoringDisplaySettings& settings) = 0;
        virtual AuthoringBooleanSettings GetBooleanSettings() = 0;
        virtual AuthoringResult SetBooleanSettings(const AuthoringBooleanSettings& settings) = 0;
        //! One-shot CSG with the boolean source entity, using the operation set in the boolean settings.
        virtual AuthoringResult ApplyBoolean() = 0;
        //! Recompute every White Box in the level against the current global cutters.
        virtual AuthoringResult RefreshGlobalBooleans() = 0;
        virtual AuthoringDrawSettings GetDrawSettings() = 0;
        virtual AuthoringResult SetDrawSettings(const AuthoringDrawSettings& settings) = 0;

        // ---- Voxel stamp, collision and export ----------------------------------------------------------------------
        //! Fill or clear 1x1x1 voxel cells given by their integer minimum corners (one undo step).
        virtual AuthoringResult SetVoxelCells(const AZStd::vector<AZ::Vector3>& cellMins, bool filled) = 0;
        virtual AuthoringResult ClearCubeStamp() = 0;
        virtual AuthoringResult AddCollision() = 0;
        //! Write the evaluated mesh (every layer, booleans applied) to an .obj file.
        virtual AuthoringResult ExportObj(const AZStd::string& filePath) = 0;

    protected:
        ~EditorWhiteBoxAuthoringRequests() = default;
    };

    using EditorWhiteBoxAuthoringRequestBus = AZ::EBus<EditorWhiteBoxAuthoringRequests>;

    //! Registers the authoring bus and its value types with the script contexts.
    void ReflectAuthoring(AZ::ReflectContext* context);
} // namespace WhiteBox
