/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/ConnectionLayer/SequenceGenerator.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    TEST(SequenceGenerator, Basic8BitSequenceTest)
    {
        EXPECT_FALSE(AzNetworking::SequenceMoreRecent(uint8_t(0), uint8_t(1)));
        EXPECT_FALSE(AzNetworking::SequenceMoreRecent(uint8_t(1), uint8_t(2)));
        EXPECT_TRUE (AzNetworking::SequenceMoreRecent(uint8_t(1), uint8_t(0)));
        EXPECT_TRUE (AzNetworking::SequenceMoreRecent(uint8_t(2), uint8_t(1)));
    }

    TEST(SequenceGenerator, Wraparound8BitSequenceTest)
    {
        EXPECT_FALSE(AzNetworking::SequenceMoreRecent(uint8_t(254), uint8_t(255)));
        EXPECT_FALSE(AzNetworking::SequenceMoreRecent(uint8_t(255), uint8_t(  0)));
        EXPECT_TRUE (AzNetworking::SequenceMoreRecent(uint8_t(255), uint8_t(254)));
        EXPECT_TRUE (AzNetworking::SequenceMoreRecent(uint8_t(  0), uint8_t(255)));
    }

    TEST(SequenceGenerator, Basic16BitSequenceTest)
    {
        EXPECT_FALSE(AzNetworking::SequenceMoreRecent(uint16_t(0), uint16_t(1)));
        EXPECT_FALSE(AzNetworking::SequenceMoreRecent(uint16_t(1), uint16_t(2)));
        EXPECT_TRUE (AzNetworking::SequenceMoreRecent(uint16_t(1), uint16_t(0)));
        EXPECT_TRUE (AzNetworking::SequenceMoreRecent(uint16_t(2), uint16_t(1)));
    }

    TEST(SequenceGenerator, Wraparound16BitSequenceTest)
    {
        EXPECT_FALSE(AzNetworking::SequenceMoreRecent(uint16_t(65534), uint16_t(65535)));
        EXPECT_FALSE(AzNetworking::SequenceMoreRecent(uint16_t(65535), uint16_t(    0)));
        EXPECT_TRUE (AzNetworking::SequenceMoreRecent(uint16_t(65535), uint16_t(65534)));
        EXPECT_TRUE (AzNetworking::SequenceMoreRecent(uint16_t(    0), uint16_t(65535)));
    }
}
