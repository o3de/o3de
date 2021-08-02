/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

namespace AZ
{
    class Transform;
}

namespace WhiteBox
{
    struct WhiteBoxMaterial;
    struct WhiteBoxRenderData;

    //! A generic interface for the White Box Component to communicate
    //! with regardless of the rendering backend.
    class RenderMeshInterface
    {
    public:
        AZ_RTTI(RenderMeshInterface, "{F3ADF2DC-6A40-4943-95BE-6C7E24605BE9}");

        virtual ~RenderMeshInterface() = 0;

        //! Take White Box render data and populate the render mesh from it.
        virtual void BuildMesh(
            const WhiteBoxRenderData& renderData, const AZ::Transform& worldFromLocal,
            AZ::EntityId entityId) = 0; // TODO: LYN-786

        //! Update the transform of the render mesh.
        virtual void UpdateTransform(const AZ::Transform& worldFromLocal) = 0;

        //! Update the material of the render mesh.
        virtual void UpdateMaterial(const WhiteBoxMaterial& material) = 0;

        // Return if the White Box mesh is visible or not.
        virtual bool IsVisible() const = 0;

        //! Set the White Box mesh visible (true) or invisible (false).
        virtual void SetVisiblity(bool visibility) = 0;
    };
} // namespace WhiteBox
