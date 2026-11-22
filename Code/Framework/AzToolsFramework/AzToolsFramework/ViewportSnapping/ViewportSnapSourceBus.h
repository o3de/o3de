/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/vector.h>

namespace AzToolsFramework
{
    //! Geometry snapping in the editor viewport.
    //!
    //! The editor's built-in snapping is to a grid or to an angle: the value being dragged is
    //! quantised, with no reference to anything else in the level. Snapping a vertex of one mesh
    //! onto a vertex of another - what most DCC tools mean by "snapping" - needs a different
    //! shape of API, because the candidate positions come from the scene rather than from a
    //! setting.
    //!
    //! The split here is deliberate and has two halves:
    //!
    //! * **Sources** publish candidate geometry. Any component that owns geometry can implement
    //!   ViewportSnapSourceRequestBus and contribute to snapping without knowing anything about
    //!   who consumes it. See this header.
    //! * **Consumers** ask for the best candidate under the cursor. See ViewportSnapping.h.
    //!
    //! Both halves are declaration-only, which is the point: a gem can provide snappable geometry,
    //! or consume snapping, without linking against whoever implements the other half - or indeed
    //! without that implementation being present at all.
    namespace ViewportSnapping
    {
        //! A single candidate snap point contributed by a component, in world space.
        struct SnapVertex
        {
            SnapVertex() = default;
            SnapVertex(const AZ::Vector3& worldPosition, const AZ::EntityId entityId, const AZ::s64 sourceIndex)
                : m_worldPosition(worldPosition)
                , m_entityId(entityId)
                , m_sourceIndex(sourceIndex)
            {
            }

            AZ::Vector3 m_worldPosition = AZ::Vector3::CreateZero();
            AZ::EntityId m_entityId; //!< The entity that contributed this vertex.

            //! Opaque, source-defined identifier for the geometry this came from.
            //!
            //! A source that has a stable notion of vertex identity (a mesh half-edge structure,
            //! say) can put its handle here, letting a consumer distinguish "the same vertex" from
            //! "a different vertex that happens to be coincident". Consumers that do not recognise
            //! the source should ignore it.
            //!
            //! @note Signed, because the handle types this typically wraps use -1 as their invalid
            //! value and a cast to an unsigned type would assert on that.
            AZ::s64 m_sourceIndex = 0;
        };

        //! A candidate edge contributed by a component, in world space.
        struct SnapEdge
        {
            SnapEdge() = default;
            SnapEdge(
                const AZ::Vector3& start, const AZ::Vector3& end, const AZ::EntityId entityId, const AZ::s64 sourceIndex)
                : m_start(start)
                , m_end(end)
                , m_entityId(entityId)
                , m_sourceIndex(sourceIndex)
            {
            }

            AZ::Vector3 m_start = AZ::Vector3::CreateZero();
            AZ::Vector3 m_end = AZ::Vector3::CreateZero();
            AZ::EntityId m_entityId;
            AZ::s64 m_sourceIndex = 0;
        };

        //! The region a snap query cares about: a thin cone around the cursor ray.
        //!
        //! Sources test each candidate against this before adding it to the result. The test is a
        //! handful of floating point operations and rejects effectively everything, which matters
        //! because it runs for every vertex of every nearby source on every mouse move.
        //!
        //! A cone rather than a bounding box around the ray, because a long ray's bounding box
        //! swallows most of the level and so rejects almost nothing - which does not remove the
        //! work, it just moves it into the much more expensive screen-space projection stage.
        struct SnapQueryVolume
        {
            AZ::Vector3 m_rayOrigin = AZ::Vector3::CreateZero();
            AZ::Vector3 m_rayDirection = AZ::Vector3::CreateAxisY(); //!< Normalised.
            float m_maxDistance = 0.0f;

            //! Cone radius grows with distance under a perspective projection...
            float m_radiusPerDistance = 0.0f;
            //! ...and is constant under an orthographic one. Summing the two covers both.
            float m_radiusConstant = 0.0f;

            //! Conservative world-space bounds of the whole cone, so an entire source can be
            //! rejected before any of its geometry is touched.
            AZ::Aabb m_bounds = AZ::Aabb::CreateNull();

            //! Is @p worldPosition inside the cone.
            bool Contains(const AZ::Vector3& worldPosition) const
            {
                const AZ::Vector3 toPoint = worldPosition - m_rayOrigin;

                const float distanceAlongRay = toPoint.Dot(m_rayDirection);
                if (distanceAlongRay < 0.0f || distanceAlongRay > m_maxDistance)
                {
                    return false;
                }

                const float radius = m_radiusConstant + m_radiusPerDistance * distanceAlongRay;
                const AZ::Vector3 offsetFromRay = toPoint - m_rayDirection * distanceAlongRay;

                return offsetFromRay.GetLengthSq() <= radius * radius;
            }

            //! Could a segment overlap the cone.
            //!
            //! Conservative: tests the segment's bounding sphere, which is cheap and never rejects
            //! a segment that really does intersect.
            bool IntersectsSegment(const AZ::Vector3& start, const AZ::Vector3& end) const
            {
                const AZ::Vector3 center = (start + end) * 0.5f;
                const float radius = (end - start).GetLength() * 0.5f;
                return IntersectsSphere(center, radius);
            }

            //! Could a sphere overlap the cone.
            //!
            //! Conservative - false means definitely no overlap. Lets a source reject a whole
            //! block of geometry at once by testing its bounding sphere, so a large mesh does not
            //! have to be walked point by point.
            bool IntersectsSphere(const AZ::Vector3& center, const float radius) const
            {
                const AZ::Vector3 toCenter = center - m_rayOrigin;

                const float distanceAlongRay = toCenter.Dot(m_rayDirection);
                if (distanceAlongRay < -radius || distanceAlongRay > m_maxDistance + radius)
                {
                    return false;
                }

                // Clamp to the segment so a sphere straddling either end is measured against the
                // nearest point on the cone axis rather than an extrapolated one.
                const float clampedDistance = AZ::GetClamp(distanceAlongRay, 0.0f, m_maxDistance);
                const float coneRadius = m_radiusConstant + m_radiusPerDistance * clampedDistance;
                const AZ::Vector3 offsetFromRay = toCenter - m_rayDirection * clampedDistance;

                const float allowed = coneRadius + radius;
                return offsetFromRay.GetLengthSq() <= allowed * allowed;
            }
        };

        //! Implemented by components that can contribute snappable geometry.
        //!
        //! Addressed by EntityId with a Multiple handler policy, so several components on one
        //! entity may each contribute.
        class ViewportSnapSourceRequests : public AZ::ComponentBus
        {
        public:
            //! Append every vertex inside @p volume to @p out.
            //!
            //! @note This runs on every mouse move while snapping is active. Implementations
            //! should keep world-space positions cached - invalidated when the geometry or the
            //! transform changes - and do nothing here but a tight loop of
            //! SnapQueryVolume::Contains. Rebuilding positions, allocating, or making per-vertex
            //! EBus calls in here is what makes dragging feel heavy.
            virtual void CollectSnapVertices(const SnapQueryVolume& volume, AZStd::vector<SnapVertex>& out) const = 0;

            //!@{
            //! Optional additional candidate sets, for the snap modes beyond plain vertices.
            //!
            //! Edge midpoints and face centres are *points*, so they run through exactly the same
            //! pipeline as vertices; a source that can provide them gets those modes for free.
            //! Edges are segments and are queried differently - the target is the closest position
            //! along the segment to the cursor ray, rather than one of a set of discrete points.
            //!
            //! All three default to contributing nothing, so a source implements only what it can
            //! describe well. A source that knows only triangles, for instance, has no meaningful
            //! notion of a polygon border and is better off leaving edges alone than publishing
            //! every internal triangulation edge as if it were one.
            virtual void CollectSnapEdges(
                [[maybe_unused]] const SnapQueryVolume& volume, [[maybe_unused]] AZStd::vector<SnapEdge>& out) const
            {
            }

            virtual void CollectSnapEdgeMidpoints(
                [[maybe_unused]] const SnapQueryVolume& volume, [[maybe_unused]] AZStd::vector<SnapVertex>& out) const
            {
            }

            virtual void CollectSnapFaceCenters(
                [[maybe_unused]] const SnapQueryVolume& volume, [[maybe_unused]] AZStd::vector<SnapVertex>& out) const
            {
            }
            //!@}

            //! Coarse world-space bounds of everything this source could contribute.
            //!
            //! Used to reject the source before any Collect call is made. Return a null Aabb when
            //! the source has nothing to offer or is not ready yet.
            virtual AZ::Aabb GetSnapVertexBounds() const = 0;

        protected:
            ~ViewportSnapSourceRequests() = default;
        };

        using ViewportSnapSourceRequestBus = AZ::EBus<ViewportSnapSourceRequests>;
    } // namespace ViewportSnapping
} // namespace AzToolsFramework
