/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_UTILS_DATA_MARSHAL
#define GM_UTILS_DATA_MARSHAL

#include <GridMate/GridMate.h>
#include <GridMate/Serialize/Buffer.h>
#include <GridMate/Serialize/MarshalerTypes.h>

#include <AzCore/std/containers/bitset.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/typetraits/underlying_type.h>

namespace GridMate
{
    template<typename T>
    inline void InPlaceNetworkEndian(T& data, EndianType endianType)
    {
        (void) data;

        switch (endianType)
        {
        case EndianType::BigEndian:
            AZStd::endian_swap(data);
            break;

        case EndianType::LittleEndian:
            break;

        case EndianType::IgnoreEndian:
            break;
        }
    }

    template<typename Type>
    struct IsFundamentalMarshalType
    {
        enum
        {
            Value =
                AZStd::is_same<Type, float>::value   ||
                AZStd::is_same<Type, double>::value  ||
                AZStd::is_same<Type, char>::value    ||
                AZStd::is_same<Type, signed char>::value ||
                AZStd::is_same<Type, unsigned char>::value ||
                AZStd::is_same<Type, AZ::u8>::value  ||
                AZStd::is_same<Type, AZ::u16>::value ||
                AZStd::is_same<Type, AZ::u32>::value ||
                AZStd::is_same<Type, AZ::u64>::value ||
                AZStd::is_same<Type, AZ::s8>::value  ||
                AZStd::is_same<Type, AZ::s16>::value ||
                AZStd::is_same<Type, AZ::s32>::value ||
                AZStd::is_same<Type, AZ::s64>::value
        };
    };

    /**
        Fundamental type marshaler. All fundamental types except bool are written here. They
        are endian swapped if necessary, and written raw directly to the buffer.
    **/
    template<typename Type>
    class Marshaler<Type, typename AZStd::Utils::enable_if_c<IsFundamentalMarshalType<Type>::Value>::type>
    {
    public:
        AZ_TYPE_INFO_LEGACY(Marshaler, "{1AE954E2-67E8-4EDC-A16A-577411F5B876}", Type);
        typedef Type DataType;

        static const AZStd::size_t MarshalSize = sizeof(DataType);

        AZ_FORCE_INLINE void Marshal(WriteBuffer& wb, const DataType& value) const
        {
            DataType temp = value;
            InPlaceNetworkEndian<DataType>(temp, wb.GetEndianType());
            wb.WriteRaw(&temp, sizeof(temp));
        }
        AZ_FORCE_INLINE void Unmarshal(DataType& value, ReadBuffer& rb) const
        {
            if (rb.ReadRaw(&value, sizeof(value)))
            {
                InPlaceNetworkEndian<DataType>(value, rb.GetEndianType());
            }
        }
    };

    /**
        Bool marshaler. This converts the bool to a u8, which is the smallest size written to the network.
        Since this isn't space efficient, if you have multiple flags to write to the same buffer, consider
        switching to a AZStd::bitset instead.
    **/
    template<>
    class Marshaler<bool>
    {
    public:
        AZ_TYPE_INFO_LEGACY(Marshaler, "{8F3A6078-DE15-4D3F-8795-6FFAF1275AF1}", bool);

        typedef bool DataType;
        typedef bool SerializedType;

        AZ_FORCE_INLINE void Marshal(WriteBuffer& wb, const DataType& value) const
        {
            DataType temp = value;
            InPlaceNetworkEndian<DataType>(temp, wb.GetEndianType());
            wb.WriteRawBit(temp);
        }
        AZ_FORCE_INLINE void Unmarshal(DataType& value, ReadBuffer& rb) const
        {
            if (rb.ReadRawBit(value))
            {
                InPlaceNetworkEndian<DataType>(value, rb.GetEndianType());
            }
        }
    };

    /**
        Encodes an enum to the buffer. The size written to the stream uses the underlying size
        of the enum, so be sure to set this appropriately on marshaled enums.
    */
    template<typename EnumType>
    class Marshaler<EnumType, typename AZStd::Utils::enable_if_c<AZStd::is_enum<EnumType>::value>::type>
    {
    public:
        typedef EnumType DataType;
        typedef typename AZStd::underlying_type<EnumType>::type SerializedType;

        static const AZStd::size_t MarshalSize = sizeof(SerializedType);

        AZ_FORCE_INLINE void Marshal(WriteBuffer& wb, EnumType value) const
        {
            SerializedType serialized = static_cast<SerializedType>(value);
            wb.Write(serialized);
        }
        AZ_FORCE_INLINE void Unmarshal(EnumType& value, ReadBuffer& rb) const
        {
            SerializedType serialized;
            rb.Read(serialized);
            value = static_cast<EnumType>(serialized);
        }
    };
}

#endif // GM_UTILS_DATA_MARSHAL
#pragma once
