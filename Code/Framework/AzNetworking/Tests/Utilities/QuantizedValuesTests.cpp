/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/Utilities/QuantizedValues.h>
#include <AzNetworking/Serialization/NetworkInputSerializer.h>
#include <AzNetworking/Serialization/NetworkOutputSerializer.h>
#include <AzCore/std/limits.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    template <uint32_t NUM_ELEMENTS>
    struct ValueFromFloat {};

    template <>
    struct ValueFromFloat<1>
    {
        using ValueType = float;
        static float Construct(float value)
        {
            return value;
        }

        static void CheckEqual(ValueType a, ValueType b)
        {
            EXPECT_FLOAT_EQ(a, b);
        }
    };

    template <>
    struct ValueFromFloat<2>
    {
        using ValueType = AZ::Vector2;
        static AZ::Vector2 Construct(float value)
        {
            return AZ::Vector2(value, value);
        }

        static void CheckEqual(ValueType a, ValueType b)
        {
            EXPECT_TRUE(a.IsClose(b, 0.01f));
        }
    };

    template <>
    struct ValueFromFloat<3>
    {
        using ValueType = AZ::Vector3;
        static AZ::Vector3 Construct(float value)
        {
            return AZ::Vector3(value, value, value);
        }

        static void CheckEqual(ValueType a, ValueType b)
        {
            EXPECT_TRUE(a.IsClose(b, 0.01f));
        }
    };

    template <>
    struct ValueFromFloat<4>
    {
        using ValueType = AZ::Quaternion;
        static AZ::Quaternion Construct(float value)
        {
            return AZ::Quaternion(value, value, value, value);
        }

        static void CheckEqual(ValueType a, ValueType b)
        {
            EXPECT_TRUE(a.IsClose(b, 0.01f));
        }
    };

    template <uint32_t NUM_ELEMENTS, uint32_t NUM_BYTES>
    void TestQuantizedValuesHelper01()
    {
        AzNetworking::QuantizedValues<NUM_ELEMENTS, NUM_BYTES, 0, 1> testIn(ValueFromFloat<NUM_ELEMENTS>::Construct(0.0f)), testOut; // Transmits float values between 0 and 1 using NUM_BYTES

        AZStd::array<uint8_t, 1024> buffer;
        AzNetworking::NetworkInputSerializer  inputSerializer(buffer.data(), static_cast<uint32_t>(buffer.size()));
        AzNetworking::NetworkOutputSerializer outputSerializer(buffer.data(), static_cast<uint32_t>(buffer.size()));

        EXPECT_EQ(static_cast<typename ValueFromFloat<NUM_ELEMENTS>::ValueType>(testIn), ValueFromFloat<NUM_ELEMENTS>::Construct(0.0f));
        testIn.Serialize(inputSerializer);
        EXPECT_EQ(inputSerializer.GetSize(), NUM_BYTES * NUM_ELEMENTS);
        testOut.Serialize(outputSerializer);
        EXPECT_EQ(testIn, testOut);

        testIn = ValueFromFloat<NUM_ELEMENTS>::Construct(1.0f);
        EXPECT_EQ(static_cast<typename ValueFromFloat<NUM_ELEMENTS>::ValueType>(testIn), ValueFromFloat<NUM_ELEMENTS>::Construct(1.0f));
        testIn.Serialize(inputSerializer);
        EXPECT_NE(testIn, testOut);
        EXPECT_NE(testIn.GetQuantizedIntegralValues()[0], testOut.GetQuantizedIntegralValues()[0]);
        testOut.Serialize(outputSerializer);
        EXPECT_EQ(testIn, testOut);

        testIn = ValueFromFloat<NUM_ELEMENTS>::Construct(0.5f);
        EXPECT_EQ(static_cast<typename ValueFromFloat<NUM_ELEMENTS>::ValueType>(testIn), ValueFromFloat<NUM_ELEMENTS>::Construct(0.5f));
        testIn.Serialize(inputSerializer);
        testOut.Serialize(outputSerializer);
        EXPECT_EQ(testIn, testOut);

        testIn = ValueFromFloat<NUM_ELEMENTS>::Construct(-1.0f);
        EXPECT_NE(static_cast<typename ValueFromFloat<NUM_ELEMENTS>::ValueType>(testIn), ValueFromFloat<NUM_ELEMENTS>::Construct(-1.0f));
        EXPECT_EQ(static_cast<typename ValueFromFloat<NUM_ELEMENTS>::ValueType>(testIn), ValueFromFloat<NUM_ELEMENTS>::Construct(0.0f));
        testIn.Serialize(inputSerializer);
        testOut.Serialize(outputSerializer);
        EXPECT_EQ(testIn, testOut);

        testIn = ValueFromFloat<NUM_ELEMENTS>::Construct(2.0f);
        EXPECT_NE(static_cast<typename ValueFromFloat<NUM_ELEMENTS>::ValueType>(testIn), ValueFromFloat<NUM_ELEMENTS>::Construct(2.0f));
        EXPECT_EQ(static_cast<typename ValueFromFloat<NUM_ELEMENTS>::ValueType>(testIn), ValueFromFloat<NUM_ELEMENTS>::Construct(1.0f));
        testIn.Serialize(inputSerializer);
        testOut.Serialize(outputSerializer);
        EXPECT_EQ(testIn, testOut);
    }

    template <uint32_t NUM_ELEMENTS, uint32_t NUM_BYTES>
    void TestQuantizedValuesHelper11()
    {
        AzNetworking::QuantizedValues<NUM_ELEMENTS, NUM_BYTES, -1, 1> testIn, testOut; // Transmits float values between -1.0 and 1.0 using NUM_BYTES

        AZStd::array<uint8_t, 1024> buffer;
        AzNetworking::NetworkInputSerializer  inputSerializer(buffer.data(), static_cast<uint32_t>(buffer.size()));
        AzNetworking::NetworkOutputSerializer outputSerializer(buffer.data(), static_cast<uint32_t>(buffer.size()));

        testIn = ValueFromFloat<NUM_ELEMENTS>::Construct(-1.0f);
        ValueFromFloat<NUM_ELEMENTS>::CheckEqual(static_cast<typename ValueFromFloat<NUM_ELEMENTS>::ValueType>(testIn), ValueFromFloat<NUM_ELEMENTS>::Construct(-1.0f));
        testIn.Serialize(inputSerializer);
        EXPECT_EQ(inputSerializer.GetSize(), NUM_BYTES * NUM_ELEMENTS);
        testOut.Serialize(outputSerializer);
        EXPECT_EQ(testIn, testOut);

        testIn = ValueFromFloat<NUM_ELEMENTS>::Construct(1.0f);
        ValueFromFloat<NUM_ELEMENTS>::CheckEqual(static_cast<typename ValueFromFloat<NUM_ELEMENTS>::ValueType>(testIn), ValueFromFloat<NUM_ELEMENTS>::Construct(1.0f));
        testIn.Serialize(inputSerializer);
        testOut.Serialize(outputSerializer);
        EXPECT_EQ(testIn, testOut);

        testIn = ValueFromFloat<NUM_ELEMENTS>::Construct(0.0f);
        ValueFromFloat<NUM_ELEMENTS>::CheckEqual(static_cast<typename ValueFromFloat<NUM_ELEMENTS>::ValueType>(testIn), ValueFromFloat<NUM_ELEMENTS>::Construct(0.0f));
        testIn.Serialize(inputSerializer);
        testOut.Serialize(outputSerializer);
        EXPECT_EQ(testIn, testOut);

        testIn = ValueFromFloat<NUM_ELEMENTS>::Construct(-2.0f);
        EXPECT_NE(static_cast<typename ValueFromFloat<NUM_ELEMENTS>::ValueType>(testIn), ValueFromFloat<NUM_ELEMENTS>::Construct(-2.0f));
        ValueFromFloat<NUM_ELEMENTS>::CheckEqual(static_cast<typename ValueFromFloat<NUM_ELEMENTS>::ValueType>(testIn), ValueFromFloat<NUM_ELEMENTS>::Construct(-1.0f));
        testIn.Serialize(inputSerializer);
        testOut.Serialize(outputSerializer);
        EXPECT_EQ(testIn, testOut);

        testIn = ValueFromFloat<NUM_ELEMENTS>::Construct(2.0f);
        EXPECT_NE(static_cast<typename ValueFromFloat<NUM_ELEMENTS>::ValueType>(testIn), ValueFromFloat<NUM_ELEMENTS>::Construct(2.0f));
        ValueFromFloat<NUM_ELEMENTS>::CheckEqual(static_cast<typename ValueFromFloat<NUM_ELEMENTS>::ValueType>(testIn), ValueFromFloat<NUM_ELEMENTS>::Construct(1.0f));
        testIn.Serialize(inputSerializer);
        testOut.Serialize(outputSerializer);
        EXPECT_EQ(testIn, testOut);
    }

    template <uint32_t NUM_ELEMENTS, uint32_t NUM_BYTES>
    void TestQuantizedValuesHelper16k()
    {
        AzNetworking::QuantizedValues<NUM_ELEMENTS, NUM_BYTES, -16384, 16384> testIn, testOut; // Transmits float values between -1.0 and 1.0 using NUM_BYTES

        AZStd::array<uint8_t, 1024> buffer;
        AzNetworking::NetworkInputSerializer  inputSerializer(buffer.data(), static_cast<uint32_t>(buffer.size()));
        AzNetworking::NetworkOutputSerializer outputSerializer(buffer.data(), static_cast<uint32_t>(buffer.size()));

        testIn = ValueFromFloat<NUM_ELEMENTS>::Construct(-2000.0f);
        ValueFromFloat<NUM_ELEMENTS>::CheckEqual(static_cast<typename ValueFromFloat<NUM_ELEMENTS>::ValueType>(testIn), ValueFromFloat<NUM_ELEMENTS>::Construct(-2000.0f));
        testIn.Serialize(inputSerializer);
        EXPECT_EQ(inputSerializer.GetSize(), NUM_BYTES * NUM_ELEMENTS);
        testOut.Serialize(outputSerializer);
        EXPECT_EQ(testIn, testOut);

        testIn = ValueFromFloat<NUM_ELEMENTS>::Construct(2000.0f);
        ValueFromFloat<NUM_ELEMENTS>::CheckEqual(static_cast<typename ValueFromFloat<NUM_ELEMENTS>::ValueType>(testIn), ValueFromFloat<NUM_ELEMENTS>::Construct(2000.0f));
        testIn.Serialize(inputSerializer);
        testOut.Serialize(outputSerializer);
        EXPECT_EQ(testIn, testOut);

        testIn = ValueFromFloat<NUM_ELEMENTS>::Construct(0.0f);
        ValueFromFloat<NUM_ELEMENTS>::CheckEqual(static_cast<typename ValueFromFloat<NUM_ELEMENTS>::ValueType>(testIn), ValueFromFloat<NUM_ELEMENTS>::Construct(0.0f));
        testIn.Serialize(inputSerializer);
        testOut.Serialize(outputSerializer);
        EXPECT_EQ(testIn, testOut);

        testIn = ValueFromFloat<NUM_ELEMENTS>::Construct(-16385.0f);
        EXPECT_NE(static_cast<typename ValueFromFloat<NUM_ELEMENTS>::ValueType>(testIn), ValueFromFloat<NUM_ELEMENTS>::Construct(-16385.0f));
        ValueFromFloat<NUM_ELEMENTS>::CheckEqual(static_cast<typename ValueFromFloat<NUM_ELEMENTS>::ValueType>(testIn), ValueFromFloat<NUM_ELEMENTS>::Construct(-16384.0f));
        testIn.Serialize(inputSerializer);
        testOut.Serialize(outputSerializer);
        EXPECT_EQ(testIn, testOut);

        testIn = ValueFromFloat<NUM_ELEMENTS>::Construct(16385.0f);
        EXPECT_NE(static_cast<typename ValueFromFloat<NUM_ELEMENTS>::ValueType>(testIn), ValueFromFloat<NUM_ELEMENTS>::Construct(16385.0f));
        EXPECT_EQ(static_cast<typename ValueFromFloat<NUM_ELEMENTS>::ValueType>(testIn), ValueFromFloat<NUM_ELEMENTS>::Construct(16384.0f));
        testIn.Serialize(inputSerializer);
        testOut.Serialize(outputSerializer);
        EXPECT_EQ(testIn, testOut);
    }

    template <uint32_t NUM_ELEMENTS, uint32_t NUM_BYTES>
    void TestQuantizedValuesHelper24bitRange()
    {
        // This gives us a hash sensitivity of around 1/128th of a unit, and will detect errors within a range of -16,777,216 to +16,777,216
        static constexpr int32_t FloatHashMinValue = (AZStd::numeric_limits<int>::min() >> 7);
        static constexpr int32_t FloatHashMaxValue = (AZStd::numeric_limits<int>::max() >> 7);

        AzNetworking::QuantizedValues<NUM_ELEMENTS, NUM_BYTES, FloatHashMinValue, FloatHashMaxValue> testIn, testOut;

        AZStd::array<uint8_t, 1024> buffer;
        AzNetworking::NetworkInputSerializer  inputSerializer(buffer.data(), static_cast<uint32_t>(buffer.size()));
        AzNetworking::NetworkOutputSerializer outputSerializer(buffer.data(), static_cast<uint32_t>(buffer.size()));

        testIn = ValueFromFloat<NUM_ELEMENTS>::Construct(-20000.0f);
        ValueFromFloat<NUM_ELEMENTS>::CheckEqual(static_cast<typename ValueFromFloat<NUM_ELEMENTS>::ValueType>(testIn), ValueFromFloat<NUM_ELEMENTS>::Construct(-20000.0f));
        testIn.Serialize(inputSerializer);
        EXPECT_EQ(inputSerializer.GetSize(), NUM_BYTES * NUM_ELEMENTS);
        testOut.Serialize(outputSerializer);
        EXPECT_EQ(testIn, testOut);

        testIn = ValueFromFloat<NUM_ELEMENTS>::Construct(20000.0f);
        ValueFromFloat<NUM_ELEMENTS>::CheckEqual(static_cast<typename ValueFromFloat<NUM_ELEMENTS>::ValueType>(testIn), ValueFromFloat<NUM_ELEMENTS>::Construct(20000.0f));
        testIn.Serialize(inputSerializer);
        testOut.Serialize(outputSerializer);
        EXPECT_EQ(testIn, testOut);

        testIn = ValueFromFloat<NUM_ELEMENTS>::Construct(0.0f);
        ValueFromFloat<NUM_ELEMENTS>::CheckEqual(static_cast<typename ValueFromFloat<NUM_ELEMENTS>::ValueType>(testIn), ValueFromFloat<NUM_ELEMENTS>::Construct(0.0f));
        testIn.Serialize(inputSerializer);
        testOut.Serialize(outputSerializer);
        EXPECT_EQ(testIn, testOut);

        testIn = ValueFromFloat<NUM_ELEMENTS>::Construct(-163850.0f);
        ValueFromFloat<NUM_ELEMENTS>::CheckEqual(static_cast<typename ValueFromFloat<NUM_ELEMENTS>::ValueType>(testIn), ValueFromFloat<NUM_ELEMENTS>::Construct(-163850.0f));
        testIn.Serialize(inputSerializer);
        testOut.Serialize(outputSerializer);
        EXPECT_EQ(testIn, testOut);

        testIn = ValueFromFloat<NUM_ELEMENTS>::Construct(163850.0f);
        ValueFromFloat<NUM_ELEMENTS>::CheckEqual(static_cast<typename ValueFromFloat<NUM_ELEMENTS>::ValueType>(testIn), ValueFromFloat<NUM_ELEMENTS>::Construct(163850.0f));
        testIn.Serialize(inputSerializer);
        testOut.Serialize(outputSerializer);
        EXPECT_EQ(testIn, testOut);
    }

    TEST(QuantizedValues, Test1Elements1Bytes)
    {
        TestQuantizedValuesHelper01<1, 1>();
        TestQuantizedValuesHelper11<1, 1>();
    }

    TEST(QuantizedValues, Test2Elements1Bytes)
    {
        TestQuantizedValuesHelper01<2, 1>();
        TestQuantizedValuesHelper11<2, 1>();
    }

    TEST(QuantizedValues, Test3Elements1Bytes)
    {
        TestQuantizedValuesHelper01<3, 1>();
        TestQuantizedValuesHelper11<3, 1>();
    }

    TEST(QuantizedValues, Test4Elements1Bytes)
    {
        TestQuantizedValuesHelper01<4, 1>();
        TestQuantizedValuesHelper11<4, 1>();
    }

    TEST(QuantizedValues, Test1Elements2Bytes)
    {
        TestQuantizedValuesHelper01<1, 2>();
        TestQuantizedValuesHelper11<1, 2>();
    }

    TEST(QuantizedValues, Test2Elements2Bytes)
    {
        TestQuantizedValuesHelper01<2, 2>();
        TestQuantizedValuesHelper11<2, 2>();
    }

    TEST(QuantizedValues, Test3Elements2Bytes)
    {
        TestQuantizedValuesHelper01<3, 2>();
        TestQuantizedValuesHelper11<3, 2>();
    }

    TEST(QuantizedValues, Test4Elements2Bytes)
    {
        TestQuantizedValuesHelper01<4, 2>();
        TestQuantizedValuesHelper11<4, 2>();
    }

    TEST(QuantizedValues, Test1Elements3Bytes)
    {
        TestQuantizedValuesHelper01 <1, 3>();
        TestQuantizedValuesHelper11 <1, 3>();
        TestQuantizedValuesHelper16k<1, 3>();
    }

    TEST(QuantizedValues, Test2Elements3Bytes)
    {
        TestQuantizedValuesHelper01 <2, 3>();
        TestQuantizedValuesHelper11 <2, 3>();
        TestQuantizedValuesHelper16k<2, 3>();
    }

    TEST(QuantizedValues, Test3Elements3Bytes)
    {
        TestQuantizedValuesHelper01 <3, 3>();
        TestQuantizedValuesHelper11 <3, 3>();
        TestQuantizedValuesHelper16k<3, 3>();
    }

    TEST(QuantizedValues, Test4Elements3Bytes)
    {
        TestQuantizedValuesHelper01 <4, 3>();
        TestQuantizedValuesHelper11 <4, 3>();
        TestQuantizedValuesHelper16k<4, 3>();
    }

    TEST(QuantizedValues, Test1Elements4Bytes)
    {
        TestQuantizedValuesHelper01 <1, 4>();
        TestQuantizedValuesHelper11 <1, 4>();
        TestQuantizedValuesHelper16k<1, 4>();
    }

    TEST(QuantizedValues, Test2Elements4Bytes)
    {
        TestQuantizedValuesHelper01 <2, 4>();
        TestQuantizedValuesHelper11 <2, 4>();
        TestQuantizedValuesHelper16k<2, 4>();
        TestQuantizedValuesHelper24bitRange<2, 4>();
    }

    TEST(QuantizedValues, Test3Elements4Bytes)
    {
        TestQuantizedValuesHelper01 <3, 4>();
        TestQuantizedValuesHelper11 <3, 4>();
        TestQuantizedValuesHelper16k<3, 4>();
        TestQuantizedValuesHelper24bitRange<3, 4>();
    }

    TEST(QuantizedValues, Test4Elements4Bytes)
    {
        TestQuantizedValuesHelper01 <4, 4>();
        TestQuantizedValuesHelper11 <4, 4>();
        TestQuantizedValuesHelper16k<4, 4>();
        TestQuantizedValuesHelper24bitRange<4, 4>();
    }
}
