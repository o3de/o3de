/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_UTILS_UTILITY_MARSHAL
#define GM_UTILS_UTILITY_MARSHAL

#include <GridMate/GridMate.h>
#include <GridMate/Serialize/Buffer.h>
#include <GridMate/Serialize/MarshalerTypes.h>
#include <GridMate/Serialize/MathMarshal.h>

#include <AzCore/std/containers/bitset.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/typetraits/underlying_type.h>

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Obb.h>
#include <AzCore/Math/Plane.h>

namespace GridMate
{
    /**
        Converts from one type to another for serialization. Note this will truncate
        data if the data value exceeds the serializable size.
    */
    template<class SerializedType, class OriginalType>
    class ConversionMarshaler
    {
    public:
        typedef OriginalType DataType;

        static constexpr AZStd::size_t MarshalSize = sizeof(SerializedType);

        void Marshal(WriteBuffer& wb, const DataType& value) const
        {
            wb.Write(static_cast<SerializedType>(value));
        }
        void Unmarshal(DataType& value, ReadBuffer& rb) const
        {
            SerializedType word;
            if (rb.Read(word))
            {
                value = static_cast<DataType>(word);
            }
        }
    };

    /**
        Encodes a CRC32 into u32
    */
    template<>
    class Marshaler<AZ::Crc32>
        : public ConversionMarshaler<AZ::u32, AZ::Crc32>
    {
    };

    /**
        Writes a bitset to the stream.
    */
    template<AZStd::size_t Bits>
    class Marshaler<AZStd::bitset<Bits> >
    {
    public:
        typedef AZStd::bitset<Bits> DataType;

        AZ_FORCE_INLINE void Marshal(WriteBuffer& wb, const DataType& value) const
        {
            typename DataType::const_pointer data = value.data();
            for (AZStd::size_t i = 0; i < value.num_words(); ++i)
            {
                wb.Write(data[i]);
            }
        }

        AZ_FORCE_INLINE void Unmarshal(DataType& value, ReadBuffer& rb) const
        {
            typename DataType::pointer data = value.data();
            for (AZStd::size_t i = 0; i < value.num_words(); ++i)
            {
                rb.Read(data[i]);
            }
        }
    };

    /**
        Writes a pair to the stream. Assumes each member of the pair has an appropriate
        marshaler defined.
    */
    template<class T1, class T2>
    class Marshaler<AZStd::pair<T1, T2> >
    {
    public:
        typedef AZStd::pair<T1, T2> DataType;

        AZ_FORCE_INLINE void Marshal(WriteBuffer& wb, const DataType& value) const
        {
            wb.Write(value.first);
            wb.Write(value.second);
        }

        AZ_FORCE_INLINE void Unmarshal(DataType& value, ReadBuffer& rb) const
        {
            rb.Read(value.first);
            rb.Read(value.second);
        }
    };

    /**
        Aabb Marshaler
    */
    template<>
    class Marshaler<AZ::Aabb>
    {
    public:
        typedef AZ::Aabb DataType;

        static constexpr AZStd::size_t MarshalSize = Marshaler<AZ::Vector3>::MarshalSize * 2;

        void Marshal(WriteBuffer& wb, const DataType& aabb) const
        {
            Marshaler<AZ::Vector3> marshaler;
            marshaler.Marshal(wb, aabb.GetMin());
            marshaler.Marshal(wb, aabb.GetMax());
        }
        void Unmarshal(DataType& aabb, ReadBuffer& rb) const
        {
            Marshaler<AZ::Vector3> marshaler;

            AZ::Vector3 min;
            marshaler.Unmarshal(min, rb);

            AZ::Vector3 max;
            marshaler.Unmarshal(max, rb);
            aabb.Set(min, max);
        }
    };

    /**
        Obb Marshaler
    */
    template<>
    class Marshaler<AZ::Obb>
    {
    public:
        typedef AZ::Obb DataType;

        static constexpr AZStd::size_t MarshalSize = Marshaler<AZ::Vector3>::MarshalSize * 2 + Marshaler<AZ::Quaternion>::MarshalSize;

        void Marshal(WriteBuffer& wb, const DataType& obb) const
        {
            Marshaler<AZ::Vector3> vector3Marshaler;
            vector3Marshaler.Marshal(wb, obb.GetPosition());
            vector3Marshaler.Marshal(wb, obb.GetHalfLengths());

            Marshaler<AZ::Quaternion> quaternionMarshaler;
            quaternionMarshaler.Marshal(wb, obb.GetRotation());
        }
        void Unmarshal(DataType& obb, ReadBuffer& rb) const
        {
            Marshaler<AZ::Vector3> vector3Marshaler;

            AZ::Vector3 position;
            vector3Marshaler.Unmarshal(position, rb);
            obb.SetPosition(position);

            AZ::Vector3 halfLengths;
            vector3Marshaler.Unmarshal(halfLengths, rb);
            obb.SetHalfLengths(halfLengths);

            Marshaler<AZ::Quaternion> quaternionMarshaler;

            AZ::Quaternion rotation;
            quaternionMarshaler.Unmarshal(rotation, rb);
            obb.SetRotation(rotation);
        }
    };

    /**
    Plane Marshaler
    */
    template<>
    class Marshaler<AZ::Plane>
    {
    public:
        typedef AZ::Plane DataType;

        static constexpr AZStd::size_t MarshalSize = Marshaler<AZ::Vector4>::MarshalSize;

        void Marshal(WriteBuffer& wb, const DataType& plane) const
        {
            Marshaler<AZ::Vector4> marshaler;
            marshaler.Marshal(wb, plane.GetPlaneEquationCoefficients());
        }
        void Unmarshal(DataType& plane, ReadBuffer& rb) const
        {
            Marshaler<AZ::Vector4> marshaler;

            AZ::Vector4 coefficients;
            marshaler.Unmarshal(coefficients, rb);
            plane.Set(coefficients);
        }
    };

    /**
    * Time Marshaler
    * Writes a specific Time duration as a 32 bit unsigned int
    */
    template<class Rep, class Period>
    class Marshaler<AZStd::chrono::duration<Rep, Period> >
    {
    public:
        typedef AZStd::chrono::duration<Rep, Period> Duration;

        static constexpr AZStd::size_t MarshalSize = sizeof(AZ::u32);

        void Marshal(WriteBuffer& wb, Duration timeDuration) const
        {
            AZStd::chrono::milliseconds tempTime = AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(timeDuration);
            wb.Write(static_cast<AZ::u32>(tempTime.count()));
        }

        void Unmarshal(Duration& timeDuration, ReadBuffer& rb) const
        {
            AZ::u32 timePeriod;
            rb.Read(timePeriod);
            AZStd::chrono::milliseconds timeValue(timePeriod);
            timeDuration = AZStd::chrono::duration_cast<Duration>(timeValue);
        }
    };
}

namespace AZ
{
    AZ_TYPE_INFO_TEMPLATE_WITH_NAME(GridMate::ConversionMarshaler, "ConversionMarshaler", "{BC451E40-837C-46F2-B73D-47ADCD3AC42D}", AZ_TYPE_INFO_CLASS, AZ_TYPE_INFO_CLASS);
}

#endif //GM_UTILS_UTILITY_MARSHAL

#pragma once
