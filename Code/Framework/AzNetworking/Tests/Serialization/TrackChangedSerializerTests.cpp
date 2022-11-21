/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/Serialization/NetworkInputSerializer.h>
#include <AzNetworking/Serialization/NetworkOutputSerializer.h>
#include <AzNetworking/Serialization/TrackChangedSerializer.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    struct TrackChangedSerializerInElement
    {
        bool testBool = false;
        char testChar = 'a';
        int8_t testInt8 = 0;
        int16_t testInt16 = 0;
        int32_t testInt32 = 0;
        int64_t testInt64 = 0;
        uint8_t testUint8 = 0;
        uint16_t testUint16 = 0;
        uint32_t testUint32 = 0;
        uint64_t testUint64 = 0;
        double testDouble = 0.0;
        float testFloat = 0.f;
        AZStd::fixed_string<32> testFixedString = "";

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

    struct TrackChangedSerializerOutElement
    {
        bool testBool = true;
        char testChar = 'b';
        int8_t testInt8 = 1;
        int16_t testInt16 = 1;
        int32_t testInt32 = 1;
        int64_t testInt64 = 1;
        uint8_t testUint8 = 1;
        uint16_t testUint16 = 1;
        uint32_t testUint32 = 1;
        uint64_t testUint64 = 1;
        double testDouble = 1.0;
        float testFloat = 1.f;
        AZStd::fixed_string<32> testFixedString = "TestFixedString";

        bool SerializeFixedString(AzNetworking::ISerializer& serializer)
        {
            return serializer.Serialize(testFixedString, "TestFixedString");
        }
    };

    class TrackChangedSerializerTests : public AllocatorsFixture
    {
    public:
        void SetUp() override
        {
            SetupAllocator();
        }
    };

    TEST_F(TrackChangedSerializerTests, TestTrackChangedSerializer)
    {
        const size_t Capacity = 2048;
        const int8_t ExpectedSerializedBytes = 50;
        AZStd::array<uint8_t, Capacity> buffer;

        TrackChangedSerializerInElement inElement;
        AzNetworking::NetworkInputSerializer inSerializer(buffer.data(), static_cast<uint32_t>(buffer.size()));

        EXPECT_TRUE(inElement.Serialize(inSerializer));

        TrackChangedSerializerOutElement outElement;
        AzNetworking::TrackChangedSerializer<AzNetworking::NetworkOutputSerializer> trackChangedSerializer(buffer.data(), static_cast<uint32_t>(buffer.size()));

        EXPECT_EQ(trackChangedSerializer.GetSerializerMode(), AzNetworking::SerializerMode::WriteToObject);

        trackChangedSerializer.ClearTrackedChangesFlag();
        trackChangedSerializer.Serialize(outElement.testBool, "TestBool");
        EXPECT_TRUE(trackChangedSerializer.GetTrackedChangesFlag());

        trackChangedSerializer.ClearTrackedChangesFlag();
        trackChangedSerializer.Serialize(outElement.testChar, "TestChar", AZStd::numeric_limits<char>::min(), AZStd::numeric_limits<char>::max());
        EXPECT_TRUE(trackChangedSerializer.GetTrackedChangesFlag());

        trackChangedSerializer.ClearTrackedChangesFlag();
        trackChangedSerializer.Serialize(outElement.testInt8, "TestInt8", AZStd::numeric_limits<int8_t>::min(), AZStd::numeric_limits<int8_t>::max());
        EXPECT_TRUE(trackChangedSerializer.GetTrackedChangesFlag());

        trackChangedSerializer.ClearTrackedChangesFlag();
        trackChangedSerializer.Serialize(outElement.testInt16, "TestInt16", AZStd::numeric_limits<int16_t>::min(), AZStd::numeric_limits<int16_t>::max());
        EXPECT_TRUE(trackChangedSerializer.GetTrackedChangesFlag());

        trackChangedSerializer.ClearTrackedChangesFlag();
        trackChangedSerializer.Serialize(outElement.testInt32, "TestInt32", AZStd::numeric_limits<int32_t>::min(), AZStd::numeric_limits<int32_t>::max());
        EXPECT_TRUE(trackChangedSerializer.GetTrackedChangesFlag());

        trackChangedSerializer.ClearTrackedChangesFlag();
        trackChangedSerializer.Serialize(outElement.testInt64, "TestInt64", AZStd::numeric_limits<int64_t>::min(), AZStd::numeric_limits<int64_t>::max());
        EXPECT_TRUE(trackChangedSerializer.GetTrackedChangesFlag());

        trackChangedSerializer.ClearTrackedChangesFlag();
        trackChangedSerializer.Serialize(outElement.testUint8, "TestUint8", AZStd::numeric_limits<uint8_t>::min(), AZStd::numeric_limits<uint8_t>::max());
        EXPECT_TRUE(trackChangedSerializer.GetTrackedChangesFlag());

        trackChangedSerializer.ClearTrackedChangesFlag();
        trackChangedSerializer.Serialize(outElement.testUint16, "TestUint16", AZStd::numeric_limits<uint16_t>::min(), AZStd::numeric_limits<uint16_t>::max());
        EXPECT_TRUE(trackChangedSerializer.GetTrackedChangesFlag());

        trackChangedSerializer.ClearTrackedChangesFlag();
        trackChangedSerializer.Serialize(outElement.testUint32, "TestUint32", AZStd::numeric_limits<uint32_t>::min(), AZStd::numeric_limits<uint32_t>::max());
        EXPECT_TRUE(trackChangedSerializer.GetTrackedChangesFlag());

        trackChangedSerializer.ClearTrackedChangesFlag();
        trackChangedSerializer.Serialize(outElement.testUint64, "TestUint64", AZStd::numeric_limits<uint64_t>::min(), AZStd::numeric_limits<uint64_t>::max());
        EXPECT_TRUE(trackChangedSerializer.GetTrackedChangesFlag());

        trackChangedSerializer.ClearTrackedChangesFlag();
        trackChangedSerializer.Serialize(outElement.testDouble, "TestDouble", AZStd::numeric_limits<double>::min(), AZStd::numeric_limits<double>::max());
        EXPECT_TRUE(trackChangedSerializer.GetTrackedChangesFlag());

        trackChangedSerializer.ClearTrackedChangesFlag();
        trackChangedSerializer.Serialize(outElement.testFloat, "TestFloat", AZStd::numeric_limits<float>::min(), AZStd::numeric_limits<float>::max());
        EXPECT_TRUE(trackChangedSerializer.GetTrackedChangesFlag());

        trackChangedSerializer.ClearTrackedChangesFlag();
        outElement.SerializeFixedString(trackChangedSerializer);
        EXPECT_TRUE(trackChangedSerializer.GetTrackedChangesFlag());

        EXPECT_NE(trackChangedSerializer.GetBuffer(), nullptr);
        EXPECT_EQ(trackChangedSerializer.GetCapacity(), Capacity);
        EXPECT_EQ(trackChangedSerializer.GetSize(), ExpectedSerializedBytes);
    }
}
