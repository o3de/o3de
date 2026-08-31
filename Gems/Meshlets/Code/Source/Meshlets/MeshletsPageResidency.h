/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/span.h>

namespace AZ::Meshlets
{
    //! Phase 7 streaming — CPU residency core (design §2, v1: conservative CPU
    //! classifier, no GPU feedback). Pure logic, no GPU/IO dependencies, unit-tested
    //! in MeshletsPageResidencyTest.cpp.
    //!
    //! Model: a fixed pool of identical page SLOTS (fixed-slot allocator — pages are
    //! builder-padded to PageMaxClusters caps, so any page fits any slot and the
    //! free-list can never fragment). Each frame the caller submits every candidate
    //! page's classifier inputs; Update() returns which pages to load (up to a
    //! per-update budget) and which to evict. Fail-safe-coarse is the CONSUMER's
    //! contract: a non-resident page's leaves simply fail the cut and their
    //! always-resident parents render instead — this class only manages residency.
    class MeshletsPageResidency
    {
    public:
        static constexpr uint32_t InvalidSlot = 0xFFFFFFFFu;

        //! Stable identity for a page across frames: (mesh render data identity,
        //! page index). The caller supplies any stable 64-bit key.
        using PageKey = uint64_t;

        struct PageRequest
        {
            PageKey m_key = 0;
            AZ::Aabb m_worldAabb;         //!< Page bounds transformed to world space.
            float m_maxParentErrorWorld = 0.0f;   //!< Object-space maxParentError * instance maxScale.
        };

        struct CameraState
        {
            AZ::Vector3 m_position = AZ::Vector3::CreateZero();
            float m_projScale = 0.0f;     //!< cot(FovY/2) * viewportHeight * 0.5.
            float m_tauPx = 1.0f;         //!< The DAG cut threshold (r_meshletsDagErrorPx).
            //! Hysteresis band: a page is WANTED when its projected parent error
            //! exceeds tau (leaves must render), and only EVICTABLE once it drops
            //! below tau / m_prefetchScale — the gap stops load/evict thrash at the
            //! boundary, and >1 values also prefetch slightly before need.
            float m_prefetchScale = 1.5f;
        };

        struct UpdateResult
        {
            AZStd::vector<PageKey> m_load;    //!< Caller uploads these (in order) and calls OnLoaded.
            AZStd::vector<PageKey> m_evicted; //!< Slots already reclaimed; caller drops its bindings.
            //! Wanted-but-unloadable pages this update: nonzero while the throttle is
            //! catching up (transient) or PERSISTENTLY when the pool is too small for
            //! the wanted set — the phase-4 budget-sweep readout ("coarser geometry
            //! carries the frame", by design, but the number tells you by how much).
            uint32_t m_starved = 0;
        };

        void Init(uint32_t slotCapacity);
        uint32_t GetSlotCapacity() const { return m_slotCapacity; }
        uint32_t GetResidentCount() const { return static_cast<uint32_t>(m_residentSlots.size()); }

        //! Classify every submitted page against the camera, evict cold pages when
        //! slots are needed (LRU by last-wanted frame, never evicting a currently
        //! wanted page), and return up to \p maxLoadsPerUpdate load requests for the
        //! most-wanted non-resident pages. Deterministic for identical inputs.
        UpdateResult Update(
            AZStd::span<const PageRequest> pages, const CameraState& camera, uint32_t maxLoadsPerUpdate);

        //! The caller confirms a page from m_load landed in a slot. Returns the slot
        //! (InvalidSlot if the load raced an eviction of the whole pool — caller drops it).
        uint32_t OnLoaded(PageKey key);

        //! The caller could NOT perform a load it was handed (oversize page, staging
        //! failure). Releases the reserved slot back to the free list — without this
        //! every skipped load leaks a slot and the pool slowly drains (found by the
        //! phase-4 soak accounting invariant).
        void CancelLoad(PageKey key);

        // ---- Phase 4 soak instrumentation ----
        //! Pages re-loaded within ChurnWindowFrames of their own eviction — the
        //! thrash signal the hysteresis band exists to suppress. Monotonic counter.
        uint64_t GetChurnCount() const { return m_churnCount; }
        static constexpr uint64_t ChurnWindowFrames = 60;
        //! Accounting invariants (soak tests): free + resident + pending == capacity.
        uint32_t GetFreeSlotCount() const { return static_cast<uint32_t>(m_freeSlots.size()); }
        uint32_t GetPendingCount() const;

        bool IsResident(PageKey key) const { return m_residentSlots.find(key) != m_residentSlots.end(); }
        uint32_t GetSlot(PageKey key) const
        {
            auto it = m_residentSlots.find(key);
            return it != m_residentSlots.end() ? it->second : InvalidSlot;
        }

        //! Drop everything (pool resize, pack reload). Slots return to the free list.
        void Clear();

    private:
        //! Projected screen error of the page's parent bound — same formula as the
        //! shader cut (MeshletsDagProjectedErrorPx), evaluated at the page's CLOSEST
        //! point to the camera (conservative for an aabb).
        static float ProjectedParentErrorPx(const PageRequest& page, const CameraState& camera);

        struct PageState
        {
            uint32_t m_slot = InvalidSlot;          //!< InvalidSlot while pending/non-resident.
            uint64_t m_lastWantedFrame = 0;
            bool m_loadPending = false;
        };

        uint32_t m_slotCapacity = 0;
        uint64_t m_frame = 0;
        uint64_t m_churnCount = 0;
        AZStd::vector<uint32_t> m_freeSlots;
        AZStd::unordered_map<PageKey, uint32_t> m_residentSlots;
        AZStd::unordered_map<PageKey, PageState> m_pageStates;
        //! key -> frame it was last evicted (churn detection window).
        AZStd::unordered_map<PageKey, uint64_t> m_evictedFrame;
    };
} // namespace AZ::Meshlets
