/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/Utilities/CidrAddress.h>
#include <AzNetworking/Utilities/IpAddress.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    class CidrAddressTests
        : public LeakDetectionFixture
    {
    };

    static const AzNetworking::IpAddress testIp1 = AzNetworking::IpAddress(127, 0, 0, 1, 0);
    static const AzNetworking::IpAddress testIp2 = AzNetworking::IpAddress(127, 0, 0, 255, 0);
    static const AzNetworking::IpAddress testIp3 = AzNetworking::IpAddress(127, 0, 255, 255, 0);
    static const AzNetworking::IpAddress testIp4 = AzNetworking::IpAddress(127, 255, 255, 255, 0);
    static const AzNetworking::IpAddress testIp5 = AzNetworking::IpAddress(255, 255, 255, 255, 0);

    TEST_F(CidrAddressTests, Test32BitMask)
    {
        AzNetworking::CidrAddress testCidr1("127.0.0.1/32");
        EXPECT_TRUE (testCidr1.IsMatch(testIp1));
        EXPECT_FALSE(testCidr1.IsMatch(testIp2));
        EXPECT_FALSE(testCidr1.IsMatch(testIp3));
        EXPECT_FALSE(testCidr1.IsMatch(testIp4));
        EXPECT_FALSE(testCidr1.IsMatch(testIp5));
    }

    TEST_F(CidrAddressTests, Test24BitMask)
    {
        AzNetworking::CidrAddress testCidr2("127.0.0.1/24");
        EXPECT_TRUE (testCidr2.IsMatch(testIp1));
        EXPECT_TRUE (testCidr2.IsMatch(testIp2));
        EXPECT_FALSE(testCidr2.IsMatch(testIp3));
        EXPECT_FALSE(testCidr2.IsMatch(testIp4));
        EXPECT_FALSE(testCidr2.IsMatch(testIp5));
    }

    TEST_F(CidrAddressTests, Test16BitMask)
    {
        AzNetworking::CidrAddress testCidr3("127.0.0.1/16");
        EXPECT_TRUE (testCidr3.IsMatch(testIp1));
        EXPECT_TRUE (testCidr3.IsMatch(testIp2));
        EXPECT_TRUE (testCidr3.IsMatch(testIp3));
        EXPECT_FALSE(testCidr3.IsMatch(testIp4));
        EXPECT_FALSE(testCidr3.IsMatch(testIp5));
    }

    TEST_F(CidrAddressTests, Test8BitMask)
    {
        AzNetworking::CidrAddress testCidr4("127.0.0.1/8");
        EXPECT_TRUE (testCidr4.IsMatch(testIp1));
        EXPECT_TRUE (testCidr4.IsMatch(testIp2));
        EXPECT_TRUE (testCidr4.IsMatch(testIp3));
        EXPECT_TRUE (testCidr4.IsMatch(testIp4));
        EXPECT_FALSE(testCidr4.IsMatch(testIp5));
    }

    TEST_F(CidrAddressTests, TestAnyMatch)
    {
        AzNetworking::CidrAddress testCidr5("127.0.0.1/0");
        EXPECT_TRUE (testCidr5.IsMatch(testIp1));
        EXPECT_TRUE (testCidr5.IsMatch(testIp2));
        EXPECT_TRUE (testCidr5.IsMatch(testIp3));
        EXPECT_TRUE (testCidr5.IsMatch(testIp4));
        EXPECT_TRUE (testCidr5.IsMatch(testIp5));
    }

    TEST_F(CidrAddressTests, TestOutOfRange)
    {
        AzNetworking::CidrAddress testCidr6("127.0.0.1/33");
        EXPECT_EQ(testCidr6.GetMask(), 0xFFFFFFFF);
        AzNetworking::CidrAddress testCidr7("127.0.0.1/-1");
        EXPECT_EQ(testCidr7.GetMask(), 0);
    }
}
