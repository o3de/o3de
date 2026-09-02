/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

// Phase 4 soak, simulated deterministically against the residency core (design section 5.4).
// The GPU half of the soak (visual coarsening, upload timing) needs a build; these
// tests pin the BEHAVIORAL half: budget sweeps converge, teleports recover within the
// throttle bound, the hysteresis band suppresses boundary thrash (and its absence
// provably causes it), slot accounting never leaks -- including through CancelLoad --
// and a long random-walk run holds every invariant.

#include <AzTest/AzTest.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/utility/pair.h>
#include <MeshletsPageResidency.h>

namespace UnitTest
{
    using AZ::Meshlets::MeshletsPageResidency;

    namespace
    {
        MeshletsPageResidency::PageRequest MakePage(uint64_t key, float centerX, float errObj = 0.1f)
        {
            MeshletsPageResidency::PageRequest req;
            req.m_key = key;
            req.m_worldAabb = AZ::Aabb::CreateCenterHalfExtents(
                AZ::Vector3(centerX, 0.0f, 0.0f), AZ::Vector3(1.0f));
            req.m_maxParentErrorWorld = errObj;
            return req;
        }

        MeshletsPageResidency::CameraState MakeCamera(float x = 0.0f, float prefetch = 1.5f)
        {
            MeshletsPageResidency::CameraState cam;
            cam.m_position = AZ::Vector3(x, 0.0f, 0.0f);
            cam.m_projScale = 1000.0f;   // errPx = 0.1 * 1000 / dist = 100 / dist
            cam.m_tauPx = 1.0f;          // wanted when dist < ~100
            cam.m_prefetchScale = prefetch;
            return cam;
        }

        void CheckAccounting(const MeshletsPageResidency& res)
        {
            EXPECT_EQ(
                res.GetFreeSlotCount() + res.GetResidentCount() + res.GetPendingCount(),
                res.GetSlotCapacity()) << "slot leak / double-allocation";
        }

        //! Run Update+OnLoaded until no more loads are issued (or the cap trips).
        uint32_t Converge(
            MeshletsPageResidency& res, AZStd::span<const MeshletsPageResidency::PageRequest> pages,
            const MeshletsPageResidency::CameraState& cam, uint32_t maxLoadsPerUpdate,
            uint32_t maxUpdates = 256)
        {
            uint32_t updates = 0;
            for (; updates < maxUpdates; ++updates)
            {
                auto ops = res.Update(pages, cam, maxLoadsPerUpdate);
                for (auto key : ops.m_load)
                {
                    res.OnLoaded(key);
                }
                CheckAccounting(res);
                if (ops.m_load.empty() && ops.m_evicted.empty())
                {
                    break;
                }
            }
            return updates;
        }
    }

    TEST(MeshletsStreamingSoak, BudgetSweepConvergesAtEveryCapacity)
    {
        // 64 pages at increasing distance; the closest N always win the N slots.
        AZStd::vector<MeshletsPageResidency::PageRequest> pages;
        for (uint32_t i = 0; i < 64; ++i)
        {
            pages.push_back(MakePage(i + 1, 5.0f + 1.2f * i));
        }
        // All are "wanted" (dist < 100 => errPx > 1) for the first ~70 pages.
        for (uint32_t capacity : { 64u, 32u, 8u, 2u, 24u })
        {
            MeshletsPageResidency res;
            res.Init(capacity);   // budget change = pool rebuild, same as the FP does
            Converge(res, pages, MakeCamera(), 8);
            const uint32_t wanted = 64;   // every page is inside the wanted radius here
            EXPECT_EQ(res.GetResidentCount(), AZStd::GetMin(capacity, wanted))
                << "capacity " << capacity;
            // The CLOSEST pages must be the resident ones (most-wanted-first).
            for (uint32_t i = 0; i < AZStd::GetMin(capacity, wanted); ++i)
            {
                EXPECT_TRUE(res.IsResident(i + 1)) << "capacity " << capacity << " page " << i + 1;
            }
            EXPECT_EQ(res.GetChurnCount(), 0u) << "static camera must never churn";
        }
    }

    TEST(MeshletsStreamingSoak, TeleportRecoversWithinThrottleBound)
    {
        // Two clusters of pages, 10000 apart. Converge at A, teleport to B.
        AZStd::vector<MeshletsPageResidency::PageRequest> pages;
        for (uint32_t i = 0; i < 16; ++i)
        {
            pages.push_back(MakePage(100 + i, 5.0f + 2.0f * i));            // near A (x=0)
            pages.push_back(MakePage(200 + i, 10000.0f + 5.0f + 2.0f * i)); // near B
        }
        MeshletsPageResidency res;
        res.Init(16);
        Converge(res, pages, MakeCamera(0.0f), 4);
        EXPECT_EQ(res.GetResidentCount(), 16u);
        EXPECT_TRUE(res.IsResident(100));
        EXPECT_FALSE(res.IsResident(200));

        // Teleport: 16 new wanted pages at 4 loads/update => exactly 4 updates of
        // loading, plus one quiet update to detect convergence.
        const uint32_t updates = Converge(res, pages, MakeCamera(10000.0f), 4);
        EXPECT_LE(updates, 6u) << "teleport recovery exceeded the throttle bound";
        EXPECT_EQ(res.GetResidentCount(), 16u);
        for (uint32_t i = 0; i < 16; ++i)
        {
            EXPECT_TRUE(res.IsResident(200 + i));
            EXPECT_FALSE(res.IsResident(100 + i)) << "old set must have been evicted";
        }
    }

    TEST(MeshletsStreamingSoak, HysteresisBandSuppressesBoundaryThrash)
    {
        // Capacity 1, two pages: A stays firmly wanted; B oscillates between wanted
        // and the in-band region. With a wide band, B's swings never evict A's
        // competitor... the point: run the SAME oscillation with band=1.0 and
        // band=3.0 and compare churn.
        auto runOscillation = [](float prefetch) -> uint64_t
        {
            MeshletsPageResidency res;
            res.Init(1);
            for (uint32_t cycle = 0; cycle < 100; ++cycle)
            {
                // Camera swings between the two pages: each half-cycle one page is
                // clearly wanted and the other sits at ~0.8 tau -- INSIDE a 1.5+ band
                // (not cold), but BELOW tau (not wanted). errPx = 100/dist.
                const bool atA = (cycle % 2) == 0;
                const MeshletsPageResidency::PageRequest pages[] = {
                    MakePage(1, atA ? 10.0f : 126.0f),   // errPx ~11 wanted : ~0.8 in-band
                    MakePage(2, atA ? 126.0f : 10.0f),
                };
                auto ops = res.Update(pages, MakeCamera(0.0f, prefetch), 8);
                for (auto key : ops.m_load)
                {
                    res.OnLoaded(key);
                }
                CheckAccounting(res);
            }
            return res.GetChurnCount();
        };

        // No band: the in-band page reads as cold => evict/reload every swing.
        EXPECT_GT(runOscillation(1.0f), 10u) << "without a band, boundary thrash must show";
        // Wide band: the not-quite-wanted page is never cold => the slot never flips.
        EXPECT_EQ(runOscillation(3.0f), 0u) << "the band exists to make this zero";
    }

    TEST(MeshletsStreamingSoak, CancelLoadNeverLeaksSlots)
    {
        MeshletsPageResidency res;
        res.Init(4);
        const MeshletsPageResidency::PageRequest pages[] = {
            MakePage(1, 5.0f), MakePage(2, 6.0f), MakePage(3, 7.0f), MakePage(4, 8.0f),
        };
        for (uint32_t cycle = 0; cycle < 50; ++cycle)
        {
            auto ops = res.Update(pages, MakeCamera(), 8);
            for (size_t i = 0; i < ops.m_load.size(); ++i)
            {
                // Alternate: half the loads "fail" (oversize page etc.) and cancel.
                if ((cycle + i) % 2 == 0)
                {
                    res.CancelLoad(ops.m_load[i]);
                }
                else
                {
                    res.OnLoaded(ops.m_load[i]);
                }
            }
            CheckAccounting(res);
        }
        // Canceled pages must remain loadable: converge and everything fits.
        Converge(res, pages, MakeCamera(), 8);
        EXPECT_EQ(res.GetResidentCount(), 4u);
        CheckAccounting(res);
    }

    TEST(MeshletsStreamingSoak, LongRandomWalkHoldsEveryInvariant)
    {
        // 128 pages along a line, capacity 16, throttled loads, 500 updates of a
        // deterministic LCG random walk. Every update: accounting holds, resident
        // count <= capacity, and no two resident pages share a slot.
        AZStd::vector<MeshletsPageResidency::PageRequest> basePages;
        for (uint32_t i = 0; i < 128; ++i)
        {
            basePages.push_back(MakePage(i + 1, 10.0f * i));
        }
        MeshletsPageResidency res;
        res.Init(16);
        uint64_t lcg = 12345;
        float cameraX = 0.0f;
        for (uint32_t update = 0; update < 500; ++update)
        {
            lcg = lcg * 6364136223846793005ull + 1442695040888963407ull;
            const float step = static_cast<float>((lcg >> 33) % 200) - 100.0f;   // [-100, +100)
            cameraX = AZStd::clamp(cameraX + step, 0.0f, 1270.0f);

            auto ops = res.Update(basePages, MakeCamera(cameraX), 4);
            for (auto key : ops.m_load)
            {
                res.OnLoaded(key);
            }
            CheckAccounting(res);
            EXPECT_LE(res.GetResidentCount(), 16u);

            AZStd::unordered_set<uint32_t> slots;
            for (const auto& page : basePages)
            {
                const uint32_t slot = res.GetSlot(page.m_key);
                if (slot != MeshletsPageResidency::InvalidSlot)
                {
                    EXPECT_TRUE(slots.insert(slot).second) << "duplicate slot at update " << update;
                    EXPECT_LT(slot, 16u);
                }
            }
        }
    }
} // namespace UnitTest
