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

        //! Number of sides the draw-shape tool uses for round / N-gon shapes
        //! (4 = box / square). Sourced from the component's "Draw Sides" property.
        virtual int GetDrawSides() { return 4; }

        //! Shape the draw-shape tool builds, from the component's "Draw Shape" property.
        virtual DrawShapeType GetDrawShape() { return DrawShapeType::Box; }

        //! Staircase build parameters (step count, step-height division mode, riser height and
        //! 90-degree rotation), sourced from the component's Staircase properties. Returned as a
        //! single snapshot so callers fetch the whole group in one request.
        virtual DrawStairInfo GetDrawStairInfo() { return DrawStairInfo{}; }

        //! When true, draw acts as a CSG boolean (same as holding Ctrl): pulling in
        //! carves/subtracts, pulling out adds/unions.
        virtual bool GetDrawCarve() { return false; }

        //! When true, draw mode click-stamps grid-snapped 1x1x1 cubes (CSG union, or
        //! subtract with Ctrl) instead of the click-drag-pull workflow.
        virtual bool GetDrawUnitCube() { return false; }

        //! Fill (@p filled true) or clear a 1x1x1 voxel cell whose minimum corner is
        //! @p cellMin (integer local coordinates). The mesh is regenerated from the
        //! voxel set as a clean, watertight, merged surface (no CSG round-trip).
        virtual void SetVoxelCell([[maybe_unused]] const AZ::Vector3& cellMin, [[maybe_unused]] bool filled) {}

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

    protected:
        ~EditorWhiteBoxComponentNotifications() = default;
    };

    using EditorWhiteBoxComponentNotificationBus = AZ::EBus<EditorWhiteBoxComponentNotifications>;
} // namespace WhiteBox
