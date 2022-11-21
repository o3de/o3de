/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Console/Console.h>
#include <AzNetworking/Serialization/NetworkInputSerializer.h>
#include <AzNetworking/Serialization/NetworkOutputSerializer.h>

#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    struct InputOutputDataElement
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

    class InputOutputSerializerTests : public AllocatorsFixture
    {
    public:
        void SetUp() override
        {
            SetupAllocator();
        }
    };

    TEST_F(InputOutputSerializerTests, TestTypeValidatingSerializer)
    {
        const size_t Capacity = 2048;
        const uint8_t ExpectedSerializedBytes = 61;
        const uint8_t CopyBufferSize = 4;
        AZStd::array<uint8_t, Capacity> buffer;
        AZStd::array<uint8_t, CopyBufferSize> copyBuffer;
        copyBuffer.fill(1);

        InputOutputDataElement inElement;
        AzNetworking::NetworkInputSerializer inSerializer(
            buffer.data(), static_cast<uint32_t>(buffer.size()));

        EXPECT_NE(inSerializer.GetBuffer(), nullptr);
        EXPECT_EQ(inSerializer.GetCapacity(), Capacity);
        EXPECT_TRUE(inElement.Serialize(inSerializer));
        EXPECT_EQ(inSerializer.GetSize(), ExpectedSerializedBytes);
        EXPECT_TRUE(inSerializer.CopyToBuffer(copyBuffer.data(), static_cast<uint32_t>(copyBuffer.size())));
        EXPECT_EQ(inSerializer.GetSize(), ExpectedSerializedBytes + CopyBufferSize);
        inSerializer.ClearTrackedChangesFlag();
        EXPECT_FALSE(inSerializer.GetTrackedChangesFlag());

        InputOutputDataElement outElement;
        AzNetworking::NetworkOutputSerializer outSerializer(
            buffer.data(), static_cast<uint32_t>(buffer.size()));

        EXPECT_NE(outSerializer.GetBuffer(), nullptr);
        EXPECT_EQ(outSerializer.GetCapacity(), Capacity);
        EXPECT_TRUE(outElement.Serialize(outSerializer));
        EXPECT_EQ(outSerializer.GetSize(), ExpectedSerializedBytes);
        outSerializer.ClearTrackedChangesFlag();
        EXPECT_FALSE(outSerializer.GetTrackedChangesFlag());

        EXPECT_EQ(inElement.testBool, outElement.testBool);
        EXPECT_EQ(inElement.testChar, outElement.testChar);
        EXPECT_EQ(inElement.testInt8, outElement.testInt8);
        EXPECT_EQ(inElement.testInt16, outElement.testInt16);
        EXPECT_EQ(inElement.testInt32, outElement.testInt32);
        EXPECT_EQ(inElement.testUint8, outElement.testUint8);
        EXPECT_EQ(inElement.testUint16, outElement.testUint16);
        EXPECT_EQ(inElement.testUint32, outElement.testUint32);
        EXPECT_EQ(inElement.testUint64, outElement.testUint64);
        EXPECT_EQ(inElement.testDouble, outElement.testDouble);
        EXPECT_EQ(inElement.testFloat, outElement.testFloat);
        EXPECT_EQ(inElement.testFixedString, outElement.testFixedString);
    }
} // namespace UnitTest
