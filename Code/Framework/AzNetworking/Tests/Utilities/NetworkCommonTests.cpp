/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
