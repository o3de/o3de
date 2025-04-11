/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/Serialization/DeltaSerializer.h>
#include <AzNetworking/Serialization/NetworkInputSerializer.h>
#include <AzNetworking/Serialization/NetworkOutputSerializer.h>
#include <AzCore/std/string/conversions.h>

namespace AzNetworking
{
    SerializerDelta::SerializerDelta()
        : m_dirtyBits()
        , m_deltaBytes()
    {
        ;
    }

    uint32_t SerializerDelta::GetNumDirtyBits() const
    {
        return m_dirtyBits.GetSize();
    }

    bool SerializerDelta::GetDirtyBit(uint32_t index) const
    {
        return m_dirtyBits.GetBit(index);
    }

    bool SerializerDelta::InsertDirtyBit(bool dirtyBit)
    {
        return m_dirtyBits.PushBack(dirtyBit);
    }

    uint8_t* SerializerDelta::GetBufferPtr()
    {
        return m_deltaBytes.GetBuffer();
    }

    uint32_t SerializerDelta::GetBufferSize() const
    {
        return static_cast<uint32_t>(m_deltaBytes.GetSize());
    }

    uint32_t SerializerDelta::GetBufferCapacity() const
    {
        return static_cast<uint32_t>(m_deltaBytes.GetCapacity());
    }

    void SerializerDelta::SetBufferSize(uint32_t size)
    {
        m_deltaBytes.Resize(size);
    }

    bool SerializerDelta::Serialize(ISerializer& serializer)
    {
        return serializer.Serialize(m_dirtyBits, "DirtyBits")
            && serializer.Serialize(m_deltaBytes, "DeltaBytes");
    }

    DeltaSerializerCreate::DeltaSerializerCreate(SerializerDelta& delta)
        : m_delta(delta)
        , m_dataSerializer(m_delta.GetBufferPtr(), m_delta.GetBufferCapacity())
    {
        ;
    }

    DeltaSerializerCreate::~DeltaSerializerCreate()
    {
        // Delete any left over records that might be hanging around
        for (auto iter : m_records)
        {
            if (iter)
            {
                // this will probably trigger ASAN, since we don't know what the type is.
                // But it should also only happen in scenarios where we are crashing or have already asserted/warned
                // about mismatched serialize/deserialize.
                delete iter;  
            }
        }
        m_records.clear();
    }

    SerializerMode DeltaSerializerCreate::GetSerializerMode() const
    {
        return SerializerMode::ReadFromObject;
    }

    bool DeltaSerializerCreate::Serialize(bool& value, const char* name)
    {
        uint32_t unused = 0;
        return SerializeHelper(value, 0, false, unused, name);
    }

    bool DeltaSerializerCreate::Serialize(int8_t& value, const char* name, [[maybe_unused]] int8_t minValue, [[maybe_unused]] int8_t maxValue)
    {
        uint32_t unused = 0;
        return SerializeHelper(value, 0, false, unused, name);
    }

    bool DeltaSerializerCreate::Serialize(int16_t& value, const char* name, [[maybe_unused]] int16_t minValue, [[maybe_unused]] int16_t maxValue)
    {
        uint32_t unused = 0;
        return SerializeHelper(value, 0, false, unused, name);
    }

    bool DeltaSerializerCreate::Serialize(int32_t& value, const char* name, [[maybe_unused]] int32_t minValue, [[maybe_unused]] int32_t maxValue)
    {
        uint32_t unused = 0;
        return SerializeHelper(value, 0, false, unused, name);
    }

    bool DeltaSerializerCreate::Serialize(long& value, const char* name, [[maybe_unused]] long minValue, [[maybe_unused]] long maxValue)
    {
        uint32_t unused = 0;
        return SerializeHelper(value, 0, false, unused, name);
    }

    bool DeltaSerializerCreate::Serialize(AZ::s64& value, const char* name, [[maybe_unused]] AZ::s64 minValue, [[maybe_unused]] AZ::s64 maxValue)
    {
        uint32_t unused = 0;
        return SerializeHelper(value, 0, false, unused, name);
    }

    bool DeltaSerializerCreate::Serialize(uint8_t& value, const char* name, [[maybe_unused]] uint8_t minValue, [[maybe_unused]] uint8_t maxValue)
    {
        uint32_t unused = 0;
        return SerializeHelper(value, 0, false, unused, name);
    }

    bool DeltaSerializerCreate::Serialize(uint16_t& value, const char* name, [[maybe_unused]] uint16_t minValue, [[maybe_unused]] uint16_t maxValue)
    {
        uint32_t unused = 0;
        return SerializeHelper(value, 0, false, unused, name);
    }

    bool DeltaSerializerCreate::Serialize(uint32_t& value, const char* name, [[maybe_unused]] uint32_t minValue, [[maybe_unused]] uint32_t maxValue)
    {
        uint32_t unused = 0;
        return SerializeHelper(value, 0, false, unused, name);
    }

    bool DeltaSerializerCreate::Serialize(unsigned long& value, const char* name, [[maybe_unused]] unsigned long minValue, [[maybe_unused]] unsigned long maxValue)
    {
        uint32_t unused = 0;
        return SerializeHelper(value, 0, false, unused, name);
    }

    bool DeltaSerializerCreate::Serialize(AZ::u64& value, const char* name, [[maybe_unused]] AZ::u64 minValue, [[maybe_unused]] AZ::u64 maxValue)
    {
        uint32_t unused = 0;
        return SerializeHelper(value, 0, false, unused, name);
    }

    bool DeltaSerializerCreate::Serialize(float& value, const char* name, [[maybe_unused]] float minValue, [[maybe_unused]] float maxValue)
    {
        uint32_t unused = 0;
        return SerializeHelper(value, 0, false, unused, name);
    }

    bool DeltaSerializerCreate::Serialize(double& value, const char* name, [[maybe_unused]] double minValue, [[maybe_unused]] double maxValue)
    {
        uint32_t unused = 0;
        return SerializeHelper(value, 0, false, unused, name);
    }

    bool DeltaSerializerCreate::SerializeBytes(uint8_t* buffer, uint32_t bufferCapacity, bool isString, uint32_t& outSize, const char* name)
    {
        return SerializeHelper(buffer, bufferCapacity, isString, outSize, name);
    }

    bool DeltaSerializerCreate::BeginObject([[maybe_unused]] const char* name)
    {
        return true;
    }

    bool DeltaSerializerCreate::EndObject([[maybe_unused]] const char* name)
    {
        return true;
    }

    const uint8_t* DeltaSerializerCreate::GetBuffer() const
    {
        return nullptr;
    }

    uint32_t DeltaSerializerCreate::GetCapacity() const
    {
        return 0;
    }

    uint32_t DeltaSerializerCreate::GetSize() const
    {
        return 0;
    }

    template <typename T>
    bool DeltaSerializerCreate::SerializeHelper(T& value, uint32_t bufferCapacity, bool isString, uint32_t& outSize, const char* name)
    {
        // The way this functions is that it expects its operation to be done in two phases:
        // First, it gathers records into the m_records vector for each object being serialized, representing the prior state
        // Then it expects to be called again, on objects in exactly the same order but with the new state.
        // It will then compare the two states and generate a delta in m_delta for the differences.
        // Once it has done this, the value stored in m_records is no longer required, and can be deleted while we still
        // are in a templated function that knows the type of T and can thus invoke the correct delete operator.

        // if this starts be a bottleneck or memory hotspot, consider using a static frame buffer for m_records, the kind of datastructure
        // that runs on pre-allocated memory and resets it every frame instead of frees it, to avoid needing to delete.

        typedef AbstractValue::ValueT<T> ValueType;

        uint32_t objectPos = m_objectCounter;
        AbstractValue::BaseValue* baseValue = m_records.size() > m_objectCounter ? m_records[m_objectCounter] : nullptr;
        ++m_objectCounter;

        // If we are in the gather records phase, just save off the value records
        if (m_gatheringRecords)
        {
            AZ_Assert(baseValue == nullptr, "Expected to create a new record but found a pre-existing one at index %d", m_objectCounter - 1);
            baseValue = new ValueType(value);
            m_records.push_back(baseValue);
        }
        else // If we are not gathering records, then we are comparing them
        {
            bool different = false;

            if (baseValue)
            {
                // This record must match the same type that was pushed into the list during the gathering phase
                ValueType* typedValue = static_cast<ValueType*>(baseValue);
                // Are the two values different?
                different = typedValue->GetValue() != value;
            }
            else
            {
                // No record? Then definitely different
                different = true;
            }

            // Record a bit to track this information
            if (!m_delta.InsertDirtyBit(different))
            {
                AZ_Assert(false, "Ran out of bits in DeltaSerializerCreate. You are probably trying to serialize an object with too many fields. Consider resizing the bitset in DeltaSerializerCreate");
                return false;
            }

            // If different, also write the data into the delta's buffer
            if (different)
            {
                if (!SerializeHelperImpl(value, bufferCapacity, isString, outSize, name))
                {
                    return false;
                }
            }

            // once we get here, we don't need the record anymore, so discard it while we know what the type is.
            delete static_cast<ValueType*>(baseValue);
            m_records[objectPos] = nullptr;
        }

        return true;
    }

    template <typename T>
    bool DeltaSerializerCreate::SerializeHelperImpl(T& value, uint32_t, bool, uint32_t&, const char* name)
    {
        ISerializer& ser = m_dataSerializer; // Use interface since it fills in defaulted type info parameters
        return ser.Serialize(value, name);
    }

    bool DeltaSerializerCreate::SerializeHelperImpl(uint8_t* buffer, uint32_t bufferCapacity, bool isString, uint32_t& outSize, const char* name)
    {
        ISerializer& ser = m_dataSerializer; // Use interface since it fills in defaulted type info parameters
        return ser.SerializeBytes(buffer, bufferCapacity, isString, outSize, name);
    }

    DeltaSerializerApply::DeltaSerializerApply(SerializerDelta& delta)
        : m_delta(delta)
        , m_dataSerializer(m_delta.GetBufferPtr(), m_delta.GetBufferSize())
    {
        ;
    }

    DeltaSerializerApply::~DeltaSerializerApply()
    {
        ;
    }

    SerializerMode DeltaSerializerApply::GetSerializerMode() const
    {
        return SerializerMode::WriteToObject;
    }

    bool DeltaSerializerApply::Serialize(bool& value, const char* name)
    {
        uint32_t unused = 0;
        return SerializeHelper(value, 0, false, unused, name);
    }

    bool DeltaSerializerApply::Serialize(int8_t& value, const char* name, [[maybe_unused]] int8_t minValue, [[maybe_unused]] int8_t maxValue)
    {
        uint32_t unused = 0;
        return SerializeHelper(value, 0, false, unused, name);
    }

    bool DeltaSerializerApply::Serialize(int16_t& value, const char* name, [[maybe_unused]] int16_t minValue, [[maybe_unused]] int16_t maxValue)
    {
        uint32_t unused = 0;
        return SerializeHelper(value, 0, false, unused, name);
    }

    bool DeltaSerializerApply::Serialize(int32_t& value, const char* name, [[maybe_unused]] int32_t minValue, [[maybe_unused]] int32_t maxValue)
    {
        uint32_t unused = 0;
        return SerializeHelper(value, 0, false, unused, name);
    }

    bool DeltaSerializerApply::Serialize(long& value, const char* name, [[maybe_unused]] long minValue, [[maybe_unused]] long maxValue)
    {
        uint32_t unused = 0;
        return SerializeHelper(value, 0, false, unused, name);
    }

    bool DeltaSerializerApply::Serialize(AZ::s64& value, const char* name, [[maybe_unused]] AZ::s64 minValue, [[maybe_unused]] AZ::s64 maxValue)
    {
        uint32_t unused = 0;
        return SerializeHelper(value, 0, false, unused, name);
    }

    bool DeltaSerializerApply::Serialize(uint8_t& value, const char* name, [[maybe_unused]] uint8_t minValue, [[maybe_unused]] uint8_t maxValue)
    {
        uint32_t unused = 0;
        return SerializeHelper(value, 0, false, unused, name);
    }

    bool DeltaSerializerApply::Serialize(uint16_t& value, const char* name, [[maybe_unused]] uint16_t minValue, [[maybe_unused]] uint16_t maxValue)
    {
        uint32_t unused = 0;
        return SerializeHelper(value, 0, false, unused, name);
    }

    bool DeltaSerializerApply::Serialize(uint32_t& value, const char* name, [[maybe_unused]] uint32_t minValue, [[maybe_unused]] uint32_t maxValue)
    {
        uint32_t unused = 0;
        return SerializeHelper(value, 0, false, unused, name);
    }

    bool DeltaSerializerApply::Serialize(unsigned long& value, const char* name, [[maybe_unused]] unsigned long minValue, [[maybe_unused]] unsigned long maxValue)
    {
        uint32_t unused = 0;
        return SerializeHelper(value, 0, false, unused, name);
    }

    bool DeltaSerializerApply::Serialize(AZ::u64& value, const char* name, [[maybe_unused]] AZ::u64 minValue, [[maybe_unused]] AZ::u64 maxValue)
    {
        uint32_t unused = 0;
        return SerializeHelper(value, 0, false, unused, name);
    }

    bool DeltaSerializerApply::Serialize(float& value, const char* name, [[maybe_unused]] float minValue, [[maybe_unused]] float maxValue)
    {
        uint32_t unused = 0;
        return SerializeHelper(value, 0, false, unused, name);
    }

    bool DeltaSerializerApply::Serialize(double& value, const char* name, [[maybe_unused]] double minValue, [[maybe_unused]] double maxValue)
    {
        uint32_t unused = 0;
        return SerializeHelper(value, 0, false, unused, name);
    }

    bool DeltaSerializerApply::SerializeBytes(uint8_t* buffer, uint32_t bufferCapacity, bool isString, uint32_t& outSize, const char* name)
    {
        return SerializeHelper(buffer, bufferCapacity, isString, outSize, name);
    }

    bool DeltaSerializerApply::BeginObject([[maybe_unused]] const char *name)
    {
        return true;
    }

    bool DeltaSerializerApply::EndObject([[maybe_unused]] const char *name)
    {
        return true;
    }

    const uint8_t* DeltaSerializerApply::GetBuffer() const
    {
        return nullptr;
    }

    uint32_t DeltaSerializerApply::GetCapacity() const
    {
        return 0;
    }

    uint32_t DeltaSerializerApply::GetSize() const
    {
        return 0;
    }

    template <typename T>
    bool DeltaSerializerApply::SerializeHelper(T& value, uint32_t bufferCapacity, bool isString, uint32_t& outSize, const char* name)
    {
        // If we have run out of delta records, something has gone wrong
        if (m_nextDirtyBit >= m_delta.GetNumDirtyBits())
        {
            return false;
        }

        const bool hasRecord = m_delta.GetDirtyBit(m_nextDirtyBit);
        ++m_nextDirtyBit;

        // No record in the delta for this field, just skip it
        if (!hasRecord)
        {
            return true; // This isn't an error
        }

        // There is a record, so serialize the value out of the delta
        return SerializeHelperImpl(value, bufferCapacity, isString, outSize, name);
    }

    template <typename T>
    bool DeltaSerializerApply::SerializeHelperImpl(T& value, uint32_t, bool, uint32_t&, const char* name)
    {
        ISerializer& ser = m_dataSerializer; // Use interface since it fills in defaulted type info parameters
        return ser.Serialize(value, name);
    }

    bool DeltaSerializerApply::SerializeHelperImpl(uint8_t* buffer, uint32_t bufferCapacity, bool isString, uint32_t& outSize, const char* name)
    {
        ISerializer& ser = m_dataSerializer; // Use interface since it fills in defaulted type info parameters
        return ser.SerializeBytes(buffer, bufferCapacity, isString, outSize, name);
    }
}
