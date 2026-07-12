/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Rendering/WhiteBoxMaterial.h"
#include "Rendering/WhiteBoxRenderData.h"
#include "Viewport/WhiteBoxViewportConstants.h"

#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/utils.h>
#include <AzCore/std/optional.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Visibility/VisibleGeometryBus.h>
#include <AzFramework/Visibility/BoundsBus.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <AzToolsFramework/ComponentMode/ComponentModeDelegate.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <WhiteBox/EditorWhiteBoxComponentBus.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace WhiteBox
{
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

    //! Editor representation of White Box Tool.
    class EditorWhiteBoxComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , public AzToolsFramework::EditorComponentSelectionRequestsBus::Handler
        , public AzFramework::BoundsRequestBus::Handler
        , public AzFramework::VisibleGeometryRequestBus::Handler
        , public EditorWhiteBoxComponentRequestBus::Handler
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
        void SetDefaultShape(DefaultShapeType defaultShape) override;
        void SetMaterialTint(const AZ::Color& tint) override;
        AZ::Color GetMaterialTint() override;
        void SetMaterialUseTexture(bool useTexture) override;
        void SetMaterialOverride(const AZ::Data::AssetId& materialAssetId) override;
        AZ::Data::AssetId GetMaterialOverride() override;
        int GetDrawSides() override { return m_drawShapeData.m_sides; }
        DrawShapeType GetDrawShape() override { return m_drawShapeData.m_shape; }
        DrawStairInfo GetDrawStairInfo() override
        {
            const DrawStairData& stair = m_drawShapeData.m_stair;
            return DrawStairInfo{stair.m_steps, stair.m_byHeight, stair.m_stepHeight, stair.m_rotation};
        }
        bool GetDrawCarve() override { return m_drawCarve; }
        bool GetDrawMergeUnion() override { return m_drawMergeUnion; }
        bool GetDrawUnitCube() override { return m_drawUnitCube; }
        //! Size of the next stamped cube; existing cubes keep their own size.
        float GetDrawUnitCubeSize() override
        {
            const float s = m_drawUnitCubeSize;
            return s < 0.05f ? 0.05f : s;
        }
        bool GetDrawUnitCubeShowGrid() override { return m_drawUnitCubeShowGrid; }
        void SetVoxelCell(const AZ::Vector3& cellMin, bool filled) override;
        void SetVoxelCells(const AZStd::vector<AZ::Vector3>& cellMins, bool filled) override;
        bool BuildColliderMesh(AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices) override;
        bool CarveCubeGrids(const WhiteBoxMesh& cutter, const AZ::Transform& cutterTransform) override;

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
        Api::WhiteBoxMeshPtr EvaluateBooleanMesh();
        //! The cached live-boolean result (null when off); safe during the game build, unlike EvaluateBooleanMesh.
        WhiteBoxMesh* GetLiveBooleanDisplayMesh();
        //! Whether the live (non-destructive) boolean is currently enabled.
        bool GetLiveBoolean() const { return m_liveBoolean; }
        //! Enter White Box component mode (undoable). The entity must be selected and the component active.
        void EnterComponentMode();

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
            DrawStairData m_stair;  //!< Staircase-specific settings.

        private:
            //! When the Draw Shape changes, set a sensible default Draw Sides for it and
            //! refresh the property grid so the Draw Sides field updates.
            AZ::u32 OnShapeChange();
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
            AZ::Vector3 m_tint = DefaultMaterialTint; //!< Per-layer render tint (used when 'Use Global Tint' is off).
            LayerCombineMode m_combineMode = LayerCombineMode::Separate; //!< How this layer combines with the ones below it.
            bool m_invertNormals = false; //!< Render this layer inside-out (reversed winding) - non-destructive.
            AZ::Vector3 m_position = AZ::Vector3::CreateZero();       //!< Per-layer translation (applied at combine time).
            AZ::Vector3 m_rotation = AZ::Vector3::CreateZero();       //!< Per-layer rotation, Euler degrees (XYZ).
            AZ::Vector3 m_scale = AZ::Vector3::CreateOne();           //!< Per-layer non-uniform scale.
            Api::WhiteBoxMeshStream m_freeformData;   //!< Serialized freeform (drawable) mesh.
            Api::WhiteBoxMeshStream m_gridData;       //!< Serialized separate-cube grid mesh.
            Api::WhiteBoxMeshStream m_gridMergedData; //!< Serialized merged-cube grid mesh.
            AZStd::vector<AZ::u64> m_voxelCells;
            AZStd::vector<float> m_voxelCellSizes;
            AZStd::vector<AZ::u8> m_voxelMerged;
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
        void ExportToFile();
        void ExportDescendantsToFile();
        AZ::Crc32 SaveAsAsset();
        //! Remove every cube placed by the Unit Cube Stamp tool (clears the voxel set and
        //! regenerates the surface, leaving any hand-edited freeform geometry intact).
        AZ::Crc32 ClearVoxelCubes();
        //! Weld coincident vertices and regroup coplanar faces (Api::RepairMesh) so booleans work again.
        AZ::Crc32 FixNonManifoldMesh();
        //! Create a new White Box "layer": a child entity with its own White Box component, then
        //! switch edit focus to it (so the next shape/cube is drawn into the child).
        AZ::Crc32 CreateChildLayer();

        // Data-layer plumbing. The working members (m_whiteBox, the two grids, occupancy)
        // always hold the ACTIVE layer; StoreLayer/LoadActiveLayer move data to/from m_layers.
        void StoreLayer(int index);          //!< Working members -> m_layers[index].
        void LoadActiveLayer();              //!< m_layers[m_activeLayerIndex] -> working members.
        Api::WhiteBoxMeshPtr BuildLayerMesh(const WhiteBoxLayer& layer); //!< Combined display mesh for a stored layer.
        AZ::u32 OnActiveLayerChange();       //!< Active Layer control changed: commit current, load new.
        AZStd::vector<AZStd::pair<int, AZStd::string>> GetLayerNames(); //!< (index, name) pairs for the Active Layer dropdown.
        AZ::Crc32 OnNewLayer();              //!< Append an empty layer and make it active.
        AZ::Crc32 OnDeleteLayer();           //!< Delete the active layer (keeps at least one).
        AZ::u32 OnLayersMetaChanged();       //!< Layer name/visibility edited: recombine + resync.
        void RefreshComponentMode();         //!< Mark the component mode intersection data dirty after a layer swap.
        void ClearWorkingLayer();            //!< Reset the working members to empty (used when there are zero layers).
        AZ::Crc32 OnApplyLayerTransform();   //!< Bake the active layer's transform into its geometry, then reset it.
        //! Build the full combined mesh from an active-layer freeform: every visible layer
        //! (active from @p activeFreeform + grids + its transform, others from their stored data),
        //! CSG-accumulated in list order per each layer's combine mode. Null == the single-active
        //! identity/no-grid fast path (caller falls back to the raw freeform).
        Api::WhiteBoxMeshPtr BuildCombined(WhiteBoxMesh* activeFreeform);
        AZ::u64 AllocLayerId();                 //!< Hand out a fresh stable layer id.
        int IndexOfLayerId(AZ::u64 id) const;   //!< Index of the layer with @p id, or -1.
        AZ::u64 LayerSignature() const;         //!< Hash of the current layer id order + count.
        void SyncLayerStructure();              //!< Reconcile working state after the list is reordered / added to / removed from.
        WhiteBoxRenderData BuildColoredRenderData(); //!< Per-layer-tinted render faces (used when Use Global Tint is off).
        void BuildLayerRenderMeshes(); //!< Build one tinted render mesh per visible layer (per-layer tint mode).
        AZ::u32 OnGlobalTintChange();           //!< Use Global Tint toggled: rebuild render + show/hide the global tint.
        AZ::Crc32 OnAddCollision();             //!< Add a White Box collider component to this entity.
        AZ::Crc32 GlobalTintVisibility() const; //!< Show the global material tint only when Use Global Tint is on.
        //! Apply a layer's Position/Rotation(Euler deg)/Scale to every vertex of a display mesh.
        static void ApplyTransformToMesh(
            WhiteBoxMesh& mesh, const AZ::Vector3& position, const AZ::Vector3& eulerDegrees, const AZ::Vector3& scale);
        AZ::Crc32 OnDefaultShapeChange();
        //! Apply a CSG boolean using the White Box mesh on m_booleanSourceEntity.
        void ApplyBoolean();
        //! Update the voxel-stamped geometry in place: remove the faces belonging to the
        //! previous voxel surface (@p oldCells at @p oldSizes) and add the surface for the
        //! new cell set (@p newCells at @p newSizes), leaving all freeform mesh edits
        //! intact. Each cell carries its own cube size (parallel arrays), so cubes of
        //! different sizes are meshed on their own grids and can coexist. Also purges any
        //! vertices orphaned by the removal.
        void RegenerateVoxelMesh(
            const AZStd::vector<AZ::u64>& oldCells, const AZStd::vector<float>& oldSizes,
            const AZStd::vector<AZ::u64>& newCells, const AZStd::vector<float>& newSizes);
        //! Ensure the per-cell size array (m_voxelCellSizes) is consistent with m_voxelCells,
        //! migrating legacy data that only stored a single baked size (m_voxelCellSize).
        void NormalizeVoxelData();
        //! Merge (@p add true) or carve (@p add false) the given occupancy cells into the
        //! base mesh with a CSG boolean, so the accumulated mesh stays watertight and
        //! 2-manifold with no T-junctions. The cells are built as a per-cell manifold operand.
        void StampCubeCells(WhiteBoxMesh* grid, const AZStd::unordered_set<AZ::u64>& cells, float cellSize, bool add);

        //! The mesh used for RENDER / collision / bounds / selection. In live
        //! (non-destructive) boolean mode this is the evaluated result; otherwise
        //! it is the editable base mesh (GetWhiteBoxMesh).
        WhiteBoxMesh* EvaluatedMesh();
        //! The grid mesh holding stamped cubes that are kept SEPARATE from the freeform mesh
        //! (appended for output). Created on first use.
        WhiteBoxMesh* GridMesh();
        //! The grid mesh holding stamped cubes that are MERGED (CSG-unioned) with the freeform
        //! mesh for output. Created on first use.
        WhiteBoxMesh* GridMergedMesh();
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
        //! ChangeNotify for the Boolean Source field: like OnLiveBooleanChange but forces a
        //! full tree rebuild so the Boolean group shows/hides when a source is set/cleared.
        AZ::u32 OnBooleanSourceChange();
        //! The Boolean group (Apply + options) only shows once a source entity is assigned.
        AZ::Crc32 BooleanGroupVisibility() const;

        void OnMaterialChange();
        AZ::Crc32 AssetVisibility() const;
        //! The Unit Cube "Cube Size" spin box only shows while the Unit Cube Stamp tool is on.
        AZ::Crc32 DrawUnitCubeSizeVisibility() const;
        //! ChangeNotify for the "Edges Only" toggle: hide/show the solid render mesh.
        void OnEdgesOnlyChange();

        using ComponentModeDelegate = AzToolsFramework::ComponentModeFramework::ComponentModeDelegate;
        ComponentModeDelegate m_componentModeDelegate; //!< Responsible for detecting ComponentMode activation
                                                       //!< and creating a concrete ComponentMode.

        Api::WhiteBoxMeshPtr m_whiteBox; //!< Handle/opaque pointer to the White Box mesh data.
        AZStd::optional<AZStd::unique_ptr<RenderMeshInterface>>
            m_renderMesh; //!< The render mesh to use for the White Box mesh data.
        //! Auxiliary render meshes, one per visible layer, each with its own tint. Only populated
        //! when 'Use Global Tint' is off; in that mode m_renderMesh draws nothing.
        AZStd::vector<AZStd::unique_ptr<RenderMeshInterface>> m_layerRenderMeshes;
        AZ::Transform m_worldFromLocal = AZ::Transform::CreateIdentity(); //!< Cached world transform of Entity.
        Api::WhiteBoxMeshStream m_whiteBoxData; //!< Serialized White Box mesh data.
        //! Holds a reference to an optional WhiteBoxMeshAsset and manages the lifecycle of adding/removing an asset.
        EditorWhiteBoxMeshAsset* m_editorMeshAsset = nullptr;
        mutable AZStd::optional<AZ::Aabb> m_worldAabb; //!< Cached world aabb (used for selection/view determination).
        mutable AZStd::optional<AZ::Aabb> m_localAabb; //!< Cached local aabb (used for center pivot calculation).
        AZStd::optional<Api::Faces> m_faces; //!< Cached faces (triangles of mesh used for intersection/selection).
        WhiteBoxRenderData m_renderData; //!< Cached render data constructed from the White Box mesh source data.
        WhiteBoxMaterial m_material = {
            DefaultMaterialTint, DefaultMaterialUseTexture}; //!< Render material for White Box mesh.
        DefaultShapeType m_defaultShape =
            DefaultShapeType::Cube; //!< Used for selecting a default shape for the White Box mesh.
        bool m_flipYZForExport = false; //!< Flips the Y and Z components of white box vertices when exporting for different coordinate systems
        DrawShapeData m_drawShapeData; //!< Draw Shape tool settings (shape, sides and staircase options).
        bool m_drawCarve = false; //!< When set, draw acts as a CSG boolean (same as holding Ctrl).
        bool m_drawMergeUnion = false; //!< When set, committing a drawn shape CSG-unions it into the mesh.
        bool m_edgesOnly = false; //!< When set, hide the solid render mesh and draw only the mesh edges.
        bool m_mergeGridWithMesh = false; //!< When set, the grid layer is CSG-unioned with the freeform mesh for OUTPUT only (non-destructive).
        bool m_useGlobalTint = true; //!< When set, every layer renders with the global material tint; otherwise each layer uses its own tint.
        AZ::Data::AssetId m_materialOverrideAssetId; //!< External material asset override (invalid = built-in material).
        bool m_drawUnitCube = false; //!< Draw mode click-stamps grid-snapped unit cubes (CSG) instead of drag-draw.
        float m_drawUnitCubeSize = 1.0f; //!< Desired world-space size of the next stamped cube.
        float m_voxelCellSize = 1.0f; //!< Legacy single baked cell size; only used to migrate old scenes.
        bool m_drawUnitCubeShowGrid = true; //!< Show the individual cubes (grid) in the stamp ghost preview.

        AZ::EntityId m_booleanSourceEntity; //!< Another entity whose White Box mesh is used as a boolean operand.
        Api::BooleanOperation m_booleanOperation =
            Api::BooleanOperation::Subtraction; //!< How to combine the source mesh with this one.
        bool m_hideSourceAfterApply = false;   //!< Hide the source entity after a successful Apply Boolean.
        bool m_deleteSourceAfterApply = false; //!< Delete the source entity after a successful Apply Boolean.
        bool m_booleanAffectActiveOnly = false; //!< When set, the boolean cuts only the active layer; otherwise the whole combined mesh.

        AZStd::vector<AZ::u64> m_voxelCells; //!< Packed integer cell coords filled by the Unit Cube Stamp tool.
        AZStd::vector<float> m_voxelCellSizes; //!< Per-cell world cube size (parallel to m_voxelCells) so mixed sizes coexist.
        AZStd::vector<AZ::u8> m_voxelMerged; //!< Per-cell flag (parallel to m_voxelCells): 1 = merged-with-mesh cube, 0 = separate.

        AZStd::vector<WhiteBoxLayer> m_layers;  //!< All editable layers (combined for output; edits target the active one).
        int m_activeLayerIndex = 0;             //!< Which layer is the current edit target.
        int m_loadedLayerIndex = -1;            //!< Runtime: which layer the working members currently hold (-1 = none).
        int m_lastLayerCount = -1;              //!< Runtime: layer count at the last sync (detects add/remove in the list).
        AZ::u64 m_loadedLayerId = 0;            //!< Runtime: stable id of the layer whose data is in the working members.
        AZ::u64 m_nextLayerId = 1;              //!< Next stable layer id to hand out (serialized so ids stay unique).
        AZ::u64 m_lastLayerSignature = 0;       //!< Runtime: hash of the layer id order (detects reorder as well as add/remove).

        bool m_liveBoolean = false; //!< Non-destructive: keep the base editable, evaluate the boolean for display only.
        Api::WhiteBoxMeshPtr m_displayMesh; //!< Evaluated (base [op] source) mesh used for display while live.
        Api::WhiteBoxMeshPtr m_gridMesh; //!< Stamped cubes kept SEPARATE from the freeform mesh (appended for output).
        Api::WhiteBoxMeshStream m_gridMeshData; //!< Serialized separate-cube grid mesh (component-local).
        Api::WhiteBoxMeshPtr m_gridMergedMesh; //!< Stamped cubes MERGED (CSG-unioned) with the freeform mesh for output.
        Api::WhiteBoxMeshStream m_gridMergedData; //!< Serialized merged-cube grid mesh (component-local).
        Api::WhiteBoxMeshPtr m_combinedMesh; //!< Non-serialized freeform+grid mesh used for render/collision/bounds/selection.
        //! Boolean render variant for the game-mode bake (serialized for clones). Empty without a boolean.
        WhiteBoxRenderData m_bakedBooleanRenderData;
        //! Uncut base render variant for the game-mode bake (serialized for clones).
        WhiteBoxRenderData m_bakedBaseRenderData;

        //! Re-evaluates this component's live boolean when the source entity moves.
        struct BooleanSourceListener : public AZ::TransformNotificationBus::Handler
        {
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;
            EditorWhiteBoxComponent* m_owner = nullptr;
        };
        BooleanSourceListener m_booleanSourceListener;
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
