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
#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/optional.h>
#include <AzFramework/Viewport/ScreenGeometry.h>
#include <AzFramework/Viewport/ViewportId.h>

namespace AzToolsFramework
{
    namespace ViewportSnapping
    {
        //! Tuning for a single snap query.
        struct SnapQueryConfig
        {
            //! How close, in viewport pixels, a candidate must project to the cursor to be
            //! considered at all.
            float m_screenRadiusPixels = 12.0f;

            //! Candidates further than this from the camera are ignored.
            float m_maxWorldDistance = 200.0f;

            //! Skip this entity entirely. Normally the entity currently being dragged, since
            //! snapping something to itself does nothing but pin it in place.
            AZ::EntityId m_excludeEntity;

            //! Skip all of these entities unconditionally.
            //!
            //! Used when dragging a whole selection: every selected entity moves together, so none
            //! of them are valid targets.
            //!
            //! @note Unlike m_excludeEntity, these are never re-included by
            //! m_includeExcludedEntity.
            AZStd::vector<AZ::EntityId> m_excludeEntities;

            //! When set, geometry from m_excludeEntity is considered after all.
            //!
            //! Sub-element editing wants this: dragging one vertex of a mesh onto another vertex
            //! of the same mesh is a normal thing to want to do.
            bool m_includeExcludedEntity = false;

            //! Candidates from m_excludeEntity whose m_sourceIndex appears here are rejected.
            //!
            //! Lets a sub-element tool exclude just the geometry it is currently dragging - one
            //! entry for a single vertex, several for an edge or a polygon - while still snapping
            //! to the rest of the same mesh. Empty means nothing is excluded.
            //!
            //! @note Searched linearly, and kept as a vector deliberately: these are always tiny,
            //! typically one to eight entries, where the constant factor of anything smarter costs
            //! more than the scan.
            AZStd::vector<AZ::s64> m_excludeSourceIndices;
        };

        //! The position chosen by a snap query.
        struct SnapResult
        {
            AZ::Vector3 m_worldPosition = AZ::Vector3::CreateZero();
            AZ::EntityId m_entityId;
            AZ::s64 m_sourceIndex = 0;
            float m_screenDistance = 0.0f; //!< Pixels from the cursor to the chosen position.
        };

        //! What kind of geometry snapping targets.
        //!
        //! One mode is active at a time, the way a DCC tool's snap-target selector works, rather
        //! than several being combined. Combining them reads well on paper but in practice means
        //! the user cannot tell which kind of target they are about to hit.
        //!
        //! Vertex, EdgeMidpoint and FaceCenter are all *point* modes: the source publishes a set
        //! of discrete positions and one pipeline picks whichever projects nearest the cursor.
        //! Edge and Face are continuous - the target is computed along a segment or across a
        //! surface - so they each need their own query.
        enum class SnapMode
        {
            Off,
            Vertex,
            Edge,
            EdgeMidpoint,
            Face,
            FaceCenter
        };

        //! Editor-wide geometry snapping service.
        //!
        //! Reached through AZ::Interface rather than as a linked function, so that an
        //! implementation can live in a gem. A gem's module is a CMake MODULE library and cannot
        //! be linked against, so a cross-gem call has to be virtual dispatch through the shared
        //! AZ::Environment. That indirection is also what makes the service optional: with no
        //! implementation registered, Get() returns null and each of the free functions below
        //! degrades to a no-op rather than failing to link.
        class ViewportSnappingInterface
        {
        public:
            AZ_RTTI(ViewportSnappingInterface, "{6B1F4C3A-2D57-4E88-9A0C-71E5D3B84F12}");

            virtual ~ViewportSnappingInterface() = default;

            //! The active snap mode, or SnapMode::Off when geometry snapping is switched off.
            virtual SnapMode GetSnapMode() const = 0;
            virtual void SetSnapMode(SnapMode mode) = 0;

            //! Is any snap mode active.
            //!
            //! Worth checking before querying: a caller can then skip building exclusion lists and
            //! gathering viewport state entirely, rather than doing that work for a query that was
            //! always going to return nothing.
            virtual bool IsSnappingEnabled() const = 0;

            //! Find the snap target under the cursor for the active mode.
            //! @return The chosen position, or nothing when snapping is off or nothing is in range.
            virtual AZStd::optional<SnapResult> QuerySnapTarget(
                AzFramework::ViewportId viewportId,
                const AzFramework::ScreenPoint& screenPoint,
                const SnapQueryConfig& config) const = 0;

            //! Append the snap candidates of specific entities to @p out, whatever their source.
            //!
            //! QuerySnapTarget answers "what is under the cursor". This answers "what could this
            //! entity offer", which is the question a tool asks about the object it is *moving*
            //! rather than the one it is moving onto - typically to choose which of its own points
            //! should land on the target.
            //!
            //! Worth going through the implementation rather than calling
            //! ViewportSnapSourceRequestBus directly: most entities publish nothing on that bus and
            //! are served by reconstructing geometry from what they render. A caller talking to the
            //! bus alone silently gets nothing back for an ordinary mesh, with no indication that a
            //! whole category of entity was skipped.
            //!
            //! @note Unaffected by the active snap mode, and does no exclusion filtering; the
            //! caller has named the entities it wants and is expected to have made those decisions
            //! already.
            virtual void CollectSnapCandidates(
                const AZStd::vector<AZ::EntityId>& entities,
                const SnapQueryVolume& volume,
                AZStd::vector<SnapVertex>& out) const = 0;
        };

        using ViewportSnappingRequests = AZ::Interface<ViewportSnappingInterface>;

        //! Find the snap target under the cursor.
        //! Safe to call with no implementation registered - returns nothing.
        inline AZStd::optional<SnapResult> QuerySnapTarget(
            const AzFramework::ViewportId viewportId,
            const AzFramework::ScreenPoint& screenPoint,
            const SnapQueryConfig& config)
        {
            if (const auto* snapping = ViewportSnappingRequests::Get())
            {
                return snapping->QuerySnapTarget(viewportId, screenPoint, config);
            }
            return AZStd::nullopt;
        }

        //! Is geometry snapping active. False with no implementation registered.
        inline bool IsSnappingEnabled()
        {
            const auto* snapping = ViewportSnappingRequests::Get();
            return snapping != nullptr && snapping->IsSnappingEnabled();
        }

        //! Collect the snap candidates of specific entities.
        //! Safe to call with no implementation registered - appends nothing.
        inline void CollectSnapCandidates(
            const AZStd::vector<AZ::EntityId>& entities,
            const SnapQueryVolume& volume,
            AZStd::vector<SnapVertex>& out)
        {
            if (const auto* snapping = ViewportSnappingRequests::Get())
            {
                snapping->CollectSnapCandidates(entities, volume, out);
            }
        }

        //! The active snap mode. SnapMode::Off with no implementation registered.
        inline SnapMode GetSnapMode()
        {
            const auto* snapping = ViewportSnappingRequests::Get();
            return snapping != nullptr ? snapping->GetSnapMode() : SnapMode::Off;
        }
    } // namespace ViewportSnapping
} // namespace AzToolsFramework
