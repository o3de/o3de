/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AzNetworking
{
    template <typename BASE_TYPE>
    TrackChangedSerializer<BASE_TYPE>::TrackChangedSerializer(const uint8_t* buffer, uint32_t bufferCapacity)
        : BASE_TYPE(buffer, bufferCapacity)
        , m_hasChanged(false)
    {
        ;
    }

    template <typename BASE_TYPE>
    SerializerMode TrackChangedSerializer<BASE_TYPE>::GetSerializerMode() const
    {
        return BASE_TYPE::GetSerializerMode();
    }

    template <typename BASE_TYPE>
    bool TrackChangedSerializer<BASE_TYPE>::Serialize(bool& value, const char* name)
    {
        const bool cached = value;
        const bool result = BASE_TYPE::Serialize(value, name);
        m_hasChanged |= (cached != value);
        return result;
    }

    template <typename BASE_TYPE>
    bool TrackChangedSerializer<BASE_TYPE>::Serialize(char& value, const char* name, char minValue, char maxValue)
    {
        const char cached = value;
        const bool result = BASE_TYPE::Serialize(value, name, minValue, maxValue);
        m_hasChanged |= (cached != value);
        return result;
    }

    template <typename BASE_TYPE>
    bool TrackChangedSerializer<BASE_TYPE>::Serialize(int8_t& value, const char* name, int8_t minValue, int8_t maxValue)
    {
        const int8_t cached = value;
        const bool result = BASE_TYPE::Serialize(value, name, minValue, maxValue);
        m_hasChanged |= (cached != value);
        return result;
    }

    template <typename BASE_TYPE>
    bool TrackChangedSerializer<BASE_TYPE>::Serialize(int16_t& value, const char* name, int16_t minValue, int16_t maxValue)
    {
        const int16_t cached = value;
        const bool result = BASE_TYPE::Serialize(value, name, minValue, maxValue);
        m_hasChanged |= (cached != value);
        return result;
    }

    template <typename BASE_TYPE>
    bool TrackChangedSerializer<BASE_TYPE>::Serialize(int32_t& value, const char* name, int32_t minValue, int32_t maxValue)
    {
        const int32_t cached = value;
        const bool result = BASE_TYPE::Serialize(value, name, minValue, maxValue);
        m_hasChanged |= (cached != value);
        return result;
    }

    template <typename BASE_TYPE>
    bool TrackChangedSerializer<BASE_TYPE>::Serialize(int64_t& value, const char* name, int64_t minValue, int64_t maxValue)
    {
        const int64_t cached = value;
        const bool result = BASE_TYPE::Serialize(value, name, minValue, maxValue);
        m_hasChanged |= (cached != value);
        return result;
    }

    template <typename BASE_TYPE>
    bool TrackChangedSerializer<BASE_TYPE>::Serialize(uint8_t& value, const char* name, uint8_t minValue, uint8_t maxValue)
    {
        const uint8_t cached = value;
        const bool result = BASE_TYPE::Serialize(value, name, minValue, maxValue);
        m_hasChanged |= (cached != value);
        return result;
    }

    template <typename BASE_TYPE>
    bool TrackChangedSerializer<BASE_TYPE>::Serialize(uint16_t& value, const char* name, uint16_t minValue, uint16_t maxValue)
    {
        const uint16_t cached = value;
        const bool result = BASE_TYPE::Serialize(value, name, minValue, maxValue);
        m_hasChanged |= (cached != value);
        return result;
    }

    template <typename BASE_TYPE>
    bool TrackChangedSerializer<BASE_TYPE>::Serialize(uint32_t& value, const char* name, uint32_t minValue, uint32_t maxValue)
    {
        const uint32_t cached = value;
        const bool result = BASE_TYPE::Serialize(value, name, minValue, maxValue);
        m_hasChanged |= (cached != value);
        return result;
    }

    template <typename BASE_TYPE>
    bool TrackChangedSerializer<BASE_TYPE>::Serialize(uint64_t& value, const char* name, uint64_t minValue, uint64_t maxValue)
    {
        const uint64_t cached = value;
        const bool result = BASE_TYPE::Serialize(value, name, minValue, maxValue);
        m_hasChanged |= (cached != value);
        return result;
    }

    template <typename BASE_TYPE>
    bool TrackChangedSerializer<BASE_TYPE>::Serialize(float& value, const char* name, float minValue, float maxValue)
    {
        const float cached = value;
        const bool result = BASE_TYPE::Serialize(value, name, minValue, maxValue);
        m_hasChanged |= (cached != value);
        return result;
    }

    template <typename BASE_TYPE>
    bool TrackChangedSerializer<BASE_TYPE>::Serialize(double& value, const char* name, double minValue, double maxValue)
    {
        const double cached = value;
        const bool result = BASE_TYPE::Serialize(value, name, minValue, maxValue);
        m_hasChanged |= (cached != value);
        return result;
    }

    template <typename BASE_TYPE>
    bool TrackChangedSerializer<BASE_TYPE>::SerializeBytes(uint8_t* buffer, uint32_t bufferCapacity, bool isString, uint32_t& outSize, const char* name)
    {
        ByteBuffer<16384> cached;
        if (!cached.CopyValues(buffer, outSize))
        {
            return false;
        }
        const bool result = BASE_TYPE::SerializeBytes(buffer, bufferCapacity, isString, outSize, name);
        m_hasChanged |= (cached.IsSame(buffer, outSize));
        return result;
    }

    template <typename BASE_TYPE>
    bool TrackChangedSerializer<BASE_TYPE>::BeginObject(const char* name, const char* typeName)
    {
        return BASE_TYPE::BeginObject(name, typeName);
    }

    template <typename BASE_TYPE>
    bool TrackChangedSerializer<BASE_TYPE>::EndObject(const char* name, const char* typeName)
    {
        return BASE_TYPE::EndObject(name, typeName);
    }

    template <typename BASE_TYPE>
    const uint8_t* TrackChangedSerializer<BASE_TYPE>::GetBuffer() const
    {
        return BASE_TYPE::GetBuffer();
    }

    template <typename BASE_TYPE>
    uint32_t TrackChangedSerializer<BASE_TYPE>::GetCapacity() const
    {
        return BASE_TYPE::GetCapacity();
    }

    template <typename BASE_TYPE>
    uint32_t TrackChangedSerializer<BASE_TYPE>::GetSize() const
    {
        return BASE_TYPE::GetSize();
    }

    template <typename BASE_TYPE>
    void TrackChangedSerializer<BASE_TYPE>::ClearTrackedChangesFlag()
    {
        m_hasChanged = false;
    }

    template <typename BASE_TYPE>
    bool TrackChangedSerializer<BASE_TYPE>::GetTrackedChangesFlag() const
    {
        return m_hasChanged;
    }
}
