/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Crc.h>

namespace AzToolsFramework
{
    namespace SettingProperty
    {
        const static AZ::Crc32 Min = AZ_CRC("Min");
        const static AZ::Crc32 Max = AZ_CRC("Max");
        const static AZ::Crc32 Step = AZ_CRC("Step");
    } // namespace SettingProperty
} // namespace AzToolsFramework
