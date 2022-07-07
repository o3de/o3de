/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/Utilities/IpAddress.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    TEST(IpAddressTests, TestIpQuads)
    {
        const AzNetworking::IpAddress ip = AzNetworking::IpAddress(127, 0, 0, 1, 12345);

        EXPECT_EQ(ip.GetQuadA(), 127);
        EXPECT_EQ(ip.GetQuadB(), 0);
        EXPECT_EQ(ip.GetQuadC(), 0);
        EXPECT_EQ(ip.GetQuadD(), 1);

        EXPECT_EQ(ip.GetString(), "127.0.0.1:12345");
        EXPECT_EQ(ip.GetIpString(), "127.0.0.1");
    }
}
