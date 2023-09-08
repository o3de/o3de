/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/Serialization/ISerializer.h>
#include <AzCore/std/containers/map.h>

namespace AzNetworking
{
    // StringifySerializer
    // Generate a debug string of a serializable object
    class StringifySerializer
        : public ISerializer
    {
    public:

        using ValueMap = AZStd::map<AZStd::string, AZStd::string>;

        StringifySerializer();
        ~StringifySerializer() override;

        //! After serializing objects, get the serialized values as a map of key/value pairs.
        const ValueMap& GetValueMap() const;

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

        template <typename T>
        bool ProcessData(const char* name, const T& value);

        ValueMap m_valueMap;
        AZStd::string m_prefix;
        AZStd::deque<AZStd::size_t> m_prefixSizeStack;
    };
}
