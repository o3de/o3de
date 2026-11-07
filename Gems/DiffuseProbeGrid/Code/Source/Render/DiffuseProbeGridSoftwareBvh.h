/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Aabb.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/containers/vector.h>

namespace AZ::Render
{
    //! Private software tracing topology. Bounds are separate so instances share immutable topology while skinned
    //! instances refit their own bounds. Preorder nodes carry escape links for traversal without a fixed-size stack.
    class DiffuseProbeGridSoftwareBvh
    {
    public:
        static constexpr uint32_t InvalidNode = 0xffffffffu;

        struct Node
        {
            uint32_t m_first = 0;       //!< First primitive reference for leaves; left child for internal nodes.
            uint32_t m_count = 0;       //!< Nonzero for leaves.
            uint32_t m_escape = 0;      //!< First node outside the subtree; node count at the root.
            uint32_t m_parent = InvalidNode;
        };

        struct Bounds
        {
            float m_min[3] = {};
            uint32_t m_padding0 = 0;
            float m_max[3] = {};
            uint32_t m_padding1 = 0;
        };

        //! Builds spatially partitioned topology for valid, finite primitive bounds. Empty input clears all data.
        void Build(AZStd::span<const Aabb> primitives);

        AZStd::vector<Node> m_nodes;
        AZStd::vector<Bounds> m_bounds;
        AZStd::vector<uint32_t> m_primitiveIndices;
        //! Bottom-up internal-node lists for synchronized parallel GPU refitting.
        AZStd::vector<AZStd::vector<uint32_t>> m_refitLevels;

    private:
        uint32_t BuildNode(AZStd::span<const Aabb> primitives, uint32_t first, uint32_t count, uint32_t parent, uint32_t depth);
    };

    static_assert(sizeof(DiffuseProbeGridSoftwareBvh::Node) == 16);
    static_assert(sizeof(DiffuseProbeGridSoftwareBvh::Bounds) == 32);
}
