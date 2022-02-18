/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>


#if defined(HAVE_BENCHMARK)

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV, UnitTest::ScopedAllocatorBenchmarkEnvironment)

#else

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);

#endif // HAVE_BENCHMARK
