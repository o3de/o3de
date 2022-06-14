/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <TerrainTestFixtures.h>

// This uses custom test / benchmark hooks so that we can load components from other gems in our unit tests and benchmarks.
AZ_UNIT_TEST_HOOK(new UnitTest::TerrainTestEnvironment, UnitTest::TerrainBenchmarkEnvironment);
