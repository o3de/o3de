/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <Atom/RHI/DrawItem.h>
#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI.Reflect/Handle.h>

#include <AtomCore/std/containers/array_view.h>

#include <AzCore/std/containers/bitset.h>

namespace AZ
{
    namespace RHI
    {
        /**
         * Draw list tags are unique ids identifying a unique list of draw items. The draw packet
         * contains multiple draw items, where each draw item is associated with a draw list tag.
         *
         * A draw list tag is designed to map to a specific type of draw call; e.g. shadows, forward-opaque,
         * forward-transparent, depth, etc. Multiple instances of these lists will exist (one per view,
         * for example).
         *
         * The number of used tags should be relatively small. As such, they are also stored in a bit mask,
         * which allows for very fast queries when building the draw lists.
         *
         * See also DrawListTagRegistry.
         */
        using DrawListTag = Handle<uint8_t>;
        using DrawListMask = AZStd::bitset<RHI::Limits::Pipeline::DrawListTagCountMax>;

        using DrawList = AZStd::vector<RHI::DrawItemKeyPair>;
        using DrawListView = AZStd::array_view<RHI::DrawItemKeyPair>;

        /// Contains a table of draw lists, indexed by the tag.
        using DrawListsByTag = AZStd::array<DrawList, RHI::Limits::Pipeline::DrawListTagCountMax>;

        /// Uniformly partitions the draw list and returns the sub-list denoted by the provided index.
        DrawListView GetDrawListPartition(DrawListView drawList, size_t partitionIndex, size_t partitionCount);

        void SortDrawList(DrawList& drawList, DrawListSortType sortType);
    }
}
