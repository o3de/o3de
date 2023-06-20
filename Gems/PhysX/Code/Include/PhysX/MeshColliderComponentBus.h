/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <PhysX/MeshAsset.h>

namespace Physics
{
    struct MaterialId;
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

        /// Sets the mesh asset ID
        /// @note Calling this function will not recreate the physics shapes of the collider.
        /// @param id The asset ID to set it to.
        virtual void SetMeshAsset(const AZ::Data::AssetId& id) = 0;
    };

    /// Bus to service the PhysX Mesh Collider Component event group.
    using MeshColliderComponentRequestsBus = AZ::EBus<MeshColliderComponentRequests>;
} // namespace PhysX
