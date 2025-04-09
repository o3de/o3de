/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/PageTileAllocator.h>

namespace AZ::RHI
{
    //! A list of tile groups in one memory page
    template<typename PageType>
    struct PageTiles
    {
        //! The memory object (heap) which is evenly divided to multiple tiles and it will contain all the tiles referenced by m_tileSpanList
        RHI::Ptr<PageType> m_heap;
        //! Multiple tile spans. Each tile span represents a continuous number of tiles in the page
        AZStd::vector<RHI::PageTileSpan> m_tileSpanList;
        //! The total amount tiles in the m_tileSpanList
        uint32_t m_totalTileCount = 0;
    };
}
