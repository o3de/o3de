/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/Serialization/StringifySerializer.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    struct StringifySerializerElement
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

        bool CompareValueMap(AzNetworking::StringifySerializer::ValueMap& valueMap)
        {
            return valueMap["TestBool"] == "false" && valueMap["TestChar"] == "97"
                && valueMap["TestInt8"] == "0" && valueMap["TestInt16"] == "1"
                && valueMap["TestInt32"] == "2" && valueMap["TestInt64"] == "3"
                && valueMap["TestUint8"] == "0" && valueMap["TestUint16"] == "1"
                && valueMap["TestUint32"] == "2" && valueMap["TestUint64"] == "3"
                && valueMap["TestDouble"] == "1.000000" && valueMap["TestFloat"] == "1.000000"
                && valueMap["TestFixedString.String"] == "FixedString" && valueMap["TestFixedString.Size"] == "11";
        }
    };

    class StringifySerializerTests : public AllocatorsFixture
    {
    public:
        void SetUp() override
        {
            SetupAllocator();
        }
    };

    TEST_F(StringifySerializerTests, TestHashSerialization)
    {
        StringifySerializerElement element;
        AzNetworking::StringifySerializer stringifySerializer;

        EXPECT_TRUE(element.Serialize(stringifySerializer));
        AzNetworking::StringifySerializer::ValueMap valueMap = stringifySerializer.GetValueMap();
        EXPECT_TRUE(element.CompareValueMap(valueMap));
    }

    TEST_F(StringifySerializerTests, TestHashSerializerDefaults)
    {
        AzNetworking::StringifySerializer stringifySerializer;
        EXPECT_EQ(stringifySerializer.GetSerializerMode(), AzNetworking::SerializerMode::ReadFromObject);
        EXPECT_EQ(stringifySerializer.GetCapacity(), 0);
        EXPECT_EQ(stringifySerializer.GetSize(), 0);
        EXPECT_EQ(stringifySerializer.GetBuffer(), nullptr);
        EXPECT_TRUE(stringifySerializer.BeginObject("Unused"));
        EXPECT_TRUE(stringifySerializer.EndObject("Unused"));
        stringifySerializer.ClearTrackedChangesFlag(); // NO-OP
        EXPECT_FALSE(stringifySerializer.GetTrackedChangesFlag());
    }
}
