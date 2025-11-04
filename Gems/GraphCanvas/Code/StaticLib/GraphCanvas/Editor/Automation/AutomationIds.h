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
        static const AZ::Crc32 CreateCommentButton = AZ_CRC_CE("GC_CreateCommentButton");
        static const AZ::Crc32 GroupButton = AZ_CRC_CE("GC_GroupButton");
        static const AZ::Crc32 UngroupButton = AZ_CRC_CE("GC_UngroupButton");
    }
}
