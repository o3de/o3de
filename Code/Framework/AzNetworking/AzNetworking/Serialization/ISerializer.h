/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <stdint.h>
#include <AzCore/base.h>
#include <AzCore/std/limits.h>

namespace AzNetworking
{
    class IBitset;

    enum class SerializerMode
    {
        ReadFromObject,
        WriteToObject
    };

    //! @class ISerializer
    //! @brief Interface class for all serializers to derive from.
    //!
    //! ISerializer defines an abstract interface for visiting an object hierarchy and performing operations upon that hierarchy,
    //! typically reading from or writing data to the object hierarchy for reasons of persistence or network transmission.
    //! 
    //! While the most common types of serializers are provided by the AzNetworking framework, users can implement custom
    //! serializers and perform complex operations on any serializable structures. A few types native to AzNetworking, many of which
    //! relate to packets, demonstrate this.
    //! 
    //! Provided serializers include NetworkInputSerializer for writing an object model into a bytestream, NetworkOutputSerializer
    //! for writing to an object model, TrackChangesSerializer which is used to efficiently serialize objects without incurring significant
    //! copy or comparison overhead, and HashSerializer which can be used to generate a hash of all visited data which is important for
    //! automated desync detection. 
    class ISerializer
    {
    public:

        ISerializer() = default;
        virtual ~ISerializer() = default;

        //! Returns true if the serializer is valid and in a consistent state.
        //! @return boolean true if the serializer is valid and in a consistent state
        virtual bool IsValid() const;

        //! Mark the serializer as invalid.
        void Invalidate();

        //! Returns an enum the represents the serializer mode.
        //! returns WriteToObject if the serializer is writing values to the objects it visits, otherwise returns ReadFromObject
        //! @return boolean true if the serializer is writing to objects that it visits
        virtual SerializerMode GetSerializerMode() const = 0;

        //! Serialize a boolean.
        //! @param value    boolean input value to serialize
        //! @param name     string name of the value being serialized
        //! @return boolean true for success, false for serialization failure
        virtual bool Serialize(bool& value, const char* name) = 0;

        //! Serialize a character.
        //! @param value    character input value to serialize
        //! @param name     string name of the value being serialized
        //! @param minValue the minimum value expected during serialization
        //! @param maxValue the maximum value expected during serialization
        //! @return boolean true for success, false for failure
        virtual bool Serialize(char& value, const char* name, char minValue = AZStd::numeric_limits<char>::min(), char maxValue = AZStd::numeric_limits<char>::max()) = 0;

        //! Serialize a signed byte.
        //! @param value    signed byte input value to serialize
        //! @param name     string name of the value being serialized
        //! @param minValue the minimum value expected during serialization
        //! @param maxValue the maximum value expected during serialization
        //! @return boolean true for success, false for failure
        virtual bool Serialize(int8_t& value, const char* name, int8_t minValue = AZStd::numeric_limits<int8_t>::min(), int8_t maxValue = AZStd::numeric_limits<int8_t>::max()) = 0;

        //! Serialize a signed short.
        //! @param value    signed short input value to serialize
        //! @param name     string name of the value being serialized
        //! @param minValue the minimum value expected during serialization
        //! @param maxValue the maximum value expected during serialization
        //! @return boolean true for success, false for failure
        virtual bool Serialize(int16_t& value, const char* name, int16_t minValue = AZStd::numeric_limits<int16_t>::min(), int16_t maxValue = AZStd::numeric_limits<int16_t>::max()) = 0;

        //! Serialize a signed integer.
        //! @param value    signed integer input value to serialize
        //! @param name     string name of the value being serialized
        //! @param minValue the minimum value expected during serialization
        //! @param maxValue the maximum value expected during serialization
        //! @return boolean true for success, false for failure
        virtual bool Serialize(int32_t& value, const char* name, int32_t minValue = AZStd::numeric_limits<int32_t>::min(), int32_t maxValue = AZStd::numeric_limits<int32_t>::max()) = 0;

        //! Serialize a signed 64-bit integer.
        //! @param value    signed 64-bit integer input value to serialize
        //! @param name     string name of the value being serialized
        //! @param minValue the minimum value expected during serialization
        //! @param maxValue the maximum value expected during serialization
        //! @return boolean true for success, false for failure
        virtual bool Serialize(int64_t& value, const char* name, int64_t minValue = AZStd::numeric_limits<int64_t>::min(), int64_t maxValue = AZStd::numeric_limits<int64_t>::max()) = 0;

        //! Serialize a signed 64-bit integer.
        //! @param value    signed 64-bit integer input value to serialize
        //! @param name     string name of the value being serialized
        //! @param minValue the minimum value expected during serialization
        //! @param maxValue the maximum value expected during serialization
        //! @return boolean true for success, false for failure
        #if AZ_TRAIT_COMPILER_INT64_T_IS_LONG
        bool Serialize(AZ::s64& value, const char* name, AZ::s64 minValue = AZStd::numeric_limits<AZ::s64>::min(), AZ::s64 maxValue = AZStd::numeric_limits<AZ::s64>::max());
        #endif

        //! Serialize an unsigned byte.
        //! @param value    unsigned byte input value to serialize
        //! @param name     string name of the value being serialized
        //! @param minValue the minimum value expected during serialization
        //! @param maxValue the maximum value expected during serialization
        //! @return boolean true for success, false for failure
        virtual bool Serialize(uint8_t& value, const char* name, uint8_t minValue = AZStd::numeric_limits<uint8_t>::min(), uint8_t maxValue = AZStd::numeric_limits<uint8_t>::max()) = 0;

        //! Serialize an unsigned short.
        //! @param value    signed integer short value to serialize
        //! @param name     string name of the value being serialized
        //! @param minValue the minimum value expected during serialization
        //! @param maxValue the maximum value expected during serialization
        //! @return boolean true for success, false for failure
        virtual bool Serialize(uint16_t& value, const char* name, uint16_t minValue = AZStd::numeric_limits<uint16_t>::min(), uint16_t maxValue = AZStd::numeric_limits<uint16_t>::max()) = 0;

        //! Serialize an unsigned integer.
        //! @param value    signed integer input value to serialize
        //! @param name     string name of the value being serialized
        //! @param minValue the minimum value expected during serialization
        //! @param maxValue the maximum value expected during serialization
        //! @return boolean true for success, false for failure
        virtual bool Serialize(uint32_t& value, const char* name, uint32_t minValue = AZStd::numeric_limits<uint32_t>::min(), uint32_t maxValue = AZStd::numeric_limits<uint32_t>::max()) = 0;

        //! Serialize an unsigned 64-bit integer.
        //! @param value    signed 64-bit integer input value to serialize
        //! @param name     string name of the value being serialized
        //! @param minValue the minimum value expected during serialization
        //! @param maxValue the maximum value expected during serialization
        //! @return boolean true for success, false for failure
        virtual bool Serialize(uint64_t& value, const char* name, uint64_t minValue = AZStd::numeric_limits<uint64_t>::min(), uint64_t maxValue = AZStd::numeric_limits<uint64_t>::max()) = 0;

        //! Serialize an unsigned 64-bit integer.
        //! @param value    signed 64-bit integer input value to serialize
        //! @param name     string name of the value being serialized
        //! @param minValue the minimum value expected during serialization
        //! @param maxValue the maximum value expected during serialization
        //! @return boolean true for success, false for failure
        #if AZ_TRAIT_COMPILER_INT64_T_IS_LONG
        bool Serialize(AZ::u64& value, const char* name, AZ::u64 minValue = AZStd::numeric_limits<AZ::u64>::min(), AZ::u64 maxValue = AZStd::numeric_limits<AZ::u64>::max());
        #endif

        //! Serialize a 32-bit floating point number.
        //! @param value    32-bit floating point input value to serialize
        //! @param name     string name of the value being serialized
        //! @param minValue the minimum value expected during serialization
        //! @param maxValue the maximum value expected during serialization
        //! @return boolean true for success, false for failure
        virtual bool Serialize(float& value, const char* name, float minValue = AZStd::numeric_limits<float>::min(), float maxValue = AZStd::numeric_limits<float>::max()) = 0;

        //! Serialize a 64-bit floating point number.
        //! @param value    64-bit floating point input value to serialize
        //! @param name     string name of the value being serialized
        //! @param minValue the minimum value expected during serialization
        //! @param maxValue the maximum value expected during serialization
        //! @return boolean true for success, false for failure
        virtual bool Serialize(double& value, const char* name, double minValue = AZStd::numeric_limits<double>::min(), double maxValue = AZStd::numeric_limits<double>::max()) = 0;

        //! Serialize a raw set of bytes.
        //! @param buffer         buffer to serialize
        //! @param bufferCapacity size of the buffer
        //! @param isString       true if the data being serialized is a string
        //! @param outSize        bytes serialized
        //! @param name           string name of the object
        //! @return boolean true for success, false for serialization failure
        virtual bool SerializeBytes(uint8_t* buffer, uint32_t bufferCapacity, bool isString, uint32_t& outSize, const char* name) = 0;

        //! Serialize interface for deducing whether or not TYPE is an enum or an object.
        //! @param value    object instance to serialize
        //! @param name     string name of the object
        //! @return boolean true for success, false for serialization failure
        template <typename TYPE>
        bool Serialize(TYPE& value, const char* name);

        //! Begins serializing an object.
        //! @param name     string name of the object
        //! @return boolean true on success, false for failure
        virtual bool BeginObject(const char* name) = 0;

        //! Ends serializing an object.
        //! @param name     string name of the object
        //! @return boolean true on success, false for failure
        virtual bool EndObject(const char* name) = 0;

        //! Returns a pointer to the internal serialization buffer.
        //! @return pointer to the internal serialization buffer
        virtual const uint8_t* GetBuffer() const = 0;

        //! Returns the total capacity serialization buffer in bytes.
        //! @return total capacity serialization buffer in bytes
        virtual uint32_t GetCapacity() const = 0;

        //! Returns the size of the data contained in the serialization buffer in bytes.
        //! @return size of the data contained in the serialization buffer in bytes
        virtual uint32_t GetSize() const = 0;

        //! This is a helper for network serialization.
        //! It clears the track changes flag internal to some serializers
        virtual void ClearTrackedChangesFlag() = 0;

        //! This is a helper for network serialization.
        //! It allows the owner of the serializer to query whether or not the serializer modified the state of an object during serialization
        //! @return boolean true if the track changes flag is raised
        virtual bool GetTrackedChangesFlag() const = 0;

    protected:

        template <bool IsEnum, bool IsTypeSafeIntegral>
        struct SerializeHelper;

        bool m_serializerValid = true; //< Here for performance reasons
    };
}

#include <AzNetworking/Serialization/ISerializer.inl>
