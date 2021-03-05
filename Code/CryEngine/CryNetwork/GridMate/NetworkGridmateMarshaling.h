/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#ifndef INCLUDE_NETWORKGRIDMATEMARSHALING_HEADER
#define INCLUDE_NETWORKGRIDMATEMARSHALING_HEADER

#pragma once

#include <Cry_Vector2.h>
#include <Cry_Vector3.h>
#include <Cry_Quat.h>
#include <TimeValue.h>

#include <GridMate/Serialize/Buffer.h>
#include <GridMate/Serialize/DataMarshal.h>

struct ILevelInfo;

namespace GridMate
{
    /*!
     * Basic marshaller for CryEngine versions of string, stack_string, etc.
     */
    template<typename T>
    class CryStringMarshalerBase
    {
    public:

        AZ_FORCE_INLINE void Marshal(WriteBuffer& wb, const T& str)
        {
            const uint16 size = static_cast<uint16>(str.length());
            wb.Write(size);
            wb.WriteRaw(str.c_str(), size);
        }

        AZ_FORCE_INLINE void Unmarshal(T& str, ReadBuffer& rb)
        {
            uint16 size = 0;
            rb.Read(size);

            str.resize(size);
            char* dest = const_cast<char*>(str.c_str());
            rb.ReadRaw(dest, size);
        }
    };

    /*!
     * Default marshaler for 2D vectors
     */
    template<>
    class Marshaler < Vec2 >
    {
    public:

        AZ_FORCE_INLINE void Marshal(WriteBuffer& wb, const Vec2& v)
        {
            wb.Write(v.x);
            wb.Write(v.y);
        }

        AZ_FORCE_INLINE void Unmarshal(Vec2& v, ReadBuffer& rb)
        {
            rb.Read(v.x);
            rb.Read(v.y);
        }
    };

    /*!
     * Default marshaler for 3D vectors
     */
    template<>
    class Marshaler < Vec3 >
    {
    public:

        AZ_FORCE_INLINE void Marshal(WriteBuffer& wb, const Vec3& v)
        {
            wb.Write(v.x);
            wb.Write(v.y);
            wb.Write(v.z);
        }

        AZ_FORCE_INLINE void Unmarshal(Vec3& v, ReadBuffer& rb)
        {
            rb.Read(v.x);
            rb.Read(v.y);
            rb.Read(v.z);
        }
    };

    /*!
     * Default marshaler for Angle-3s
     */
    template<>
    class Marshaler < Ang3 >
    {
    public:

        AZ_FORCE_INLINE void Marshal(WriteBuffer& wb, const Ang3& v)
        {
            wb.Write(v.x);
            wb.Write(v.y);
            wb.Write(v.z);
        }

        AZ_FORCE_INLINE void Unmarshal(Ang3& v, ReadBuffer& rb)
        {
            rb.Read(v.x);
            rb.Read(v.y);
            rb.Read(v.z);
        }
    };

    /*!
     * Default marshaler for Quats
     */
    template<>
    class Marshaler < Quat >
    {
    public:

        AZ_FORCE_INLINE void Marshal(WriteBuffer& wb, const Quat& v)
        {
            wb.Write(v.v.x);
            wb.Write(v.v.y);
            wb.Write(v.v.z);
            wb.Write(v.w);
        }

        AZ_FORCE_INLINE void Unmarshal(Quat& v, ReadBuffer& rb)
        {
            rb.Read(v.v.x);
            rb.Read(v.v.y);
            rb.Read(v.v.z);
            rb.Read(v.w);
        }
    };

    /*!
     * Default marshaler for time stamps.
     */
    template<>
    class Marshaler < CTimeValue >
    {
    public:

        AZ_FORCE_INLINE void Marshal(WriteBuffer& wb, const CTimeValue& v)
        {
            int64 temp = v.GetValue();
            wb.Write(temp);
        }

        AZ_FORCE_INLINE void Unmarshal(CTimeValue& v, ReadBuffer& rb)
        {
            int64 temp;
            rb.Read(temp);
            v.SetValue(temp);
        }
    };

    /*!
     * Default marshaler specializations for various engine string types.
     */
    class CryStringMarshaler
        : public CryStringMarshalerBase < CryStringT<char> >
    {
    };
    template<size_t Size>
    class CryFixedStringMarshaler
        : public CryStringMarshalerBase < CryFixedStringT<Size> >
    {
    };
    template<size_t Size>
    class CryStackStringMarshaler
        : public CryStringMarshalerBase < CryStackStringT<char, Size> >
    {
    };

    /*!
     * Unsupported marshaler. Right now this is just used for types that legacy engine defines
     * require compile-time serialization handlers for, but we don't actually desire to use.
     */
    template<typename T>
    class UnsupportedMarshaler
    {
    public:

        AZ_FORCE_INLINE void Marshal(WriteBuffer& wb, const T& v)
        {
            (void)wb;
            (void)v;

            CRY_ASSERT_MESSAGE(0, "Marshaling not valid for this type");
        }

        AZ_FORCE_INLINE void Unmarshal(T& v, ReadBuffer& rb)
        {
            (void)v;
            (void)rb;

            CRY_ASSERT_MESSAGE(0, "Marshaling not valid for this type");
        }
    };

    template<>
    class Marshaler<SNetObjectID>
        : public UnsupportedMarshaler < SNetObjectID >
    {
    };
    template<>
    class Marshaler<XmlNodeRef>
        : public UnsupportedMarshaler < XmlNodeRef >
    {
    };
} // namespace GridMate (marshalers)

#endif // INCLUDE_NETWORKGRIDMATEMARSHALING_HEADER
