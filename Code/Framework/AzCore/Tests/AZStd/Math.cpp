/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/math.h>

namespace UnitTest
{
    template<typename T>
    static void performTests()
    {
        using ::testing::Eq;
        using AZStd::lerp;

        constexpr T inf = std::numeric_limits<T>::infinity();
        constexpr T maxNumber = std::numeric_limits<T>::max();
        constexpr T eps = std::numeric_limits<T>::epsilon();
        constexpr T a = T(42);

        // Following the properties defined in the c++ standard p0811r3 proposal

        //exactness: lerp(a,b,0)==a && lerp(a,b,1)==b
        EXPECT_THAT(lerp(eps, maxNumber, T(0)), Eq(eps));
        EXPECT_THAT(lerp(eps, maxNumber, T(1)), Eq(maxNumber));

        //monotonicity: cmp(lerp(a,b,t2),lerp(a,b,t1)) * cmp(t2,t1) * cmp(b,a) >= 0, where cmp is an arithmetic three-way comparison function

        //determinacy: result is NaN only for lerp(a,a,INFINITY)
        //TODO: do we care ?
        //boundedness: t<0 || t>1 || isfinite(lerp(a,b,t))
        //TODO: ?

        //consistency: lerp(a,a,t)==a
        EXPECT_THAT(lerp(a, a, T(0.5)), Eq(a));
        EXPECT_THAT(lerp(eps, eps, T(0.5)), Eq(eps));
    }
    TEST(StdMath, LerpOperations)
    {
        performTests<float>();
        performTests<double>();
        performTests<long double>();
    }
} // namespace UnitTest
