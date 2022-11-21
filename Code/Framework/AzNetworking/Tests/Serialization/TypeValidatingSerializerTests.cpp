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
#include <AzNetworking/Serialization/TypeValidatingSerializer.h>

#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    struct TypeValidatingDataElement
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

    class TypeValidatingSerializerTests : public AllocatorsFixture
    {
    public:
        void SetUp() override
        {
            SetupAllocator();
            m_console = AZStd::make_unique<AZ::Console>();
            m_console->LinkDeferredFunctors(AZ::ConsoleFunctorBase::GetDeferredHead());
            m_console->PerformCommand("net_validateSerializedTypes true");
            AZ::Interface<AZ::IConsole>::Register(m_console.get());
        }

        void TearDown() override
        {
            AZ::Interface<AZ::IConsole>::Unregister(m_console.get());
            m_console = nullptr;
        }

        AZStd::unique_ptr<AZ::Console> m_console;
    };

    TEST_F(TypeValidatingSerializerTests, TestTypeValidatingSerializer)
    {
        const int16_t ExpectedSerializedBytes = 274;
        const size_t Capacity = 2048;
        AZStd::array<uint8_t, Capacity> buffer;

        TypeValidatingDataElement inElement;
        AzNetworking::TypeValidatingSerializer<AzNetworking::NetworkInputSerializer> inSerializer(buffer.data(), static_cast<uint32_t>(buffer.size()));

        EXPECT_NE(inSerializer.GetBuffer(), nullptr);
        EXPECT_EQ(inSerializer.GetCapacity(), Capacity);
        EXPECT_TRUE(inElement.Serialize(inSerializer)); 
        EXPECT_EQ(inSerializer.GetSize(), ExpectedSerializedBytes);

        TypeValidatingDataElement outElement;
        AzNetworking::TypeValidatingSerializer<AzNetworking::NetworkOutputSerializer> outSerializer(buffer.data(), static_cast<uint32_t>(buffer.size()));

        EXPECT_TRUE(outElement.Serialize(outSerializer));

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

    TEST_F(TypeValidatingSerializerTests, TestTypeValidatingNameMismatch)
    {
        TypeValidatingDataElement inElement;
        AZStd::array<uint8_t, 2048> buffer;
        AzNetworking::TypeValidatingSerializer<AzNetworking::NetworkInputSerializer> inSerializer(buffer.data(), static_cast<uint32_t>(buffer.size()));

        EXPECT_TRUE(inElement.Serialize(inSerializer));

        TypeValidatingDataElement outElement;
        AzNetworking::TypeValidatingSerializer<AzNetworking::NetworkOutputSerializer> outSerializer(buffer.data(), static_cast<uint32_t>(buffer.size()));

        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(outSerializer.Serialize(outElement.testBool, "NotTestBool"));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(TypeValidatingSerializerTests, TestTypeValidatingTypeMismatch)
    {
        TypeValidatingDataElement inElement;
        AZStd::array<uint8_t, 2048> buffer;
        AzNetworking::TypeValidatingSerializer<AzNetworking::NetworkInputSerializer> inSerializer(
            buffer.data(), static_cast<uint32_t>(buffer.size()));

        EXPECT_TRUE(inElement.Serialize(inSerializer));

        TypeValidatingDataElement outElement;
        AzNetworking::TypeValidatingSerializer<AzNetworking::NetworkOutputSerializer> outSerializer(
            buffer.data(), static_cast<uint32_t>(buffer.size()));

        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(outSerializer.Serialize(outElement.testUint8, "TestBool", 0, 32));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }
} // namespace UnitTest
