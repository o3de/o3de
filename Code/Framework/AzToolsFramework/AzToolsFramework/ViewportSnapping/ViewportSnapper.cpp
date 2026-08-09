/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ViewportSnapping/ViewportSnapper.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/math.h>
#include <AzCore/std/sort.h>
#include <AzFramework/Render/GeometryIntersectionStructures.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzFramework/Viewport/ScreenGeometry.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzFramework/Visibility/BoundsBus.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

AZ_CVAR(
    bool, ed_snapOcclusion, true, nullptr, AZ::ConsoleFunctorFlags::Null,
    "Geometry snapping ignores candidates hidden behind other geometry. Turn off to snap to "
    "interior or back-facing corners.");
AZ_CVAR(
    int, ed_snapMaxOcclusionTests, 8, nullptr, AZ::ConsoleFunctorFlags::Null,
    "Most candidates to occlusion test per snap query. Each test is a scene raycast; once the "
    "budget is spent the remaining candidates are accepted untested.");
AZ_CVAR(
    float, ed_snapOcclusionBias, 0.005f, nullptr, AZ::ConsoleFunctorFlags::Null,
    "Occlusion depth tolerance as a fraction of the distance to the candidate. A vertex lies on "
    "its own surface, so some slack is needed or everything reports itself occluded.");
AZ_CVAR(
    float, ed_snapOcclusionMinBias, 0.01f, nullptr, AZ::ConsoleFunctorFlags::Null,
    "Floor for the occlusion depth tolerance in metres, for candidates very close to the camera.");

namespace AzToolsFramework
{
    namespace ViewportSnapping
    {
        namespace
        {
            //! Build the query cone around the cursor ray.
            //!
            //! Handles both projections without branching on them: under perspective the rays
            //! diverge, so the radius grows with distance; under orthographic they are parallel but
            //! offset, so the cone is really a cylinder. Measuring both terms from an actual offset
            //! ray covers each case and any projection in between.
            SnapQueryVolume BuildSnapQueryVolume(
                const AzFramework::CameraState& cameraState,
                const AzFramework::ScreenPoint& screenPoint,
                const SnapQueryConfig& config)
            {
                using AzToolsFramework::ViewportInteraction::ViewportScreenToWorldRay;

                const auto centreRay = ViewportScreenToWorldRay(cameraState, screenPoint);

                // static_cast rather than aznumeric_cast: a fractional radius is legal here and
                // must not trip the round-trip assert. +1 rather than a ceil, since this only has
                // to be an over-estimate.
                const AzFramework::ScreenPoint offsetScreenPoint{ screenPoint.m_x +
                                                                     static_cast<int>(config.m_screenRadiusPixels) + 1,
                                                                 screenPoint.m_y };
                const auto offsetRay = ViewportScreenToWorldRay(cameraState, offsetScreenPoint);

                SnapQueryVolume volume;
                volume.m_rayOrigin = centreRay.m_origin;
                volume.m_rayDirection = centreRay.m_direction;
                volume.m_maxDistance = config.m_maxWorldDistance;
                volume.m_radiusPerDistance = (offsetRay.m_direction - centreRay.m_direction).GetLength();
                volume.m_radiusConstant = (offsetRay.m_origin - centreRay.m_origin).GetLength() + 0.01f;

                // Conservative AABB of the cone, used only to reject a whole source before its
                // geometry is touched.
                const float padding = volume.m_radiusConstant + volume.m_radiusPerDistance * volume.m_maxDistance;
                AZ::Aabb bounds = AZ::Aabb::CreateFromPoint(centreRay.m_origin);
                bounds.AddPoint(centreRay.m_origin + centreRay.m_direction * volume.m_maxDistance);

                const AZ::Vector3 pad(padding);
                volume.m_bounds = AZ::Aabb::CreateFromMinMax(bounds.GetMin() - pad, bounds.GetMax() + pad);

                return volume;
            }

            //! The point on segment [start, end] closest to the infinite ray from @p rayOrigin.
            //!
            //! Standard closest-approach of two lines, with the segment parameter clamped to its
            //! ends and a fallback for the degenerate parallel case. Used to turn an edge into the
            //! single position it would snap to.
            AZ::Vector3 ClosestPointOnSegmentToRay(
                const AZ::Vector3& start,
                const AZ::Vector3& end,
                const AZ::Vector3& rayOrigin,
                const AZ::Vector3& rayDirection)
            {
                const AZ::Vector3 segment = end - start;
                const float segmentLengthSq = segment.GetLengthSq();
                if (segmentLengthSq <= AZ::Constants::FloatEpsilon)
                {
                    return start;
                }

                const AZ::Vector3 originToStart = start - rayOrigin;
                const float segmentDotRay = segment.Dot(rayDirection);
                const float startDotSegment = originToStart.Dot(segment);
                const float startDotRay = originToStart.Dot(rayDirection);

                // rayDirection is normalised, so its own dot product is 1 and drops out of the
                // usual formula's denominator.
                const float denominator = segmentLengthSq - segmentDotRay * segmentDotRay;

                float segmentParameter = 0.0f;
                if (denominator > AZ::Constants::FloatEpsilon)
                {
                    segmentParameter = (segmentDotRay * startDotRay - startDotSegment) / denominator;
                }
                else
                {
                    // The edge is parallel to the line of sight, so every point on it is equally
                    // close. Take the end nearest the camera, which at least keeps the result
                    // stable as the cursor moves rather than flickering between the two.
                    segmentParameter = startDotSegment < 0.0f ? 1.0f : 0.0f;
                }

                return start + segment * AZ::GetClamp(segmentParameter, 0.0f, 1.0f);
            }
        } // namespace

        void ViewportSnapper::Register()
        {
            if (!m_registered)
            {
                ViewportSnappingRequests::Register(this);
                m_registered = true;
            }
        }

        void ViewportSnapper::Unregister()
        {
            if (m_registered)
            {
                ViewportSnappingRequests::Unregister(this);
                m_registered = false;
            }
        }

        SnapMode ViewportSnapper::GetSnapMode() const
        {
            return m_mode;
        }

        void ViewportSnapper::SetSnapMode(const SnapMode mode)
        {
            m_mode = mode;

            if (m_mode == SnapMode::Off)
            {
                // Do not hold mesh caches while the feature is off.
                m_visibleGeometrySource.Clear();
            }
        }

        bool ViewportSnapper::IsSnappingEnabled() const
        {
            return m_mode != SnapMode::Off;
        }

        AZStd::optional<SnapResult> ViewportSnapper::QuerySnapTarget(
            const AzFramework::ViewportId viewportId,
            const AzFramework::ScreenPoint& screenPoint,
            const SnapQueryConfig& config) const
        {
            if (m_mode == SnapMode::Off)
            {
                m_visibleGeometrySource.Clear();
                return AZStd::nullopt;
            }

            m_visibleGeometrySource.EvictStaleCaches();

            const AzFramework::CameraState cameraState = GetCameraState(viewportId);
            if (cameraState.m_viewportSize.m_width <= 0 || cameraState.m_viewportSize.m_height <= 0)
            {
                // No valid viewport - GetCameraState returned a default-constructed state.
                return AZStd::nullopt;
            }

            const SnapQueryVolume volume = BuildSnapQueryVolume(cameraState, screenPoint, config);

            // Face snapping is entirely a scene raycast - it needs no candidate gathering at all.
            if (m_mode == SnapMode::Face)
            {
                return QueryFaceSnap(cameraState, volume, config);
            }

            CollectVisibleSourceEntities(viewportId, volume, config);

            if (m_mode == SnapMode::Edge)
            {
                return QueryEdgeSnap(cameraState, screenPoint, volume, config);
            }

            return QueryPointSnap(cameraState, screenPoint, config);
        }

        void ViewportSnapper::CollectVisibleSourceEntities(
            const AzFramework::ViewportId viewportId, const SnapQueryVolume& volume, const SnapQueryConfig& config) const
        {
            m_visibleEntities.clear();
            ViewportInteraction::EditorEntityViewportInteractionRequestBus::Event(
                viewportId, &ViewportInteraction::EditorEntityViewportInteractionRequests::FindVisibleEntities,
                m_visibleEntities);

            m_candidates.clear();
            m_edgeCandidates.clear();

            for (const AZ::EntityId entityId : m_visibleEntities)
            {
                if (entityId == config.m_excludeEntity && !config.m_includeExcludedEntity)
                {
                    continue;
                }

                if (!config.m_excludeEntities.empty() &&
                    AZStd::find(config.m_excludeEntities.begin(), config.m_excludeEntities.end(), entityId) !=
                        config.m_excludeEntities.end())
                {
                    continue;
                }

                if (!ViewportSnapSourceRequestBus::HasHandlers(entityId))
                {
                    // No dedicated source, so fall back to generic visible geometry. Rejected on
                    // its own bounds first, so entities nowhere near the cursor are never built -
                    // which matters, because building one means fetching and welding a whole mesh.
                    AZ::Aabb visibleGeometryBounds = AZ::Aabb::CreateNull();
                    AzFramework::BoundsRequestBus::EventResult(
                        visibleGeometryBounds, entityId, &AzFramework::BoundsRequests::GetWorldBounds);

                    if (!visibleGeometryBounds.IsValid() || !visibleGeometryBounds.Overlaps(volume.m_bounds))
                    {
                        continue;
                    }

                    // The source hands back a triangle soup with no polygon topology, so it
                    // reconstructs the edges and faces a modeller would recognise - see
                    // VisibleGeometrySnapSource. Every mode is therefore served here, not just
                    // Vertex.
                    switch (m_mode)
                    {
                    case SnapMode::Edge:
                        m_visibleGeometrySource.CollectSnapEdges(entityId, volume, m_edgeCandidates);
                        break;
                    case SnapMode::EdgeMidpoint:
                        m_visibleGeometrySource.CollectSnapEdgeMidpoints(entityId, volume, m_candidates);
                        break;
                    case SnapMode::FaceCenter:
                        m_visibleGeometrySource.CollectSnapFaceCenters(entityId, volume, m_candidates);
                        break;
                    case SnapMode::Vertex:
                    default:
                        m_visibleGeometrySource.CollectSnapVertices(entityId, volume, m_candidates);
                        break;
                    }

                    continue;
                }

                // Coarse rejection before walking any geometry.
                //
                // @note With several sources on one entity, EventResult only reports the last
                // one's bounds, so the check is deliberately conservative: an invalid or absent
                // result falls through to the collect call rather than skipping the entity. The
                // per-candidate cone test inside the source is what guarantees correctness; this
                // is purely an optimisation and is allowed to give up.
                AZ::Aabb sourceBounds = AZ::Aabb::CreateNull();
                ViewportSnapSourceRequestBus::EventResult(
                    sourceBounds, entityId, &ViewportSnapSourceRequests::GetSnapVertexBounds);

                if (sourceBounds.IsValid() && !sourceBounds.Overlaps(volume.m_bounds))
                {
                    continue;
                }

                switch (m_mode)
                {
                case SnapMode::Edge:
                    ViewportSnapSourceRequestBus::Event(
                        entityId, &ViewportSnapSourceRequests::CollectSnapEdges, volume, m_edgeCandidates);
                    break;
                case SnapMode::EdgeMidpoint:
                    ViewportSnapSourceRequestBus::Event(
                        entityId, &ViewportSnapSourceRequests::CollectSnapEdgeMidpoints, volume, m_candidates);
                    break;
                case SnapMode::FaceCenter:
                    ViewportSnapSourceRequestBus::Event(
                        entityId, &ViewportSnapSourceRequests::CollectSnapFaceCenters, volume, m_candidates);
                    break;
                case SnapMode::Vertex:
                default:
                    ViewportSnapSourceRequestBus::Event(
                        entityId, &ViewportSnapSourceRequests::CollectSnapVertices, volume, m_candidates);
                    break;
                }
            }
        }

        void ViewportSnapper::CollectSnapCandidates(
            const AZStd::vector<AZ::EntityId>& entities,
            const SnapQueryVolume& volume,
            AZStd::vector<SnapVertex>& out) const
        {
            for (const AZ::EntityId entityId : entities)
            {
                // The same two-way split as CollectVisibleSourceEntities, and the reason this
                // method exists at all: an entity either publishes candidates itself, or has them
                // reconstructed from what it renders. A caller that only knew about the first case
                // would silently see nothing for an ordinary mesh.
                if (ViewportSnapSourceRequestBus::HasHandlers(entityId))
                {
                    ViewportSnapSourceRequestBus::Event(
                        entityId, &ViewportSnapSourceRequests::CollectSnapVertices, volume, out);
                }
                else
                {
                    m_visibleGeometrySource.CollectSnapVertices(entityId, volume, out);
                }
            }
        }

        bool ViewportSnapper::IsExcludedBySourceIndex(const SnapVertex& candidate, const SnapQueryConfig& config) const
        {
            // The exclusion list is a set of *vertex* identifiers - it exists so the vertex being
            // dragged cannot snap to itself. In every other mode the candidate index means
            // something else entirely, an edge handle or a polygon ordinal, so applying it there
            // would arbitrarily blank out whichever edge or face happened to share a number with
            // the dragged vertex.
            if (m_mode != SnapMode::Vertex || config.m_excludeSourceIndices.empty())
            {
                return false;
            }

            return candidate.m_entityId == config.m_excludeEntity &&
                AZStd::find(
                    config.m_excludeSourceIndices.begin(), config.m_excludeSourceIndices.end(),
                    candidate.m_sourceIndex) != config.m_excludeSourceIndices.end();
        }

        AZStd::optional<SnapResult> ViewportSnapper::QueryPointSnap(
            const AzFramework::CameraState& cameraState,
            const AzFramework::ScreenPoint& screenPoint,
            const SnapQueryConfig& config) const
        {
            if (m_candidates.empty())
            {
                return AZStd::nullopt;
            }

            const float cursorX = aznumeric_cast<float>(screenPoint.m_x);
            const float cursorY = aznumeric_cast<float>(screenPoint.m_y);

            // Score every candidate that survives the screen-space test, then consider them in
            // order. Scoring and picking are separated because the occlusion test below has to be
            // able to fall through to the next best candidate, which a single-pass "keep the best"
            // loop cannot do.
            m_scored.clear();
            for (const SnapVertex& candidate : m_candidates)
            {
                if (IsExcludedBySourceIndex(candidate, config))
                {
                    continue;
                }

                const AZ::Vector3 cameraToVertex = candidate.m_worldPosition - cameraState.m_position;
                const float depth = cameraToVertex.Dot(cameraState.m_forward);
                if (depth < cameraState.m_nearClip || depth > config.m_maxWorldDistance)
                {
                    continue;
                }

                const AzFramework::ScreenPoint projected =
                    AzFramework::WorldToScreen(candidate.m_worldPosition, cameraState);
                const float deltaX = aznumeric_cast<float>(projected.m_x) - cursorX;
                const float deltaY = aznumeric_cast<float>(projected.m_y) - cursorY;
                const float screenDistance = AZStd::sqrt(deltaX * deltaX + deltaY * deltaY);
                if (screenDistance > config.m_screenRadiusPixels)
                {
                    continue;
                }

                m_scored.push_back(ScoredCandidate{ candidate, screenDistance, depth });
            }

            return PickBestUnoccluded(cameraState, config);
        }

        AZStd::optional<SnapResult> ViewportSnapper::QueryEdgeSnap(
            const AzFramework::CameraState& cameraState,
            const AzFramework::ScreenPoint& screenPoint,
            const SnapQueryVolume& volume,
            const SnapQueryConfig& config) const
        {
            if (m_edgeCandidates.empty())
            {
                return AZStd::nullopt;
            }

            const float cursorX = aznumeric_cast<float>(screenPoint.m_x);
            const float cursorY = aznumeric_cast<float>(screenPoint.m_y);

            // Unlike a vertex, an edge has no single position - the target is wherever along the
            // segment comes closest to the line of sight. Each edge is reduced to that one point
            // and then scored exactly like a vertex candidate, so ranking and occlusion are shared
            // rather than reimplemented.
            m_scored.clear();
            for (const SnapEdge& edge : m_edgeCandidates)
            {
                const AZ::Vector3 closestOnEdge =
                    ClosestPointOnSegmentToRay(edge.m_start, edge.m_end, volume.m_rayOrigin, volume.m_rayDirection);

                if (!volume.Contains(closestOnEdge))
                {
                    continue;
                }

                const AZ::Vector3 cameraToPoint = closestOnEdge - cameraState.m_position;
                const float depth = cameraToPoint.Dot(cameraState.m_forward);
                if (depth < cameraState.m_nearClip || depth > config.m_maxWorldDistance)
                {
                    continue;
                }

                const AzFramework::ScreenPoint projected = AzFramework::WorldToScreen(closestOnEdge, cameraState);
                const float deltaX = aznumeric_cast<float>(projected.m_x) - cursorX;
                const float deltaY = aznumeric_cast<float>(projected.m_y) - cursorY;
                const float screenDistance = AZStd::sqrt(deltaX * deltaX + deltaY * deltaY);
                if (screenDistance > config.m_screenRadiusPixels)
                {
                    continue;
                }

                m_scored.push_back(ScoredCandidate{ SnapVertex(closestOnEdge, edge.m_entityId, edge.m_sourceIndex),
                                                    screenDistance, depth });
            }

            return PickBestUnoccluded(cameraState, config);
        }

        AZStd::optional<SnapResult> ViewportSnapper::QueryFaceSnap(
            [[maybe_unused]] const AzFramework::CameraState& cameraState,
            const SnapQueryVolume& volume,
            const SnapQueryConfig& config) const
        {
            AzFramework::RenderGeometry::RayRequest rayRequest;
            rayRequest.m_startWorldPosition = volume.m_rayOrigin;
            rayRequest.m_endWorldPosition = volume.m_rayOrigin + volume.m_rayDirection * volume.m_maxDistance;
            rayRequest.m_onlyVisible = true;

            // Whatever is being dragged must not be its own snap target.
            if (config.m_excludeEntity.IsValid() && !config.m_includeExcludedEntity)
            {
                rayRequest.m_entityFilter.m_ignoreEntities.insert(config.m_excludeEntity);
            }

            for (const AZ::EntityId entityId : config.m_excludeEntities)
            {
                rayRequest.m_entityFilter.m_ignoreEntities.insert(entityId);
            }

            const AZStd::optional<AZ::Vector3> hit = FindClosestPickIntersection(rayRequest);
            if (!hit.has_value())
            {
                return AZStd::nullopt;
            }

            // No occlusion test is needed here: the raycast already returns the *first* surface
            // along the ray, so the result is visible by construction.
            SnapResult result;
            result.m_worldPosition = hit.value();
            result.m_screenDistance = 0.0f;

            // This overload of the intersector reports a position but not which entity owned it.
            // The snapped point is all a consumer needs for face snapping, so rather than run a
            // second, more expensive query to recover an entity id nobody asked for, the field is
            // left invalid.
            return result;
        }

        AZStd::optional<SnapResult> ViewportSnapper::PickBestUnoccluded(
            const AzFramework::CameraState& cameraState, const SnapQueryConfig& config) const
        {
            if (m_scored.empty())
            {
                return AZStd::nullopt;
            }

            // Nearest to the cursor wins. Within half a pixel treat it as a tie and prefer the one
            // closest to the camera, so front faces snap before the geometry behind them.
            AZStd::sort(
                m_scored.begin(), m_scored.end(),
                [](const ScoredCandidate& lhs, const ScoredCandidate& rhs)
                {
                    if (lhs.m_screenDistance < rhs.m_screenDistance - 0.5f)
                    {
                        return true;
                    }
                    if (rhs.m_screenDistance < lhs.m_screenDistance - 0.5f)
                    {
                        return false;
                    }
                    return lhs.m_depth < rhs.m_depth;
                });

            const bool occlusionEnabled = ed_snapOcclusion;
            const int maxOcclusionTests = ed_snapMaxOcclusionTests;
            int occlusionTests = 0;

            for (const ScoredCandidate& scored : m_scored)
            {
                if (occlusionEnabled && occlusionTests < maxOcclusionTests)
                {
                    ++occlusionTests;
                    if (IsOccluded(scored.m_vertex, cameraState, config))
                    {
                        continue;
                    }
                }

                SnapResult result;
                result.m_worldPosition = scored.m_vertex.m_worldPosition;
                result.m_entityId = scored.m_vertex.m_entityId;
                result.m_sourceIndex = scored.m_vertex.m_sourceIndex;
                result.m_screenDistance = scored.m_screenDistance;
                return result;
            }

            return AZStd::nullopt;
        }

        bool ViewportSnapper::IsOccluded(
            const SnapVertex& candidate, const AzFramework::CameraState& cameraState, const SnapQueryConfig& config) const
        {
            const AZ::Vector3 cameraToVertex = candidate.m_worldPosition - cameraState.m_position;
            const float distanceToVertex = cameraToVertex.GetLength();
            if (distanceToVertex <= AZ::Constants::FloatEpsilon)
            {
                return false;
            }

            // A vertex sits *on* a surface, so a ray run all the way to it would reliably hit the
            // very face it belongs to and report every vertex as occluded. Stopping the ray short
            // by a bias means anything it does hit is genuinely in front, and the test reduces to
            // "did we hit something". The bias scales with distance so it stays meaningful at any
            // zoom level.
            //
            // Read the CVARs into plain floats first: AZ_CVAR declares a wrapper type, and passing
            // two of them - or one plus a float - straight to AZStd::max leaves T ambiguous between
            // the wrapper and its underlying type.
            const float biasFraction = ed_snapOcclusionBias;
            const float minimumBias = ed_snapOcclusionMinBias;
            const float bias = AZStd::max(biasFraction * distanceToVertex, minimumBias);

            if (distanceToVertex <= bias)
            {
                return false;
            }

            const AZ::Vector3 directionToVertex = cameraToVertex / distanceToVertex;

            AzFramework::RenderGeometry::RayRequest rayRequest;
            rayRequest.m_startWorldPosition = cameraState.m_position;
            rayRequest.m_endWorldPosition = cameraState.m_position + directionToVertex * (distanceToVertex - bias);
            rayRequest.m_onlyVisible = true;

            // Never let the geometry being dragged occlude its own target. The entity that *owns*
            // the candidate is deliberately not ignored - that is what makes a vertex on the far
            // side of a solid correctly count as hidden.
            if (config.m_excludeEntity.IsValid())
            {
                rayRequest.m_entityFilter.m_ignoreEntities.insert(config.m_excludeEntity);
            }

            for (const AZ::EntityId entityId : config.m_excludeEntities)
            {
                rayRequest.m_entityFilter.m_ignoreEntities.insert(entityId);
            }

            return FindClosestPickIntersection(rayRequest).has_value();
        }
    } // namespace ViewportSnapping
} // namespace AzToolsFramework
