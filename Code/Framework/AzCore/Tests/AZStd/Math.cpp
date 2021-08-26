/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/math.h>

// Unittest code copied from LLVM's libcxx under the following license:
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

namespace UnitTest
{
    template<typename T>
    static void performTests()
    {
        using ::testing::Eq;

        constexpr T maxV = std::numeric_limits<T>::max();
        constexpr T inf = std::numeric_limits<T>::infinity();

        //  Things that can be compared exactly
        EXPECT_THAT(AZStd::lerp(T(0), T(12), T(0)), Eq(T(0)));
        EXPECT_THAT(AZStd::lerp(T(0), T(12), T(1)), Eq(T(12)));
        EXPECT_THAT(AZStd::lerp(T(12), T(0), T(0)), Eq(T(12)));
        EXPECT_THAT(AZStd::lerp(T(12), T(0), T(1)), Eq(T(0)));

        EXPECT_THAT(AZStd::lerp(T(0), T(12), T(0.5)), Eq(T(6)));
        EXPECT_THAT(AZStd::lerp(T(12), T(0), T(0.5)), Eq(T(6)));
        EXPECT_THAT(AZStd::lerp(T(0), T(12), T(2)), Eq(T(24)));
        EXPECT_THAT(AZStd::lerp(T(12), T(0), T(2)), Eq(T(-12)));

        EXPECT_THAT(AZStd::lerp(maxV, maxV / 10, T(0)), Eq(maxV));
        EXPECT_THAT(AZStd::lerp(maxV / 10, maxV, T(1)), Eq(maxV));

        EXPECT_THAT(AZStd::lerp(T(2.3), T(2.3), inf), Eq(T(2.3)));

        EXPECT_THAT(AZStd::lerp(T(0), T(0), T(23)), Eq(T(0)));
        EXPECT_THAT(std::isnan(AZStd::lerp(T(0), T(0), inf)), Eq(true));
    }
    TEST(StdMath, LerpOperations)
    {
        performTests<float>();
        performTests<double>();
        performTests<long double>();
    }
} // namespace UnitTest
