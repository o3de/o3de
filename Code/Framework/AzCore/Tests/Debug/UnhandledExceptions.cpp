/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    class UnhandledExceptions
        : public ScopedAllocatorSetupFixture
    {

    public:
        void causeAccessViolation()
        {
            int* someVariable = reinterpret_cast<int*>(0);
            *someVariable = 0;
        }
    };

#if GTEST_HAS_DEATH_TEST
    TEST_F(UnhandledExceptions, Handle)
    {
        EXPECT_DEATH(causeAccessViolation(), "");
    }
#endif
}
