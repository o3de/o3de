/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Render/DiffuseProbeGridSoftwareBvh.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>
#include <cmath>

namespace UnitTest
{
    using AZ::Aabb;
    using AZ::Vector3;
    using AZ::Render::DiffuseProbeGridSoftwareBvh;
    class DiffuseProbeGridSoftwareBvhTests : public LeakDetectionFixture {};

    Aabb CpuBounds(const DiffuseProbeGridSoftwareBvh::Bounds& bounds)
    {
        return Aabb::CreateFromMinMax(Vector3::CreateFromFloat3(bounds.m_min), Vector3::CreateFromFloat3(bounds.m_max));
    }

    bool HitBounds(const Aabb& bounds, const Vector3& origin, const Vector3& reciprocal, float& distance)
    {
        float start = 0.0f;
        float end = distance;
        const auto result = AZ::Intersect::IntersectRayAABB2(origin, reciprocal, bounds, start, end);
        if (result != AZ::Intersect::ISECT_RAY_AABB_NONE && end >= 0.0f && start < distance)
        {
            distance = AZStd::max(0.0f, start);
            return true;
        }
        return false;
    }

    float Trace(const DiffuseProbeGridSoftwareBvh& tree, AZStd::span<const Aabb> primitives,
        const Vector3& origin, const Vector3& reciprocal)
    {
        float closest = 1000.0f;
        uint32_t index = 0;
        uint32_t visited = 0;
        while (index < tree.m_nodes.size())
        {
            EXPECT_LE(++visited, tree.m_nodes.size());
            if (visited > tree.m_nodes.size())
            {
                break;
            }
            const auto& node = tree.m_nodes[index];
            float distance = closest;
            if (!HitBounds(CpuBounds(tree.m_bounds[index]), origin, reciprocal, distance))
            {
                index = node.m_escape;
                continue;
            }
            if (!node.m_count)
            {
                index = node.m_first;
                continue;
            }
            for (uint32_t item = 0; item < node.m_count; ++item)
            {
                HitBounds(primitives[tree.m_primitiveIndices[node.m_first + item]], origin, reciprocal, closest);
            }
            index = node.m_escape;
        }
        return closest;
    }

    TEST_F(DiffuseProbeGridSoftwareBvhTests, EmptyInputClearsPreviousTree)
    {
        DiffuseProbeGridSoftwareBvh tree;
        AZStd::vector<Aabb> boxes{Aabb::CreateFromMinMax(Vector3(-1.0f), Vector3(1.0f))};
        tree.Build(boxes);
        ASSERT_EQ(tree.m_nodes.size(), 1);
        EXPECT_EQ(tree.m_nodes[0].m_escape, 1);
        tree.Build({});
        EXPECT_TRUE(tree.m_nodes.empty());
        EXPECT_TRUE(tree.m_bounds.empty());
        EXPECT_TRUE(tree.m_primitiveIndices.empty());
        EXPECT_TRUE(tree.m_refitLevels.empty());
    }

    TEST_F(DiffuseProbeGridSoftwareBvhTests, SpatialTraversalMatchesBruteForce)
    {
        AZStd::vector<Aabb> boxes;
        for (uint32_t i = 0; i < 257; ++i)
        {
            const Vector3 center(float(i % 17) - 8.0f, float(i / 17) - 8.0f, std::sin(float(i)) * 2.0f);
            boxes.push_back(Aabb::CreateCenterHalfExtents(center, Vector3(0.4f, 0.3f, 0.02f)));
        }
        DiffuseProbeGridSoftwareBvh tree;
        tree.Build(boxes);
        AZStd::vector<uint32_t> seen(boxes.size(), 0);
        for (const auto& node : tree.m_nodes)
        {
            ASSERT_LE(node.m_escape, tree.m_nodes.size());
            for (uint32_t i = 0; i < node.m_count; ++i)
            {
                ASSERT_LT(node.m_first + i, tree.m_primitiveIndices.size());
                ++seen[tree.m_primitiveIndices[node.m_first + i]];
            }
        }
        for (const auto count : seen)
        {
            EXPECT_EQ(count, 1);
        }
        for (uint32_t i = 0; i < 512; ++i)
        {
            const Vector3 origin(std::sin(float(i) * 0.4f) * 15.0f, std::cos(float(i) * 0.7f) * 15.0f, 12.0f);
            const Vector3 target(float(i % 17) - 8.0f, float((i * 7) % 17) - 8.0f, -3.0f);
            const Vector3 reciprocal = (target - origin).GetNormalized().GetReciprocal();
            float brute = 1000.0f;
            for (const auto& box : boxes)
            {
                HitBounds(box, origin, reciprocal, brute);
            }
            EXPECT_NEAR(Trace(tree, boxes, origin, reciprocal), brute, 1e-4f);
        }
    }

    TEST_F(DiffuseProbeGridSoftwareBvhTests, CoincidentPrimitivesTerminateAndRefitAfterDeformation)
    {
        AZStd::vector<Aabb> boxes(513, Aabb::CreateFromMinMax(Vector3(-1.0f), Vector3(1.0f)));
        DiffuseProbeGridSoftwareBvh tree;
        tree.Build(boxes);
        ASSERT_LT(tree.m_nodes.size(), boxes.size() * 2);
        ASSERT_LE(tree.m_refitLevels.size(), 64);
        boxes[0] = Aabb::CreateFromMinMax(Vector3(50.0f, -1.0f, -1.0f), Vector3(52.0f, 1.0f, 1.0f));
        auto store = [&](uint32_t node, const Aabb& bounds)
        {
            bounds.GetMin().StoreToFloat3(tree.m_bounds[node].m_min);
            bounds.GetMax().StoreToFloat3(tree.m_bounds[node].m_max);
        };
        for (uint32_t index = 0; index < tree.m_nodes.size(); ++index)
        {
            const auto& node = tree.m_nodes[index];
            if (node.m_count)
            {
                Aabb bounds = Aabb::CreateNull();
                for (uint32_t item = 0; item < node.m_count; ++item)
                {
                    bounds.AddAabb(boxes[tree.m_primitiveIndices[node.m_first + item]]);
                }
                store(index, bounds);
            }
        }
        for (const auto& level : tree.m_refitLevels)
        {
            for (const auto index : level)
            {
                const auto left = tree.m_nodes[index].m_first;
                const auto right = tree.m_nodes[left].m_escape;
                auto bounds = CpuBounds(tree.m_bounds[left]);
                bounds.AddAabb(CpuBounds(tree.m_bounds[right]));
                store(index, bounds);
            }
        }
        EXPECT_TRUE(CpuBounds(tree.m_bounds[0]).Contains(boxes[0]));
        const Vector3 origin(60.0f, 0.1f, 0.1f);
        const Vector3 reciprocal(-1.0f, 1e20f, 1e20f);
        EXPECT_NEAR(Trace(tree, boxes, origin, reciprocal), 8.0f, 1e-4f);
    }
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
