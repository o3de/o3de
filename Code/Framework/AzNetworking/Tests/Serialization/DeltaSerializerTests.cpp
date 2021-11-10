/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/Serialization/DeltaSerializer.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    struct DeltaDataElement
    {
        AzNetworking::PacketId m_packetId = AzNetworking::InvalidPacketId;
        uint32_t m_id = 0;
        AZ::TimeMs m_timeMs = AZ::TimeMs{ 0 };
        float m_blendFactor = 0.f;
        AZStd::vector<int> m_growVector, m_shrinkVector;

        bool Serialize(AzNetworking::ISerializer& serializer)
        {
            if (!serializer.Serialize(m_packetId, "PacketId")
             || !serializer.Serialize(m_id, "Id")
             || !serializer.Serialize(m_timeMs, "TimeMs")
             || !serializer.Serialize(m_blendFactor, "BlendFactor")
             || !serializer.Serialize(m_growVector, "GrowVector")
             || !serializer.Serialize(m_shrinkVector, "ShrinkVector"))
            {
                return false;
            }

            return true;
        }
    };

    struct DeltaDataContainer
    {
        AZStd::string m_containerName;
        AZStd::array<DeltaDataElement, 32> m_container;

        // This logic is modeled after NetworkInputArray serialization in the Multiplayer Gem
        bool Serialize(AzNetworking::ISerializer& serializer)
        {
            // Always serialize the full first element
            if(!m_container[0].Serialize(serializer))
            {
                return false;
            }

            for (uint32_t i = 1; i < m_container.size(); ++i)
            {
                if (serializer.GetSerializerMode() == AzNetworking::SerializerMode::WriteToObject)
                {
                    AzNetworking::SerializerDelta deltaSerializer;
                    // Read out the delta
                    if (!deltaSerializer.Serialize(serializer))
                    {
                        return false;
                    }

                    // Start with previous value
                    m_container[i] = m_container[i - 1];
                    // Then apply delta
                    AzNetworking::DeltaSerializerApply applySerializer(deltaSerializer);
                    if (!applySerializer.ApplyDelta(m_container[i]))
                    {
                        return false;
                    }
                }
                else
                {
                    AzNetworking::SerializerDelta deltaSerializer;
                    // Create the delta
                    AzNetworking::DeltaSerializerCreate createSerializer(deltaSerializer);
                    if (!createSerializer.CreateDelta(m_container[i - 1], m_container[i]))
                    {
                        return false;
                    }

                    // Then write out the delta
                    if (!deltaSerializer.Serialize(serializer))
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        // This logic is modeled after NetworkInputArray serialization in the Multiplayer Gem
        bool SerializeNoDelta(AzNetworking::ISerializer& serializer)
        {
            for (uint32_t i = 0; i < m_container.size(); ++i)
            {
                if(!m_container[i].Serialize(serializer))
                {
                    return false;
                }
            }

            return true;
        }
    };

    class DeltaSerializerTests
        : public UnitTest::AllocatorsTestFixture
    {
    public:
        void SetUp() override
        {
            UnitTest::AllocatorsTestFixture::SetUp();
        }

        void TearDown() override
        {
            UnitTest::AllocatorsTestFixture::TearDown();
        }
    };

    static constexpr float BLEND_FACTOR_SCALE = 1.1f;
    static constexpr uint32_t TIME_SCALE = 10;

    DeltaDataContainer TestDeltaContainer()
    {
        DeltaDataContainer testContainer;
        AZStd::vector<int> growVector, shrinkVector;
        shrinkVector.resize(testContainer.m_container.array_size);

        testContainer.m_containerName = "TestContainer";
        for (int i = 0; i < testContainer.m_container.array_size; ++i)
        {
            testContainer.m_container[i].m_packetId = AzNetworking::PacketId(i);
            testContainer.m_container[i].m_id = i;
            testContainer.m_container[i].m_timeMs = AZ::TimeMs(i * TIME_SCALE);
            testContainer.m_container[i].m_blendFactor = BLEND_FACTOR_SCALE * i;
            growVector.push_back(i);
            testContainer.m_container[i].m_growVector = growVector;
            shrinkVector.resize(testContainer.m_container.array_size - i);
            testContainer.m_container[i].m_shrinkVector = shrinkVector;
        }

        return testContainer;
    }

    TEST_F(DeltaSerializerTests, DeltaArray)
    {
        DeltaDataContainer inContainer = TestDeltaContainer();
        AZStd::array<uint8_t, 2048> buffer;
        AzNetworking::NetworkInputSerializer inSerializer(buffer.data(), static_cast<uint32_t>(buffer.size()));

        // Always serialize the full first element
        EXPECT_TRUE(inContainer.Serialize(inSerializer));

        DeltaDataContainer outContainer;
        AzNetworking::NetworkOutputSerializer outSerializer(buffer.data(), static_cast<uint32_t>(buffer.size()));

        EXPECT_TRUE(outContainer.Serialize(outSerializer));

        for (uint32_t i = 0; i > outContainer.m_container.size(); ++i)
        {
            EXPECT_EQ(inContainer.m_container[i].m_blendFactor, outContainer.m_container[i].m_blendFactor);
            EXPECT_EQ(inContainer.m_container[i].m_id, outContainer.m_container[i].m_id);
            EXPECT_EQ(inContainer.m_container[i].m_packetId, outContainer.m_container[i].m_packetId);
            EXPECT_EQ(inContainer.m_container[i].m_timeMs, outContainer.m_container[i].m_timeMs);
            EXPECT_EQ(inContainer.m_container[i].m_growVector[i], outContainer.m_container[i].m_growVector[i]);
            EXPECT_EQ(inContainer.m_container[i].m_growVector.size(), outContainer.m_container[i].m_growVector.size());
            EXPECT_EQ(inContainer.m_container[i].m_shrinkVector.size(), outContainer.m_container[i].m_shrinkVector.size());
        }
    }

    TEST_F(DeltaSerializerTests, DeltaSerializerCreateUnused)
    {
        // Every function here should return a constant value regardless of inputs
        AzNetworking::SerializerDelta deltaSerializer;
        AzNetworking::DeltaSerializerCreate createSerializer(deltaSerializer);

        EXPECT_EQ(createSerializer.GetCapacity(), 0);
        EXPECT_EQ(createSerializer.GetSize(), 0);
        EXPECT_EQ(createSerializer.GetBuffer(), nullptr);
        EXPECT_EQ(createSerializer.GetSerializerMode(), AzNetworking::SerializerMode::ReadFromObject);

        createSerializer.ClearTrackedChangesFlag(); //NO-OP
        EXPECT_FALSE(createSerializer.GetTrackedChangesFlag());
        EXPECT_TRUE(createSerializer.BeginObject("CreateSerializer", "Begin"));
        EXPECT_TRUE(createSerializer.EndObject("CreateSerializer", "End"));
    }

    TEST_F(DeltaSerializerTests, DeltaArraySize)
    {
        DeltaDataContainer deltaContainer = TestDeltaContainer();
        DeltaDataContainer noDeltaContainer = TestDeltaContainer();

        AZStd::array<uint8_t, 2048> deltaBuffer;
        AzNetworking::NetworkInputSerializer deltaSerializer(deltaBuffer.data(), static_cast<uint32_t>(deltaBuffer.size()));
        AZStd::array<uint8_t, 2048> noDeltaBuffer;
        AzNetworking::NetworkInputSerializer noDeltaSerializer(noDeltaBuffer.data(), static_cast<uint32_t>(noDeltaBuffer.size()));

        EXPECT_TRUE(deltaContainer.Serialize(deltaSerializer));
        EXPECT_FALSE(noDeltaContainer.SerializeNoDelta(noDeltaSerializer)); // Should run out of space
        EXPECT_EQ(noDeltaSerializer.GetCapacity(), noDeltaSerializer.GetSize()); // Verify that the serializer filled up
        EXPECT_FALSE(noDeltaSerializer.IsValid()); // and that it is no longer valid due to lack of space
    }

    TEST_F(DeltaSerializerTests, DeltaSerializerApplyUnused)
    {
        // Every function here should return a constant value regardless of inputs
        AzNetworking::SerializerDelta deltaSerializer;
        AzNetworking::DeltaSerializerApply applySerializer(deltaSerializer);

        EXPECT_EQ(applySerializer.GetCapacity(), 0);
        EXPECT_EQ(applySerializer.GetSize(), 0);
        EXPECT_EQ(applySerializer.GetBuffer(), nullptr);
        EXPECT_EQ(applySerializer.GetSerializerMode(), AzNetworking::SerializerMode::WriteToObject);

        applySerializer.ClearTrackedChangesFlag(); //NO-OP
        EXPECT_FALSE(applySerializer.GetTrackedChangesFlag());
        EXPECT_TRUE(applySerializer.BeginObject("CreateSerializer", "Begin"));
        EXPECT_TRUE(applySerializer.EndObject("CreateSerializer", "End"));
    }
}
