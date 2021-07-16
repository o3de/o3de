/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Crc.h>

namespace GraphCanvas
{
    namespace AutomationIds
    {
        // ToolBar
        static const AZ::Crc32 CreateCommentButton = AZ_CRC("GC_CreateCommentButton", 0xb83d2bef);
        static const AZ::Crc32 GroupButton = AZ_CRC("GC_GroupButton", 0x4ee8f5e9);
        static const AZ::Crc32 UngroupButton = AZ_CRC("GC_UngroupButton", 0xe7f2c4fd);
    }
}
