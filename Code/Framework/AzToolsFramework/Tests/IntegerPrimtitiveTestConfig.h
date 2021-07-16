/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

namespace UnitTest
{
    // Supported integer types for spinbox control
    using IntegerPrimtitiveTestConfigs = ::testing::Types
        <
        AZ::s8,
        AZ::u8,
        AZ::s16,
        AZ::u16,
        AZ::s32,
        AZ::u32,
        AZ::s64,
        AZ::u64
        >;
} // namespace UnitTest
