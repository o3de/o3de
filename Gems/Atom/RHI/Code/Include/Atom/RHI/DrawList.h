/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DrawItem.h>
#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI.Reflect/Handle.h>

#include <AzCore/std/containers/span.h>

#include <AzCore/std/containers/bitset.h>

namespace AZ::RHI
{
    //! Draw list tags are unique ids identifying a unique list of draw items. The draw packet
    //! contains multiple draw items, where each draw item is associated with a draw list tag.
    //!
    //! A draw list tag is designed to map to a specific type of draw call; e.g. shadows, forward-opaque,
    //! forward-transparent, depth, etc. Multiple instances of these lists will exist (one per view,
    //! for example).
    //!
    //! The number of used tags should be relatively small. As such, they are also stored in a bit mask,
    //! which allows for very fast queries when building the draw lists.
    //!
    //! See also DrawListTagRegistry.
    using DrawListTag = Handle<uint8_t>;
    using DrawListMask = AZStd::bitset<RHI::Limits::Pipeline::DrawListTagCountMax>;

    using DrawList = AZStd::vector<RHI::DrawItemProperties>;
    using DrawListView = AZStd::span<const RHI::DrawItemProperties>;

    /// Contains a table of draw lists, indexed by the tag.
    using DrawListsByTag = AZStd::array<DrawList, RHI::Limits::Pipeline::DrawListTagCountMax>;

    /// Uniformly partitions the draw list and returns the sub-list denoted by the provided index.
    DrawListView GetDrawListPartition(DrawListView drawList, size_t partitionIndex, size_t partitionCount);

    void SortDrawList(DrawList& drawList, DrawListSortType sortType);
}
