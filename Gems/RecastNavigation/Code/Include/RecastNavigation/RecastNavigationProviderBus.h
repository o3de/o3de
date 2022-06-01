/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <Misc/RecastHelpers.h>

namespace RecastNavigation
{
    //! The interface for @RecastNavigationProviderRequestBus.
    class RecastNavigationProviderRequests
        : public AZ::ComponentBus
    {
    public:
        //! Collects the geometry (triangles) within the configured area.
        //! @param tileSize A navigation mesh is made up of tiles. Each tile is a square of the same size.
        //! @param borderSize An additional extent in each dimension around each tile. In order for navigation tiles to connect
        //!               to their respective neighboring tiles, they need additional geometry in the near vicinity.
        //! @returns a container with triangle data for each tile.
        virtual AZStd::vector<AZStd::shared_ptr<TileGeometry>> CollectGeometry(float tileSize, float borderSize) = 0;

        //! A navigation mesh is made up of tiles. Each tile is a square of the same size.
        //! @param tileSize size of square tiles that make up a navigation mesh.
        //! @returns number of tiles that would be necessary to the cover the required area provided by @GetWorldBounds.
        virtual int GetNumberOfTiles(float tileSize) const = 0;

        //! Returns the world bounds that this surveyor is configured to collect geometry.
        //! @returns an axis aligned bounding box of the world bounds.
        virtual AZ::Aabb GetWorldBounds() const = 0;
    };

    //! Request EBus for a navigation provider component that collects geometry data.
    using RecastNavigationProviderRequestBus = AZ::EBus<RecastNavigationProviderRequests>;
} // namespace RecastNavigation
