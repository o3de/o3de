/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "EditorWhiteBoxDefaultShapeTypes.h"

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/vector.h>

namespace WhiteBox
{
    struct WhiteBoxMesh;

    //! Wrapper around WhiteBoxMesh address.
    struct WhiteBoxMeshHandle
    {
        uintptr_t m_whiteBoxMeshAddress; //!< The raw address of the WhiteBoxMesh pointer.
    };

    //! EditorWhiteBoxComponent requests.
    class EditorWhiteBoxComponentRequests : public AZ::EntityComponentBus
    {
    public:
        // EBusTraits overrides ...
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Return a pointer to the WhiteBoxMesh.
        virtual WhiteBoxMesh* GetWhiteBoxMesh() = 0;

        //! The mesh used for render / collision / selection (stamps and live boolean folded in).
        virtual WhiteBoxMesh* GetEvaluatedWhiteBoxMesh() { return GetWhiteBoxMesh(); }

        //! Subtract @p cutter from the stamped-cube grids. Returns true if any grid changed.
        virtual bool CarveCubeGrids(
            [[maybe_unused]] const WhiteBoxMesh& cutter, [[maybe_unused]] const AZ::Transform& cutterTransform)
        {
            return false;
        }

        //! Map entity-local geometry into the active layer's storage space (inverse layer transform),
        //! for vertices from @p firstVertexIndex on. No-op for an identity layer transform.
        virtual void MapMeshToActiveLayerSpace(
            [[maybe_unused]] WhiteBoxMesh& mesh, [[maybe_unused]] size_t firstVertexIndex)
        {
        }

        //! Return a handle wrapping the raw address of the WhiteBoxMesh pointer.
        //! @note This is currently used to address the WhiteBoxMesh via script.
        virtual WhiteBoxMeshHandle GetWhiteBoxMeshHandle();

        //! Serialize the current mesh.
        //! Take the in-memory representation of the WhiteBoxMesh and write it to
        //! an output stream.
        //! @note The data is either stored directly on the Component or in an Asset.
        virtual void SerializeWhiteBox() = 0;

        //! Deserialize the stored mesh data.
        //! Take the previously serialized (stored) WhiteBoxMesh data and create a new
        //! WhiteBoxMesh from it.
        //! @note The data is either loaded directly from the Component or from an Asset.
        virtual void DeserializeWhiteBox() = 0;

        //! If an Asset is in use, write the data from it back to be stored directly on the Component.
        virtual void WriteAssetToComponent() = 0;

        //! Rebuild the White Box representation.
        //! @note Includes the render mesh and physics mesh (if present).
        virtual void RebuildWhiteBox() = 0;

        //! Set the white box mesh default shape.
        virtual void SetDefaultShape(DefaultShapeType defaultShape) = 0;

        //! Set the global material tint (used when "Use Global Tint" is on).
        virtual void SetMaterialTint([[maybe_unused]] const AZ::Color& tint) {}
        //! Get the global material tint.
        virtual AZ::Color GetMaterialTint() { return AZ::Color(1.0f, 1.0f, 1.0f, 1.0f); }
        //! Turn the material's texture on/off (off gives a flat solid colour).
        virtual void SetMaterialUseTexture([[maybe_unused]] bool useTexture) {}
        //! Override the render material with an external material asset (e.g. from another gem).
        //! Pass an invalid AssetId to revert to the built-in White Box material.
        virtual void SetMaterialOverride([[maybe_unused]] const AZ::Data::AssetId& materialAssetId) {}
        //! Get the current material override asset id (invalid if using the built-in material).
        virtual AZ::Data::AssetId GetMaterialOverride() { return AZ::Data::AssetId(); }

        //! Number of sides the draw-shape tool uses for round / N-gon shapes
        //! (4 = box / square). Sourced from the component's "Draw Sides" property.
        virtual int GetDrawSides() { return 4; }
        //! Tube tessellation for a drawn torus (see DefaultTubeSides).
        virtual int GetDrawTubeSides()
        {
            return 12;
        }
        virtual float GetDrawHoleRatio() { return 0.5f; }

        //! Shape the draw-shape tool builds, from the component's "Draw Shape" property.
        virtual DrawShapeType GetDrawShape() { return DrawShapeType::Box; }
        //! Pick the solid the Draw Shape tool builds. Also resets Draw Sides to that shape's default,
        //! the same as choosing it in the pane.
        virtual void SetDrawShapeType(DrawShapeType /*shape*/) {}

        //! Staircase build parameters (step count, step-height division mode, riser height and
        //! 90-degree rotation), sourced from the component's Staircase properties. Returned as a
        //! single snapshot so callers fetch the whole group in one request.
        virtual DrawStairInfo GetDrawStairInfo() { return DrawStairInfo{}; }

        //! When true, draw acts as a CSG boolean (same as holding Ctrl): pulling in
        //! carves/subtracts, pulling out adds/unions.
        virtual bool GetDrawCarve() { return false; }

        //! When true, committing a drawn shape CSG-unions it into the mesh (a clean,
        //! watertight, manifold merge) instead of adding overlapping geometry.
        virtual bool GetDrawMergeUnion() { return false; }
        virtual bool GetDrawPolygonExtrude() { return false; }

        //! When true, draw mode click-stamps grid-snapped 1x1x1 cubes (CSG union, or
        //! subtract with Ctrl) instead of the click-drag-pull workflow.
        //! Whether the latched Extrude / Inset toggles are on (see SetStickyExtrude).
        virtual bool GetStickyExtrude() { return false; }
        virtual bool GetStickyInset() { return false; }
        virtual bool GetDrawUnitCube() { return false; }

        //! World-space edge length of one stamped cube. Each stamp places a single atomic
        //! cube (one grid cell) of this size; the grid spacing equals this value.
        virtual float GetDrawUnitCubeSize() { return 1.0f; }

        //! Whether the Unit Cube ghost preview shows the individual cubes (grid) or just
        //! the single outer box.
        virtual bool GetDrawUnitCubeShowGrid() { return true; }

        //! Fill (@p filled true) or clear a 1x1x1 voxel cell whose minimum corner is
        //! @p cellMin (integer local coordinates). The mesh is regenerated from the
        //! voxel set as a clean, watertight, merged surface (no CSG round-trip).
        virtual void SetVoxelCell([[maybe_unused]] const AZ::Vector3& cellMin, [[maybe_unused]] bool filled) {}

        //! Fill or clear many voxel cells at once (one undo batch, a single mesh
        //! regeneration). @p cellMins are the integer-local minimum corners.
        virtual void SetVoxelCells(
            [[maybe_unused]] const AZStd::vector<AZ::Vector3>& cellMins, [[maybe_unused]] bool filled) {}

        //! Build a simplified collision triangle set for a voxel-stamped mesh; false if it has no stamps.
        virtual bool BuildColliderMesh(
            [[maybe_unused]] AZStd::vector<AZ::Vector3>& vertices, [[maybe_unused]] AZStd::vector<AZ::u32>& indices)
        {
            return false;
        }

    protected:
        ~EditorWhiteBoxComponentRequests() = default;
    };

    inline WhiteBoxMeshHandle EditorWhiteBoxComponentRequests::GetWhiteBoxMeshHandle()
    {
        return WhiteBoxMeshHandle{reinterpret_cast<uintptr_t>(static_cast<void*>(GetWhiteBoxMesh()))};
    }

    using EditorWhiteBoxComponentRequestBus = AZ::EBus<EditorWhiteBoxComponentRequests>;

    //! EditorWhiteBoxComponent notifications.
    class EditorWhiteBoxComponentNotifications : public AZ::EntityComponentBus
    {
    public:
        //! Notify the component the mesh has been modified.
        virtual void OnWhiteBoxMeshModified() {}

        //! Notify listeners when the default shape of the white box mesh changes.
        virtual void OnDefaultShapeTypeChanged([[maybe_unused]] DefaultShapeType defaultShape) {}

        //! Notify listeners when the LAYER STRUCTURE changed (layer added / removed / reordered /
        //! auto-created by drawing into an empty white box) so any layer UI can resync.
        virtual void OnLayerStructureChanged() {}

    protected:
        ~EditorWhiteBoxComponentNotifications() = default;
    };

    using EditorWhiteBoxComponentNotificationBus = AZ::EBus<EditorWhiteBoxComponentNotifications>;
} // namespace WhiteBox
