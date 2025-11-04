/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/Serialization/NetworkInputSerializer.h>
#include <AzNetworking/Serialization/TypeValidatingSerializer.h>
#include <AzNetworking/AzNetworking_Traits_Platform.h>
#include <AzNetworking/Utilities/Endian.h>
#include <AzNetworking/Utilities/NetworkIncludes.h>
#include <memory>

namespace AzNetworking
{
    NetworkInputSerializer::NetworkInputSerializer(uint8_t* buffer, uint32_t bufferCapacity)
        : m_bufferSize(0)
        , m_bufferCapacity(bufferCapacity)
        , m_buffer(buffer)
    {
        ;
    }

    SerializerMode NetworkInputSerializer::GetSerializerMode() const
    {
        return SerializerMode::ReadFromObject;
    }

    bool NetworkInputSerializer::Serialize(bool& value, [[maybe_unused]] const char* name)
    {
        uint8_t serializeValue = (value) ? 1 : 0;
        return SerializeBytes((const uint8_t*)&serializeValue, sizeof(uint8_t));
    }

    bool NetworkInputSerializer::Serialize(int8_t& value, [[maybe_unused]] const char* name, int8_t minValue, int8_t maxValue)
    {
        return SerializeBoundedValue<int8_t>(minValue, maxValue, value);
    }

    bool NetworkInputSerializer::Serialize(int16_t& value, [[maybe_unused]] const char* name, int16_t minValue, int16_t maxValue)
    {
        return SerializeBoundedValue<int16_t>(minValue, maxValue, value);
    }

    bool NetworkInputSerializer::Serialize(int32_t& value, [[maybe_unused]] const char* name, int32_t minValue, int32_t maxValue)
    {
        return SerializeBoundedValue<int32_t>(minValue, maxValue, value);
    }

    bool NetworkInputSerializer::Serialize(long& value, [[maybe_unused]] const char* name, long minValue, long maxValue)
    {
        return SerializeBoundedValue<long>(minValue, maxValue, value);
    }

    bool NetworkInputSerializer::Serialize(AZ::s64& value, [[maybe_unused]] const char* name, AZ::s64 minValue, AZ::s64 maxValue)
    {
        return SerializeBoundedValue<AZ::s64>(minValue, maxValue, value);
    }

    bool NetworkInputSerializer::Serialize(uint8_t& value, [[maybe_unused]] const char* name, uint8_t minValue, uint8_t maxValue)
    {
        return SerializeBoundedValue<uint8_t>(minValue, maxValue, value);
    }

    bool NetworkInputSerializer::Serialize(uint16_t& value, [[maybe_unused]] const char* name, uint16_t minValue, uint16_t maxValue)
    {
        return SerializeBoundedValue<uint16_t>(minValue, maxValue, value);
    }

    bool NetworkInputSerializer::Serialize(uint32_t& value, [[maybe_unused]] const char* name, uint32_t minValue, uint32_t maxValue)
    {
        return SerializeBoundedValue<uint32_t>(minValue, maxValue, value);
    }

    bool NetworkInputSerializer::Serialize(unsigned long& value, [[maybe_unused]] const char* name, unsigned long minValue, unsigned long maxValue)
    {
        return SerializeBoundedValue<unsigned long>(minValue, maxValue, value);
    }

    bool NetworkInputSerializer::Serialize(AZ::u64& value, [[maybe_unused]] const char* name, AZ::u64 minValue, AZ::u64 maxValue)
    {
        return SerializeBoundedValue<AZ::u64>(minValue, maxValue, value);
    }

    bool NetworkInputSerializer::Serialize(float& value, [[maybe_unused]] const char* name, [[maybe_unused]] float minValue, [[maybe_unused]] float maxValue)
    {
        uint32_t hostOrder = *reinterpret_cast<uint32_t*>(&value);
        uint32_t networkOrder = ntohl(hostOrder);
        return SerializeBytes((const uint8_t*)&networkOrder, sizeof(float));
    }

    bool NetworkInputSerializer::Serialize(double& value, [[maybe_unused]] const char* name, [[maybe_unused]] double minValue, [[maybe_unused]] double maxValue)
    {
        uint64_t hostOrder = *reinterpret_cast<uint64_t*>(&value);
        uint64_t networkOrder = ntohll(hostOrder);
        return SerializeBytes((const uint8_t*)&networkOrder, sizeof(double));
    }

    bool NetworkInputSerializer::SerializeBytes(uint8_t* buffer, uint32_t bufferCapacity, [[maybe_unused]] bool isString, uint32_t& outSize, [[maybe_unused]] const char* name)
    {
        return SerializeBoundedValue<uint32_t>(0, bufferCapacity, outSize) && SerializeBytes(reinterpret_cast<uint8_t*>(buffer), outSize);
    }

    bool NetworkInputSerializer::BeginObject([[maybe_unused]] const char* name)
    {
        return true;
    }

    bool NetworkInputSerializer::EndObject([[maybe_unused]] const char* name)
    {
        return true;
    }

    const uint8_t* NetworkInputSerializer::GetBuffer() const
    {
        return m_buffer;
    }

    uint32_t NetworkInputSerializer::GetCapacity() const
    {
        return m_bufferCapacity;
    }

    uint32_t NetworkInputSerializer::GetSize() const
    {
        return m_bufferSize;
    }

    template <typename ORIGINAL_TYPE>
    bool NetworkInputSerializer::SerializeBoundedValue(ORIGINAL_TYPE minValue, ORIGINAL_TYPE maxValue, ORIGINAL_TYPE inputValue)
    {
        m_serializerValid &= (inputValue >= minValue);
        m_serializerValid &= (inputValue <= maxValue);
        const uint64_t valueRange = static_cast<uint64_t>(maxValue) - static_cast<uint64_t>(minValue);
        const ORIGINAL_TYPE adjustedValue = inputValue - minValue;
        if (valueRange <= AZStd::numeric_limits<uint8_t>::max())
        {
            return SerializeBoundedValueHelper<uint8_t>(static_cast<uint8_t>(adjustedValue));
        }
        else if (valueRange <= AZStd::numeric_limits<uint16_t>::max())
        {
            return SerializeBoundedValueHelper<uint16_t>(static_cast<uint16_t>(adjustedValue));
        }
        else if (valueRange <= AZStd::numeric_limits<uint32_t>::max())
        {
            return SerializeBoundedValueHelper<uint32_t>(static_cast<uint32_t>(adjustedValue));
        }
        return SerializeBoundedValueHelper<uint64_t>(static_cast<uint64_t>(adjustedValue));
    }

    inline uint8_t HostToNetwork(uint8_t value)
    {
        return value;
    }

    inline uint16_t HostToNetwork(uint16_t value)
    {
        return htons(value);
    }

    inline uint32_t HostToNetwork(uint32_t value)
    {
        return htonl(value);
    }

    inline uint64_t HostToNetwork(uint64_t value)
    {
        return htonll(value);
    }

    template <typename SERIALIZE_TYPE>
    bool NetworkInputSerializer::SerializeBoundedValueHelper(SERIALIZE_TYPE serializeValue)
    {
        const SERIALIZE_TYPE networkOrder = HostToNetwork(serializeValue);
        return m_serializerValid && SerializeBytes((const uint8_t*)&networkOrder, sizeof(SERIALIZE_TYPE));
    }

    bool NetworkInputSerializer::SerializeBytes(const uint8_t* data, uint32_t count)
    {
        const uint32_t currSize = m_bufferSize;
        const uint32_t nextSize = m_bufferSize + count;

        if (!m_serializerValid || (nextSize > m_bufferCapacity))
        {
            // Keep the failed boolean so we can verify serialization success
            m_serializerValid = false;
            return false;
        }

        uint8_t* writeBuffer = (uint8_t*)(m_buffer + currSize);
        memcpy(writeBuffer, data, count);
        m_bufferSize += count;
        return true;
    }

    bool NetworkInputSerializer::CopyToBuffer(const uint8_t* data, uint32_t dataSize)
    {
        return NetworkInputSerializer::SerializeBytes(data, dataSize);
    }

    template class TypeValidatingSerializer<NetworkInputSerializer>;
}
