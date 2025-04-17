/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/Serialization/NetworkOutputSerializer.h>
#include <AzNetworking/Serialization/TypeValidatingSerializer.h>
#include <AzNetworking/Serialization/TrackChangedSerializer.h>
#include <AzNetworking/AzNetworking_Traits_Platform.h>
#include <AzNetworking/Utilities/Endian.h>
#include <AzNetworking/Utilities/NetworkIncludes.h>

namespace AzNetworking
{
    NetworkOutputSerializer::NetworkOutputSerializer(const uint8_t* buffer, uint32_t bufferCapacity)
        : m_bufferPosition(0)
        , m_bufferCapacity(bufferCapacity)
        , m_buffer(buffer)
    {
        ;
    }

    SerializerMode NetworkOutputSerializer::GetSerializerMode() const
    {
        return SerializerMode::WriteToObject;
    }

    bool NetworkOutputSerializer::Serialize(bool& value, [[maybe_unused]] const char* name)
    {
        uint8_t byteValue = 0;
        SerializeBytes((uint8_t*)&byteValue, sizeof(byteValue));
        value = (byteValue > 0);
        return m_serializerValid;
    }

    bool NetworkOutputSerializer::Serialize(int8_t& value, [[maybe_unused]] const char* name, int8_t minValue, int8_t maxValue)
    {
        return SerializeBoundedValue<int8_t>(minValue, maxValue, value);
    }

    bool NetworkOutputSerializer::Serialize(int16_t& value, [[maybe_unused]] const char* name, int16_t minValue, int16_t maxValue)
    {
        return SerializeBoundedValue<int16_t>(minValue, maxValue, value);
    }

    bool NetworkOutputSerializer::Serialize(int32_t& value, [[maybe_unused]] const char* name, int32_t minValue, int32_t maxValue)
    {
        return SerializeBoundedValue<int32_t>(minValue, maxValue, value);
    }

    bool NetworkOutputSerializer::Serialize(long& value, [[maybe_unused]] const char* name, long minValue, long maxValue)
    {
        return SerializeBoundedValue<long>(minValue, maxValue, value);
    }

    bool NetworkOutputSerializer::Serialize(AZ::s64& value, [[maybe_unused]] const char* name, AZ::s64 minValue, AZ::s64 maxValue)
    {
        return SerializeBoundedValue<AZ::s64>(minValue, maxValue, value);
    }

    bool NetworkOutputSerializer::Serialize(uint8_t& value, [[maybe_unused]] const char* name, uint8_t minValue, uint8_t maxValue)
    {
        return SerializeBoundedValue<uint8_t>(minValue, maxValue, value);
    }

    bool NetworkOutputSerializer::Serialize(uint16_t& value, [[maybe_unused]] const char* name, uint16_t minValue, uint16_t maxValue)
    {
        return SerializeBoundedValue<uint16_t>(minValue, maxValue, value);
    }

    bool NetworkOutputSerializer::Serialize(uint32_t& value, [[maybe_unused]] const char* name, uint32_t minValue, uint32_t maxValue)
    {
        return SerializeBoundedValue<uint32_t>(minValue, maxValue, value);
    }

    bool NetworkOutputSerializer::Serialize(unsigned long& value, [[maybe_unused]] const char* name, unsigned long minValue, unsigned long maxValue)
    {
        return SerializeBoundedValue<unsigned long>(minValue, maxValue, value);
    }

    bool NetworkOutputSerializer::Serialize(AZ::u64& value, [[maybe_unused]] const char* name, AZ::u64 minValue, AZ::u64 maxValue)
    {
        return SerializeBoundedValue<AZ::u64>(minValue, maxValue, value);
    }

    bool NetworkOutputSerializer::Serialize(float& value, [[maybe_unused]] const char* name, [[maybe_unused]] float minValue, [[maybe_unused]] float maxValue)
    {
        uint32_t networkOrder = 0;
        m_serializerValid &= SerializeBytes((uint8_t*)&networkOrder, sizeof(float));
        networkOrder = ntohl(networkOrder);
        value = m_serializerValid ? *reinterpret_cast<float*>(&networkOrder) : value;
        return m_serializerValid;
    }

    bool NetworkOutputSerializer::Serialize(double& value, [[maybe_unused]] const char* name, [[maybe_unused]] double minValue, [[maybe_unused]] double maxValue)
    {
        uint64_t networkOrder = 0;
        m_serializerValid &= SerializeBytes((uint8_t *)&networkOrder, sizeof(double));
        networkOrder = ntohll(networkOrder);
        value = m_serializerValid ? *reinterpret_cast<double*>(&networkOrder) : value;
        return m_serializerValid;
    }

    bool NetworkOutputSerializer::SerializeBytes(uint8_t* buffer, uint32_t bufferCapacity, [[maybe_unused]] bool isString, uint32_t& outSize, [[maybe_unused]] const char* name)
    {
        return SerializeBoundedValue<uint32_t>(0, bufferCapacity, outSize) && SerializeBytes(reinterpret_cast<uint8_t*>(buffer), outSize);
    }

    bool NetworkOutputSerializer::BeginObject([[maybe_unused]] const char* name)
    {
        return true;
    }

    bool NetworkOutputSerializer::EndObject([[maybe_unused]] const char* name)
    {
        return true;
    }

    const uint8_t* NetworkOutputSerializer::GetBuffer() const
    {
        return m_buffer;
    }

    uint32_t NetworkOutputSerializer::GetCapacity() const
    {
        return m_bufferCapacity;
    }

    uint32_t NetworkOutputSerializer::GetSize() const
    {
        return m_bufferPosition;
    }

    template <typename ORIGINAL_TYPE>
    bool NetworkOutputSerializer::SerializeBoundedValue(ORIGINAL_TYPE minValue, ORIGINAL_TYPE maxValue, ORIGINAL_TYPE& outValue)
    {
        const uint64_t valueRange = static_cast<uint64_t>(maxValue) - static_cast<uint64_t>(minValue);
        if (valueRange <= AZStd::numeric_limits<uint8_t>::max())
        {
            outValue = static_cast<ORIGINAL_TYPE>(SerializeBoundedValueHelper<uint8_t>(static_cast<uint8_t>(valueRange))) + minValue;
        }
        else if (valueRange <= AZStd::numeric_limits<uint16_t>::max())
        {
            outValue = static_cast<ORIGINAL_TYPE>(SerializeBoundedValueHelper<uint16_t>(static_cast<uint16_t>(valueRange))) + minValue;
        }
        else if (valueRange <= AZStd::numeric_limits<uint32_t>::max())
        {
            outValue = static_cast<ORIGINAL_TYPE>(SerializeBoundedValueHelper<uint32_t>(static_cast<uint32_t>(valueRange))) + minValue;
        }
        else
        {
            outValue = static_cast<ORIGINAL_TYPE>(SerializeBoundedValueHelper<uint64_t>(static_cast<uint64_t>(valueRange))) + minValue;
        }
        return m_serializerValid;
    }

    inline uint8_t NetworkToHost(uint8_t value)
    {
        return value;
    }

    inline uint16_t NetworkToHost(uint16_t value)
    {
        return ntohs(value);
    }

    inline uint32_t NetworkToHost(uint32_t value)
    {
        return ntohl(value);
    }

    inline uint64_t NetworkToHost(uint64_t value)
    {
        return ntohll(value);
    }

    template <typename SERIALIZE_TYPE>
    SERIALIZE_TYPE NetworkOutputSerializer::SerializeBoundedValueHelper(SERIALIZE_TYPE maxValue)
    {
        SERIALIZE_TYPE result = 0;
        m_serializerValid &= SerializeBytes((uint8_t*)&result, sizeof(SERIALIZE_TYPE));
        result = m_serializerValid ? NetworkToHost(result) : result;
        m_serializerValid &= (result <= maxValue);
        return result;
    }

    bool NetworkOutputSerializer::SerializeBytes(uint8_t* data, uint32_t count)
    {
        const uint32_t currSize = m_bufferPosition;
        const uint32_t nextSize = m_bufferPosition + count;

        if (!m_serializerValid || (nextSize > m_bufferCapacity))
        {
            // Keep the failed boolean so we can verify serialization success
            m_serializerValid = false;
            return false;
        }

        const uint8_t* readBuffer = (const uint8_t*)(m_buffer + currSize);
        memcpy(data, readBuffer, count);
        m_bufferPosition += count;
        return true;
    }

    template class TypeValidatingSerializer<NetworkOutputSerializer>;
    template class TypeValidatingSerializer<TrackChangedSerializer<NetworkOutputSerializer>>;
}
