/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/Serialization/ISerializer.h>

namespace AzNetworking
{
    enum class ValidateSerializeType : uint8_t
    {
        Bool,
        Char,
        Int8,
        Int16,
        Int32,
        Int64,
        Uint8,
        Uint16,
        Uint32,
        Uint64,
        Float,
        Double,
        ByteArray,
        ObjectStart,
        ObjectEnd
    };

    const char* GetEnumString(ValidateSerializeType value);

    //! @class SerializerTypeValidator
    //! @brief A helper that can be used by any serializer to inject type information into the serialized data.
    template <typename BASE_TYPE>
    class TypeValidatingSerializer final
        : public BASE_TYPE
    {
    public:

        //! Constructor.
        //! @param buffer         output buffer to read from
        //! @param bufferCapacity capacity of the buffer in bytes
        TypeValidatingSerializer(uint8_t* buffer, uint32_t bufferCapacity);
        TypeValidatingSerializer(const uint8_t* buffer, uint32_t bufferCapacity);

        // ISerializer interfaces
        SerializerMode GetSerializerMode() const override;
        bool Serialize(bool& value, const char* name) override;
        bool Serialize(int8_t& value, const char* name, int8_t minValue, int8_t maxValue) override;
        bool Serialize(int16_t& value, const char* name, int16_t minValue, int16_t maxValue) override;
        bool Serialize(int32_t& value, const char* name, int32_t minValue, int32_t maxValue) override;
        bool Serialize(long& value, const char* name, long minValue, long maxValue) override;
        bool Serialize(AZ::s64& value, const char* name, AZ::s64 minValue, AZ::s64 maxValue) override;
        bool Serialize(uint8_t& value, const char* name, uint8_t minValue, uint8_t maxValue) override;
        bool Serialize(uint16_t& value, const char* name, uint16_t minValue, uint16_t maxValue) override;
        bool Serialize(uint32_t& value, const char* name, uint32_t minValue, uint32_t maxValue) override;
        bool Serialize(unsigned long& value, const char* name, unsigned long minValue, unsigned long maxValue) override;
        bool Serialize(AZ::u64& value, const char* name, AZ::u64 minValue, AZ::u64 maxValue) override;
        bool Serialize(float& value, const char* name, float minValue, float maxValue) override;
        bool Serialize(double& value, const char* name, double minValue, double maxValue) override;
        bool SerializeBytes(uint8_t* buffer, uint32_t bufferCapacity, bool isString, uint32_t& outSize, const char* name) override;
        bool BeginObject(const char* name) override;
        bool EndObject(const char* name) override;

        const uint8_t* GetBuffer() const override;
        uint32_t GetCapacity() const override;
        uint32_t GetSize() const override;
        void ClearTrackedChangesFlag() override;
        bool GetTrackedChangesFlag() const override;
        // ISerializer interfaces

    private:

        bool Validate(const char* name, ValidateSerializeType type);

        bool m_enabled = false;
        bool m_validating = false;
    };
}

#include <AzNetworking/Serialization/TypeValidatingSerializer.inl>
