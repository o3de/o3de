/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <PhysX/MeshAsset.h>

namespace Physics
{
    class MaterialId;
}

namespace PhysX
{
    /// Services provided by the PhysX Mesh Collider Component.
    class MeshColliderComponentRequests
        : public AZ::ComponentBus
    {
    public:
        /// Gets Mesh information from this entity.
        /// @return Asset pointer to mesh asset.
        virtual AZ::Data::Asset<Pipeline::MeshAsset> GetMeshAsset() const = 0;

        /// Gets the mesh triangles as a list of verts and indices.
        /// @param verts The list of verts in the mesh
        /// @param indices The ordering of the verts into triangles
        virtual void GetStaticWorldSpaceMeshTriangles(AZStd::vector<AZ::Vector3>& verts, AZStd::vector<AZ::u32>& indices) const = 0;

        /// Gets the material id from the material library for this entity.
        /// @return The asset ID to set it to.
        virtual Physics::MaterialId GetMaterialId() const = 0;

        /// Sets the mesh asset ID
        /// @param id The asset ID to set it to.
        virtual void SetMeshAsset(const AZ::Data::AssetId& id) = 0;

        /// Sets the material library asset to the collider.
        /// @param id The asset ID to set it to.
        virtual void SetMaterialAsset(const AZ::Data::AssetId& id) = 0;

        /// Sets the material id from the material library.
        /// @param id The asset ID to set it to.
        virtual void SetMaterialId(const Physics::MaterialId& id) = 0;
    };

    /// Bus to service the PhysX Mesh Collider Component event group.
    using MeshColliderComponentRequestsBus = AZ::EBus<MeshColliderComponentRequests>;
} // namespace PhysX