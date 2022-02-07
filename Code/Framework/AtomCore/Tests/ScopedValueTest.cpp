/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomCore/Utils/ScopedValue.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    TEST(ScopedValueTest, TestBoolValue)
    {
        bool localValue = false;

        {
            AZ::ScopedValue<bool> scopedValue(&localValue, true, false);
            EXPECT_EQ(true, localValue);
        }
        
        EXPECT_EQ(false, localValue);
    }
    
    TEST(ScopedValueTest, TestIntValue)
    {
        int localValue = 0;

        {
            AZ::ScopedValue<int> scopedValue(&localValue, 1, 2);
            EXPECT_EQ(1, localValue);
        }
        
        EXPECT_EQ(2, localValue);
    }
}
