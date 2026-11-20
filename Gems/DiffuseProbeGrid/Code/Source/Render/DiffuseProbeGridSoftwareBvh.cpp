/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Render/DiffuseProbeGridSoftwareBvh.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/algorithm.h>
#include <algorithm>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/limits.h>

namespace AZ::Render
{
    namespace
    {
        constexpr uint32_t BinCount = 12;
        constexpr uint32_t LeafSize = 4;

        float Area(const Aabb& bounds)
        {
            if (!bounds.IsValid())
            {
                return 0.0f;
            }
            const Vector3 extents = bounds.GetExtents();
            return extents.GetX() * extents.GetY() + extents.GetY() * extents.GetZ() + extents.GetZ() * extents.GetX();
        }
    }

    void DiffuseProbeGridSoftwareBvh::Build(AZStd::span<const Aabb> primitives)
    {
        m_nodes.clear();
        m_bounds.clear();
        m_primitiveIndices.clear();
        m_refitLevels.clear();
        // Two nodes per primitive is an upper bound; all GPU offsets are 32-bit.
        if (primitives.size() > AZStd::numeric_limits<uint32_t>::max() / 2)
        {
            AZ_Error("DiffuseProbeGrid", false, "Software BVH exceeds 32-bit node addressing");
            return;
        }
        m_primitiveIndices.reserve(primitives.size());
        for (uint32_t index = 0; index < primitives.size(); ++index)
        {
            if (primitives[index].IsValid() && primitives[index].GetMin().IsFinite() && primitives[index].GetMax().IsFinite())
            {
                m_primitiveIndices.push_back(index);
            }
        }
        if (!m_primitiveIndices.empty())
        {
            m_nodes.reserve(m_primitiveIndices.size() * 2);
            m_bounds.reserve(m_primitiveIndices.size() * 2);
            BuildNode(primitives, 0, aznumeric_cast<uint32_t>(m_primitiveIndices.size()), InvalidNode, 0);
        }
    }

    uint32_t DiffuseProbeGridSoftwareBvh::BuildNode(
        AZStd::span<const Aabb> primitives, uint32_t first, uint32_t count, uint32_t parent, uint32_t depth)
    {
        const uint32_t nodeIndex = aznumeric_cast<uint32_t>(m_nodes.size());
        m_nodes.emplace_back();
        Aabb bounds = Aabb::CreateNull();
        Aabb centroids = Aabb::CreateNull();
        for (uint32_t i = first; i < first + count; ++i)
        {
            const auto& primitive = primitives[m_primitiveIndices[i]];
            bounds.AddAabb(primitive);
            centroids.AddPoint(primitive.GetCenter());
        }
        Bounds gpuBounds;
        bounds.GetMin().StoreToFloat3(gpuBounds.m_min);
        bounds.GetMax().StoreToFloat3(gpuBounds.m_max);
        m_bounds.push_back(gpuBounds);
        m_nodes[nodeIndex].m_parent = parent;
        if (count <= LeafSize)
        {
            m_nodes[nodeIndex].m_first = first;
            m_nodes[nodeIndex].m_count = count;
            m_nodes[nodeIndex].m_escape = nodeIndex + 1;
            return 0;
        }

        float bestCost = AZStd::numeric_limits<float>::max();
        uint32_t bestAxis = 0;
        uint32_t bestBin = 0;
        bool hasSplit = false;
        // Binned surface-area splits improve incoherent diffuse-ray traversal. Limit skewed splits to keep
        // construction/refit depth bounded even for pathological input; the balanced fallback always makes progress.
        if (depth < 32)
        {
            for (uint32_t axis = 0; axis < 3; ++axis)
            {
                const float extent = centroids.GetExtents().GetElement(axis);
                if (extent <= 0.0f)
                {
                    continue;
                }
                AZStd::array<Aabb, BinCount> bins;
                AZStd::array<uint32_t, BinCount> sizes{};
                for (auto& bin : bins)
                {
                    bin = Aabb::CreateNull();
                }
                const float scale = BinCount / extent;
                const float origin = centroids.GetMin().GetElement(axis);
                for (uint32_t i = first; i < first + count; ++i)
                {
                    const auto& primitive = primitives[m_primitiveIndices[i]];
                    const uint32_t bin = AZStd::min(BinCount - 1, static_cast<uint32_t>(
                        AZStd::max(0.0f, (primitive.GetCenter().GetElement(axis) - origin) * scale)));
                    bins[bin].AddAabb(primitive);
                    ++sizes[bin];
                }
                AZStd::array<Aabb, BinCount> suffix;
                AZStd::array<uint32_t, BinCount> suffixCounts{};
                Aabb right = Aabb::CreateNull();
                uint32_t rightCount = 0;
                for (uint32_t bin = BinCount; bin-- > 0;)
                {
                    if (sizes[bin])
                    {
                        right.AddAabb(bins[bin]);
                    }
                    rightCount += sizes[bin];
                    suffix[bin] = right;
                    suffixCounts[bin] = rightCount;
                }
                Aabb left = Aabb::CreateNull();
                uint32_t leftCount = 0;
                for (uint32_t bin = 0; bin + 1 < BinCount; ++bin)
                {
                    if (sizes[bin])
                    {
                        left.AddAabb(bins[bin]);
                    }
                    leftCount += sizes[bin];
                    if (leftCount && suffixCounts[bin + 1])
                    {
                        const float cost = Area(left) * leftCount + Area(suffix[bin + 1]) * suffixCounts[bin + 1];
                        if (cost < bestCost)
                        {
                            bestCost = cost;
                            bestAxis = axis;
                            bestBin = bin;
                            hasSplit = true;
                        }
                    }
                }
            }
        }
        uint32_t leftCount = count / 2;
        if (hasSplit)
        {
            const float position = centroids.GetMin().GetElement(bestAxis) +
                centroids.GetExtents().GetElement(bestAxis) * (static_cast<float>(bestBin + 1) / BinCount);
            auto begin = m_primitiveIndices.begin() + first;
            const auto split = std::partition(begin, begin + count, [&](uint32_t primitive)
            {
                return primitives[primitive].GetCenter().GetElement(bestAxis) < position;
            });
            const uint32_t candidateCount = aznumeric_cast<uint32_t>(split - begin);
            if (candidateCount > 0 && candidateCount < count)
            {
                leftCount = candidateCount;
            }
        }
        m_nodes[nodeIndex].m_first = nodeIndex + 1;
        const uint32_t leftHeight = BuildNode(primitives, first, leftCount, nodeIndex, depth + 1);
        const uint32_t rightHeight = BuildNode(primitives, first + leftCount, count - leftCount, nodeIndex, depth + 1);
        m_nodes[nodeIndex].m_escape = aznumeric_cast<uint32_t>(m_nodes.size());
        const uint32_t height = 1 + AZStd::max(leftHeight, rightHeight);
        if (m_refitLevels.size() < height)
        {
            m_refitLevels.resize(height);
        }
        m_refitLevels[height - 1].push_back(nodeIndex);
        return height;
    }
}
