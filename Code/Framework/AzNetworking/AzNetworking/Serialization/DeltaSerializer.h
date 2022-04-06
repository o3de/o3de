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
#include <AzNetworking/Serialization/AbstractValue.h>
#include <AzNetworking/Serialization/NetworkInputSerializer.h>
#include <AzNetworking/Serialization/NetworkOutputSerializer.h>
#include <AzNetworking/DataStructures/FixedSizeVectorBitset.h>
#include <AzNetworking/DataStructures/ByteBuffer.h>
#include <AzCore/Utils/TypeHash.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AzNetworking
{
    //! SerializerDelta
    //! Encodes information used by DeltaSerializer to create and apply serialization deltas
    class SerializerDelta
    {
    public:

        AZNETWORKING_API SerializerDelta();

        AZNETWORKING_API uint32_t GetNumDirtyBits() const;
        AZNETWORKING_API bool GetDirtyBit(uint32_t index) const;
        AZNETWORKING_API bool InsertDirtyBit(bool dirtyBit);

        AZNETWORKING_API uint8_t* GetBufferPtr();
        AZNETWORKING_API uint32_t GetBufferSize() const;
        AZNETWORKING_API uint32_t GetBufferCapacity() const;

        AZNETWORKING_API void SetBufferSize(uint32_t size);

        AZNETWORKING_API bool Serialize(ISerializer& serializer);

    private:

        FixedSizeVectorBitset<255> m_dirtyBits;
        ByteBuffer<1024> m_deltaBytes;
    };

    //! A serializer that is used to produce a SerializerDelta between two objects.
    //! This delta can be reapplied to the same base object to reconstruct the second object using 
    //! the DeltaSerializerApply serializer
    //! NOTE: The objects serialized must have a consistent serialization footprint i.e. no changes in branches during serialization
    class DeltaSerializerCreate
        : public ISerializer
    {
    public:

        AZNETWORKING_API DeltaSerializerCreate(SerializerDelta& delta);
        AZNETWORKING_API ~DeltaSerializerCreate();

        template <typename TYPE>
        bool CreateDelta(TYPE& base, TYPE& current);

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

        DeltaSerializerCreate(const DeltaSerializerCreate&) = delete;
        DeltaSerializerCreate& operator=(const DeltaSerializerCreate&) = delete;

        template <typename T>
        bool SerializeHelper(T& value, uint32_t bufferCapacity, bool isString, uint32_t& outSize, const char* name);

        template <typename T>
        bool SerializeHelperImpl(T& value, uint32_t, bool, uint32_t&, const char* name);
        bool SerializeHelperImpl(uint8_t* buffer, uint32_t bufferCapacity, bool isString, uint32_t& outSize, const char* name);

    private:

        SerializerDelta& m_delta;

        bool m_gatheringRecords = false;
        uint32_t m_objectCounter = 0;
        AZStd::vector<AbstractValue::BaseValue*> m_records;
        NetworkInputSerializer m_dataSerializer;
    };

    //! A serializer that is used to apply a SerializerDelta to a base object in order to reconstruct the second object.
    //! NOTE: The objects serialized must have a consistent serialization footprint i.e. no changes in branches during serialization
    class DeltaSerializerApply
        : public ISerializer
    {
    public:

        AZNETWORKING_API DeltaSerializerApply(SerializerDelta& delta);

        template <typename TYPE>
        bool ApplyDelta(TYPE& output);

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
        AZNETWORKING_API bool BeginObject(const char *name, const char* typeName) override;
        AZNETWORKING_API bool EndObject(const char* name, const char* typeName) override;

        AZNETWORKING_API const uint8_t* GetBuffer() const override;
        AZNETWORKING_API uint32_t GetCapacity() const override;
        AZNETWORKING_API uint32_t GetSize() const override;
        AZNETWORKING_API void ClearTrackedChangesFlag() override {}
        AZNETWORKING_API bool GetTrackedChangesFlag() const override { return false; }
        // ISerializer interfaces

    private:
 
        DeltaSerializerApply(const DeltaSerializerApply&) = delete;
        DeltaSerializerApply& operator=(const DeltaSerializerApply&) = delete;

        template <typename T>
        bool SerializeHelper(T& value, uint32_t bufferCapacity, bool isString, uint32_t& outSize, const char* name);

        template <typename T>
        bool SerializeHelperImpl(T& value, uint32_t, bool, uint32_t&, const char* name);
        bool SerializeHelperImpl(uint8_t* buffer, uint32_t bufferCapacity, bool isString, uint32_t& outSize, const char* name);

    private:

        SerializerDelta& m_delta;
        uint32_t m_nextDirtyBit = 0;
        NetworkOutputSerializer m_dataSerializer;
    };
}

#include <AzNetworking/Serialization/DeltaSerializer.inl>
