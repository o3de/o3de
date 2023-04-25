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
#include <AzCore/std/string/memorytoascii.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzNetworking/Utilities/NetworkIncludes.h>

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

    class InputOutputSerializerTests : public LeakDetectionFixture
    {
    };

    TEST_F(InputOutputSerializerTests, TestTypeValidatingSerializer)
    {
        const size_t Capacity = 2048;
        const uint8_t ExpectedSerializedBytes = 57;
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


    /**
    * Internal helper function to test network serialization for a specific type against a known expected value
    *
    * \param[in] value         The value to serialize
    * \param[in] label         The label to identify the type under test in case of a failed expectation
    * \param[in] expectedData  The expected buffer to compare the results to
    * \param[in] expectedSize  The expected buffer size to compare the results to
    */
    template<typename T>
    void InternalTestSerializeType(T value, const char* label, const void* expectedData, const size_t expectedSize)
    {
        constexpr size_t Capacity = 32;
        AZStd::array<uint8_t, Capacity> buffer;
        buffer.fill(0);

        AzNetworking::NetworkInputSerializer inSerializer(buffer.data(), static_cast<uint32_t>(buffer.size()));

        AzNetworking::ISerializer& serializer = inSerializer;

        EXPECT_TRUE(serializer.Serialize(value, label)) << "Serialize failed for " << label;

        EXPECT_EQ(serializer.GetSize(), expectedSize) << "Resulting buffer size for " << label << "does not match expected (" << serializer.GetSize() << " != " << expectedSize << ")";

        auto resultHexStr = AZStd::MemoryToASCII::ToString(serializer.GetBuffer(), serializer.GetSize(), serializer.GetSize(), 512, AZStd::MemoryToASCII::Options::Binary);
        AZ::StringFunc::TrimWhiteSpace(resultHexStr, true, true);
        auto expectedHexStr = AZStd::MemoryToASCII::ToString(expectedData, expectedSize,expectedSize, 512, AZStd::MemoryToASCII::Options::Binary);
        AZ::StringFunc::TrimWhiteSpace(expectedHexStr, true, true);

        EXPECT_TRUE(memcmp(serializer.GetBuffer(), expectedData, serializer.GetSize())==0) << "Resulting bytes for " << label << " do not match expected (" << resultHexStr.c_str() << " != " << expectedHexStr.c_str() << ")";
    }

    TEST_F(InputOutputSerializerTests, TestFixedSerialization)
    {
        {
            bool testValue = true;
            unsigned char expected[] = { 0x01 };
            InternalTestSerializeType(testValue, "bool(true)", expected, sizeof(expected));
        }
        {
            bool testValue = false;
            unsigned char expected[] = { 0x00 };
            InternalTestSerializeType(testValue, "boolfalse)", expected, sizeof(expected));
        }
        {
            int8_t testValue = 0x12;
            unsigned char expected[] = { 0x92 };
            InternalTestSerializeType(testValue, "int8", expected, sizeof(expected));
        }
        {
            int16_t testValue = 0x1234;
            unsigned char expected[] = { 0x92, 0x34 };
            InternalTestSerializeType(testValue, "int16", expected, sizeof(expected));
        }
        {
            int32_t testValue = 0x12345678;
            unsigned char expected[] = { 0x92, 0x34, 0x56, 0x78 };
            InternalTestSerializeType(testValue, "int32", expected, sizeof(expected));
        }
        {
            int64_t testValue = 0x123456789abcdef;
            unsigned char expected[] = { 0x81, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };
            InternalTestSerializeType(testValue, "int64", expected, sizeof(expected));
        }
        {
            uint8_t testValue = 0x12;
            unsigned char expected[] = { 0x12 };
            InternalTestSerializeType(testValue, "uint8", expected, sizeof(expected));
        }
        {
            uint16_t testValue = 0x1234;
            unsigned char expected[] = { 0x12, 0x34 };
            InternalTestSerializeType(testValue, "uint16", expected, sizeof(expected));
        }
        {
            uint32_t testValue = 0x12345678;
            unsigned char expected[] = { 0x12, 0x34, 0x56, 0x78 };
            InternalTestSerializeType(testValue, "uint32", expected, sizeof(expected));
        }
        {
            uint64_t testValue = 0x123456789abcdef;
            unsigned char expected[] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };
            InternalTestSerializeType(testValue, "uint64", expected, sizeof(expected));
        }
        {
            double testValue = 1.0;
            unsigned char expected[] = { 0x3f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            InternalTestSerializeType(testValue, "double", expected, sizeof(expected));
        }
        {
            float testValue = 1.;
            unsigned char expected[] = { 0x3f, 0x80, 0x00, 0x00 };
            InternalTestSerializeType(testValue, "float", expected, sizeof(expected));
        }
        {
            AZStd::fixed_string<32> testValue = "Fixed";
            unsigned char expected[] = { 0x05, 0x05, 0x46, 0x69, 0x78, 0x65, 0x64 };
            InternalTestSerializeType(testValue, "string", expected, sizeof(expected));
        }

    }


} // namespace UnitTest
