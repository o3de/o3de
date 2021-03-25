/*
* All or Portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
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
