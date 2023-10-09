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
    //! @class NetworkInputSerializer
    //! @brief Input serializer for writing an object model into a bytestream.
    class NetworkInputSerializer
        : public ISerializer
    {
    public:

        //! Constructor.
        //! @param buffer         input buffer to write to
        //! @param bufferCapacity capacity of the buffer in bytes
        NetworkInputSerializer(uint8_t* buffer, uint32_t bufferCapacity);

        //! Copies the provided bytes into the serialization output buffer.
        //! @param data     pointer to the data buffer to copy
        //! @param dataSize size of the data in bytes
        //! @return boolean true on success, false if there was insufficient space to store all the data
        bool CopyToBuffer(const uint8_t* data, uint32_t dataSize);

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

         //! Private copy operator, do not allow copying instances
        NetworkInputSerializer& operator=(const NetworkInputSerializer&) = delete;

        template <typename ORIGINAL_TYPE>
        bool SerializeBoundedValue(ORIGINAL_TYPE minValue, ORIGINAL_TYPE maxValue, ORIGINAL_TYPE inputValue);

        template <typename SERIALIZE_TYPE>
        bool SerializeBoundedValueHelper(SERIALIZE_TYPE serializeValue);

        bool SerializeBytes(const uint8_t* data, uint32_t count);

        uint32_t       m_bufferSize = 0;
        const uint32_t m_bufferCapacity;
        const uint8_t* m_buffer;
    };
}
