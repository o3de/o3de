/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/Serialization/ISerializer.h>
#include <AzCore/Utils/TypeHash.h>

namespace AzNetworking
{
    //! @class HashSerializer
    //! @brief Generate a 32bit integer hash for a serializable object.
    //! NOTE: This hash is not designed to be cryptographically secure
    class HashSerializer
        : public ISerializer
    {
    public:

        HashSerializer();
        ~HashSerializer() override;

        AZ::HashValue32 GetHash() const;

        // ISerializer interfaces
        SerializerMode GetSerializerMode() const override;
        bool Serialize(bool& value, const char* name) override;
        bool Serialize(char& value, const char* name, char minValue, char maxValue) override;
        bool Serialize(int8_t& value, const char* name, int8_t minValue, int8_t maxValue) override;
        bool Serialize(int16_t& value, const char* name, int16_t minValue, int16_t maxValue) override;
        bool Serialize(int32_t& value, const char* name, int32_t minValue, int32_t maxValue) override;
        bool Serialize(int64_t& value, const char* name, int64_t minValue, int64_t maxValue) override;
        bool Serialize(uint8_t& value, const char* name, uint8_t minValue, uint8_t maxValue) override;
        bool Serialize(uint16_t& value, const char* name, uint16_t minValue, uint16_t maxValue) override;
        bool Serialize(uint32_t& value, const char* name, uint32_t minValue, uint32_t maxValue) override;
        bool Serialize(uint64_t& value, const char* name, uint64_t minValue, uint64_t maxValue) override;
        bool Serialize(float& value, const char* name, float minValue, float maxValue) override;
        bool Serialize(double& value, const char* name, double minValue, double maxValue) override;
        bool SerializeBytes(uint8_t* buffer, uint32_t bufferCapacity, bool isString, uint32_t& outSize, const char* name) override;
        bool BeginObject(const char* name, const char* typeName) override;
        bool EndObject(const char* name, const char* typeName) override;

        const uint8_t* GetBuffer() const override;
        uint32_t GetCapacity() const override;
        uint32_t GetSize() const override;
        void ClearTrackedChangesFlag() override {}
        bool GetTrackedChangesFlag() const override { return false; }
        // ISerializer interfaces

    private:

        AZ::HashValue64 m_hash = AZ::HashValue64{ 0 };
    };
}
