/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "SubComponentModes/WhiteBoxPaintSettings.h"

#include "Rendering/WhiteBoxMaterial.h"
#include "Rendering/WhiteBoxRenderData.h"
#include "Viewport/WhiteBoxShapeBuilders.h"
#include "Viewport/WhiteBoxViewportConstants.h"

#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/utils.h>
#include <AzCore/std/optional.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Visibility/VisibleGeometryBus.h>
#include <AzFramework/Render/GeometryIntersectionBus.h>
#include <AzFramework/Visibility/BoundsBus.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <AzToolsFramework/ComponentMode/ComponentModeDelegate.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzToolsFramework/ViewportSnapping/ViewportSnapSourceBus.h>
#include <WhiteBox/EditorWhiteBoxComponentBus.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace WhiteBox
{
    //! Tube Sides used to be derived from the shape's side count. Data written before it became its
    //! own parameter is migrated through this so existing tori keep the tessellation they had.
    int LegacyTubeSidesFromSides(int sides);

    class EditorWhiteBoxAuthoring;
    class EditorWhiteBoxMeshAsset;
    class RenderMeshInterface;

    //! How a layer combines with the accumulation of the layers beneath it (in list order).
    enum class LayerCombineMode
    {
        Separate,   //!< Kept as its own island (no CSG) - the default, original behaviour.
        Union,      //!< CSG-union onto the layers below (fuses solids).
        Subtract,   //!< CSG-subtract from the layers below (carves a hole).
        Intersect   //!< CSG-intersect with the layers below (keeps only the overlap).
    };

    //! What happens to the boolean source entity after a successful Apply Boolean.
    enum class SourceAfterApply : int
    {
        Keep,   //!< Leave the source entity untouched.
        Hide,   //!< Hide the source entity.
        Delete  //!< Delete the source entity.
    };

    //! Editor representation of White Box Tool.
    class EditorWhiteBoxComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , public AzToolsFramework::EditorComponentSelectionRequestsBus::Handler
        , public AzFramework::BoundsRequestBus::Handler
        , public AzFramework::VisibleGeometryRequestBus::Handler
        , public AzFramework::RenderGeometry::IntersectionRequestBus::Handler
        , public EditorWhiteBoxComponentRequestBus::Handler
        , public AzToolsFramework::ViewportSnapping::ViewportSnapSourceRequestBus::Handler
        , private EditorWhiteBoxComponentNotificationBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private AzToolsFramework::EditorVisibilityNotificationBus::Handler
        , private AZ::TickBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorWhiteBoxComponent, "{C9F2D913-E275-49BB-AB4F-2D221C16170A}", EditorComponentBase);
        static void Reflect(AZ::ReflectContext* context);

        EditorWhiteBoxComponent();
        EditorWhiteBoxComponent(const EditorWhiteBoxComponent&) = delete;
        EditorWhiteBoxComponent& operator=(const EditorWhiteBoxComponent&) = delete;
        ~EditorWhiteBoxComponent();

        // AZ::Component overrides ...
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // AZ::TickBus::Handler ... (polls for layer add/remove that the container UI does not notify)
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        // EditorWhiteBoxComponentRequestBus overrides ...
        WhiteBoxMesh* GetWhiteBoxMesh() override;
        void SerializeWhiteBox() override;
        void DeserializeWhiteBox() override;
        void WriteAssetToComponent() override;
        void RebuildWhiteBox() override;
        //! RebuildWhiteBox with the collider cook and bake deferred to OnTick, for streaming edits.
        //! The caller owes a real rebuild or SerializeWhiteBox when the gesture ends.
        void RebuildWhiteBoxDeferred();
        void SetDefaultShape(DefaultShapeType defaultShape) override;
        void SetMaterialTint(const AZ::Color& tint) override;
        AZ::Color GetMaterialTint() override;
        void SetMaterialUseTexture(bool useTexture) override;
        void SetMaterialOverride(const AZ::Data::AssetId& materialAssetId) override;
        AZ::Data::AssetId GetMaterialOverride() override;
        void AssignPolygonMaterial(const Api::PolygonHandles& polygons, const AZ::Data::AssetId& materialAssetId);
        //! Latched Extrude / Inset. Dragging a polygon while one is on behaves as if Ctrl were held, so
        //! the gesture is reachable without knowing about the modifier key. Ctrl still works either way.
        bool GetStickyExtrude() override { return m_stickyExtrude; }
        void SetStickyExtrude(bool sticky) { m_stickyExtrude = sticky; }
        bool GetStickyInset() override { return m_stickyInset; }
        void SetStickyInset(bool sticky) { m_stickyInset = sticky; }
        FacePaintSettings GetFacePaintSettings() const { return m_facePaintSettings; }
        void SetFacePaintSettings(const FacePaintSettings& settings) { m_facePaintSettings = settings; }
        int GetDrawSides() override { return m_drawShapeData.m_sides; }
        float GetDrawHoleRatio() override { return m_drawShapeData.m_holeRatio; }
        int GetDrawTubeSides() override { return m_drawShapeData.m_tubeSides; }
        DrawShapeType GetDrawShape() override { return m_drawShapeData.m_shape; }
        void SetDrawShapeType(DrawShapeType shape) override { SetDrawShape(shape); }
        DrawStairInfo GetDrawStairInfo() override
        {
            const DrawStairData& stair = m_drawShapeData.m_stair;
            return DrawStairInfo{stair.m_steps, stair.m_byHeight, stair.m_stepHeight, stair.m_rotation};
        }
        bool GetDrawCarve() override { return m_drawShapeData.m_carve; }
        bool GetDrawMergeUnion() override { return m_drawShapeData.m_mergeUnion; }
        bool GetDrawPolygonExtrude() override { return m_drawShapeData.m_polygonExtrude; }
        bool GetDrawUnitCube() override { return m_drawShapeData.m_unitCube; }
        //! Size of the next stamped cube; existing cubes keep their own size.
        float GetDrawUnitCubeSize() override
        {
            const float s = m_drawShapeData.m_unitCubeSize;
            return s < 0.05f ? 0.05f : s;
        }
        bool GetDrawUnitCubeShowGrid() override { return m_drawShapeData.m_unitCubeShowGrid; }
        void SetVoxelCell(const AZ::Vector3& cellMin, bool filled) override;
        void SetVoxelCells(const AZStd::vector<AZ::Vector3>& cellMins, bool filled) override;
        bool BuildColliderMesh(AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices) override;
        bool CarveCubeGrids(const WhiteBoxMesh& cutter, const AZ::Transform& cutterTransform) override;
        void MapMeshToActiveLayerSpace(WhiteBoxMesh& mesh, size_t firstVertexIndex) override;

        // EditorComponentSelectionRequestsBus overrides ...
        AZ::Aabb GetEditorSelectionBoundsViewport(const AzFramework::ViewportInfo& viewportInfo) override;
        bool EditorSelectionIntersectRayViewport(
            const AzFramework::ViewportInfo& viewportInfo, const AZ::Vector3& src, const AZ::Vector3& dir,
            float& distance) override;
        bool SupportsEditorRayIntersect() override;

        // BoundsRequestBus overrides ...
        AZ::Aabb GetWorldBounds() const override;
        AZ::Aabb GetLocalBounds() const override;

        // AzFramework::VisibleGeometryRequestBus::Handler overrides ...
        void BuildVisibleGeometry(const AZ::Aabb& bounds, AzFramework::VisibleGeometryContainer& geometryContainer) const override;

        // AzFramework::RenderGeometry::IntersectionRequestBus overrides ...
        //! Ray test for the editor's render geometry intersector (scene raycasts, snapping occlusion).
        AzFramework::RenderGeometry::RayResult RenderGeometryIntersect(
            const AzFramework::RenderGeometry::RayRequest& ray) const override;

        // AzToolsFramework::ViewportSnapping::ViewportSnapSourceRequestBus overrides ...
        // Publishes this mesh's vertices to the editor-wide vertex snapper.
        void CollectSnapVertices(
            const AzToolsFramework::ViewportSnapping::SnapQueryVolume& volume, AZStd::vector<AzToolsFramework::ViewportSnapping::SnapVertex>& out) const override;
        void CollectSnapEdges(
            const AzToolsFramework::ViewportSnapping::SnapQueryVolume& volume, AZStd::vector<AzToolsFramework::ViewportSnapping::SnapEdge>& out) const override;
        void CollectSnapEdgeMidpoints(
            const AzToolsFramework::ViewportSnapping::SnapQueryVolume& volume, AZStd::vector<AzToolsFramework::ViewportSnapping::SnapVertex>& out) const override;
        void CollectSnapFaceCenters(
            const AzToolsFramework::ViewportSnapping::SnapQueryVolume& volume, AZStd::vector<AzToolsFramework::ViewportSnapping::SnapVertex>& out) const override;
        AZ::Aabb GetSnapVertexBounds() const override;

        //! Drop the cached snap candidates. Call on any mesh, transform or non-uniform scale change.
        void InvalidateSnapVertexCache()
        {
            m_snapVertexCacheValid = false;
            m_snapEdgeCacheValid = false;
            m_snapEdgeMidpointCacheValid = false;
            m_snapFaceCenterCacheValid = false;
        }

        //! Returns if the component currently has an instance of RenderMeshInterface.
        bool HasRenderMesh() const;
        //! Returns if the component is currently using a White Box mesh asset to store its data.
        bool AssetInUse() const;

        //! Override the internal EditorWhiteBoxMeshAsset with an external instance.
        //! @note EditorWhiteBoxComponent takes ownership of the editorMeshAsset and will handle deleting it
        void OverrideEditorWhiteBoxMeshAsset(EditorWhiteBoxMeshAsset* editorMeshAsset);

        //! The mesh used for render / collision / selection (the live-boolean result when active).
        WhiteBoxMesh* GetEvaluatedWhiteBoxMesh();
        //! Evaluate base [op] source into a new mesh regardless of the live flag; null if unset or failed.
        //! Editor only (uses the CSG API). Used to bake the boolean variant.
        Api::WhiteBoxMeshPtr EvaluateBooleanMesh(bool physicsPass = false);
        //! An overlapping global cutter, cloned into this entity's unscaled local space (identity operand transform).
        struct ResolvedCutter
        {
            Api::WhiteBoxMeshPtr m_mesh;         //!< Cutter geometry baked into this target's local space.
            Api::BooleanOperation m_operation = Api::BooleanOperation::Subtraction;
        };
        //! Global cutters overlapping this entity, in its local space. @p physicsPass uses collision-filtered meshes.
        AZStd::vector<ResolvedCutter> CollectOverlappingCutters(bool physicsPass);
        //! Lightweight AABB-only test: is there at least one overlapping global cutter? Used to set
        //! m_globalBooleanActive without doing any mesh/CSG work.
        bool HasOverlappingCutter() const;
        //! Whether this entity's evaluated (m_displayMesh) result should be shown/baked: either the
        //! single-source live boolean is on, or it is an active global-boolean target.
        bool BooleanDisplayActive() const { return m_boolean.m_live || m_globalBooleanActive; }
        //! The cached live-boolean result (null when off); safe during the game build, unlike EvaluateBooleanMesh.
        WhiteBoxMesh* GetLiveBooleanDisplayMesh();
        //! Whether the live (non-destructive) boolean is currently enabled.
        bool GetLiveBoolean() const { return m_boolean.m_live; }
        //! Collision-filtered meshes for the collider (layers with Collision off removed). The displayed
        //! variant; empty when nothing collides, null on a clone.
        WhiteBoxMesh* GetPhysicsFilteredMesh() { return GetPhysicsMesh(); }
        //! The cached collision-filtered mesh or null - never the unfiltered fallback - for the game-entity cook.
        WhiteBoxMesh* GetPhysicsCombinedMeshStrict() { return m_physicsCombinedMesh.get(); }
        //! The physics-filtered boolean-evaluated mesh (null when no boolean source is set).
        WhiteBoxMesh* GetPhysicsFilteredBooleanMesh() { return m_physicsDisplayMesh.get(); }
        //! The uncut collision-filtered base, for the base variant while the live boolean is on. Null on
        //! the single identity layer fast path.
        Api::WhiteBoxMeshPtr BuildPhysicsFilteredBaseMesh() { return BuildCombined(GetWhiteBoxMesh(), true); }
        //! Enter White Box component mode (undoable). The entity must be selected and the component active.
        void EnterComponentMode();

        // ---- Pane API -------------------------------------------------------------------
        // The White Box pane drives all editing through this block.

        //! Metadata for one layer (everything the pane edits - not the mesh data itself).
        struct LayerMeta
        {
            AZStd::string m_name;
            bool m_visible = true;
            bool m_collision = true;
            AZ::Vector3 m_tint = DefaultMaterialTint;
            LayerCombineMode m_combineMode = LayerCombineMode::Separate;
            bool m_invertNormals = false;
            bool m_edgesOnly = false;
            AZ::Vector3 m_position = AZ::Vector3::CreateZero();
            AZ::Vector3 m_rotation = AZ::Vector3::CreateZero();
            AZ::Vector3 m_scale = AZ::Vector3::CreateOne();
            bool m_mirrorX = false;
            bool m_mirrorY = false;
            bool m_mirrorZ = false;
            int m_arrayCount = 1;
            AZ::Vector3 m_arrayOffset = AZ::Vector3(2.0f, 0.0f, 0.0f);
        };

        // Default shape (SetDefaultShape - the full entry point - is declared above with the bus overrides).
        DefaultShapeType GetDefaultShape() const { return m_defaultShape; }

        // Draw Shape tool settings.
        void SetDrawShape(DrawShapeType shape)
        {
            m_drawShapeData.m_shape = shape;
            m_drawShapeData.OnShapeChange(); // resets Draw Sides to a sensible default for the shape
        }
        void SetDrawSides(int sides) { m_drawShapeData.m_sides = AZStd::clamp(sides, 3, 128); }
        void SetDrawHoleRatio(float ratio) { m_drawShapeData.m_holeRatio = AZStd::clamp(ratio, 0.05f, 0.95f); }
        void SetDrawTubeSides(int tubeSides)
        {
            m_drawShapeData.m_tubeSides = AZStd::clamp(tubeSides, MinTubeSides, MaxTubeSides);
        }
        void SetDrawStairInfo(const DrawStairInfo& info)
        {
            m_drawShapeData.m_stair.m_steps = info.m_steps;
            m_drawShapeData.m_stair.m_byHeight = info.m_byHeight;
            m_drawShapeData.m_stair.m_stepHeight = info.m_stepHeight;
            m_drawShapeData.m_stair.m_rotation = info.m_rotation;
        }
        void SetDrawCarve(bool carve) { m_drawShapeData.m_carve = carve; }
        void SetDrawMergeUnion(bool mergeUnion) { m_drawShapeData.m_mergeUnion = mergeUnion; }
        void SetDrawPolygonExtrude(bool extrude) { m_drawShapeData.m_polygonExtrude = extrude; }
        bool GetDrawParametric() const { return m_drawShapeData.m_parametric; }
        void SetDrawParametric(bool parametric) { m_drawShapeData.m_parametric = parametric; }

        // Unit Cube Stamp settings.
        void SetDrawUnitCube(bool unitCube) { m_drawShapeData.m_unitCube = unitCube; }
        void SetDrawUnitCubeSize(float size) { m_drawShapeData.m_unitCubeSize = AZStd::clamp(size, 0.05f, 100.0f); }
        void SetDrawUnitCubeShowGrid(bool showGrid) { m_drawShapeData.m_unitCubeShowGrid = showGrid; }
        void ClearCubeStamp() { ClearVoxelCubes(); }

        // Display settings.
        bool GetEdgesOnly() const { return m_edgesOnly; }
        void SetEdgesOnly(bool edgesOnly)
        {
            m_edgesOnly = edgesOnly;
            OnEdgesOnlyChange();
        }
        bool GetUseGlobalTint() const { return m_useGlobalTint; }
        void SetUseGlobalTint(bool useGlobalTint)
        {
            m_useGlobalTint = useGlobalTint;
            OnGlobalTintChange();
        }
        bool GetMaterialUseTexture() const { return m_material.m_useTexture; }
        //! The entity's default material, or invalid when it uses the built-in White Box material.
        AZ::Data::AssetId GetDefaultMaterialAssetId() const
        {
            return m_defaultMaterialAsset.GetId().IsValid() ? m_defaultMaterialAsset.GetId() : m_materialOverrideAssetId;
        }

        // Material list (the card's Polygon Materials, also what the Paint Material brush picks from).
        //! A material's file name without extension, or its id when the catalog does not know it.
        static AZStd::string MaterialDisplayName(const AZ::Data::AssetId& material);
        //! The listed materials as numbered names ("2. Brick") with their ids, empty entries skipped.
        AZStd::vector<AZStd::pair<AZStd::string, AZ::Data::AssetId>> GetPaletteEntries() const;
        //! The material the card's Material combo names, or invalid when it names nothing listed.
        AZ::Data::AssetId PaletteChoiceAsset() const;
        //! Point the card's Material combo at a listed material (no-op when it is not listed).
        void SetPaletteChoice(const AZ::Data::AssetId& material);
        //! Append a material to the list (one undo step) and choose it; an already listed one is just chosen.
        void AddToMaterialPalette(const AZ::Data::AssetId& material);

        //! A material's trim sheet: band edges in V (0 top, 1 bottom), empty when none is set. Invalid is the built-in one.
        AZStd::vector<float> GetTrimEdges(const AZ::Data::AssetId& material) const;
        //! Store a material's band edges as one undo step (sorted, clamped to 0..1); empty removes them.
        void SetTrimEdges(const AZ::Data::AssetId& material, AZStd::vector<float> edges);

        // Layers.
        //! Layer @p index's vertex positions in entity-local space (layer transform applied), for the layer
        //! gizmo's snap anchor. Not cheap: call once per drag.
        AZStd::vector<AZ::Vector3> GetLayerVertexPositionsEntityLocal(int index);

        int GetLayerCount() const { return static_cast<int>(m_layers.size()); }
        //! The active layer's Position/Rotation/Scale as one transform, in the order
        //! ApplyTransformToMesh uses it (scale, rotate, translate). Identity when there is no layer.
        AZ::Transform GetActiveLayerTransform() const;
        int GetActiveLayerIndex() const { return m_activeLayerIndex; }
        //! Stable identity for floating tools; an index can be reused after layer removal/reordering.
        AZ::u64 GetActiveLayerId() const
        {
            return m_activeLayerIndex >= 0 && m_activeLayerIndex < GetLayerCount()
                ? m_layers[m_activeLayerIndex].m_id : 0;
        }
        void SetActiveLayer(int index)
        {
            if (index >= 0 && index < GetLayerCount() && index != m_activeLayerIndex)
            {
                m_activeLayerIndex = index;
                OnActiveLayerChange();
            }
        }
        AZStd::vector<AZStd::pair<int, AZStd::string>> GetLayerNames(); //!< (index, name) pairs.
        void AddLayer() { OnNewLayer(); }

        //! Parameters of a parametric shape layer (the pane edits these; the mesh regenerates live).
        struct ShapeParams
        {
            DrawShapeType m_shape = DrawShapeType::Box;
            float m_width = 1.0f;  //!< Full extent along local X. For a Room this is the INTERIOR width.
            float m_depth = 1.0f;  //!< Full extent along local Y. For a Room this is the INTERIOR depth.
            float m_height = 1.0f; //!< Full extent along local Z (a tall sphere makes a bullet). Room: INTERIOR height.
            int m_sides = 4;       //!< Side count (round shapes) / sphere subdivision.
            int m_steps = 8;       //!< Staircase step count.
            bool m_stepsByHeight = false;
            float m_stepHeight = 0.25f; //!< Target riser height; count is rounded to fit the total height.
            // Room-only parameters (ignored by the other shapes).
            float m_wallThickness = 0.15f; //!< Thickness of EACH wall leaf (inner and outer).
            float m_cavityGap = 0.1f;      //!< Empty gap between the inner and outer wall leaves (double wall).
            bool m_floor = true;           //!< Add a floor slab beneath the interior.
            bool m_ceiling = false;        //!< Add a ceiling slab above the interior.
            bool m_doorFrame = true;       //!< Open frame instead of a solid door panel.
            float m_archHeight = 0.0f;     //!< Rise of the door arch; zero gives a rectangular door.
            float m_innerRadius = 0.5f;    //!< Circular stairs: radius of the central opening.
            float m_sweepAngle = 270.0f;   //!< Circular stairs: counterclockwise sweep in degrees.
            float m_holeRatio = 0.5f;      //!< Pipe/torus hole diameter divided by outer diameter.
            int m_tubeSides = DefaultTubeSides; //!< Torus: segments around the tube's cross-section.
        };
        //! Append a new PARAMETRIC layer generating @p shape (made active). Returns its index.
        int AddParametricShapeLayer(DrawShapeType shape);
        //! Append an active parametric layer placed in entity-local space. Returns its index.
        int AddParametricShapeLayer(
            const ShapeParams& params, const AZ::Vector3& position, const AZ::Vector3& rotationDegrees,
            LayerCombineMode combineMode);
        bool IsLayerParametric(int index) const
        {
            return index >= 0 && index < GetLayerCount() && m_layers[index].m_parametric;
        }
        ShapeParams GetLayerShapeParams(int index) const;
        //! Write new shape parameters and regenerate the layer's mesh (live rebuild).
        void SetLayerShapeParams(int index, const ShapeParams& params);
        //! SetLayerShapeParams for intermediate drag values: the mesh updates now, the stream write, cook and
        //! bake wait. Follow up with SetLayerShapeParams (in an undo batch) when the value settles.
        void SetLayerShapeParamsPreview(int index, const ShapeParams& params);
        //! Freeze a parametric layer into an ordinary mesh layer (enables vertex editing;
        //! the shape parameters stop driving it).
        void BakeParametricLayer(int index);
        struct BevelParams
        {
            float m_width = 0.1f;
            int m_segments = 1;
            float m_profile = 0.5f;
        };
        bool HasActiveBevel() const;
        //! The live bevel's parameters while one is running, otherwise the ones the next bevel will
        //! start with. Shared so the pane's spin boxes and the viewport cluster agree on the values.
        BevelParams GetBevelParams() const;
        void SetBevelParams(const BevelParams& params) { m_pendingBevelParams = params; }
        //! Empty selection updates the existing modifier from its saved source mesh.
        bool SetParametricBevel(const Api::EdgeHandles& edges, const BevelParams& params, AZStd::string& error);
        void BakeBevel();
        void CancelBevel();
        void DeleteActiveLayer() { OnDeleteLayer(); }
        //! Copy the active layer, geometry and settings alike, and insert it directly above.
        void DuplicateActiveLayer() { OnDuplicateLayer(); }
        //! A caller-owned copy of this component (all settings) holding only layer @p index. Null if
        //! @p index is not a layer or the mesh lives in a shared asset.
        EditorWhiteBoxComponent* CloneWithSingleLayer(int index);
        //! Move polygons of the active layer into a new layer directly above it, which becomes active. Keeps transform and settings.
        bool DetachPolygonsToLayer(const Api::PolygonHandles& polygons, AZStd::string& error);
        //! Move layer @p from to index @p to; order is the order combine modes accumulate in. The edited
        //! layer stays the edit target.
        void MoveLayer(int from, int to);
        void ApplyActiveLayerTransform() { OnApplyLayerTransform(); }
        //! Bake the active layer's mirror and array into its mesh and switch them off.
        void ApplyActiveLayerModifiers() { OnApplyLayerModifiers(); }
        AZ::Crc32 CreateChildLayer(); //!< New child entity with its own White Box component; edit focus moves to it.
        LayerMeta GetLayerMeta(int index) const
        {
            LayerMeta meta;
            if (index >= 0 && index < GetLayerCount())
            {
                const WhiteBoxLayer& layer = m_layers[index];
                meta.m_name = layer.m_name;
                meta.m_visible = layer.m_visible;
                meta.m_collision = layer.m_collision;
                meta.m_tint = layer.m_tint;
                meta.m_combineMode = layer.m_combineMode;
                meta.m_invertNormals = layer.m_invertNormals;
                meta.m_edgesOnly = layer.m_edgesOnly;
                meta.m_position = layer.m_position;
                meta.m_rotation = layer.m_rotation;
                meta.m_scale = layer.m_scale;
                meta.m_mirrorX = layer.m_mirrorX;
                meta.m_mirrorY = layer.m_mirrorY;
                meta.m_mirrorZ = layer.m_mirrorZ;
                meta.m_arrayCount = layer.m_arrayCount;
                meta.m_arrayOffset = layer.m_arrayOffset;
            }
            return meta;
        }
        void SetLayerMeta(int index, const LayerMeta& meta)
        {
            if (index < 0 || index >= GetLayerCount())
            {
                return;
            }
            WhiteBoxLayer& layer = m_layers[index];
            // The editing manipulators are built in a space that carries this transform, so they have
            // to be rebuilt when it moves - otherwise the handles hang where the layer used to be.
            const bool transformChanged = !layer.m_position.IsClose(meta.m_position) ||
                !layer.m_rotation.IsClose(meta.m_rotation) || !layer.m_scale.IsClose(meta.m_scale);
            layer.m_name = meta.m_name;
            layer.m_visible = meta.m_visible;
            layer.m_collision = meta.m_collision;
            layer.m_tint = meta.m_tint;
            layer.m_combineMode = meta.m_combineMode;
            layer.m_invertNormals = meta.m_invertNormals;
            layer.m_edgesOnly = meta.m_edgesOnly;
            layer.m_position = meta.m_position;
            layer.m_rotation = meta.m_rotation;
            layer.m_scale = AZ::Vector3(
                AZStd::max(meta.m_scale.GetX(), 0.001f), AZStd::max(meta.m_scale.GetY(), 0.001f),
                AZStd::max(meta.m_scale.GetZ(), 0.001f));
            layer.m_mirrorX = meta.m_mirrorX;
            layer.m_mirrorY = meta.m_mirrorY;
            layer.m_mirrorZ = meta.m_mirrorZ;
            layer.m_arrayCount = AZ::GetClamp(meta.m_arrayCount, 1, 256);
            layer.m_arrayOffset = meta.m_arrayOffset;
            m_layerRuntime.m_meshCache.erase(layer.m_id); // this layer's cached display mesh is stale now
            OnLayersMetaChanged(); // recombine + resync
            if (transformChanged)
            {
                RefreshManipulatorSpaces();
            }
        }

        // Boolean.
        AZ::EntityId GetBooleanSourceEntity() const { return m_boolean.m_sourceEntity; }
        void SetBooleanSourceEntity(AZ::EntityId sourceEntity)
        {
            m_boolean.m_sourceEntity = sourceEntity;
            OnBooleanSourceChange();
        }
        Api::BooleanOperation GetBooleanOperation() const { return m_boolean.m_operation; }
        void SetBooleanOperation(Api::BooleanOperation operation)
        {
            m_boolean.m_operation = operation;
            OnLiveBooleanChange();
        }
        void SetLiveBoolean(bool live)
        {
            m_boolean.m_live = live;
            OnLiveBooleanChange();
        }
        bool GetBooleanAffectActiveOnly() const { return m_boolean.m_affectActiveOnly; }
        void SetBooleanAffectActiveOnly(bool activeOnly)
        {
            m_boolean.m_affectActiveOnly = activeOnly;
            OnLiveBooleanChange();
        }
        SourceAfterApply GetSourceAfterApply() const { return m_boolean.m_sourceAfterApply; }
        void SetSourceAfterApply(SourceAfterApply mode) { m_boolean.m_sourceAfterApply = mode; }
        void ApplyBoolean(); //!< One-shot CSG apply using the source entity's mesh.

        // Global (scene-wide) boolean: cutter entities affect every overlapping non-excluded
        // White Box entity, independent of the single-Source workflow above.
        bool GetExcludeFromBoolean() const { return m_boolean.m_excludeFromBoolean; }
        void SetExcludeFromBoolean(bool exclude)
        {
            m_boolean.m_excludeFromBoolean = exclude;
            OnLiveBooleanChange(); // this entity's own result may change (it is/ isn't a target)
        }
        bool GetBooleanOthers() const { return m_boolean.m_booleanOthers; }
        void SetBooleanOthers(bool booleanOthers)
        {
            m_boolean.m_booleanOthers = booleanOthers;
            OnEdgesOnlyChange(); // a subtract cutter renders edges-only; refresh this entity's display
        }
        Api::BooleanOperation GetCutterOperation() const { return m_boolean.m_cutterOperation; }
        void SetCutterOperation(Api::BooleanOperation operation)
        {
            m_boolean.m_cutterOperation = operation;
            OnEdgesOnlyChange(); // edges-only display depends on the operation being Subtract
        }
        //! True when this entity should draw as a wireframe brush: either the explicit Edges Only
        //! toggle, or it is a global cutter using the Subtract operation.
        bool ShowEdgesOnly() const
        {
            return m_edgesOnly ||
                (m_boolean.m_booleanOthers && m_boolean.m_cutterOperation == Api::BooleanOperation::Subtraction);
        }
        //! Recompute every non-excluded White Box entity in the level against the current set of
        //! global cutters (manual "Refresh Global Booleans"). Static: it walks the whole scene.
        static void RefreshGlobalBooleans();

        // CSG solver for every boolean of this component: Fast (face-based BSP) or Manifold (volumetric).
        Api::CsgSolver GetCsgSolver() const { return m_csgSolver; }
        void SetCsgSolver(Api::CsgSolver solver)
        {
            if (m_csgSolver != solver)
            {
                m_csgSolver = solver;
                m_layerRuntime.m_meshCache.clear(); // cached combined layers were built with the old solver
                RebuildWhiteBox();                  // re-evaluate all booleans with the new solver
            }
        }

        // Mesh / entity operations.
        void AddCollision() { OnAddCollision(); }
        void FixNonManifold() { FixNonManifoldMesh(); }
        void SaveMeshAsAsset() { SaveAsAsset(); }
        void ExportToFile();
        //! Export what is rendered (every layer, paint, materials and smoothing) as .gltf or .glb.
        void ExportToGltf();
        void ExportDescendantsToFile();
        bool GetFlipYZForExport() const { return m_flipYZForExport; }
        void SetFlipYZForExport(bool flip) { m_flipYZForExport = flip; }
        //! Voxel stamp cell records (for "Clear Cube Stamp") and legacy grid streams folded in on load.
        struct VoxelData
        {
            AZ_TYPE_INFO(VoxelData, "{7D2B4F63-9A11-4E5B-8C27-3A6F0D51B9E2}");
            static void Reflect(AZ::ReflectContext* context);

            AZStd::vector<AZ::u64> m_cells; //!< Packed integer cell coords of stamped cubes.
            AZStd::vector<float> m_sizes;   //!< Per-cell world cube size (parallel to m_cells).
            float m_legacySize = 1.0f;                      //!< LEGACY single baked cell size (migration only).
            AZStd::vector<AZ::u8> m_legacyMerged;           //!< LEGACY per-cube merged flags (cleared on load).
            Api::WhiteBoxMeshStream m_legacyGridData;       //!< LEGACY separate-grid stream (folded on load).
            Api::WhiteBoxMeshStream m_legacyGridMergedData; //!< LEGACY merged-grid stream (folded on load).
        };

        //! Entity-boolean settings. Public for the version converter, like VoxelData.
        struct BooleanSettings
        {
            AZ_TYPE_INFO(BooleanSettings, "{58E9A1C4-2F76-4B0D-9E3A-B14C7D82F065}");
            static void Reflect(AZ::ReflectContext* context);

            AZ::EntityId m_sourceEntity; //!< Another entity whose White Box mesh is the boolean operand.
            Api::BooleanOperation m_operation = Api::BooleanOperation::Subtraction;
            bool m_live = false;             //!< Non-destructive: evaluate the boolean for display only.
            bool m_affectActiveOnly = false; //!< Cut only the active layer instead of the whole combined mesh.
            SourceAfterApply m_sourceAfterApply = SourceAfterApply::Keep; //!< Source entity fate after Apply.

            //! Global (scene-wide) boolean settings. Independent of the single m_sourceEntity above.
            bool m_excludeFromBoolean = false; //!< When true this entity is never cut by global cutters.
            bool m_booleanOthers = false;      //!< When true this entity is a cutter: it booleans every overlapping non-excluded entity.
            Api::BooleanOperation m_cutterOperation = Api::BooleanOperation::Subtraction; //!< Operation this cutter applies to its targets.
        };
        // ---- end Pane API ---------------------------------------------------------------

    private:
        //! Staircase-specific settings for the Draw Shape tool (only relevant when the
        //! draw shape is a Staircase). Grouped to keep the related members together.
        struct DrawStairData
        {
            AZ_TYPE_INFO(DrawStairData, "{EA16269C-D4DF-42A6-BC29-EE70B305D896}");
            static void Reflect(AZ::ReflectContext* context);

            bool m_byHeight = false;    //!< Staircase divided by a fixed riser height instead of a fixed step count.
            int m_steps = 8;            //!< Number of steps the Draw Shape tool uses when building a Staircase (count mode).
            float m_stepHeight = 0.25f; //!< Riser height used when a Staircase is divided by step height.
            int m_rotation = 0;         //!< Staircase orientation in 90-degree steps (0..3) about the surface normal.

        private:
            //! Step Count only shows for a Staircase in step-count mode.
            AZ::Crc32 StepsVisibility() const;
            //! Step Height only shows for a Staircase in step-height mode.
            AZ::Crc32 StepHeightVisibility() const;
        };

        //! Settings for the Draw Shape tool, grouped to keep the related members together.
        struct DrawShapeData
        {
            AZ_TYPE_INFO(DrawShapeData, "{A3794143-8F9E-49E9-B172-9025713D6553}");
            static void Reflect(AZ::ReflectContext* context);

            DrawShapeType m_shape = DrawShapeType::Box; //!< Shape the Draw Shape tool builds.
            int m_sides = 4;        //!< Side count the Draw Shape tool uses for round / N-gon shapes (4 = box/square).
            float m_holeRatio = 0.5f;
            int m_tubeSides = DefaultTubeSides; //!< Torus tube tessellation (independent of m_sides).
            //! Tube Sides only means anything for a torus.
            AZ::Crc32 TubeSidesVisibility() const;
            DrawStairData m_stair;  //!< Staircase-specific settings.
            bool m_carve = false;           //!< Draw acts as a CSG boolean (same as holding Ctrl).
            bool m_polygonExtrude = false; //!< Pull a solid from a closed freeform outline before committing.
            bool m_mergeUnion = false;      //!< Committing a drawn shape CSG-unions it into the mesh.
            bool m_parametric = false;      //!< Drawn shapes become parametric layers.
            bool m_unitCube = false;        //!< Draw mode click-stamps grid-snapped cubes.
            float m_unitCubeSize = 1.0f;    //!< Desired world-space size of the next stamped cube.
            bool m_unitCubeShowGrid = true; //!< Show the per-cube grid in the stamp ghost preview.

            //! When the Draw Shape changes, set a sensible default Draw Sides for it and
            //! refresh the property grid so the Draw Sides field updates.
            AZ::u32 OnShapeChange();

        private:
            //! Draw Sides only matters for round / N-gon shapes; hide it for a Staircase.
            AZ::Crc32 SidesVisibility() const;
            //! The Staircase-only controls only show when the draw shape is a Staircase.
            AZ::Crc32 StairVisibility() const;
        };

        //! One editable layer: geometry, name, visibility and transform. Layers are combined for output;
        //! edits target the active one.
        struct WhiteBoxLayer
        {
            AZ_TYPE_INFO(WhiteBoxLayer, "{3F2A9B84-7C1E-4D6A-9E2F-1B5C8A0D6E44}");
            static void Reflect(AZ::ReflectContext* context);

            AZStd::string m_name = "Layer";
            AZ::u64 m_id = 0; //!< Stable unique id so the active/working layer survives list reordering.
            bool m_visible = true;
            bool m_collision = true;
            AZ::Vector3 m_tint = DefaultMaterialTint; //!< Per-layer render tint (used when 'Use Global Tint' is off).
            LayerCombineMode m_combineMode = LayerCombineMode::Separate; //!< How this layer combines with the ones below it.
            bool m_invertNormals = false; //!< Flip this layer's winding (render + collision + selection) - non-destructive.
            bool m_edgesOnly = false;     //!< Hide this layer's solid faces and draw only its edges (visual only).
            AZ::Vector3 m_position = AZ::Vector3::CreateZero();       //!< Per-layer translation (applied at combine time).
            AZ::Vector3 m_rotation = AZ::Vector3::CreateZero();       //!< Per-layer rotation, Euler degrees (XYZ).
            AZ::Vector3 m_scale = AZ::Vector3::CreateOne();           //!< Per-layer non-uniform scale.
            // Modifiers, applied in layer space before the transform: mirror, then array. Editing touches only the source.
            bool m_mirrorX = false; //!< Reflect across the layer's YZ plane.
            bool m_mirrorY = false;
            bool m_mirrorZ = false;
            int m_arrayCount = 1;   //!< Copies in the array (1 = no array).
            AZ::Vector3 m_arrayOffset = AZ::Vector3(2.0f, 0.0f, 0.0f); //!< Step between copies, layer units.
            // Parametric: the mesh is generated from the parameters below and rebuilt when they change.
            bool m_parametric = false;
            DrawShapeType m_paramShape = DrawShapeType::Box;
            float m_paramWidth = 1.0f;  //!< Full extent along local X.
            float m_paramDepth = 1.0f;  //!< Full extent along local Y.
            float m_paramHeight = 1.0f; //!< Full extent along local Z.
            int m_paramSides = 4;       //!< Side count (round shapes) / sphere subdivision.
            int m_paramSteps = 8;       //!< Staircase step count.
            bool m_paramStepsByHeight = false;
            float m_paramStepHeight = 0.25f;
            // Room-only parameters (m_paramShape == DrawShapeType::Room).
            float m_paramWallThickness = 0.15f; //!< Thickness of each wall leaf.
            float m_paramCavityGap = 0.1f;      //!< Gap between the inner and outer wall leaves.
            bool m_paramFloor = true;           //!< Generate a floor slab.
            bool m_paramCeiling = false;        //!< Generate a ceiling slab.
            bool m_paramDoorFrame = true;
            float m_paramArchHeight = 0.0f;
            float m_paramInnerRadius = 0.5f;
            float m_paramSweepAngle = 270.0f;
            float m_paramHoleRatio = 0.5f;
            int m_paramTubeSides = DefaultTubeSides;

            bool m_bevelSourceParametric = false;
            Api::WhiteBoxMeshStream m_bevelSource;
            AZStd::vector<int> m_bevelEdges;
            float m_bevelWidth = 0.1f;
            int m_bevelSegments = 1;
            float m_bevelProfile = 0.5f;

            Api::WhiteBoxMeshStream m_freeformData;   //!< Serialized freeform (drawable) mesh.
            Api::WhiteBoxMeshStream m_gridData;       //!< Serialized stamped-cube grid mesh.
            Api::WhiteBoxMeshStream m_gridMergedData; //!< LEGACY (feature removed): folded into m_gridData on load.
            AZStd::vector<AZ::u64> m_voxelCells;
            AZStd::vector<float> m_voxelCellSizes;
            AZStd::vector<AZ::u8> m_voxelMerged;      //!< LEGACY (feature removed): cleared on load.
        };

        //! Runtime rebuild coalescing / debouncing (never serialized).
        struct RebuildState
        {
            bool m_liveBooleanPending = false; //!< Boolean source moved: rebuild once on the next tick.
            bool m_physicsPending = false;     //!< Physics rebuild deferred until the drag settles.
            float m_physicsTimer = 0.0f;
            bool m_bakedDataDirty = false;     //!< Game-mode bake caches need recomputing (debounced).
            float m_bakedDataDelay = 0.0f;
            //! A streaming edit (slider/spin-box drag) is in flight: skip work that only the serialized
            //! state needs, since a committing rebuild always follows once the value settles.
            bool m_streaming = false;
            //! A preview replaced the working mesh without writing it back; the debounce tick flushes it.
            bool m_workingMeshUnwritten = false;
        };

        //! Runtime layer bookkeeping (never serialized): which layer the working members hold,
        //! change detection for the reflected container, and the per-layer display mesh cache.
        struct LayerRuntime
        {
            int m_loadedIndex = -1;      //!< Which layer the working members currently hold (-1 = none).
            AZ::u64 m_loadedId = 0;      //!< Stable id of the loaded layer (survives reordering).
            int m_lastCount = -1;        //!< Layer count at the last sync (detects add/remove).
            AZ::u64 m_lastSignature = 0; //!< Hash of the layer id order (detects reorder as well).
            AZStd::unordered_map<AZ::u64, Api::WhiteBoxMeshPtr> m_meshCache; //!< Display mesh per NON-ACTIVE layer.
        };

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        // EditorComponentBase overrides ...
        void BuildGameEntity(AZ::Entity* gameEntity) override;

        // EditorVisibilityNotificationBus overrides ...
        void OnEntityVisibilityChanged(bool visibility) override;

        // AzFramework::EntityDebugDisplayEventBus overrides ...
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

        // TransformNotificationBus overrides ...
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // EditorWhiteBoxComponentNotificationBus overrides ...
        void OnWhiteBoxMeshModified() override;

        void ShowRenderMesh();
        void HideRenderMesh();
        void RebuildRenderMesh();
        void RebuildPhysicsMesh();
        AZ::Crc32 SaveAsAsset();
        //! Remove every cube placed by the Unit Cube Stamp tool (clears the voxel set and
        //! regenerates the surface, leaving any hand-edited freeform geometry intact).
        AZ::Crc32 ClearVoxelCubes();
        //! Weld coincident vertices and regroup coplanar faces (Api::RepairMesh) so booleans work again.
        AZ::Crc32 FixNonManifoldMesh();
        //! Boolean group button: re-evaluate every White Box in the level against the current global
        //! cutters (RefreshGlobalBooleans) as one undo step.
        AZ::Crc32 OnRefreshGlobalBooleans();
        // Data-layer plumbing. The working members (m_whiteBox, the two grids, occupancy)
        // always hold the ACTIVE layer; StoreLayer/LoadActiveLayer move data to/from m_layers.
        void StoreLayer(int index);          //!< Working members -> m_layers[index].
        void LoadActiveLayer();              //!< m_layers[m_activeLayerIndex] -> working members.
        Api::WhiteBoxMeshPtr BuildLayerMesh(const WhiteBoxLayer& layer); //!< Combined display mesh for a stored layer.
        //! True when a layer mirrors or arrays its mesh.
        static bool HasLayerModifiers(const WhiteBoxLayer& layer)
        {
            return layer.m_mirrorX || layer.m_mirrorY || layer.m_mirrorZ || layer.m_arrayCount > 1;
        }
        //! A layer's mesh with its mirror and array applied (in layer space); @p mesh itself when it has none.
        static Api::WhiteBoxMeshPtr ApplyLayerModifiers(Api::WhiteBoxMeshPtr mesh, const WhiteBoxLayer& layer);
        AZ::u32 OnActiveLayerChange();       //!< Active Layer control changed: commit current, load new.
        AZ::Crc32 OnNewLayer();              //!< Append an empty layer and make it active.
        AZ::Crc32 OnDeleteLayer();           //!< Delete the active layer (keeps at least one).
        AZ::Crc32 OnDuplicateLayer();        //!< Copy the active layer and insert it directly above.
        //! @p base with a copy suffix, stepped until no other layer answers to it.
        AZStd::string UniqueLayerName(const AZStd::string& base, const char* suffix = " copy") const;
        AZ::u32 OnLayersMetaChanged();       //!< Layer name/visibility edited: recombine + resync.
        void RefreshComponentMode();
        //! Rebuild the manipulators for a changed layer transform, keeping the selection (cheaper than a refresh).
        void RefreshManipulatorSpaces();         //!< Mark the component mode intersection data dirty after a layer swap.
        void ClearWorkingLayer();            //!< Reset the working members to empty (used when there are zero layers).
        AZ::Crc32 OnApplyLayerTransform();   //!< Bake the active layer's transform into its geometry, then reset it.
        AZ::Crc32 OnApplyLayerModifiers();
        //! Combine every visible layer in list order per its combine mode (the active one from
        //! @p activeFreeform). Null on the single identity layer fast path.
        Api::WhiteBoxMeshPtr BuildCombined(WhiteBoxMesh* activeFreeform, bool physicsPass = false);
        AZ::u64 AllocLayerId();                 //!< Hand out a fresh stable layer id.
        int IndexOfLayerId(AZ::u64 id) const;   //!< Index of the layer with @p id, or -1.
        AZ::u64 LayerSignature() const;         //!< Hash of the current layer id order + count.
        void SyncLayerStructure();              //!< Reconcile working state after the list is reordered / added to / removed from.
        //! Per-layer render faces with each layer's tint and Invert Normals. @p freeformOverride picks
        //! the active-layer mesh (null: the live freeform or boolean display mesh).
        WhiteBoxRenderData BuildColoredRenderData(WhiteBoxMesh* freeformOverride = nullptr);
        //! Render faces with inter-layer booleans applied, each face tinted by the layer it came from.
        WhiteBoxRenderData BuildColoredBooleanRenderData(WhiteBoxMesh* freeformOverride);
        //! True when at least one visible non-base layer uses a boolean (Union/Subtract/Intersect)
        //! combine mode, so the merged surface must be re-coloured per source layer.
        bool AnyVisibleLayerBoolean() const;
        //! True when the render/bake path must build per-layer faces: per-layer tint is on, or any
        //! visible layer inverts its normals. Both need per-layer geometry (winding flip / vertex colour).
        bool PerLayerRenderActive() const;
        void BuildLayerRenderMeshes(); //!< Build one tinted render mesh per visible layer (per-layer tint mode).
        AZ::u32 OnGlobalTintChange();           //!< Use Global Tint toggled: rebuild render + show/hide the global tint.
        AZ::Crc32 OnAddCollision();             //!< Add a White Box collider component to this entity.
        AZ::Crc32 OnAddMaterial();              //!< Add a Material component to this entity.
        AZ::Crc32 LegacyDefaultMaterialVisibility() const;
        AZ::Crc32 OnLightmapChange();
        AZ::Crc32 LightmapMarginVisibility() const;
        //! The card's material list as numbered names, for the Material combo.
        AZStd::vector<AZStd::string> GetPaletteChoices() const;
        AZ::u32 OnMaterialPaletteChange();
        AZ::u32 OnPaletteChoiceChange();
        AZ::Crc32 OnAssignPaletteMaterial();
        AZ::Crc32 OnResetPolygonMaterial();
        //! The polygons selected in edit mode (Transform mode's selection, else the default mode's).
        Api::PolygonHandles SelectedPolygonsInEditMode() const;
        //! Assign (invalid resets) on the edit-mode selection as one undo step.
        void AssignMaterialToSelection(const AZ::Data::AssetId& material, const char* undoLabel);
        AZ::Crc32 GlobalTintVisibility() const; //!< Show the global material tint only when Use Global Tint is on.
        //! Apply a layer's Position/Rotation(Euler deg)/Scale to every vertex of a display mesh.
        static void ApplyTransformToMesh(
            WhiteBoxMesh& mesh, const AZ::Vector3& position, const AZ::Vector3& eulerDegrees, const AZ::Vector3& scale);
        AZ::Crc32 OnDefaultShapeChange();
        //! Ensure the per-cell size array (m_voxel.m_sizes) is consistent with m_voxel.m_cells,
        //! migrating legacy data that only stored a single baked size (m_voxel.m_legacySize).
        void NormalizeVoxelData();
        //! Clamp @p params into the layer's valid ranges and store them. Returns false (and changes
        //! nothing) when @p index is not a parametric layer.
        bool StoreLayerShapeParams(int index, const ShapeParams& params);
        //! Regenerate a parametric layer from its parameters and rebuild. With @p commit false (a preview)
        //! the stream write, cook and bake are deferred.
        void RegenerateParametricLayer(int index, bool commit = true);

        //! The mesh used for RENDER / collision / bounds / selection. In live
        //! (non-destructive) boolean mode this is the evaluated result; otherwise
        //! it is the editable base mesh (GetWhiteBoxMesh).
        WhiteBoxMesh* EvaluatedMesh();
        //! The grid mesh holding the stamped cubes (appended to the freeform for output).
        //! Created on first use.
        WhiteBoxMesh* GridMesh();
        //! Rebuild m_combinedMesh (freeform plus grid) for EvaluatedMesh; null when the grid is empty.
        void RebuildCombinedMesh();
        //! Return a new mesh = @p freeform with the grid layer appended, or null if there is
        //! no grid (caller uses @p freeform as-is) or @p freeform is null.
        Api::WhiteBoxMeshPtr CombinedWithGrid(WhiteBoxMesh* freeform);
        //! Rebuild m_displayMesh = base (boolean) source, when live mode is active.
        void EvaluateLiveBoolean();
        //! Connect/disconnect the listener that re-evaluates when the source moves.
        void UpdateBooleanSourceListener();
        //! ChangeNotify for the live-boolean / operation fields.
        AZ::u32 OnLiveBooleanChange();
        //! ChangeNotify for the card's Solver field: cached layer combines were built with the old solver.
        AZ::u32 OnCsgSolverChange();
        //! ChangeNotify for the Boolean Source field: like OnLiveBooleanChange but forces a
        //! full tree rebuild so the Boolean group shows/hides when a source is set/cleared.
        AZ::u32 OnBooleanSourceChange();
        //! The Boolean group (Apply + options) only shows once a source entity is assigned.
        AZ::Crc32 BooleanGroupVisibility() const;

        void OnMaterialChange();
        //! Default Material picked on the card: push it through the same path the pane used.
        void OnDefaultMaterialChange();
        //! Open the dockable White Box pane (the component card's only button).
        AZ::Crc32 OnOpenPane();
        AZ::Crc32 AssetVisibility() const;
        //! The Unit Cube "Cube Size" spin box only shows while the Unit Cube Stamp tool is on.
        AZ::Crc32 DrawUnitCubeSizeVisibility() const;
        //! ChangeNotify for the "Edges Only" toggle: hide/show the solid render mesh.
        void OnEdgesOnlyChange();
        //! Display > Wireframe changed: build, rebuild or drop the GPU wireframe mesh and show or hide the shaded one.
        void OnWireframeModeChange();
        //! Build the wireframe mesh from m_renderData when m_wireframeMode asks for one, otherwise drop it.
        void RebuildWireframeMesh();
        //! The shaded meshes are hidden while the mode is WireframeOnly. Meshes are created visible, so this runs
        //! after every rebuild.
        void ApplyWireframeVisibility();

        using ComponentModeDelegate = AzToolsFramework::ComponentModeFramework::ComponentModeDelegate;
        ComponentModeDelegate m_componentModeDelegate; //!< Responsible for detecting ComponentMode activation
                                                       //!< and creating a concrete ComponentMode.

        //! Answers the Python / MCP authoring bus for this entity while the component is active.
        AZStd::unique_ptr<EditorWhiteBoxAuthoring> m_authoring;

        Api::WhiteBoxMeshPtr m_whiteBox; //!< Handle/opaque pointer to the White Box mesh data.
        AZStd::optional<AZStd::unique_ptr<RenderMeshInterface>>
            m_renderMesh; //!< The render mesh to use for the White Box mesh data.
        //! Auxiliary render meshes, one per visible layer, each with its own tint. Only populated
        //! when 'Use Global Tint' is off; in that mode m_renderMesh draws nothing.
        AZStd::vector<AZStd::unique_ptr<RenderMeshInterface>> m_layerRenderMeshes;
        //! Extra mesh that draws the edges with the White Box wireframe material (editor only, never in game mode).
        AZStd::unique_ptr<RenderMeshInterface> m_wireframeRenderMesh;
        WireframeMode m_wireframeMode = WireframeMode::Off; //!< Display > Wireframe.
        AZ::Transform m_worldFromLocal = AZ::Transform::CreateIdentity(); //!< Cached world transform of Entity.
        Api::WhiteBoxMeshStream m_whiteBoxData; //!< Serialized White Box mesh data.
        //! Holds a reference to an optional WhiteBoxMeshAsset and manages the lifecycle of adding/removing an asset.
        EditorWhiteBoxMeshAsset* m_editorMeshAsset = nullptr;
        //!@{
        //! Rebuild a snap candidate set from the evaluated mesh (lazily, on first use after invalidation).
        void RebuildSnapVertexCache() const;
        void RebuildSnapEdgeCache() const;
        void RebuildSnapEdgeMidpointCache() const;
        void RebuildSnapFaceCenterCache() const;
        //!@}

        //! World position for the snap caches, including Non-Uniform Scale (as GetWorldBounds).
        AZ::Vector3 SnapWorldPosition(const AZ::Vector3& localPosition, const AZ::Vector3& nonUniformScale) const;

        //!@{
        //! Snap candidates in world space, cached because the snapper queries them on every mouse move.
        mutable AZStd::vector<AzToolsFramework::ViewportSnapping::SnapVertex> m_snapVertexCache;
        mutable AZ::Aabb m_snapVertexCacheBounds = AZ::Aabb::CreateNull();
        mutable bool m_snapVertexCacheValid = false;

        //! Polygon border edges (the wireframe the user sees), not triangulation edges.
        mutable AZStd::vector<AzToolsFramework::ViewportSnapping::SnapEdge> m_snapEdgeCache;
        mutable bool m_snapEdgeCacheValid = false;

        mutable AZStd::vector<AzToolsFramework::ViewportSnapping::SnapVertex> m_snapEdgeMidpointCache;
        mutable bool m_snapEdgeMidpointCacheValid = false;

        mutable AZStd::vector<AzToolsFramework::ViewportSnapping::SnapVertex> m_snapFaceCenterCache;
        mutable bool m_snapFaceCenterCacheValid = false;
        //!@}

        mutable AZStd::optional<AZ::Aabb> m_worldAabb; //!< Cached world aabb (used for selection/view determination).
        mutable AZStd::optional<AZ::Aabb> m_localAabb; //!< Cached local aabb (used for center pivot calculation).
        //! Cached triangles for intersection/selection, built lazily (hence mutable).
        mutable AZStd::optional<Api::Faces> m_faces;
        WhiteBoxRenderData m_renderData; //!< Cached render data constructed from the White Box mesh source data.
        WhiteBoxMaterial m_material = {
            DefaultMaterialTint, DefaultMaterialUseTexture}; //!< Render material for White Box mesh.
        DefaultShapeType m_defaultShape =
            DefaultShapeType::Cube; //!< Used for selecting a default shape for the White Box mesh.
        bool m_flipYZForExport = false; //!< Flips the Y and Z components of white box vertices when exporting for different coordinate systems
        DrawShapeData m_drawShapeData; //!< Draw Shape tool settings (shape, sides and staircase options).
        VoxelData m_voxel;             //!< Stamp cell records + legacy grid streams.
        BooleanSettings m_boolean;     //!< Entity-boolean settings.
        Api::CsgSolver m_csgSolver = Api::CsgSolver::Manifold; //!< CSG backend for all of this component's booleans.
        RebuildState m_rebuild;        //!< Runtime rebuild coalescing/debouncing.
        LayerRuntime m_layerRuntime;   //!< Runtime layer bookkeeping + display mesh cache.
        bool m_edgesOnly = false; //!< When set, hide the solid render mesh and draw only the mesh edges.
        bool m_lightmapUvs = false;     //!< Generate lightmap UVs (UV1); mirrored into m_material for the render path.
        float m_lightmapMargin = 0.005f; //!< Gap between lightmap charts, in UV units.
        bool m_useGlobalTint = true; //!< When set, every layer renders with the global material tint; otherwise each layer uses its own tint.
        AZ::Data::AssetId m_materialOverrideAssetId; //!< External material asset override (invalid = built-in material).
        //! What the card's Default Material picker binds to; re-derived from the asset id above on Activate.
        AZ::Data::Asset<AZ::RPI::MaterialAsset> m_defaultMaterialAsset;
        //! A trim sheet: the horizontal bands of one material's texture that UVs can be fitted into.
        struct TrimSheet
        {
            AZ_TYPE_INFO(TrimSheet, "{5E2B7C1A-9D34-4F6B-8A21-C3E0F4B7D915}");
            static void Reflect(AZ::ReflectContext* context);
            AZ::Data::AssetId m_material;
            AZStd::vector<float> m_edges;
        };
        AZStd::vector<TrimSheet> m_trimSheets; //!< Per material, set from the UV editor.
        AZStd::vector<AZ::Data::Asset<AZ::RPI::MaterialAsset>> m_materialPalette; //!< The card's list to assign polygons from.
        AZStd::string m_paletteChoice; //!< The numbered name chosen in the card's Material combo.
        FacePaintSettings m_facePaintSettings; //!< Transient brush settings, not scene data.
        //! Latched Extrude / Inset, transient like the brush settings above.
        bool m_stickyExtrude = false;
        bool m_stickyInset = false;
        //! What the next bevel starts with. Transient tool state like the brush settings above, not
        //! scene data - a running bevel keeps its own values on the layer.
        BevelParams m_pendingBevelParams;
        //! Scratch storage for the batched viewport debug lines, kept so drawing does not reallocate
        //! every frame. Never serialized; contents are meaningless between draws.
        AZStd::vector<AZ::Vector3> m_debugLineBuffer;



        AZStd::vector<WhiteBoxLayer> m_layers;  //!< All editable layers (combined for output; edits target the active one).
        int m_activeLayerIndex = 0;             //!< Which layer is the current edit target.
        AZ::u64 m_nextLayerId = 1;              //!< Next stable layer id to hand out (serialized so ids stay unique).

        //! Recompute m_bakedBaseRenderData / m_bakedBooleanRenderData (the game-mode bake caches).
        void RebuildBakedRenderData();

        // Add a getter for the new physics mesh:
        WhiteBoxMesh* GetPhysicsMesh();

        //! The entity's non-uniform scale (from an optional Non-Uniform Scale component); (1,1,1) if none.
        AZ::Vector3 EntityNonUniformScale() const;
        //! Bake the entity's non-uniform scale into m_physicsCombinedMesh so the cooked collider (which
        //! does not reliably honour a non-uniform cooked-shape scale) matches the rendered geometry.
        void BakeEntityScaleIntoPhysicsMesh();

        Api::WhiteBoxMeshPtr m_displayMesh; //!< Evaluated (base [op] source) mesh used for display while live.
        Api::WhiteBoxMeshPtr m_physicsDisplayMesh;
        //! A global-boolean target with an overlapping cutter: show and bake m_displayMesh. Serialized.
        bool m_globalBooleanActive = false;
        Api::WhiteBoxMeshPtr m_gridMesh; //!< Stamped cubes kept SEPARATE from the freeform mesh (appended for output).
        Api::WhiteBoxMeshPtr m_combinedMesh; //!< Non-serialized freeform+grid mesh used for render/collision/bounds/selection.
        Api::WhiteBoxMeshPtr m_physicsCombinedMesh; // <-- ADD THIS
        //! Boolean render variant for the game-mode bake (serialized for clones). Empty without a boolean.
        WhiteBoxRenderData m_bakedBooleanRenderData;
        WhiteBoxRenderData m_bakedPhysicsBooleanRenderData;
        //! Uncut base render variant for the game-mode bake (serialized for clones).
        WhiteBoxRenderData m_bakedBaseRenderData;
        WhiteBoxRenderData m_bakedPhysicsBaseRenderData;
        //! The collision-filtered physics data has been baked, so an empty bake means collision off rather
        //! than legacy data. Serialized for the game build.
        bool m_physicsBaked = false;
        //! m_renderData and the bake caches as byte streams: reflected face vectors put every triangle
        //! in the undo system's prefab DOM, which made dense meshes crawl.
        AZStd::vector<AZ::u8> m_renderDataBlob;
        AZStd::vector<AZ::u8> m_bakedDataBlob;
        //! Refresh the blobs above from the live render data. Called wherever that data is rebuilt, so
        //! whatever the undo system or a level save serializes next is current.
        void PackRenderDataBlob();
        void PackBakedDataBlob();
        //! Restore the live render data from the serialized blobs (on load, undo and redo).
        void UnpackRenderDataBlobs();
        //! UnpackRenderDataBlobs, but only when the live render data is empty and a blob is not - for
        //! the game-entity build, which can run on a clone that was never activated.
        void EnsureRenderDataUnpacked();
        //! Re-evaluates this component's live boolean when the source entity moves.
        struct BooleanSourceListener : public AZ::TransformNotificationBus::Handler
        {
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;
            EditorWhiteBoxComponent* m_owner = nullptr;
        };
        BooleanSourceListener m_booleanSourceListener;

        //! Refreshes render / physics / bounds when the entity's Non-Uniform Scale component changes
        //! (that scale is not part of the Transform, so no OnTransformChanged fires for it).
        AZ::NonUniformScaleChangedEvent::Handler m_nonUniformScaleChangedHandler;
    };

    inline bool EditorWhiteBoxComponent::SupportsEditorRayIntersect()
    {
        return true;
    };

    //! The outcome of attempting to save a white box mesh.
    struct WhiteBoxSaveResult
    {
        AZStd::optional<AZStd::string> m_relativeAssetPath; //!< Optional relative asset path (the file may not have
                                                            //!< been saved in the project folder).
        AZStd::string m_absoluteFilePath; // The absolute path of the saved file (valid wherever the file is saved).
    };

    //! Attempt to create a WhiteBoxSaveResult so that a WhiteBoxMeshAsset may be created.
    //! An optional relative path determines if a WhiteBoxMeshAsset can be created or not (was it saved inside the
    //! project folder) and an absolute path is returned for the White Box Mesh to be written to disk (wbm file).
    //! The operation can fail or be cancelled in which case an empty optional is returned.
    //! @param entityName The name of the entity the WhiteBoxMesh is on.
    //! @param absoluteSavePathFn Returns the absolute path for where the asset should be saved. Takes as its only
    //! argument a first guess at where the file should be saved (this can then be overridden by the user in the Editor
    //! by using a file dialog.
    //! @param relativePathFn Takes as its first argument the absolute path returned by absoluteSavePathFn and then
    //! attempts to create a relative path from it. In the Editor, if the asset was saved inside the project folder a
    //! relative path is returned. The function can fail to return a valid relative path but still have a valid
    //! absolute path.
    //! @param saveDecision Returns if the user decided to save the asset when attempting to save outside the project
    //! root or if they cancelled the operation (QMessageBox::Save or QMessageBox::Cancel are the expected return
    //! values).
    AZStd::optional<WhiteBoxSaveResult> TrySaveAs(
        AZStd::string_view entityName,
        const AZStd::function<AZStd::string(const AZStd::string& initialAbsolutePath)>& absoluteSavePathFn,
        const AZStd::function<AZStd::optional<AZStd::string>(const AZStd::string& absolutePath)>& relativePathFn,
        const AZStd::function<int()>& saveDecisionFn);
} // namespace WhiteBox
