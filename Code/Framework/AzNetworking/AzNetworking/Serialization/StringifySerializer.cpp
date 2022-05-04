/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/Serialization/StringifySerializer.h>

namespace AzNetworking
{
    StringifySerializer::StringifySerializer()
    {
        ;
    }

    StringifySerializer::~StringifySerializer()
    {
        ;
    }

    const StringifySerializer::ValueMap& StringifySerializer::GetValueMap() const
    {
        return m_valueMap;
    }

    SerializerMode StringifySerializer::GetSerializerMode() const
    {
        return SerializerMode::ReadFromObject;
    }

    bool StringifySerializer::Serialize(bool& value, const char* name)
    {
        return ProcessData(name, value);
    }

    bool StringifySerializer::Serialize(char& value, const char* name, char, char)
    {
        const int val = value; // Print chars as integers
        return ProcessData(name, val);
    }

    bool StringifySerializer::Serialize(int8_t& value, const char* name, int8_t, int8_t)
    {
        return ProcessData(name, value);
    }

    bool StringifySerializer::Serialize(int16_t& value, const char* name, int16_t, int16_t)
    {
        return ProcessData(name, value);
    }

    bool StringifySerializer::Serialize(int32_t& value, const char* name, int32_t, int32_t)
    {
        return ProcessData(name, value);
    }

    bool StringifySerializer::Serialize(int64_t& value, const char* name, int64_t, int64_t)
    {
        return ProcessData(name, value);
    }

    bool StringifySerializer::Serialize(uint8_t& value, const char* name, uint8_t, uint8_t)
    {
        return ProcessData(name, value);
    }

    bool StringifySerializer::Serialize(uint16_t& value, const char* name, uint16_t, uint16_t)
    {
        return ProcessData(name, value);
    }

    bool StringifySerializer::Serialize(uint32_t& value, const char* name, uint32_t, uint32_t)
    {
        return ProcessData(name, value);
    }

    bool StringifySerializer::Serialize(uint64_t& value, const char* name, uint64_t, uint64_t)
    {
        return ProcessData(name, value);
    }

    bool StringifySerializer::Serialize(float& value, const char* name, float, float)
    {
        return ProcessData(name, value);
    }

    bool StringifySerializer::Serialize(double& value, const char* name, double, double)
    {
        return ProcessData(name, value);
    }

    bool StringifySerializer::SerializeBytes(uint8_t* buffer, uint32_t, bool isString, uint32_t&, const char* name)
    {
        if (isString)
        {
            AZ::CVarFixedString value = reinterpret_cast<char*>(buffer);
            return ProcessData(name, value);
        }
        return false;
    }

    bool StringifySerializer::BeginObject(const char* name, const char*)
    {
        m_prefixSizeStack.push_back(m_prefix.size());
        m_prefix += name;
        m_prefix += ".";
        return true;
    }

    bool StringifySerializer::EndObject(const char*, const char*)
    {
        m_prefix.resize(m_prefixSizeStack.back());
        m_prefixSizeStack.pop_back();
        return true;
    }

    const uint8_t* StringifySerializer::GetBuffer() const
    {
        return nullptr;
    }

    uint32_t StringifySerializer::GetCapacity() const
    {
        return 0;
    }

    uint32_t StringifySerializer::GetSize() const
    {
        return 0;
    }

    template <typename T>
    bool StringifySerializer::ProcessData(const char* name, const T& value)
    {
        const AZStd::string keyString = m_prefix + name;
        AZ::CVarFixedString valueString = AZ::ConsoleTypeHelpers::ValueToString(value);
        m_valueMap[keyString] = valueString.c_str();
        return true;
    }
}
