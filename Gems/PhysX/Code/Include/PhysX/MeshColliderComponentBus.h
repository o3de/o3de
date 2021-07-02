/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

        /// Gets the material id from the material library for this entity.
        /// @return The asset ID to set it to.
        virtual Physics::MaterialId GetMaterialId() const = 0;

        /// Sets the mesh asset ID
        /// @param id The asset ID to set it to.
        virtual void SetMeshAsset(const AZ::Data::AssetId& id) = 0;

        /// Sets the material id from the material library.
        /// @param id The asset ID to set it to.
        virtual void SetMaterialId(const Physics::MaterialId& id) = 0;
    };

    /// Bus to service the PhysX Mesh Collider Component event group.
    using MeshColliderComponentRequestsBus = AZ::EBus<MeshColliderComponentRequests>;
} // namespace PhysX
