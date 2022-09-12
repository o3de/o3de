/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AZTestShared/Utils/Utils.h>

namespace UnitTest
{
    // O3DE_DEPRECATED - Use AZ::Test::ScopedAutoTempDirectory
    // The UnitTest::ScopedTemporaryDirectory was another implementation
    // of a temporary directory workflow, but in AzFrameworkTestShared instead of
    // AzTestShared
    using ScopedTemporaryDirectory = AZ::Test::ScopedAutoTempDirectory;
}
