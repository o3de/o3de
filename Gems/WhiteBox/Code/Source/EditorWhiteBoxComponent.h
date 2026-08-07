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

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/std/containers/unordered_set.h>
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

        // EditorWhiteBoxComponentRequestBus overrides ...
        WhiteBoxMesh* GetWhiteBoxMesh() override;
        void SerializeWhiteBox() override;
        void DeserializeWhiteBox() override;
        void WriteAssetToComponent() override;
        void RebuildWhiteBox() override;
        void SetDefaultShape(DefaultShapeType defaultShape) override;
        int GetDrawSides() override { return m_drawShapeData.m_sides; }
        DrawShapeType GetDrawShape() override { return m_drawShapeData.m_shape; }
        DrawStairInfo GetDrawStairInfo() override
        {
            const DrawStairData& stair = m_drawShapeData.m_stair;
            return DrawStairInfo{stair.m_steps, stair.m_byHeight, stair.m_stepHeight, stair.m_rotation};
        }
        bool GetDrawCarve() override { return m_drawCarve; }
        bool GetDrawUnitCube() override { return m_drawUnitCube; }
        void SetVoxelCell(const AZ::Vector3& cellMin, bool filled) override;

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
        AZ::Crc32 OnDefaultShapeChange();
        //! Apply a CSG boolean using the White Box mesh on m_booleanSourceEntity.
        void ApplyBoolean();
        //! Update the voxel-stamped geometry in place: remove the faces belonging to
        //! the previous voxel surface (@p oldCells) and add the surface for the new
        //! cell set (@p newCells), leaving all freeform mesh edits intact.
        void RegenerateVoxelMesh(
            const AZStd::unordered_set<AZ::u64>& oldCells, const AZStd::unordered_set<AZ::u64>& newCells);

        //! The mesh used for RENDER / collision / bounds / selection. In live
        //! (non-destructive) boolean mode this is the evaluated result; otherwise
        //! it is the editable base mesh (GetWhiteBoxMesh).
        WhiteBoxMesh* EvaluatedMesh();
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

        using ComponentModeDelegate = AzToolsFramework::ComponentModeFramework::ComponentModeDelegate;
        ComponentModeDelegate m_componentModeDelegate; //!< Responsible for detecting ComponentMode activation
                                                       //!< and creating a concrete ComponentMode.

        Api::WhiteBoxMeshPtr m_whiteBox; //!< Handle/opaque pointer to the White Box mesh data.
        AZStd::optional<AZStd::unique_ptr<RenderMeshInterface>>
            m_renderMesh; //!< The render mesh to use for the White Box mesh data.
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
        bool m_drawUnitCube = false; //!< Draw mode click-stamps grid-snapped unit cubes (CSG) instead of drag-draw.

        AZ::EntityId m_booleanSourceEntity; //!< Another entity whose White Box mesh is used as a boolean operand.
        Api::BooleanOperation m_booleanOperation =
            Api::BooleanOperation::Subtraction; //!< How to combine the source mesh with this one.
        bool m_hideSourceAfterApply = false;   //!< Hide the source entity after a successful Apply Boolean.
        bool m_deleteSourceAfterApply = false; //!< Delete the source entity after a successful Apply Boolean.

        AZStd::vector<AZ::u64> m_voxelCells; //!< Packed integer cell coords filled by the Unit Cube Stamp tool.

        bool m_liveBoolean = false; //!< Non-destructive: keep the base editable, evaluate the boolean for display only.
        Api::WhiteBoxMeshPtr m_displayMesh; //!< Evaluated (base [op] source) mesh used for display while live.

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
