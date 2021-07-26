/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GridMate/Serialize/MathMarshal.h>
#include <GridMate/Serialize/CompressionMarshal.h>
#include <AzCore/Casting/numeric_cast.h>                // for aznumeric_cast

using namespace GridMate;

//=========================================================================
// Vec2CompMarshaler::Marshal
//=========================================================================
void
Vec2CompMarshaler::Marshal(WriteBuffer& wb, const AZ::Vector2& vec) const
{
    // \todo use VMX vector packing
    HalfMarshaler mHalf;
    mHalf.Marshal(wb, vec.GetX());
    mHalf.Marshal(wb, vec.GetY());
}

//=========================================================================
// Vec2CompMarshaler::Unmarshal
//=========================================================================
void
Vec2CompMarshaler::Unmarshal(AZ::Vector2& vec, ReadBuffer& rb) const
{
    HalfMarshaler mHalf;
    float x, y;
    mHalf.Unmarshal(x, rb);
    mHalf.Unmarshal(y, rb);
    vec.Set(x, y);
}

//=========================================================================
// Vec3CompMarshaler::Marshal
//=========================================================================
void
Vec3CompMarshaler::Marshal(WriteBuffer& wb, const AZ::Vector3& vec) const
{
    // \todo use VMX vector packing
    HalfMarshaler mHalf;
    mHalf.Marshal(wb, vec.GetX());
    mHalf.Marshal(wb, vec.GetY());
    mHalf.Marshal(wb, vec.GetZ());
}

//=========================================================================
// Vec3CompMarshaler::Unmarshal
//=========================================================================
void
Vec3CompMarshaler::Unmarshal(AZ::Vector3& vec, ReadBuffer& rb) const
{
    HalfMarshaler mHalf;
    float x, y, z;
    mHalf.Unmarshal(x, rb);
    mHalf.Unmarshal(y, rb);
    mHalf.Unmarshal(z, rb);
    vec.Set(x, y, z);
}

//=========================================================================
// Vec3CompNormMarshaler::Marshal
//=========================================================================
void
Vec3CompNormMarshaler::Marshal(WriteBuffer& wb, const AZ::Vector3& norVec) const
{
    // \todo use VMX vector packing
    Float16Marshaler mF16(-1.0f, 1.0f);
    float x = norVec.GetX();
    float y = norVec.GetY();
    float z = norVec.GetZ();
    AZ::u8 flags = 0; // we waste 5 bits here. If WriteBuffer starts supporting bits this might be useful.

    if (x < 0.0f)
    {
        flags |= X_NEG;
    }

    if (y == 0.0f)
    {
        flags |= Y_ZERO;
    }
    else if (y == 1.0f)
    {
        flags |= Y_ONE;
    }

    if (z == 0.0f)
    {
        flags |= Z_ZERO;
    }
    else if (z == 1.0f)
    {
        flags |= Z_ONE;
    }

    wb.Write(flags);

    if ((flags & (Y_ZERO | Y_ONE)) == 0)
    {
        mF16.Marshal(wb, y);
    }

    if ((flags & (Z_ZERO | Z_ONE)) == 0)
    {
        mF16.Marshal(wb, z);
    }
}

//=========================================================================
// Vec3CompNormMarshaler::Unmarshal
//=========================================================================
void
Vec3CompNormMarshaler::Unmarshal(AZ::Vector3& vec, ReadBuffer& rb) const
{
    Float16Marshaler mF16(-1.0f, 1.0f);
    AZ::u8 flags;
    rb.Read(flags);

    float x, y, z;

    if (flags & Y_ZERO)
    {
        y = 0.0f;
    }
    else if (flags & Y_ONE)
    {
        y = 1.0f;
    }
    else
    {
        mF16.Unmarshal(y, rb);
    }

    if (flags & Z_ZERO)
    {
        z = 0.0f;
    }
    else if (flags & Z_ONE)
    {
        z = 1.0f;
    }
    else
    {
        mF16.Unmarshal(z, rb);
    }

    x = AZ::Sqrt(AZ::GetMax(0.0f, 1.0f - y * y - z * z));
    if (flags & X_NEG)
    {
        x = -x;
    }

    vec.Set(x, y, z);
}

//=========================================================================
// QuatCompMarshaler::Marshal
//=========================================================================
void
QuatCompMarshaler::Marshal(WriteBuffer& wb, const AZ::Quaternion& quat) const
{
    // \todo use VMX vector packing
    HalfMarshaler mHalf;
    mHalf.Marshal(wb, quat.GetX());
    mHalf.Marshal(wb, quat.GetY());
    mHalf.Marshal(wb, quat.GetZ());
    mHalf.Marshal(wb, quat.GetW());
}

//=========================================================================
// QuatCompMarshaler::Unmarshal
//=========================================================================
void
QuatCompMarshaler::Unmarshal(AZ::Quaternion& quat, ReadBuffer& rb) const
{
    HalfMarshaler mHalf;
    float x, y, z, w;
    mHalf.Unmarshal(x, rb);
    mHalf.Unmarshal(y, rb);
    mHalf.Unmarshal(z, rb);
    mHalf.Unmarshal(w, rb);
    quat.Set(x, y, z, w);
}

//=========================================================================
// QuatCompNormMarshaler::Marshal
//=========================================================================
void
QuatCompNormMarshaler::Marshal(WriteBuffer& wb, const AZ::Quaternion& norQuat) const
{
    AZ_Assert(AZ::IsClose(norQuat.GetLengthSq(), 1.0f, AZ::Constants::Tolerance), "Input quaternion is not normalized!");

    Float16Marshaler mF16(-1.0f, 1.0f);
    float x = norQuat.GetX();
    float y = norQuat.GetY();
    float z = norQuat.GetZ();
    float w = norQuat.GetW();
    AZ::u8 flags = 0; // we waste 7 bits here. If WriteBuffer starts supporting bits this might be useful.

    if (x == 0.0f)
    {
        flags |= X_ZERO;
    }
    else if (x == 1.0f)
    {
        flags |= X_ONE;
    }

    if (y == 0.0f)
    {
        flags |= Y_ZERO;
    }
    else if (y == 1.0f)
    {
        flags |= Y_ONE;
    }

    if (z == 0.0f)
    {
        flags |= Z_ZERO;
    }
    else if (z == 1.0f)
    {
        flags |= Z_ONE;
    }

    if (w < 0.0f)
    {
        flags |= W_NEG;
    }

    wb.Write(flags);

    if ((flags & (X_ZERO | X_ONE)) == 0)
    {
        mF16.Marshal(wb, x);
    }

    if ((flags & (Y_ZERO | Y_ONE)) == 0)
    {
        mF16.Marshal(wb, y);
    }

    if ((flags & (Z_ZERO | Z_ONE)) == 0)
    {
        mF16.Marshal(wb, z);
    }
}

//=========================================================================
// QuatCompNormMarshaler::Unmarshal
//=========================================================================
void
QuatCompNormMarshaler::Unmarshal(AZ::Quaternion& quat, ReadBuffer& rb) const
{
    Float16Marshaler mF16(-1.0f, 1.0f);
    AZ::u8 flags;
    rb.Read(flags);

    float x, y, z, w;

    if (flags & X_ZERO)
    {
        x = 0.0f;
    }
    else if (flags & X_ONE)
    {
        x = 1.0f;
    }
    else
    {
        mF16.Unmarshal(x, rb);
    }

    if (flags & Y_ZERO)
    {
        y = 0.0f;
    }
    else if (flags & Y_ONE)
    {
        y = 1.0f;
    }
    else
    {
        mF16.Unmarshal(y, rb);
    }

    if (flags & Z_ZERO)
    {
        z = 0.0f;
    }
    else if (flags & Z_ONE)
    {
        z = 1.0f;
    }
    else
    {
        mF16.Unmarshal(z, rb);
    }

    w = AZ::Sqrt(AZ::GetMax(0.f, 1.0f - x * x - y * y - z * z));
    if (flags & W_NEG)
    {
        w = -w;
    }

    quat.Set(x, y, z, w);
    quat.Normalize();
}

//=========================================================================
// QuatCompNormQuantizedMarshaler::Marshal
//=========================================================================
void
QuatCompNormQuantizedMarshaler::Marshal(WriteBuffer& wb, const AZ::Quaternion& norQuat) const
{
    AZ_Assert(AZ::IsClose(norQuat.GetLengthSq(), 1.0f, AZ::Constants::Tolerance), "Input quaternion is not normalized!");

    float x;
    float y;
    float z;
    AZ::Vector3 eulerAnglesInDeg;
    AZ::u8 tempQuantized;
    AZ::u8 flags = 0;

    eulerAnglesInDeg.CreateZero();
    eulerAnglesInDeg = norQuat.GetEulerDegrees();
    x = eulerAnglesInDeg.GetX();
    if (x == 0.0f)
    {
        flags |= X_ZERO;
    }
    else if (x == 1.0f)
    {
        flags |= X_ONE;
    }

    y = eulerAnglesInDeg.GetY();
    if (y == 0.0f)
    {
        flags |= Y_ZERO;
    }
    else if (y == 1.0f)
    {
        flags |= Y_ONE;
    }

    z = eulerAnglesInDeg.GetZ();
    if (z == 0.0f)
    {
        flags |= Z_ZERO;
    }
    else if (z == 1.0f)
    {
        flags |= Z_ONE;
    }

    wb.Write(flags);

    if ((flags & (X_ZERO | X_ONE)) == 0)
    {
        // quantize value into a byte and clamp to the max
        tempQuantized = aznumeric_cast<AZ::u8>(AZStd::GetMin(255.f, x / kDegreesPerQuantizedValue));
        wb.Write(tempQuantized);
    }

    if ((flags & (Y_ZERO | Y_ONE)) == 0)
    {
        // quantize value into a byte and clamp to the max
        tempQuantized = aznumeric_cast<AZ::u8>(AZStd::GetMin(255.f, y / kDegreesPerQuantizedValue));
        wb.Write(tempQuantized);
    }

    if ((flags & (Z_ZERO | Z_ONE)) == 0)
    {
        // quantize value into a byte and clamp to the max
        tempQuantized = aznumeric_cast<AZ::u8>(AZStd::GetMin(255.f, z / kDegreesPerQuantizedValue));
        wb.Write(tempQuantized);
    }
}

//=========================================================================
// QuatCompNormQuantizedMarshaler::Unmarshal
//=========================================================================
void
QuatCompNormQuantizedMarshaler::Unmarshal(AZ::Quaternion& quat, ReadBuffer& rb) const
{
    AZ::u8 quantizedAngleFromNetwork;
    AZ::s16 quantizedAngle;
    AZ::Vector3 eulerAnglesInDeg;
    AZ::u8 flags = 0;

    rb.Read(flags);

    eulerAnglesInDeg.CreateZero();

    // process angle X
    if (flags & X_ZERO)
    {
        quantizedAngle = 0;
    }
    else if (flags & X_ONE)
    {
        quantizedAngle = 1;
    }
    else
    {
        if (!rb.Read(quantizedAngleFromNetwork))
        {
            AZ_TracePrintf("GridMate", "Error unmarshaling X angle for QuatCompNormQuantizedMarshaler! Aborting Unmarshal!\n");
            return;
        }
        quantizedAngle = quantizedAngleFromNetwork;
    }

    eulerAnglesInDeg.SetX(quantizedAngle * kDegreesPerQuantizedValue);

    // process angle Y
    if (flags & Y_ZERO)
    {
        quantizedAngle = 0;
    }
    else if (flags & Y_ONE)
    {
        quantizedAngle = 1;
    }
    else
    {
        if (!rb.Read(quantizedAngleFromNetwork))
        {
            AZ_TracePrintf("GridMate", "Error unmarshaling Y angle for QuatCompNormQuantizedMarshaler! Aborting Unmarshal!\n");
            return;
        }
        quantizedAngle = quantizedAngleFromNetwork;
    }

    eulerAnglesInDeg.SetY(quantizedAngle * kDegreesPerQuantizedValue);

    // process angle Z
    if (flags & Z_ZERO)
    {
        quantizedAngle = 0;
    }
    else if (flags & Z_ONE)
    {
        quantizedAngle = 1;
    }
    else
    {
        if (!rb.Read(quantizedAngleFromNetwork))
        {
            AZ_TracePrintf("GridMate", "Error unmarshaling Z angle for QuatCompNormQuantizedMarshaler! Aborting Unmarshal!\n");
            return;
        }
        quantizedAngle = quantizedAngleFromNetwork;
    }

    eulerAnglesInDeg.SetZ(quantizedAngle * kDegreesPerQuantizedValue);
    quat.SetFromEulerDegrees(eulerAnglesInDeg);
    quat.Normalize();
}

//=========================================================================
// Float16Marshaler
//=========================================================================
Float16Marshaler::Float16Marshaler(float rangeMin, float rangeMax)
    : m_min(rangeMin)
    , m_range(rangeMax - rangeMin)
{
    AZ_Assert(rangeMax > rangeMin, "rangeMax MUST be > than rangeMin");
}

//=========================================================================
// TransformCompressor::Marshal
//=========================================================================
void TransformCompressor::Marshal(WriteBuffer& wb, const AZ::Transform& value) const
{
    AZ::u8 flags = 0;
    auto flagsMarker = wb.InsertMarker(flags);
    float scale = value.GetUniformScale();
    AZ::Quaternion rot = value.GetRotation();
    if (!rot.IsIdentity())
    {
        flags |= HAS_ROT;
        wb.Write(rot, QuatCompMarshaler());
    }
    if (!AZ::IsClose(scale, 1.0f, AZ::Constants::Tolerance))
    {
        flags |= HAS_SCALE;
        wb.Write(scale, HalfMarshaler());
    }
    AZ::Vector3 pos = value.GetTranslation();
    if (!pos.IsZero())
    {
        flags |= HAS_POS;
        wb.Write(pos);
    }

    flagsMarker.SetData(flags);
}

//=========================================================================
// TransformCompressor::Unmarshal
//=========================================================================
void TransformCompressor::Unmarshal(AZ::Transform& value, ReadBuffer& rb) const
{
    AZ::u8 flags = 0;
    rb.Read(flags);
    AZ::Transform xform = AZ::Transform::CreateIdentity();
    if (flags & HAS_ROT)
    {
        AZ::Quaternion rot;
        rb.Read(rot, QuatCompMarshaler());
        xform.SetRotation(rot);
    }
    if (flags & HAS_SCALE)
    {
        float scale;
        rb.Read(scale, HalfMarshaler());
        xform.MultiplyByUniformScale(scale);
    }
    if (flags & HAS_POS)
    {
        AZ::Vector3 pos;
        rb.Read(pos);
        xform.SetTranslation(pos);
    }
    value = xform;
}

//=========================================================================
// Float16Marshaler::Marshal
//=========================================================================
void
Float16Marshaler::Marshal(WriteBuffer& wb, float value) const
{
    AZ_Assert(value >= (m_min - AZ::Constants::FloatEpsilon) && value <= (m_min + m_range + AZ::Constants::FloatEpsilon), "Data is outside the range!");
    AZ::u16 data = static_cast<AZ::u16>(AZ::GetClamp(65535.0f * (value - m_min) / m_range, 0.0f, 65535.0f));
    wb.Write(data);
}

//=========================================================================
// Float16Marshaler::Unmarshal
//=========================================================================
void
Float16Marshaler::Unmarshal(float& f, ReadBuffer& rb) const
{
    AZ::u16 data;
    rb.Read(data);
    f = AZ::GetClamp(m_min + (static_cast<float>(data) / 65535.0f) * m_range, m_min, m_min + m_range);
}

union Data32Bits
{
    float    fValue;
    AZ::u32    u32Value;
};

//=========================================================================
// HalfMarshaler::Marshal
//
// Filename:    ieeehalfprecision.c
// Programmer:  James Tursa
// Version:     1.0
// Date:        March 3, 2009
//
// \todo Move this to AZBase if other people can use them.
// \todo If used often optimize with vmx and use internal float16 format (like the vector pack/unpack unstructions on PowerPC)
//=========================================================================
void
HalfMarshaler::Marshal(WriteBuffer& wb, float value) const
{
    typedef AZ::u16 UINT16_TYPE;
    AZ::u16 r, hs, he, hm;
    AZ::u32 x, xs, xe, xm;
    int hes;
    Data32Bits data;
    data.fValue = value;
    x = data.u32Value;
    if ((x & 0x7FFFFFFFu) == 0)    // Signed zero
    {
        r = (UINT16_TYPE)(x >> 16);  // Return the signed zero
    }
    else   // Not zero
    {
        xs = x & 0x80000000u;  // Pick off sign bit
        xe = x & 0x7F800000u;  // Pick off exponent bits
        xm = x & 0x007FFFFFu;  // Pick off mantissa bits
        if (xe == 0)    // Denormal will underflow, return a signed zero
        {
            r = (UINT16_TYPE)(xs >> 16);
        }
        else if (xe == 0x7F800000u)    // Inf or NaN (all the exponent bits are set)
        {
            if (xm == 0)   // If mantissa is zero ...
            {
                r = (UINT16_TYPE)((xs >> 16) | 0x7C00u); // Signed Inf
            }
            else
            {
                r = (UINT16_TYPE)0xFE00u; // NaN, only 1st mantissa bit set
            }
        }
        else   // Normalized number
        {
            hs = (UINT16_TYPE)(xs >> 16); // Sign bit
            hes = ((int)(xe >> 23)) - 127 + 15; // Exponent unbias the single, then bias the halfp
            if (hes >= 0x1F)    // Overflow
            {
                r = (UINT16_TYPE)((xs >> 16) | 0x7C00u); // Signed Inf
            }
            else if (hes <= 0)    // Underflow
            {
                if ((14 - hes) > 24)    // Mantissa shifted all the way off & no rounding possibility
                {
                    hm = (UINT16_TYPE)0u;  // Set mantissa to zero
                }
                else
                {
                    xm |= 0x00800000u;  // Add the hidden leading bit
                    hm = (UINT16_TYPE)(xm >> (14 - hes)); // Mantissa
                    if ((xm >> (13 - hes)) & 0x00000001u) // Check for rounding
                    {
                        hm += (UINT16_TYPE)1u; // Round, might overflow into exp bit, but this is OK
                    }
                }
                r = (hs | hm); // Combine sign bit and mantissa bits, biased exponent is zero
            }
            else
            {
                he = (UINT16_TYPE)(hes << 10); // Exponent
                hm = (UINT16_TYPE)(xm >> 13); // Mantissa
                if (xm & 0x00001000u) // Check for rounding
                {
                    r = (hs | he | hm) + (UINT16_TYPE)1u; // Round, might overflow to inf, this is OK
                }
                else
                {
                    r = (hs | he | hm);  // No rounding
                }
            }
        }
    }

    wb.Write(r);
}

//=========================================================================
// HalfMarshaler::Unmarshal
//
// Filename:    ieeehalfprecision.c
// Programmer:  James Tursa
// Version:     1.0
// Date:        March 3, 2009
//
// [12/9/2010]
//=========================================================================
void
HalfMarshaler::Unmarshal(float& f, ReadBuffer& rb) const
{
    typedef AZ::u32 UINT32_TYPE;
    typedef AZ::s32 INT32_TYPE;
    AZ::u16 h, hs, he, hm;
    AZ::u32 r, xs, xe, xm;
    AZ::u32 xes;
    int e;
    rb.Read(h);
    if ((h & 0x7FFFu) == 0)    // Signed zero
    {
        r = ((UINT32_TYPE)h) << 16;  // Return the signed zero
    }
    else   // Not zero
    {
        hs = h & 0x8000u;  // Pick off sign bit
        he = h & 0x7C00u;  // Pick off exponent bits
        hm = h & 0x03FFu;  // Pick off mantissa bits
        if (he == 0)    // Denormal will convert to normalized
        {
            e = -1; // The following loop figures out how much extra to adjust the exponent
            do
            {
                e++;
                hm <<= 1;
            } while ((hm & 0x0400u) == 0); // Shift until leading bit overflows into exponent bit
            xs = ((UINT32_TYPE)hs) << 16; // Sign bit
            xes = ((INT32_TYPE)(he >> 10)) - 15 + 127 - e; // Exponent unbias the halfp, then bias the single
            xe = (UINT32_TYPE)(xes << 23); // Exponent
            xm = ((UINT32_TYPE)(hm & 0x03FFu)) << 13; // Mantissa
            r = (xs | xe | xm); // Combine sign bit, exponent bits, and mantissa bits
        }
        else if (he == 0x7C00u)    // Inf or NaN (all the exponent bits are set)
        {
            if (hm == 0)   // If mantissa is zero ...
            {
                r = (((UINT32_TYPE)hs) << 16) | ((UINT32_TYPE)0x7F800000u); // Signed Inf
            }
            else
            {
                r = (UINT32_TYPE)0xFFC00000u; // NaN, only 1st mantissa bit set
            }
        }
        else   // Normalized number
        {
            xs = ((UINT32_TYPE)hs) << 16; // Sign bit
            xes = ((INT32_TYPE)(he >> 10)) - 15 + 127; // Exponent unbias the halfp, then bias the single
            xe = (UINT32_TYPE)(xes << 23); // Exponent
            xm = ((UINT32_TYPE)hm) << 13; // Mantissa
            r = (xs | xe | xm); // Combine sign bit, exponent bits, and mantissa bits
        }
    }
    Data32Bits data;
    data.u32Value = r;
    f = data.fValue;
}
