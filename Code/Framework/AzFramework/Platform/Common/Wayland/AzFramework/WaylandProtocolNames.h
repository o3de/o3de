/*
* Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once


namespace AzFramework
{
    static constexpr AZ::Crc32 WaylandSeatName = AZ_CRC_CE("wl_seat");
    static constexpr AZ::Crc32 WaylandOutputName = AZ_CRC_CE("wl_output");
    static constexpr AZ::Crc32 CursorShapeManagerName = AZ_CRC_CE("wp_cursor_shape_manager_v1");
    static constexpr AZ::Crc32 PointerConstraintsName = AZ_CRC_CE("zwp_pointer_constraints_v1");
    static constexpr AZ::Crc32 RelativePointerManagerName = AZ_CRC_CE("zwp_relative_pointer_manager_v1");
    static constexpr AZ::Crc32 XdgWmBaseName = AZ_CRC_CE("xdg_wm_base");
    static constexpr AZ::Crc32 XdgDecorationManagerName = AZ_CRC_CE("zxdg_decoration_manager_v1");

    //Minimum versions we require and bind for.
    static constexpr uint32_t WaylandSeatVersion = 6;
    static constexpr uint32_t XdgWmBaseVersion = 3;
    static constexpr uint32_t XdgDecorationManagerVersion = 1;
    static constexpr uint32_t CursorShapeVersion = 2;
    static constexpr uint32_t PointerConstraintsVersion = 1;
    static constexpr uint32_t RelativePointerManagerVersion = 1;
}
