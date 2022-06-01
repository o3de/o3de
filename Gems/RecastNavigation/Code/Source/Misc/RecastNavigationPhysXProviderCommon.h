/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Task/TaskExecutor.h>
#include <AzCore/Task/TaskGraph.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <Misc/RecastHelpers.h>

namespace RecastNavigation
{
    //! Common logic for Recast navigation tiled collector components. Recommended use is as a base class.
    //! The method provided are not thread-safe. Synchronize as necessary at the higher level.
    class RecastNavigationPhysXProviderCommon
    {
    public:
        AZ_RTTI(RecastNavigationPhysXProviderCommon, "{182D93F8-9E76-409B-9939-6816509A6F52}");
        //! Use it to configure geometry collection in either Editor PhysX scene or game scene.
        //! @param useEditorScene if true, collect geometry from Editor PhysX scene, otherwise game scene.
        explicit RecastNavigationPhysXProviderCommon(bool useEditorScene);
        virtual ~RecastNavigationPhysXProviderCommon() = default;

        //! A container of PhysX overlap scene hits (has PhysX colliders and their position/orientation).
        using QueryHits = AZStd::vector<AzPhysics::SceneQueryHit>;

        //! Blocking call. Collects all the relevant PhysX geometry within a provided volume.
        //! @param tileSize the result is packaged in tiles, which are squares covering the provided volume of @worldVolume
        //! @param borderSize an additional extend in all direction around the tile volume, this additional geometry will allow Recast to connect tiles together.
        //! @param worldVolume the overall volume to collect static PhysX geometry
        //! @param debugDrawInputData if true, debug draw will show the geometry collected
        //! @returns an array of tiles, each containing indexed geometry
        AZStd::vector<AZStd::shared_ptr<TileGeometry>> CollectGeometryImpl(
            float tileSize,
            float borderSize,
            const AZ::Aabb& worldVolume,
            bool debugDrawInputData);

        //! Finds all the static PhysX colliders within a given volume.
        //! @param volume the world to look for static colliders
        //! @param overlapHits (out) found colliders will be attached to this container
        void CollectCollidersWithinVolume(const AZ::Aabb& volume, QueryHits& overlapHits);

        //! Given a container of static colliders return indexed triangle data.
        //! @param geometry (out) triangle data will be added
        //! @param overlapHits (in) an array of static PhysX colliders
        //! @param debugDrawInputData (optional) debug visualization options to show found triangles
        void AppendColliderGeometry(TileGeometry& geometry, const QueryHits& overlapHits, bool debugDrawInputData);

        //! Returns the built-in names for the PhysX scene, either Editor or game scene.
        const char* GetSceneName() const;

        //! Either use Editor PhysX world or game PhysX world.
        bool m_useEditorScene;
    };

} // namespace RecastNavigation
