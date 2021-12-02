/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <CryCommon/Cry_Math.h>

enum EVertexFormat : uint8
{
    eVF_Unknown,

    // Base stream
    eVF_P3F_C4B_T2F,
    eVF_P3S_C4B_T2S,

    // Additional streams
    eVF_W4B_I4S,  // Skinned weights/indices stream.
    eVF_P3F,       // Velocity stream.

    // Lens effects simulation

    eVF_P2F_C4B_T2F_F4B, // UI
    eVF_P3F_C4B,// Auxiliary geometry
    eVF_Max,
};

struct UCol
{
    union
    {
        uint32 dcolor;
        uint8  bcolor[4];

        struct
        {
            uint8 b, g, r, a;
        };
        struct
        {
            uint8 z, y, x, w;
        };
    };

    // get normal vector from unsigned 8bit integers (can't point up/down and is not normal)
    ILINE Vec3 GetN()
    {
        return Vec3
               (
            (bcolor[0] - 128.0f) / 127.5f,
            (bcolor[1] - 128.0f) / 127.5f,
            (bcolor[2] - 128.0f) / 127.5f
               );
    }
};

struct Vec3f16
    : public CryHalf4
{
    _inline Vec3f16()
    {
    }
    _inline Vec3f16(f32 _x, f32 _y, f32 _z)
    {
        x = CryConvertFloatToHalf(_x);
        y = CryConvertFloatToHalf(_y);
        z = CryConvertFloatToHalf(_z);
        w = CryConvertFloatToHalf(1.0f);
    }
    float operator[](int i) const
    {
        assert(i <= 3);
        return CryConvertHalfToFloat(((CryHalf*)this)[i]);
    }
    _inline Vec3f16& operator = (const Vec3& sl)
    {
        x = CryConvertFloatToHalf(sl.x);
        y = CryConvertFloatToHalf(sl.y);
        z = CryConvertFloatToHalf(sl.z);
        w = CryConvertFloatToHalf(1.0f);
        return *this;
    }
    _inline Vec3f16& operator = (const Vec4& sl)
    {
        x = CryConvertFloatToHalf(sl.x);
        y = CryConvertFloatToHalf(sl.y);
        z = CryConvertFloatToHalf(sl.z);
        w = CryConvertFloatToHalf(sl.w);
        return *this;
    }
    _inline Vec3 ToVec3() const
    {
        Vec3 v;
        v.x = CryConvertHalfToFloat(x);
        v.y = CryConvertHalfToFloat(y);
        v.z = CryConvertHalfToFloat(z);
        return v;
    }
};

struct SVF_P3F_C4B
{
    Vec3 xyz;
    UCol color;
};

struct SVF_P3F_C4B_T2F
{
    Vec3 xyz;
    UCol color;
    Vec2 st;
};

struct SVF_P2F_C4B_T2F_F4B
{
    Vec2 xy;
    UCol color;
    Vec2 st;
    uint8 texIndex;
    uint8 texHasColorChannel;
    uint8 texIndex2;
    uint8 pad;
};

struct SVF_P3F
{
    Vec3 xyz;
};
