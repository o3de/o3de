/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/Serialization/HashSerializer.h>
#include <AzNetworking/Utilities/QuantizedValues.h>

namespace AzNetworking
{
    // This gives us a hash sensitivity of around 1/512th of a unit, and will detect errors within a range of -4,194,304 to +4,194,304
    static const int32_t FloatHashMinValue = (INT_MIN >> 9);
    static const int32_t FloatHashMaxValue = (INT_MAX >> 9);

    HashSerializer::HashSerializer()
    {
        ;
    }

    HashSerializer::~HashSerializer()
    {
        ;
    }

    AZ::HashValue32 HashSerializer::GetHash() const
    {
        // Just truncate the upper bits
        const AZ::HashValue32 lower = static_cast<AZ::HashValue32>(m_hash);
        const AZ::HashValue32 upper = static_cast<AZ::HashValue32>(m_hash >> 32);
        return lower ^ upper;
    }

    SerializerMode HashSerializer::GetSerializerMode() const
    {
        return SerializerMode::ReadFromObject;
    }

    bool HashSerializer::Serialize(bool& value, const char*)
    {
        m_hash = AZ::TypeHash64(value, m_hash);
        return true;
    }

    bool HashSerializer::Serialize(char& value, [[maybe_unused]] const char* name, [[maybe_unused]] char minValue, [[maybe_unused]] char maxValue)
    {
        m_hash = AZ::TypeHash64(value, m_hash);
        return true;
    }

    bool HashSerializer::Serialize(int8_t& value, [[maybe_unused]] const char* name, [[maybe_unused]] int8_t minValue, [[maybe_unused]] int8_t maxValue)
    {
        m_hash = AZ::TypeHash64(value, m_hash);
        return true;
    }

    bool HashSerializer::Serialize(int16_t& value, [[maybe_unused]] const char* name, [[maybe_unused]] int16_t minValue, [[maybe_unused]] int16_t maxValue)
    {
        m_hash = AZ::TypeHash64(value, m_hash);
        return true;
    }

    bool HashSerializer::Serialize(int32_t& value, [[maybe_unused]] const char* name, [[maybe_unused]] int32_t minValue, [[maybe_unused]] int32_t maxValue)
    {
        m_hash = AZ::TypeHash64(value, m_hash);
        return true;
    }

    bool HashSerializer::Serialize(int64_t& value, [[maybe_unused]] const char* name, [[maybe_unused]] int64_t minValue, [[maybe_unused]] int64_t maxValue)
    {
        m_hash = AZ::TypeHash64(value, m_hash);
        return true;
    }

    bool HashSerializer::Serialize(uint8_t& value, [[maybe_unused]] const char* name, [[maybe_unused]] uint8_t minValue, [[maybe_unused]] uint8_t maxValue)
    {
        m_hash = AZ::TypeHash64(value, m_hash);
        return true;
    }

    bool HashSerializer::Serialize(uint16_t& value, [[maybe_unused]] const char* name, [[maybe_unused]] uint16_t minValue, [[maybe_unused]] uint16_t maxValue)
    {
        m_hash = AZ::TypeHash64(value, m_hash);
        return true;
    }

    bool HashSerializer::Serialize(uint32_t& value, [[maybe_unused]] const char* name, [[maybe_unused]] uint32_t minValue, [[maybe_unused]] uint32_t maxValue)
    {
        m_hash = AZ::TypeHash64(value, m_hash);
        return true;
    }

    bool HashSerializer::Serialize(uint64_t& value, [[maybe_unused]] const char* name, [[maybe_unused]] uint64_t minValue, [[maybe_unused]] uint64_t maxValue)
    {
        m_hash = AZ::TypeHash64(value, m_hash);
        return true;
    }

    bool HashSerializer::Serialize(float& value, [[maybe_unused]] const char* name, [[maybe_unused]] float minValue, [[maybe_unused]] float maxValue)
    {
        // This hashing serializer is used to detect desyncs between the predicted and authoritative state of all predictive values
        // If either of these asserts triggers, it means desyncs *will not* be detected for the value being serialized
        // You should consider using a quantized float for the failing value, or potentially adjust the min/max quantized values
        AZ_Assert(value > FloatHashMinValue, "Out of range float value passed to hashing serializer, this will clamp the float value");
        AZ_Assert(value < FloatHashMaxValue, "Out of range float value passed to hashing serializer, this will clamp the float value");
        QuantizedValues<1, 4, FloatHashMinValue, FloatHashMaxValue> quantizedValue(value);
        const int32_t hashableValue = quantizedValue.GetQuantizedIntegralValues()[0];
        m_hash = AZ::TypeHash64(hashableValue, m_hash);
        return true;
    }

    bool HashSerializer::Serialize(double& value, [[maybe_unused]] const char* name, [[maybe_unused]] double minValue, [[maybe_unused]] double maxValue)
    {
        m_hash = AZ::TypeHash64(value, m_hash);
        return true;
    }

    bool HashSerializer::SerializeBytes(uint8_t* buffer, uint32_t , bool, uint32_t& outSize, [[maybe_unused]] const char* name)
    {
        m_hash = AZ::TypeHash64(buffer, outSize, m_hash);
        return true;
    }

    bool HashSerializer::BeginObject([[maybe_unused]] const char* name, [[maybe_unused]] const char* typeName)
    {
        return true;
    }

    bool HashSerializer::EndObject([[maybe_unused]] const char* name, [[maybe_unused]] const char* typeName)
    {
        return true;
    }

    const uint8_t* HashSerializer::GetBuffer() const
    {
        return nullptr;
    }

    uint32_t HashSerializer::GetCapacity() const
    {
        return 0;
    }

    uint32_t HashSerializer::GetSize() const
    {
        return 0;
    }
}
