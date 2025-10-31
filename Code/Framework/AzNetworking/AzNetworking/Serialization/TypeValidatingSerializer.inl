/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/Serialization/TypeValidatingSerializer.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Utils/TypeHash.h>

namespace AzNetworking
{
    inline const char* GetEnumString(ValidateSerializeType value)
    {
        switch (value)
        {
        case ValidateSerializeType::Bool:
            return "bool";
        case ValidateSerializeType::Char:
            return "char";
        case ValidateSerializeType::Int8:
            return "int8_t";
        case ValidateSerializeType::Int16:
            return "int16_t";
        case ValidateSerializeType::Int32:
            return "int32_t";
        case ValidateSerializeType::Int64:
            return "int64_t";
        case ValidateSerializeType::Uint8:
            return "uint8_t";
        case ValidateSerializeType::Uint16:
            return "uint16_t";
        case ValidateSerializeType::Uint32:
            return "uint32_t";
        case ValidateSerializeType::Uint64:
            return "uint64_t";
        case ValidateSerializeType::Float:
            return "float";
        case ValidateSerializeType::Double:
            return "double";
        case ValidateSerializeType::ByteArray:
            return "byte-array";
        case ValidateSerializeType::ObjectStart:
            return "object-start";
        case ValidateSerializeType::ObjectEnd:
            return "object-end";
        }
        return "Unknown Type";
    }

    template <typename BASE_TYPE>
    TypeValidatingSerializer<BASE_TYPE>::TypeValidatingSerializer(uint8_t* buffer, uint32_t bufferCapacity)
        : BASE_TYPE(buffer, bufferCapacity)
    {
        AZ::Interface<AZ::IConsole>::Get()->GetCvarValue("net_validateSerializedTypes", m_enabled);
    }

    template <typename BASE_TYPE>
    TypeValidatingSerializer<BASE_TYPE>::TypeValidatingSerializer(const uint8_t* buffer, uint32_t bufferCapacity)
        : BASE_TYPE(const_cast<uint8_t*>(buffer), bufferCapacity)
    {
        AZ::Interface<AZ::IConsole>::Get()->GetCvarValue("net_validateSerializedTypes", m_enabled);
    }

    template <typename BASE_TYPE>
    SerializerMode TypeValidatingSerializer<BASE_TYPE>::GetSerializerMode() const
    {
        return BASE_TYPE::GetSerializerMode();
    }

    template <typename BASE_TYPE>
    bool TypeValidatingSerializer<BASE_TYPE>::Serialize(bool& value, const char* name)
    {
        bool result = Validate(name, ValidateSerializeType::Bool);
        return BASE_TYPE::Serialize(value, name) && result;
    }

    template <typename BASE_TYPE>
    bool TypeValidatingSerializer<BASE_TYPE>::Serialize(int8_t& value, const char* name, int8_t minValue, int8_t maxValue)
    {
        bool result = Validate(name, ValidateSerializeType::Int8);
        return BASE_TYPE::Serialize(value, name, minValue, maxValue) && result;
    }

    template <typename BASE_TYPE>
    bool TypeValidatingSerializer<BASE_TYPE>::Serialize(int16_t& value, const char* name, int16_t minValue, int16_t maxValue)
    {
        bool result = Validate(name, ValidateSerializeType::Int16);
        return BASE_TYPE::Serialize(value, name, minValue, maxValue) && result;
    }

    template <typename BASE_TYPE>
    bool TypeValidatingSerializer<BASE_TYPE>::Serialize(int32_t& value, const char* name, int32_t minValue, int32_t maxValue)
    {
        bool result = Validate(name, ValidateSerializeType::Int32);
        return BASE_TYPE::Serialize(value, name, minValue, maxValue) && result;
    }

    template <typename BASE_TYPE>
    bool TypeValidatingSerializer<BASE_TYPE>::Serialize(long& value, const char* name, long minValue, long maxValue)
    {
        bool result = Validate(name, sizeof(value) == 8 ? ValidateSerializeType::Int64 : ValidateSerializeType::Int32);
        return BASE_TYPE::Serialize(value, name, minValue, maxValue) && result;
    }

    template <typename BASE_TYPE>
    bool TypeValidatingSerializer<BASE_TYPE>::Serialize(AZ::s64& value, const char* name, AZ::s64 minValue, AZ::s64 maxValue)
    {
        bool result = Validate(name, ValidateSerializeType::Int64);
        return BASE_TYPE::Serialize(value, name, minValue, maxValue) && result;
    }

    template <typename BASE_TYPE>
    bool TypeValidatingSerializer<BASE_TYPE>::Serialize(uint8_t& value, const char* name, uint8_t minValue, uint8_t maxValue)
    {
        bool result = Validate(name, ValidateSerializeType::Uint8);
        return BASE_TYPE::Serialize(value, name, minValue, maxValue) && result;
    }

    template <typename BASE_TYPE>
    bool TypeValidatingSerializer<BASE_TYPE>::Serialize(uint16_t& value, const char* name, uint16_t minValue, uint16_t maxValue)
    {
        bool result = Validate(name, ValidateSerializeType::Uint16);
        return BASE_TYPE::Serialize(value, name, minValue, maxValue) && result;
    }

    template <typename BASE_TYPE>
    bool TypeValidatingSerializer<BASE_TYPE>::Serialize(uint32_t& value, const char* name, uint32_t minValue, uint32_t maxValue)
    {
        bool result = Validate(name, ValidateSerializeType::Uint32);
        return BASE_TYPE::Serialize(value, name, minValue, maxValue) && result;
    }

    template <typename BASE_TYPE>
    bool TypeValidatingSerializer<BASE_TYPE>::Serialize(unsigned long& value, const char* name, unsigned long minValue, unsigned long maxValue)
    {
        bool result = Validate(name, sizeof(value) == 8 ? ValidateSerializeType::Uint64 : ValidateSerializeType::Uint32);
        return BASE_TYPE::Serialize(value, name, minValue, maxValue) && result;
    }

    template <typename BASE_TYPE>
    bool TypeValidatingSerializer<BASE_TYPE>::Serialize(AZ::u64& value, const char* name, AZ::u64 minValue, AZ::u64 maxValue)
    {
        bool result = Validate(name, ValidateSerializeType::Uint64);
        return BASE_TYPE::Serialize(value, name, minValue, maxValue) && result;
    }

    template <typename BASE_TYPE>
    bool TypeValidatingSerializer<BASE_TYPE>::Serialize(float& value, const char* name, float minValue, float maxValue)
    {
        bool result = Validate(name, ValidateSerializeType::Float);
        return BASE_TYPE::Serialize(value, name, minValue, maxValue) && result;
    }

    template <typename BASE_TYPE>
    bool TypeValidatingSerializer<BASE_TYPE>::Serialize(double& value, const char* name, double minValue, double maxValue)
    {
        bool result = Validate(name, ValidateSerializeType::Double);
        return BASE_TYPE::Serialize(value, name, minValue, maxValue) && result;
    }

    template <typename BASE_TYPE>
    bool TypeValidatingSerializer<BASE_TYPE>::SerializeBytes(uint8_t* buffer, uint32_t bufferCapacity, bool isString, uint32_t& outSize, const char* name)
    {
        bool result = Validate(name, ValidateSerializeType::ByteArray);
        return BASE_TYPE::SerializeBytes(buffer, bufferCapacity, isString, outSize, name) && result;
    }

    template <typename BASE_TYPE>
    bool TypeValidatingSerializer<BASE_TYPE>::BeginObject(const char* name)
    {
        bool result = Validate(name, ValidateSerializeType::ObjectStart);
        return BASE_TYPE::BeginObject(name) && result;
    }

    template <typename BASE_TYPE>
    bool TypeValidatingSerializer<BASE_TYPE>::EndObject(const char* name)
    {
        bool result = Validate(name, ValidateSerializeType::ObjectEnd);
        return BASE_TYPE::EndObject(name) && result;
    }

    template <typename BASE_TYPE>
    const uint8_t* TypeValidatingSerializer<BASE_TYPE>::GetBuffer() const
    {
        return BASE_TYPE::GetBuffer();
    }

    template <typename BASE_TYPE>
    uint32_t TypeValidatingSerializer<BASE_TYPE>::GetCapacity() const
    {
        return BASE_TYPE::GetCapacity();
    }

    template <typename BASE_TYPE>
    uint32_t TypeValidatingSerializer<BASE_TYPE>::GetSize() const
    {
        return BASE_TYPE::GetSize();
    }

    template <typename BASE_TYPE>
    void TypeValidatingSerializer<BASE_TYPE>::ClearTrackedChangesFlag()
    {
        return BASE_TYPE::ClearTrackedChangesFlag();
    }

    template <typename BASE_TYPE>
    bool TypeValidatingSerializer<BASE_TYPE>::GetTrackedChangesFlag() const
    {
        return BASE_TYPE::GetTrackedChangesFlag();
    }

    struct ScopedEnableValidation
    {
        ScopedEnableValidation(bool& validating)
            : m_validating(validating)
        {
            m_validating = true;
        }
        ~ScopedEnableValidation()
        {
            m_validating = false;
        }
        bool& m_validating;
    };

    template <typename BASE_TYPE>
    bool TypeValidatingSerializer<BASE_TYPE>::Validate(const char* name, ValidateSerializeType type)
    {
        if (m_enabled && !m_validating)
        {
            // This guards against recursion during name serialization
            ScopedEnableValidation validateScope(m_validating);

            AZ::CVarFixedString nameValue = name;
            AzNetworking::SerializeAzContainer<AZ::CVarFixedString>::Serialize(*this, nameValue);
            if (nameValue != name)
            {
                AZ_Assert(false, "Name validation failed during serialization, expected %s but encountered %s",
                    nameValue.c_str(), name);
                return false;
            }

            uint8_t typeValue = static_cast<uint8_t>(type);
            BASE_TYPE::Serialize(typeValue, "Type", 0, AZStd::numeric_limits<uint8_t>::max());
            if (typeValue != static_cast<uint8_t>(type))
            {
                AZ_Assert(false, "Type validation failed during serialization, expected %s but encountered %s",
                    GetEnumString(type), GetEnumString(static_cast<ValidateSerializeType>(typeValue)));
                return false;
            }
        }
        return true;
    }
}
