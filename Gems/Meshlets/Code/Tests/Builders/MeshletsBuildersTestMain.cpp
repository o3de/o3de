/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>

// The Meshlets.Builders.Tests module had no test hook, so AzTestRunner loaded the DLL and then
// failed with "FAILED to find symbol: AzRunUnitTests" -- every test in
// meshlets_builders_tests_files.cmake compiled but none of them could ever be executed.
// Mirrors the hook in Tests/MeshletsTest.cpp, kept in its own file because the builders tests
// are split across six sources with no obvious "main" among them.
namespace UnitTest
{
    AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
}
