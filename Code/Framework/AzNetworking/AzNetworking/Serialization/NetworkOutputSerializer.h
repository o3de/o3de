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
    //! @class NetworkOutputSerializer
    //! @brief Output serializer for inflating and writing out a bytestream into an object model.
    class NetworkOutputSerializer
        : public ISerializer
    {
    public:

        //! Constructor.
        //! @param buffer         output buffer to read from
        //! @param bufferCapacity capacity of the buffer in bytes
        NetworkOutputSerializer(const uint8_t* buffer, uint32_t bufferCapacity);

        //! Returns the unread portion of the data stream.
        //! @return the unread portion of the data stream
        const uint8_t* GetUnreadData() const;

        //! Returns the number of bytes not yet consumed from the serialization buffer.
        //! @return number of bytes not yet consumed from the serialization buffer
        uint32_t GetUnreadSize() const;

        //! Returns the number of bytes consumed by serialization.
        //! @return number of bytes consumed by serialization
        uint32_t GetReadSize() const;

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
        void ClearTrackedChangesFlag() override {}
        bool GetTrackedChangesFlag() const override { return false; }
        // ISerializer interfaces

    private:

        //! Private copy operator, do not allow copying instances.
        NetworkOutputSerializer& operator=(const NetworkOutputSerializer&) = delete;

        template <typename ORIGINAL_TYPE>
        bool SerializeBoundedValue(ORIGINAL_TYPE minValue, ORIGINAL_TYPE maxValue, ORIGINAL_TYPE& outValue);

        template <typename SERIALIZE_TYPE>
        SERIALIZE_TYPE SerializeBoundedValueHelper(SERIALIZE_TYPE maxValue);

        bool SerializeBytes(uint8_t* data, uint32_t count);

        uint32_t       m_bufferPosition = 0;
        const uint32_t m_bufferCapacity;
        const uint8_t* m_buffer;
    };
}

#include <AzNetworking/Serialization/NetworkOutputSerializer.inl>
