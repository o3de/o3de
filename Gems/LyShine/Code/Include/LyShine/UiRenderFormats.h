/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/intrusive_slist.h>

namespace LyShine
{
    struct UCol
    {
        union
        {
            uint32 dcolor;
            uint8  bcolor[4];

            struct
            {
                uint8 b, g, r, a;
            };
            struct
            {
                uint8 z, y, x, w;
            };
        };
    };

    struct UiPrimitiveVertex
    {
        Vec2 xy;
        UCol color;
        Vec2 st;
        uint8 texIndex;
        uint8 texHasColorChannel;
        uint8 texIndex2;
        uint8 pad;
    };

    using UiIndice = AZ::u16;

    struct UiPrimitive : public AZStd::intrusive_slist_node<UiPrimitive>
    {
        UiPrimitiveVertex* m_vertices = nullptr;
        uint16* m_indices = nullptr;
        int m_numVertices = 0;
        int m_numIndices = 0;
    };
    using UiPrimitiveList = AZStd::intrusive_slist<UiPrimitive, AZStd::slist_base_hook<UiPrimitive>>;
};
