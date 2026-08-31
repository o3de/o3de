/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <MeshletsPageResidency.h>

#include <AzCore/std/sort.h>
#include <limits>

namespace AZ::Meshlets
{
    void MeshletsPageResidency::Init(uint32_t slotCapacity)
    {
        Clear();
        m_slotCapacity = slotCapacity;
        m_freeSlots.clear();
        m_freeSlots.reserve(slotCapacity);
        // Pop from the back => slot 0 handed out first (stable, test-friendly).
        for (uint32_t s = slotCapacity; s > 0; --s)
        {
            m_freeSlots.push_back(s - 1);
        }
    }

    void MeshletsPageResidency::Clear()
    {
        m_residentSlots.clear();
        m_pageStates.clear();
        m_evictedFrame.clear();
        m_freeSlots.clear();
        for (uint32_t s = m_slotCapacity; s > 0; --s)
        {
            m_freeSlots.push_back(s - 1);
        }
    }

    uint32_t MeshletsPageResidency::GetPendingCount() const
    {
        uint32_t pending = 0;
        for (const auto& [key, state] : m_pageStates)
        {
            if (state.m_loadPending)
            {
                ++pending;
            }
        }
        return pending;
    }

    void MeshletsPageResidency::CancelLoad(PageKey key)
    {
        auto it = m_pageStates.find(key);
        if (it == m_pageStates.end() || !it->second.m_loadPending)
        {
            return;
        }
        if (it->second.m_slot != InvalidSlot)
        {
            m_freeSlots.push_back(it->second.m_slot);
        }
        it->second.m_slot = InvalidSlot;
        it->second.m_loadPending = false;
    }

    float MeshletsPageResidency::ProjectedParentErrorPx(const PageRequest& page, const CameraState& camera)
    {
        if (page.m_maxParentErrorWorld >= std::numeric_limits<float>::max())
        {
            return std::numeric_limits<float>::max();   // roots: leaves are the only detail
        }
        if (!page.m_worldAabb.IsValid())
        {
            return 0.0f;
        }
        // Distance to the CLOSEST point of the aabb — conservative (the shader cut
        // subtracts the sphere radius the same way). Inside the box => refine.
        const AZ::Vector3 closest = camera.m_position.GetClamp(
            page.m_worldAabb.GetMin(), page.m_worldAabb.GetMax());
        const float dist = closest.GetDistance(camera.m_position);
        if (dist <= 1e-4f)
        {
            return std::numeric_limits<float>::max();
        }
        return (page.m_maxParentErrorWorld * camera.m_projScale) / dist;
    }

    MeshletsPageResidency::UpdateResult MeshletsPageResidency::Update(
        AZStd::span<const PageRequest> pages, const CameraState& camera, uint32_t maxLoadsPerUpdate)
    {
        UpdateResult result;
        ++m_frame;

        struct Want
        {
            PageKey m_key;
            float m_errPx;
        };
        AZStd::vector<Want> wantedNonResident;
        // Pages submitted this frame whose error is under the hysteresis floor are
        // eviction candidates; pages NOT submitted at all (instance gone) are too.
        const float evictFloor = camera.m_tauPx / AZStd::GetMax(1.0f, camera.m_prefetchScale);

        AZStd::unordered_map<PageKey, float> submittedErr;
        submittedErr.reserve(pages.size());
        for (const PageRequest& page : pages)
        {
            const float errPx = ProjectedParentErrorPx(page, camera);
            submittedErr.emplace(page.m_key, errPx);
            PageState& state = m_pageStates[page.m_key];
            const bool wanted = errPx > camera.m_tauPx;
            if (wanted)
            {
                state.m_lastWantedFrame = m_frame;
                if (state.m_slot == InvalidSlot && !state.m_loadPending)
                {
                    wantedNonResident.push_back({ page.m_key, errPx });
                }
            }
        }

        // Most-wanted first, key as the deterministic tiebreak.
        AZStd::sort(wantedNonResident.begin(), wantedNonResident.end(),
            [](const Want& a, const Want& b)
            {
                return a.m_errPx != b.m_errPx ? a.m_errPx > b.m_errPx : a.m_key < b.m_key;
            });

        auto tryEvictOne = [&]() -> bool
        {
            // LRU over resident pages that are cold: below the hysteresis floor this
            // frame, or not submitted at all. Never evict a currently wanted page.
            PageKey victim = 0;
            uint64_t oldest = std::numeric_limits<uint64_t>::max();
            bool found = false;
            for (const auto& [key, slot] : m_residentSlots)
            {
                const PageState& state = m_pageStates[key];
                if (state.m_lastWantedFrame == m_frame)
                {
                    continue;   // wanted right now
                }
                auto it = submittedErr.find(key);
                const bool cold = (it == submittedErr.end()) || (it->second < evictFloor);
                if (cold && state.m_lastWantedFrame < oldest)
                {
                    oldest = state.m_lastWantedFrame;
                    victim = key;
                    found = true;
                }
            }
            if (!found)
            {
                return false;
            }
            const uint32_t slot = m_residentSlots[victim];
            m_residentSlots.erase(victim);
            m_pageStates[victim].m_slot = InvalidSlot;
            m_freeSlots.push_back(slot);
            m_evictedFrame[victim] = m_frame;   // churn-window bookkeeping
            result.m_evicted.push_back(victim);
            return true;
        };

        uint32_t considered = 0;
        for (const Want& want : wantedNonResident)
        {
            if (result.m_load.size() >= maxLoadsPerUpdate)
            {
                break;
            }
            ++considered;
            // Multiple instances may submit the same page key in one Update — the
            // first emission reserved a slot and set the pending flag; skip repeats.
            {
                PageState& state = m_pageStates[want.m_key];
                if (state.m_loadPending || state.m_slot != InvalidSlot)
                {
                    continue;
                }
            }
            if (m_freeSlots.empty() && !tryEvictOne())
            {
                --considered;   // this one was not served
                break;   // pool full of wanted/pending pages — coarser geometry carries the frame
            }
            // Reserve the slot NOW so loads can never over-commit the pool.
            const uint32_t slot = m_freeSlots.back();
            m_freeSlots.pop_back();
            PageState& state = m_pageStates[want.m_key];
            state.m_slot = slot;
            state.m_loadPending = true;
            result.m_load.push_back(want.m_key);
            // Thrash signal: re-loading a page evicted only moments ago means the
            // hysteresis band is too tight (or the pool too small for the wanted set).
            auto evictedIt = m_evictedFrame.find(want.m_key);
            if (evictedIt != m_evictedFrame.end() &&
                m_frame - evictedIt->second <= ChurnWindowFrames)
            {
                ++m_churnCount;
            }
        }

        result.m_starved =
            static_cast<uint32_t>(wantedNonResident.size()) - AZStd::GetMin(
                considered, static_cast<uint32_t>(wantedNonResident.size()));
        return result;
    }

    uint32_t MeshletsPageResidency::OnLoaded(PageKey key)
    {
        auto it = m_pageStates.find(key);
        if (it == m_pageStates.end() || !it->second.m_loadPending || it->second.m_slot == InvalidSlot)
        {
            return InvalidSlot;   // raced a Clear() — caller drops the upload
        }
        it->second.m_loadPending = false;
        m_residentSlots[key] = it->second.m_slot;
        return it->second.m_slot;
    }
} // namespace AZ::Meshlets
