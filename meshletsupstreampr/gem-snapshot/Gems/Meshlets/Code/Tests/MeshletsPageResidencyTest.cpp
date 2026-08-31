/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

// Phase 7 streaming — residency core behavior (design §2). Pure-CPU checks: near
// pages load, far pages don't, the budget is a hard cap, LRU eviction reclaims cold
// pages under pressure, the hysteresis band prevents boundary thrash, and duplicate
// per-instance submissions of one page never double-allocate.

#include <AzTest/AzTest.h>
#include <MeshletsPageResidency.h>

namespace UnitTest
{
    using AZ::Meshlets::MeshletsPageResidency;

    namespace
    {
        MeshletsPageResidency::PageRequest MakePage(uint64_t key, float centerX, float errObj)
        {
            MeshletsPageResidency::PageRequest req;
            req.m_key = key;
            req.m_worldAabb = AZ::Aabb::CreateCenterHalfExtents(
                AZ::Vector3(centerX, 0.0f, 0.0f), AZ::Vector3(1.0f));
            req.m_maxParentErrorWorld = errObj;
            return req;
        }

        MeshletsPageResidency::CameraState MakeCamera()
        {
            MeshletsPageResidency::CameraState cam;
            cam.m_position = AZ::Vector3::CreateZero();
            cam.m_projScale = 1000.0f;   // errPx = err * 1000 / dist
            cam.m_tauPx = 1.0f;
            cam.m_prefetchScale = 1.5f;
            return cam;
        }
    }

    TEST(MeshletsPageResidency, NearLoadsFarDoesNot)
    {
        MeshletsPageResidency res;
        res.Init(8);
        // Near page: dist ~9 (closest aabb point), errPx = 0.1*1000/9 ~= 11 > tau.
        // Far page: dist ~999, errPx ~= 0.1 < tau.
        const MeshletsPageResidency::PageRequest pages[] = {
            MakePage(1, 10.0f, 0.1f),
            MakePage(2, 1000.0f, 0.1f),
        };
        auto ops = res.Update(pages, MakeCamera(), 8);
        ASSERT_EQ(ops.m_load.size(), 1u);
        EXPECT_EQ(ops.m_load[0], 1u);
        EXPECT_EQ(res.OnLoaded(1), 0u);
        EXPECT_TRUE(res.IsResident(1));
        EXPECT_FALSE(res.IsResident(2));
    }

    TEST(MeshletsPageResidency, BudgetIsAHardCapAndMostWantedWinFirst)
    {
        MeshletsPageResidency res;
        res.Init(2);
        // Three wanted pages, two slots: the two CLOSEST (highest errPx) load.
        const MeshletsPageResidency::PageRequest pages[] = {
            MakePage(1, 10.0f, 0.1f),
            MakePage(2, 5.0f, 0.1f),
            MakePage(3, 20.0f, 0.1f),
        };
        auto ops = res.Update(pages, MakeCamera(), 8);
        ASSERT_EQ(ops.m_load.size(), 2u);
        EXPECT_EQ(ops.m_load[0], 2u);   // closest first
        EXPECT_EQ(ops.m_load[1], 1u);
        res.OnLoaded(2);
        res.OnLoaded(1);
        // Page 3 stays wanted but the pool is full of equally-wanted pages: no churn.
        ops = res.Update(pages, MakeCamera(), 8);
        EXPECT_TRUE(ops.m_load.empty());
        EXPECT_TRUE(ops.m_evicted.empty());
        EXPECT_EQ(res.GetResidentCount(), 2u);
    }

    TEST(MeshletsPageResidency, LruEvictionUnderPressureAndHysteresis)
    {
        MeshletsPageResidency res;
        res.Init(1);
        MeshletsPageResidency::PageRequest near = MakePage(1, 10.0f, 0.1f);
        MeshletsPageResidency::PageRequest far  = MakePage(2, 10.0f, 0.1f);

        {
            const MeshletsPageResidency::PageRequest pages[] = { near };
            auto ops = res.Update(pages, MakeCamera(), 8);
            ASSERT_EQ(ops.m_load.size(), 1u);
            res.OnLoaded(1);
        }
        // Page 1 drifts into the hysteresis band (errPx between tau/1.5 and tau):
        // still resident, must NOT be evicted even when another page wants the slot?
        // It must: in-band means "not wanted now" but also "not cold" — the design's
        // band only stops eviction while below budget pressure. Under pressure the
        // wanted page wins ONLY if the resident one is cold (below the floor).
        {
            // errPx for page 1 now ~0.9 (in band: floor=0.66, tau=1.0) => not cold.
            MeshletsPageResidency::PageRequest inBand = MakePage(1, 112.0f, 0.1f);
            const MeshletsPageResidency::PageRequest pages[] = { inBand, far };
            auto ops = res.Update(pages, MakeCamera(), 8);
            EXPECT_TRUE(ops.m_evicted.empty()) << "in-band page must not thrash out";
            EXPECT_TRUE(ops.m_load.empty());
        }
        {
            // Page 1 now far (cold, errPx ~0.03): pressure from page 2 evicts it.
            MeshletsPageResidency::PageRequest cold = MakePage(1, 3000.0f, 0.1f);
            const MeshletsPageResidency::PageRequest pages[] = { cold, far };
            auto ops = res.Update(pages, MakeCamera(), 8);
            ASSERT_EQ(ops.m_evicted.size(), 1u);
            EXPECT_EQ(ops.m_evicted[0], 1u);
            ASSERT_EQ(ops.m_load.size(), 1u);
            EXPECT_EQ(ops.m_load[0], 2u);
            EXPECT_EQ(res.OnLoaded(2), 0u);   // the single slot, recycled
        }
    }

    TEST(MeshletsPageResidency, DuplicateSubmissionsAllocateOnce)
    {
        MeshletsPageResidency res;
        res.Init(4);
        // Two instances of the same mesh submit the same page key.
        const MeshletsPageResidency::PageRequest pages[] = {
            MakePage(7, 10.0f, 0.1f),
            MakePage(7, 12.0f, 0.1f),
        };
        auto ops = res.Update(pages, MakeCamera(), 8);
        ASSERT_EQ(ops.m_load.size(), 1u);
        EXPECT_EQ(ops.m_load[0], 7u);
        res.OnLoaded(7);
        EXPECT_EQ(res.GetResidentCount(), 1u);
    }

    TEST(MeshletsPageResidency, MaxLoadsPerUpdateSpreadsWork)
    {
        MeshletsPageResidency res;
        res.Init(8);
        const MeshletsPageResidency::PageRequest pages[] = {
            MakePage(1, 5.0f, 0.1f), MakePage(2, 6.0f, 0.1f),
            MakePage(3, 7.0f, 0.1f), MakePage(4, 8.0f, 0.1f),
        };
        auto ops = res.Update(pages, MakeCamera(), 2);
        EXPECT_EQ(ops.m_load.size(), 2u);
        for (auto key : ops.m_load) { res.OnLoaded(key); }
        ops = res.Update(pages, MakeCamera(), 2);
        EXPECT_EQ(ops.m_load.size(), 2u);
    }
} // namespace UnitTest
