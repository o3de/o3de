/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ViewportSnapping/ViewportSnapping.h>
#include <AzToolsFramework/ViewportSnapping/VisibleGeometrySnapSource.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzFramework/Viewport/CameraState.h>

namespace AzToolsFramework
{
    namespace ViewportSnapping
    {
        //! The editor's implementation of geometry snapping.
        //!
        //! Owned by ViewportSnappingSystemComponent, which registers it with AZ::Interface for the
        //! lifetime of the editor. Deliberately a plain class rather than the component itself, so
        //! it can be constructed and driven directly in a test without an entity or a component
        //! application.
        //!
        //! Query cost is bounded in three stages, each far cheaper than the one it protects:
        //! FindVisibleEntities gives only what the viewport can see, a per-entity bounds test drops
        //! most of that, and a bounding-sphere test per block of geometry drops most of what
        //! remains. Only geometry genuinely near the cursor is examined point by point.
        class ViewportSnapper : public ViewportSnappingInterface
        {
        public:
            AZ_RTTI(ViewportSnapper, "{3C8E27D6-9B44-4A15-8E0F-C2A6D719B043}", ViewportSnappingInterface);
            AZ_CLASS_ALLOCATOR(ViewportSnapper, AZ::SystemAllocator);

            ViewportSnapper() = default;
            ~ViewportSnapper() override = default;

            void Register();
            void Unregister();

            // ViewportSnappingInterface overrides ...
            SnapMode GetSnapMode() const override;
            void SetSnapMode(SnapMode mode) override;
            bool IsSnappingEnabled() const override;
            AZStd::optional<SnapResult> QuerySnapTarget(
                AzFramework::ViewportId viewportId,
                const AzFramework::ScreenPoint& screenPoint,
                const SnapQueryConfig& config) const override;
            void CollectSnapCandidates(
                const AZStd::vector<AZ::EntityId>& entities,
                const SnapQueryVolume& volume,
                AZStd::vector<SnapVertex>& out) const override;

        private:
            //! Gather candidates from every visible source into m_candidates or m_edgeCandidates,
            //! according to the active mode.
            void CollectVisibleSourceEntities(
                AzFramework::ViewportId viewportId, const SnapQueryVolume& volume, const SnapQueryConfig& config) const;

            //! Vertex, edge-midpoint and face-centre modes: pick the gathered point nearest the
            //! cursor.
            AZStd::optional<SnapResult> QueryPointSnap(
                const AzFramework::CameraState& cameraState,
                const AzFramework::ScreenPoint& screenPoint,
                const SnapQueryConfig& config) const;

            //! Edge mode: reduce each candidate edge to its closest point to the line of sight,
            //! then rank those the same way points are ranked.
            AZStd::optional<SnapResult> QueryEdgeSnap(
                const AzFramework::CameraState& cameraState,
                const AzFramework::ScreenPoint& screenPoint,
                const SnapQueryVolume& volume,
                const SnapQueryConfig& config) const;

            //! Face mode: a scene raycast, which already returns the nearest visible surface point.
            AZStd::optional<SnapResult> QueryFaceSnap(
                const AzFramework::CameraState& cameraState,
                const SnapQueryVolume& volume,
                const SnapQueryConfig& config) const;

            //! Should this candidate be skipped because it is the geometry currently being dragged.
            bool IsExcludedBySourceIndex(const SnapVertex& candidate, const SnapQueryConfig& config) const;

            //! Sort m_scored and return the best candidate that is not occluded.
            AZStd::optional<SnapResult> PickBestUnoccluded(
                const AzFramework::CameraState& cameraState, const SnapQueryConfig& config) const;

            //! Is @p candidate hidden behind geometry from the camera's point of view.
            //!
            //! Casts a ray from the camera towards the candidate and asks whether it hits anything
            //! on the way. The entities being dragged are filtered out so they cannot occlude their
            //! own target.
            bool IsOccluded(
                const SnapVertex& candidate,
                const AzFramework::CameraState& cameraState,
                const SnapQueryConfig& config) const;

            //! A candidate that passed the screen-space test, with the values used to rank it.
            struct ScoredCandidate
            {
                SnapVertex m_vertex;
                float m_screenDistance = 0.0f;
                float m_depth = 0.0f;
            };

            SnapMode m_mode = SnapMode::Off;
            bool m_registered = false;

            //!@{
            //! Scratch buffers reused across queries, so a per-mouse-move query does not allocate.
            //! Mutable because QuerySnapTarget is logically const.
            mutable AZStd::vector<AZ::EntityId> m_visibleEntities;
            mutable AZStd::vector<SnapVertex> m_candidates;
            mutable AZStd::vector<SnapEdge> m_edgeCandidates;
            mutable AZStd::vector<ScoredCandidate> m_scored;
            //!@}

            //! Fallback source for entities that do not publish candidates themselves - ordinary
            //! meshes, and anything else implementing AzFramework::VisibleGeometryRequestBus.
            mutable VisibleGeometrySnapSource m_visibleGeometrySource;
        };
    } // namespace ViewportSnapping
} // namespace AzToolsFramework
