/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ViewportSnapping/ViewportSnapSourceBus.h>

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>

namespace AzToolsFramework
{
    namespace ViewportSnapping
    {
        //! Snap candidates for entities that do not implement ViewportSnapSourceRequestBus.
        //!
        //! Without this, snapping would only work against the handful of component types that had
        //! been taught about it, which is not much use in a level made mostly of ordinary meshes.
        //!
        //! Rather than reading model assets directly, this goes through
        //! AzFramework::VisibleGeometryRequestBus, which Atom's mesh component implements alongside
        //! anything else that wants to publish geometry. Two things follow: AzToolsFramework needs
        //! no renderer dependency, and any future component implementing that bus becomes a snap
        //! source for free.
        //!
        //! What the bus hands back is an indexed triangle soup with no polygon topology, so edges
        //! and face centres have to be *reconstructed*. Publishing raw triangle edges would be
        //! useless - a flat wall would offer a diagonal across itself as an edge to snap to, and a
        //! cube would offer twelve diagonals that do not exist in the model the artist built.
        //! Instead adjacent triangles are compared and only edges where the surface genuinely
        //! creases (or ends) are kept, which recovers the wireframe a modeller would recognise.
        //!
        //! BuildVisibleGeometry returns the whole soup and allocates while doing it, far too
        //! expensive to call per mouse move. Everything is cached per entity, keyed on the world
        //! transform it was built with, and evicted once it stops being queried.
        //!
        //! @note Not thread safe, and not intended to be: it is owned by the snapping
        //! implementation and only ever touched from the main thread during a viewport query.
        class VisibleGeometrySnapSource
        {
        public:
            //!@{
            //! Append this entity's candidates that fall inside @p volume.
            //!
            //! Builds or rebuilds the entity's cache on demand, and does nothing for entities with
            //! no visible geometry.
            void CollectSnapVertices(AZ::EntityId entityId, const SnapQueryVolume& volume, AZStd::vector<SnapVertex>& out);
            void CollectSnapEdges(AZ::EntityId entityId, const SnapQueryVolume& volume, AZStd::vector<SnapEdge>& out);
            void CollectSnapEdgeMidpoints(
                AZ::EntityId entityId, const SnapQueryVolume& volume, AZStd::vector<SnapVertex>& out);
            void CollectSnapFaceCenters(
                AZ::EntityId entityId, const SnapQueryVolume& volume, AZStd::vector<SnapVertex>& out);
            //!@}

            //! Drop caches for entities that have not been queried recently. Call once per query.
            void EvictStaleCaches();

            //! Forget everything. Used when snapping is switched off, so the memory is not held for
            //! a feature that is not running.
            void Clear();

        private:
            //! A run of spatially adjacent items plus their bounding sphere.
            //!
            //! Testing the sphere against the query cone rejects the whole run at once, which is
            //! what keeps a large mesh from being walked item by item on every mouse move.
            struct Block
            {
                AZ::Vector3 m_center = AZ::Vector3::CreateZero();
                float m_radius = 0.0f;
                AZ::u32 m_begin = 0;
                AZ::u32 m_end = 0;
            };

            struct EntityCache
            {
                //! The transform the world positions were baked with - a mismatch means rebuild.
                AZ::Transform m_worldTransform = AZ::Transform::CreateIdentity();

                AZStd::vector<AZ::Vector3> m_worldPositions;
                AZStd::vector<Block> m_positionBlocks;

                //! Feature edges only - creases and boundaries, not triangulation diagonals.
                AZStd::vector<SnapEdge> m_edges;
                AZStd::vector<Block> m_edgeBlocks;

                //! Centroid of each group of coplanar connected triangles: the centre of the quad
                //! or n-gon a modeller would see, rather than of one of its triangles.
                AZStd::vector<AZ::Vector3> m_faceCenters;
                AZStd::vector<Block> m_faceCenterBlocks;

                //! Query counter when this cache was last used, for eviction.
                AZ::u64 m_lastUsedQuery = 0;

                //! Query counter when this cache was last built.
                //!
                //! Used to retry entities that came back empty: a model still streaming in reports
                //! no geometry, and would otherwise never be looked at again.
                AZ::u64 m_lastBuildQuery = 0;

                //! The entity was inspected and had no usable geometry. Kept so it is not rebuilt
                //! on every single query.
                bool m_empty = false;

                //! Set once the cache has been built at least once, so a genuinely empty result is
                //! distinguishable from a default-constructed entry.
                bool m_built = false;

                //! Whether the edge and face arrays were populated. Reconstructing topology costs
                //! far more than collecting positions, so it is only done when a mode needs it.
                bool m_topologyBuilt = false;
            };

            //! Fetch the entity's cache, rebuilding it if the entity has moved or it is new.
            //!
            //! @param needTopology when true the edge and face-centre arrays are guaranteed
            //! populated, at the cost of the reconstruction pass. Vertex snapping does not need
            //! them and should not pay for them.
            //! @return Null when the entity has no usable geometry.
            EntityCache* AcquireCache(AZ::EntityId entityId, bool needTopology);

            //! Build or rebuild the cache for @p entityId at @p worldTransform.
            void BuildCache(
                AZ::EntityId entityId, const AZ::Transform& worldTransform, bool buildTopology, EntityCache& cache);

            //! Group an already spatially sorted point list into bounding-sphere blocks.
            static void BuildPointBlocks(const AZStd::vector<AZ::Vector3>& positions, AZStd::vector<Block>& blocks);

            AZStd::unordered_map<AZ::EntityId, EntityCache> m_caches;
            AZ::u64 m_queryCounter = 0;
        };
    } // namespace ViewportSnapping
} // namespace AzToolsFramework
