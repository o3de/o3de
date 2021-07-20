/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/Utilities/NetworkCommon.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    TEST(NetworkCommon, GenerateSerializerIndexLabelByte)
    {
        EXPECT_STREQ(AzNetworking::GenerateIndexLabel<255>(0).c_str(), "00");
        EXPECT_STREQ(AzNetworking::GenerateIndexLabel<255>(1).c_str(), "01");
        EXPECT_STREQ(AzNetworking::GenerateIndexLabel<255>(10).c_str(), "0A");
        EXPECT_STREQ(AzNetworking::GenerateIndexLabel<255>(100).c_str(), "64");
    }

    TEST(NetworkCommon, GenerateSerializerIndexLabelShort)
    {
        EXPECT_STREQ(AzNetworking::GenerateIndexLabel<65535>(0).c_str(), "0000");
        EXPECT_STREQ(AzNetworking::GenerateIndexLabel<65535>(1).c_str(), "0001");
        EXPECT_STREQ(AzNetworking::GenerateIndexLabel<65535>(10).c_str(), "000A");
        EXPECT_STREQ(AzNetworking::GenerateIndexLabel<65535>(100).c_str(), "0064");
    }

    TEST(NetworkCommon, GenerateSerializerIndexLabelMax)
    {
        EXPECT_STREQ(AzNetworking::GenerateIndexLabel<UINT_MAX>(UINT_MAX).c_str(), "FFFFFFFF");
    }
}
