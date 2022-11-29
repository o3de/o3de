/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/Serialization/HashSerializer.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    struct HashSerializerElement
    {
        bool testBool = false;
        char testChar = 'a';
        int8_t testInt8 = 0;
        int16_t testInt16 = 1;
        int32_t testInt32 = 2;
        int64_t testInt64 = 3;
        uint8_t testUint8 = 0;
        uint16_t testUint16 = 1;
        uint32_t testUint32 = 2;
        uint64_t testUint64 = 3;
        double testDouble = 1.0;
        float testFloat = 1.f;
        AZStd::fixed_string<32> testFixedString = "FixedString";

        bool Serialize(AzNetworking::ISerializer& serializer)
        {
            if (!serializer.Serialize(testBool, "TestBool") || !serializer.Serialize(testChar, "TestChar") ||
                !serializer.Serialize(testInt8, "TestInt8") || !serializer.Serialize(testInt16, "TestInt16") ||
                !serializer.Serialize(testInt32, "TestInt32") || !serializer.Serialize(testInt64, "TestInt64") ||
                !serializer.Serialize(testUint8, "TestUint8") || !serializer.Serialize(testUint16, "TestUint16") ||
                !serializer.Serialize(testUint32, "TestUint32") || !serializer.Serialize(testUint64, "TestUint64") ||
                !serializer.Serialize(testDouble, "TestDouble") || !serializer.Serialize(testFloat, "TestFloat") ||
                !serializer.Serialize(testFixedString, "TestFixedString"))
            {
                return false;
            }

            return true;
        }
    };

    class HashSerializerTests : public AllocatorsFixture
    {
    public:
        void SetUp() override
        {
            SetupAllocator();
        }
    };

    TEST_F(HashSerializerTests, TestHashSerialization)
    {
        HashSerializerElement element1;
        AzNetworking::HashSerializer hashSerializer1;

        EXPECT_TRUE(element1.Serialize(hashSerializer1));

        HashSerializerElement element2;
        AzNetworking::HashSerializer hashSerializer2;

        EXPECT_TRUE(element2.Serialize(hashSerializer2));

        EXPECT_EQ(hashSerializer1.GetHash(), hashSerializer2.GetHash());
    }

    TEST_F(HashSerializerTests, TestHashSerializerDefaults)
    {
        AzNetworking::HashSerializer hashSerializer;
        EXPECT_EQ(hashSerializer.GetSerializerMode(), AzNetworking::SerializerMode::ReadFromObject);
        EXPECT_EQ(hashSerializer.GetCapacity(), 0);
        EXPECT_EQ(hashSerializer.GetSize(), 0);
        EXPECT_EQ(hashSerializer.GetBuffer(), nullptr);
        EXPECT_TRUE(hashSerializer.BeginObject("Unused"));
        EXPECT_TRUE(hashSerializer.EndObject("Unused"));
        hashSerializer.ClearTrackedChangesFlag(); // NO-OP
        EXPECT_FALSE(hashSerializer.GetTrackedChangesFlag());
    }
}
