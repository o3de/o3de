/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/AzNetworkingConfiguration.h>
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

        AZNETWORKING_API HashSerializer() = default;

        AZNETWORKING_API AZ::HashValue32 GetHash() const;

        // ISerializer interfaces
        AZNETWORKING_API SerializerMode GetSerializerMode() const override;
        AZNETWORKING_API bool Serialize(bool& value, const char* name) override;
        AZNETWORKING_API bool Serialize(char& value, const char* name, char minValue, char maxValue) override;
        AZNETWORKING_API bool Serialize(int8_t& value, const char* name, int8_t minValue, int8_t maxValue) override;
        AZNETWORKING_API bool Serialize(int16_t& value, const char* name, int16_t minValue, int16_t maxValue) override;
        AZNETWORKING_API bool Serialize(int32_t& value, const char* name, int32_t minValue, int32_t maxValue) override;
        AZNETWORKING_API bool Serialize(int64_t& value, const char* name, int64_t minValue, int64_t maxValue) override;
        AZNETWORKING_API bool Serialize(uint8_t& value, const char* name, uint8_t minValue, uint8_t maxValue) override;
        AZNETWORKING_API bool Serialize(uint16_t& value, const char* name, uint16_t minValue, uint16_t maxValue) override;
        AZNETWORKING_API bool Serialize(uint32_t& value, const char* name, uint32_t minValue, uint32_t maxValue) override;
        AZNETWORKING_API bool Serialize(uint64_t& value, const char* name, uint64_t minValue, uint64_t maxValue) override;
        AZNETWORKING_API bool Serialize(float& value, const char* name, float minValue, float maxValue) override;
        AZNETWORKING_API bool Serialize(double& value, const char* name, double minValue, double maxValue) override;
        AZNETWORKING_API bool SerializeBytes(uint8_t* buffer, uint32_t bufferCapacity, bool isString, uint32_t& outSize, const char* name) override;
        AZNETWORKING_API bool BeginObject(const char* name, const char* typeName) override;
        AZNETWORKING_API bool EndObject(const char* name, const char* typeName) override;

        AZNETWORKING_API const uint8_t* GetBuffer() const override;
        AZNETWORKING_API uint32_t GetCapacity() const override;
        AZNETWORKING_API uint32_t GetSize() const override;
        AZNETWORKING_API void ClearTrackedChangesFlag() override {}
        AZNETWORKING_API bool GetTrackedChangesFlag() const override { return false; }
        // ISerializer interfaces

    private:

        AZ::HashValue64 m_hash = AZ::HashValue64{ 0 };
    };
}
